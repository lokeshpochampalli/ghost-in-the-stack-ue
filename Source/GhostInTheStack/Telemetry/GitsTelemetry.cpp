#include "GitsTelemetry.h"
#include "Interpreter/GitsRng.h"
#include "Serialization/JsonSerializer.h"
#include "Serialization/JsonWriter.h"
#include "Policies/PrettyJsonPrintPolicy.h"
#include "Misc/FileHelper.h"
#include "Misc/Paths.h"
#include "Misc/Guid.h"
#include "Misc/DateTime.h"
#include "HAL/PlatformFileManager.h"
#include "Internationalization/Regex.h"

DEFINE_LOG_CATEGORY_STATIC(LogGitsTelemetry, Display, All);

const TCHAR* UGitsTelemetrySubsystem::ConsentVersion = TEXT("2026-08-1");

const TArray<FString>& UGitsTelemetrySubsystem::EventTypes()
{
	static const TArray<FString> Types = {
		TEXT("session_start"), TEXT("level_start"), TEXT("prediction_shown"), TEXT("prediction_submitted"),
		TEXT("prediction_unresolvable"), TEXT("scrub_gate_satisfied"), TEXT("run_executed"), TEXT("scrub"),
		TEXT("edit"), TEXT("error_shown"), TEXT("hint_requested"), TEXT("reserve_drawn"), TEXT("level_complete"),
		TEXT("level_abandoned"), TEXT("instrument_started"), TEXT("instrument_response"),
		TEXT("instrument_completed"), TEXT("session_end"),
	};
	return Types;
}

bool UGitsTelemetrySubsystem::IsEventType(const FString& Type) { return EventTypes().Contains(Type); }

FString UGitsTelemetrySubsystem::GenerateParticipantCode(uint32 Seed)
{
	// Concrete words, easy to transcribe from a screen onto paper; nothing a participant could
	// read as being about them (the reference's list).
	static const TCHAR* Words[] = {
		TEXT("anchor"), TEXT("beacon"), TEXT("cinder"), TEXT("copper"), TEXT("ember"), TEXT("fathom"), TEXT("glacier"),
		TEXT("harbour"), TEXT("ingot"), TEXT("kelp"), TEXT("lantern"), TEXT("mercury"), TEXT("north"), TEXT("otter"),
		TEXT("pewter"), TEXT("quarry"), TEXT("rivet"), TEXT("slate"), TEXT("tundra"), TEXT("vessel"), TEXT("walrus"), TEXT("zinc"),
	};
	constexpr int32 Count = UE_ARRAY_COUNT(Words);
	GitsRng::FMulberry32 Rng(Seed);
	const TCHAR* First = Words[Rng.NextInt(Count)];
	const TCHAR* Second = Words[Rng.NextInt(Count)];
	const int32 Number = 10 + Rng.NextInt(90);
	return FString::Printf(TEXT("%s-%s-%d"), First, Second, Number);
}

TArray<FString> UGitsTelemetrySubsystem::FindPii(const FString& Serialised)
{
	// A tripwire for the realistic failure: a future call site putting a name or an email into
	// a payload because it was to hand. Deliberately paranoid and deliberately cheap.
	static const TArray<TPair<FString, FString>> Patterns = {
		{ TEXT("email address"), TEXT("[\\w.+-]+@[\\w-]+\\.[\\w.]{2,}") },
		{ TEXT("IPv4 address"), TEXT("\\b\\d{1,3}\\.\\d{1,3}\\.\\d{1,3}\\.\\d{1,3}\\b") },
		{ TEXT("a name field"), TEXT("\"(name|fullName|firstName|lastName|username)\"\\s*:") },
		{ TEXT("an email field"), TEXT("\"e?mail\"\\s*:") },
		{ TEXT("an IP field"), TEXT("\"(ip|ipAddress|remoteAddr)\"\\s*:") },
		{ TEXT("a user-agent field"), TEXT("\"userAgent\"\\s*:") },
	};
	TArray<FString> Findings;
	for (const auto& P : Patterns)
	{
		FRegexPattern Pattern(P.Value, ERegexPatternFlags::CaseInsensitive);
		FRegexMatcher Matcher(Pattern, Serialised);
		if (Matcher.FindNext()) { Findings.Add(P.Key); }
	}
	return Findings;
}

void UGitsTelemetrySubsystem::Initialize(FSubsystemCollectionBase& Collection)
{
	Super::Initialize(Collection);
	// The one place a fresh, unpredictable value is wanted: the clock seeds the code.
	const uint32 Seed = (uint32)(FDateTime::UtcNow().GetTicks() & 0xffffffff) ^ (uint32)FPlatformTime::Cycles();
	ParticipantCode = GenerateParticipantCode(Seed);
	SessionId = FGuid::NewGuid().ToString(EGuidFormats::DigitsLower);
	StartedAtMs = NowMs();
	bPersist = true;
	UE_LOG(LogGitsTelemetry, Display, TEXT("participant code %s, session %s, no consent yet"), *ParticipantCode, *SessionId);
}

void UGitsTelemetrySubsystem::InitialiseForTest(const FString& InParticipantCode, const FString& InSessionId)
{
	ParticipantCode = InParticipantCode;
	SessionId = InSessionId;
	Consent = FGitsConsent();
	Events.Reset();
	Sequence = 0;
	LevelId.Reset();
	bPersist = false;
	StartedAtMs = NowMs();
}

void UGitsTelemetrySubsystem::SetParticipantCode(const FString& Code)
{
	if (Consent.bGiven) { UE_LOG(LogGitsTelemetry, Warning, TEXT("participant code cannot change after consent")); return; }
	ParticipantCode = Code;
}

double UGitsTelemetrySubsystem::NowMs() const
{
	return FDateTime::UtcNow().ToUnixTimestampDecimal() * 1000.0;
}

// --- the gate -------------------------------------------------------------------------------

void UGitsTelemetrySubsystem::RecordConsent(bool bCodeCapture, const FString& Experience, uint32 Seed)
{
	Consent.Version = ConsentVersion;
	Consent.bCodeCapture = bCodeCapture;
	Consent.RecordedAt = NowMs();
	Consent.bGiven = true;
	PersistConsent();
	TSharedPtr<FJsonObject> P = Payload();
	P->SetStringField(TEXT("consentVersion"), Consent.Version);
	P->SetBoolField(TEXT("codeCapture"), bCodeCapture);
	P->SetNumberField(TEXT("seed"), (double)Seed);
	P->SetStringField(TEXT("experience"), Experience);
	Record(TEXT("session_start"), P);
}

void UGitsTelemetrySubsystem::Erase()
{
	Events.Reset();
	Consent = FGitsConsent();
	Sequence = 0;
	if (bPersist)
	{
		IPlatformFile& PF = FPlatformFileManager::Get().GetPlatformFile();
		PF.DeleteFile(*(StoreDirectory() / ParticipantCode + TEXT(".events.jsonl")));
		PF.DeleteFile(*(StoreDirectory() / ParticipantCode + TEXT(".consent.json")));
	}
	UE_LOG(LogGitsTelemetry, Display, TEXT("erased everything held for %s"), *ParticipantCode);
}

bool UGitsTelemetrySubsystem::Record(const FString& Type, TSharedPtr<FJsonObject> InPayload)
{
	if (!Consent.bGiven)
	{
		// Loud, never silent: a study that quietly discarded events would look exactly like one
		// that recorded none, and the difference matters.
		UE_LOG(LogGitsTelemetry, Error, TEXT("refused to record %s: no consent is on file for %s"), *Type, *ParticipantCode);
		return false;
	}
	if (!IsEventType(Type))
	{
		UE_LOG(LogGitsTelemetry, Error, TEXT("refused to record %s: not one of the eighteen event types"), *Type);
		return false;
	}
	FGitsTelemetryEvent E;
	E.SessionId = SessionId;
	E.ParticipantCode = ParticipantCode;
	E.LevelId = LevelId;
	E.Timestamp = NowMs();
	E.Sequence = Sequence++;
	E.Type = Type;
	E.Payload = InPayload.IsValid() ? InPayload : Payload();
	Events.Add(E);
	PersistEvent(E);
	return true;
}

bool UGitsTelemetrySubsystem::Edit(int32 Line, const FString& Before, const FString& After)
{
	// The caller does not choose: passing the text unconditionally and letting the log decide is
	// what stops a future call site leaking code from a participant who declined (ADR-014).
	const bool bCapture = HasCodeCaptureConsent();
	TSharedPtr<FJsonObject> P = Payload();
	P->SetNumberField(TEXT("line"), Line);
	P->SetStringField(TEXT("before"), bCapture ? Before : GitsRng::HashToken(Before));
	P->SetStringField(TEXT("after"), bCapture ? After : GitsRng::HashToken(After));
	P->SetBoolField(TEXT("hashed"), !bCapture);
	return Record(TEXT("edit"), P);
}

// --- storage ---------------------------------------------------------------------------------

FString UGitsTelemetrySubsystem::StoreDirectory() const { return FPaths::ProjectSavedDir() / TEXT("Telemetry"); }
FString UGitsTelemetrySubsystem::ExportDirectory() const { return StoreDirectory() / TEXT("exports"); }

TSharedPtr<FJsonObject> UGitsTelemetrySubsystem::EventToJson(const FGitsTelemetryEvent& E) const
{
	TSharedPtr<FJsonObject> O = MakeShared<FJsonObject>();
	O->SetStringField(TEXT("sessionId"), E.SessionId);
	O->SetStringField(TEXT("participantCode"), E.ParticipantCode);
	if (E.LevelId.IsEmpty()) { O->SetField(TEXT("levelId"), MakeShared<FJsonValueNull>()); }
	else { O->SetStringField(TEXT("levelId"), E.LevelId); }
	O->SetNumberField(TEXT("timestamp"), E.Timestamp);
	O->SetNumberField(TEXT("sequence"), E.Sequence);
	O->SetStringField(TEXT("type"), E.Type);
	O->SetObjectField(TEXT("payload"), E.Payload.IsValid() ? E.Payload : MakeShared<FJsonObject>());
	return O;
}

void UGitsTelemetrySubsystem::PersistConsent() const
{
	if (!bPersist) { return; }
	TSharedPtr<FJsonObject> O = MakeShared<FJsonObject>();
	O->SetStringField(TEXT("version"), Consent.Version);
	O->SetBoolField(TEXT("codeCapture"), Consent.bCodeCapture);
	O->SetNumberField(TEXT("recordedAt"), Consent.RecordedAt);
	FString Out;
	TSharedRef<TJsonWriter<>> W = TJsonWriterFactory<>::Create(&Out);
	FJsonSerializer::Serialize(O.ToSharedRef(), W);
	FFileHelper::SaveStringToFile(Out, *(StoreDirectory() / ParticipantCode + TEXT(".consent.json")));
}

void UGitsTelemetrySubsystem::PersistEvent(const FGitsTelemetryEvent& E) const
{
	if (!bPersist) { return; }
	FString Line;
	TSharedRef<TJsonWriter<>> W = TJsonWriterFactory<>::Create(&Line);
	FJsonSerializer::Serialize(EventToJson(E).ToSharedRef(), W);
	Line += TEXT("\n");
	FFileHelper::SaveStringToFile(Line, *(StoreDirectory() / ParticipantCode + TEXT(".events.jsonl")), FFileHelper::EEncodingOptions::AutoDetect, &IFileManager::Get(), FILEWRITE_Append);
}

// --- the export --------------------------------------------------------------------------------

FString UGitsTelemetrySubsystem::BuildBundleJson() const
{
	TSharedPtr<FJsonObject> Root = MakeShared<FJsonObject>();
	Root->SetNumberField(TEXT("formatVersion"), 1);
	Root->SetStringField(TEXT("participantCode"), ParticipantCode);
	Root->SetNumberField(TEXT("exportedAt"), NowMs());
	TSharedPtr<FJsonObject> C = MakeShared<FJsonObject>();
	C->SetStringField(TEXT("version"), Consent.Version);
	C->SetBoolField(TEXT("codeCapture"), Consent.bCodeCapture);
	C->SetNumberField(TEXT("recordedAt"), Consent.RecordedAt);
	Root->SetObjectField(TEXT("consent"), C);
	TArray<TSharedPtr<FJsonValue>> Arr;
	for (const FGitsTelemetryEvent& E : Events) { Arr.Add(MakeShared<FJsonValueObject>(EventToJson(E))); }
	Root->SetArrayField(TEXT("events"), Arr);
	FString Out;
	TSharedRef<TJsonWriter<TCHAR, TPrettyJsonPrintPolicy<TCHAR>>> W = TJsonWriterFactory<TCHAR, TPrettyJsonPrintPolicy<TCHAR>>::Create(&Out);
	FJsonSerializer::Serialize(Root.ToSharedRef(), W);
	return Out;
}

bool UGitsTelemetrySubsystem::Export(FString& OutPath, FString& OutError) const
{
	if (!Consent.bGiven)
	{
		OutError = FString::Printf(TEXT("No consent is on file for %s, so there is nothing that may be exported."), *ParticipantCode);
		return false;
	}
	const FString Json = BuildBundleJson();
	const TArray<FString> Findings = FindPii(Json);
	if (Findings.Num() > 0)
	{
		OutError = FString::Printf(TEXT("Refused to export: the bundle contains %s. No participant data leaves this machine with anything identifying in it."), *FString::Join(Findings, TEXT(", ")));
		return false;
	}
	const FString Stamp = FDateTime::UtcNow().ToString(TEXT("%Y-%m-%d"));
	OutPath = ExportDirectory() / FString::Printf(TEXT("gits-%s-%s.json"), *ParticipantCode, *Stamp);
	if (!FFileHelper::SaveStringToFile(Json, *OutPath, FFileHelper::EEncodingOptions::ForceUTF8WithoutBOM))
	{
		OutError = TEXT("could not write ") + OutPath;
		return false;
	}
	UE_LOG(LogGitsTelemetry, Display, TEXT("exported %d events to %s"), Events.Num(), *OutPath);
	return true;
}

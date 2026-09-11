// Ghost in the Stack — the event log, the consent gate, and the export (port of src/telemetry).
//
// NO EVENT IS WRITTEN BEFORE CONSENT IS RECORDED. The gate is a property of the log, not of
// the consent screen: there is no other way to reach the store. The log is append-only; the
// one exception is erasure by participant code. NO PII, EVER: a participant is a pseudonymous
// code they write down themselves. The export is the reference's bundle, format version 1,
// so its scripts/analyse.ts reads it unchanged.
#pragma once

#include "CoreMinimal.h"
#include "Subsystems/GameInstanceSubsystem.h"
#include "Dom/JsonObject.h"
#include "GitsTelemetry.generated.h"

/** The consent a participant gave; the version is bumped when the sheet changes. */
USTRUCT(BlueprintType)
struct FGitsConsent
{
	GENERATED_BODY()

	UPROPERTY(BlueprintReadOnly, Category = "Telemetry") FString Version;
	/** The separate opt-in of ADR-014. False means edits are stored as hashes. */
	UPROPERTY(BlueprintReadOnly, Category = "Telemetry") bool bCodeCapture = false;
	/** Milliseconds since the epoch. */
	UPROPERTY(BlueprintReadOnly, Category = "Telemetry") double RecordedAt = 0.0;
	UPROPERTY(BlueprintReadOnly, Category = "Telemetry") bool bGiven = false;
};

struct FGitsTelemetryEvent
{
	FString SessionId;
	FString ParticipantCode;
	/** Empty means null: the event belongs to no level. */
	FString LevelId;
	double Timestamp = 0.0;
	int32 Sequence = 0;
	FString Type;
	TSharedPtr<FJsonObject> Payload;
};

UCLASS()
class GHOSTINTHESTACK_API UGitsTelemetrySubsystem : public UGameInstanceSubsystem
{
	GENERATED_BODY()

public:
	static const TCHAR* ConsentVersion;
	/** The eighteen event types of the reference taxonomy. Anything else is refused. */
	static const TArray<FString>& EventTypes();
	static bool IsEventType(const FString& Type);
	/** A code of the form copper-lantern-47, seeded so nothing here uses Math.random. */
	static FString GenerateParticipantCode(uint32 Seed);
	/** Every PII pattern that matches a serialised bundle, by name. Empty means clean. */
	static TArray<FString> FindPii(const FString& Serialised);
	static TSharedPtr<FJsonObject> Payload() { return MakeShared<FJsonObject>(); }

	virtual void Initialize(FSubsystemCollectionBase& Collection) override;
	/** Tests build the subsystem without a game instance; this sets it up without disk persistence. */
	void InitialiseForTest(const FString& InParticipantCode, const FString& InSessionId);

	// --- the participant
	UFUNCTION(BlueprintPure, Category = "Telemetry") const FString& GetParticipantCode() const { return ParticipantCode; }
	UFUNCTION(BlueprintPure, Category = "Telemetry") const FString& GetSessionId() const { return SessionId; }
	/** A returning participant quotes their code; everything recorded goes under it. */
	UFUNCTION(BlueprintCallable, Category = "Telemetry") void SetParticipantCode(const FString& Code);

	// --- the gate
	UFUNCTION(BlueprintPure, Category = "Telemetry") bool HasConsent() const { return Consent.bGiven; }
	UFUNCTION(BlueprintPure, Category = "Telemetry") bool HasCodeCaptureConsent() const { return Consent.bGiven && Consent.bCodeCapture; }
	const FGitsConsent& GetConsent() const { return Consent; }
	/** The only write that may happen before the gate opens, because it is the write that opens it. Records session_start. */
	void RecordConsent(bool bCodeCapture, const FString& Experience, uint32 Seed);
	/** Forgets everything held for this participant, consent included. */
	UFUNCTION(BlueprintCallable, Category = "Telemetry") void Erase();

	// --- recording
	/** Appends an event. THE GATE: false, with nothing stored, before consent or for an unknown type. */
	bool Record(const FString& Type, TSharedPtr<FJsonObject> InPayload = nullptr);
	void SetLevel(const FString& InLevelId) { LevelId = InLevelId; }
	void ClearLevel() { LevelId.Reset(); }
	/** An edit, honouring the code-capture opt-in: text with it, hashes without (ADR-014). */
	bool Edit(int32 Line, const FString& Before, const FString& After);
	const TArray<FGitsTelemetryEvent>& GetEvents() const { return Events; }
	double NowMs() const;

	// --- the export
	/** The bundle as JSON: formatVersion 1, participantCode, exportedAt, consent, events. */
	FString BuildBundleJson() const;
	/** Writes gits-<code>-<date>.json under Saved/Telemetry/exports. False with the reason on refusal (no consent, PII). */
	bool Export(FString& OutPath, FString& OutError) const;
	FString ExportDirectory() const;

private:
	void PersistConsent() const;
	void PersistEvent(const FGitsTelemetryEvent& Event) const;
	FString StoreDirectory() const;
	TSharedPtr<FJsonObject> EventToJson(const FGitsTelemetryEvent& Event) const;

	FString ParticipantCode;
	FString SessionId;
	FGitsConsent Consent;
	FString LevelId;
	int32 Sequence = 0;
	TArray<FGitsTelemetryEvent> Events;
	bool bPersist = true;
	double StartedAtMs = 0.0;
};

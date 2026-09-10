#include "GitsWallDisplay.h"
#include "GitsStationSubsystem.h"
#include "UI/GitsScreen.h"
#include "Interpreter/GitsTrace.h"
#include "Components/StaticMeshComponent.h"
#include "Components/WidgetComponent.h"
#include "Engine/World.h"

AGitsWallDisplay::AGitsWallDisplay()
{
	PrimaryActorTick.bCanEverTick = false;
	Body = CreateDefaultSubobject<UStaticMeshComponent>(TEXT("Body"));
	SetRootComponent(Body);
	Body->SetCollisionProfileName(TEXT("BlockAll"));

	Screen = CreateDefaultSubobject<UWidgetComponent>(TEXT("Screen"));
	Screen->SetupAttachment(Body);
	// The panel's face is at x=-6, 120 wide, 70 high, origin at the bottom centre of the back.
	Screen->SetRelativeLocation(FVector(-6.6f, 0.f, 35.f));
	Screen->SetRelativeRotation(FRotator(0.f, 180.f, 0.f));
	Screen->SetRelativeScale3D(FVector(0.1f));
	Screen->SetDrawSize(FVector2D(1140.f, 640.f));
	Screen->SetWidgetSpace(EWidgetSpace::World);
	Screen->SetWidgetClass(UGitsScreenWidget::StaticClass());
	Screen->SetCollisionProfileName(TEXT("NoCollision"));
	Screen->SetTwoSided(false);
}

UGitsStationSubsystem* AGitsWallDisplay::Station() const
{
	return GetWorld() ? GetWorld()->GetSubsystem<UGitsStationSubsystem>() : nullptr;
}

void AGitsWallDisplay::BeginPlay()
{
	Super::BeginPlay();
	if (Screen)
	{
		Screen->InitWidget();
		if (UGitsScreenWidget* W = Cast<UGitsScreenWidget>(Screen->GetUserWidgetObject())) { W->FontSize = 22; W->MaxMessageRows = 14; }
	}
	if (UGitsStationSubsystem* S = Station())
	{
		StepHandle = S->OnStepChanged.AddUObject(this, &AGitsWallDisplay::HandleStep);
		StartHandle = S->OnRunStarted.AddUObject(this, &AGitsWallDisplay::HandleRun);
		FinishHandle = S->OnRunFinished.AddUObject(this, &AGitsWallDisplay::HandleRun);
		MessageHandle = S->OnMessage.AddUObject(this, &AGitsWallDisplay::HandleMessage);
		StateHandle = S->OnPlayStateChanged.AddUObject(this, &AGitsWallDisplay::HandleState);
	}
	Refresh();
}

void AGitsWallDisplay::EndPlay(const EEndPlayReason::Type Reason)
{
	if (UGitsStationSubsystem* S = Station())
	{
		S->OnStepChanged.Remove(StepHandle);
		S->OnRunStarted.Remove(StartHandle);
		S->OnRunFinished.Remove(FinishHandle);
		S->OnMessage.Remove(MessageHandle);
		S->OnPlayStateChanged.Remove(StateHandle);
	}
	Super::EndPlay(Reason);
}

FString AGitsWallDisplay::DescribeEffect(const FGitsEffect& E)
{
	switch (E.Kind)
	{
	case FGitsEffect::EKind::Set:
	{
		FString Kind, Id;
		if (!E.Key.Split(TEXT("."), &Kind, &Id)) { Kind = E.Key; }
		if (E.Value.Kind == FGitsWorldValue::EKind::Bool)
		{
			const bool bOn = E.Value.Bool;
			if (Kind == TEXT("door")) { return FString::Printf(TEXT("door %s: %s"), *Id, bOn ? TEXT("open") : TEXT("shut")); }
			if (Kind == TEXT("valve")) { return FString::Printf(TEXT("valve %s: %s"), *Id, bOn ? TEXT("open") : TEXT("closed")); }
			return FString::Printf(TEXT("%s %s: %s"), *Kind, *Id, bOn ? TEXT("on") : TEXT("off"));
		}
		return FString::Printf(TEXT("%s%s%s = %s"), *Kind, Id.IsEmpty() ? TEXT("") : TEXT(" "), *Id, *E.Value.ToText());
	}
	case FGitsEffect::EKind::Log: return TEXT("log: ") + E.Message;
	case FGitsEffect::EKind::Wait: return FString::Printf(TEXT("waited %lld"), E.Ticks);
	}
	return TEXT("");
}

void AGitsWallDisplay::HandleState(EGitsPlayState NewState)
{
	Refresh();
}

// While rewinding the display is VANT's recorder: where the play head is, which loop it is
// inside (one line, not every iteration flickering past), the bindings at that step and the
// world as it was. Nothing here is simulated; it is all read off the trace at the step.
void AGitsWallDisplay::BuildRewindView(FGitsScreenModel& M, UGitsStationSubsystem* S) const
{
	const FGitsTrace& T = S->GetTrace();
	const FGitsRecorder& R = S->GetRecorder();
	const int32 Head = S->GetPlayIndex();
	const int32 Beat = R.BeatOfStep(Head);
	M.Title = TEXT("RECORDER  rewind");
	if (Head < 0)
	{
		M.MessageLines.Add(TEXT("VANT: before the first line ran."));
		M.Status = TEXT("release to run it again");
		return;
	}
	const FGitsStep& Step = T.Steps[Head];
	M.MessageLines.Add(FString::Printf(TEXT("line %d   %s"), Step.Span.Start.Line, *Step.Label));
	for (const FGitsLoopContext& L : R.LoopsAt(Head)) { M.MessageLines.Add(TEXT("VANT: ") + L.Describe()); }
	TArray<TPair<FString, FGitsValue>> Bindings = GitsTrace::BindingsAt(T, Head);
	if (Bindings.Num() > 0)
	{
		FString Line;
		for (const auto& B : Bindings)
		{
			if (B.Value.Kind == EGitsValueKind::Function) { continue; }
			Line += (Line.IsEmpty() ? TEXT("") : TEXT("   ")) + B.Key + TEXT(" = ") + GitsValue::Repr(B.Value);
		}
		if (!Line.IsEmpty()) { M.MessageLines.Add(Line); }
	}
	const FGitsWorldState& World = S->GetCurrentWorld();
	TArray<FString> Keys;
	for (const auto& P : World) { if (!P.Key.StartsWith(TEXT("sensor")) && P.Key != GitsWorld::LogCountKey) { Keys.Add(P.Key); } }
	Keys.Sort();
	FString State;
	for (const FString& K : Keys) { State += (State.IsEmpty() ? TEXT("") : TEXT("   ")) + K + TEXT("=") + World[K].ToText(); }
	if (!State.IsEmpty()) { M.MessageLines.Add(State); }
	M.Status = FString::Printf(TEXT("statement %d of %d   step %d of %d"), Beat + 1, R.NumBeats(), Head + 1, T.Steps.Num());
}

void AGitsWallDisplay::Refresh()
{
	if (!Screen) { return; }
	UGitsScreenWidget* W = Cast<UGitsScreenWidget>(Screen->GetUserWidgetObject());
	if (!W) { return; }
	FGitsScreenModel M;
	M.Title = Title;
	UGitsStationSubsystem* S = Station();
	if (S && S->HasTrace() && S->IsRewinding())
	{
		BuildRewindView(M, S);
		W->SetModel(M);
		return;
	}
	if (!S || !S->HasTrace())
	{
		if (S && S->GetLastSummary().bParseFailed) { M.MessageLines.Add(TEXT("!") + S->GetLastMessage()); M.Status = TEXT("did not run"); M.bStatusIsError = true; }
		else if (S && S->GetLastSummary().bRefused) { M.MessageLines.Add(TEXT("VANT: ") + S->GetLastMessage()); M.bStatusIsError = true; }
		else { M.MessageLines.Add(IdleText); }
		W->SetModel(M);
		return;
	}
	const FGitsTrace& T = S->GetTrace();
	const int32 Head = S->GetPlayIndex();
	// Everything the run said and did, in order, up to the play head.
	for (int32 i = 0; i <= Head && i < T.Steps.Num(); ++i)
	{
		for (const FString& L : T.Steps[i].Output) { M.MessageLines.Add(TEXT("> ") + L); }
		for (const FGitsEffect& E : T.Steps[i].Effects) { M.MessageLines.Add(DescribeEffect(E)); }
		if (T.Steps[i].bHasOracleRead)
		{
			M.MessageLines.Add(FString::Printf(TEXT("read %s: %s"), *T.Steps[i].OracleRead.Query.Id, *GitsValue::Display(T.Steps[i].OracleRead.Value)));
		}
	}
	const FGitsRunSummary& Sum = S->GetLastSummary();
	if (S->IsPlaying())
	{
		M.Status = FString::Printf(TEXT("running  line %d"), S->CurrentLine());
	}
	else if (Sum.bRefused)
	{
		M.MessageLines.Add(TEXT("VANT: ") + Sum.Message);
		M.bStatusIsError = true;
	}
	else if (Sum.Outcome == TEXT("completed"))
	{
		M.Status = FString::Printf(TEXT("run complete  %d statements"), Sum.Statements);
	}
	else
	{
		M.MessageLines.Add(TEXT("!") + Sum.Message);
		M.Status = Sum.Outcome;
		M.bStatusIsError = true;
	}
	// The world as the run left it, so a door that never opened is visible as a fact.
	const FGitsWorldState& World = S->GetCurrentWorld();
	TArray<FString> Keys;
	for (const auto& P : World) { if (!P.Key.StartsWith(TEXT("sensor")) && P.Key != GitsWorld::LogCountKey) { Keys.Add(P.Key); } }
	Keys.Sort();
	if (Keys.Num() > 0)
	{
		FString State;
		for (const FString& K : Keys) { State += (State.IsEmpty() ? TEXT("") : TEXT("   ")) + K + TEXT("=") + World[K].ToText(); }
		M.MessageLines.Add(TEXT(""));
		M.MessageLines.Add(State);
	}
	W->SetModel(M);
}

#include "GitsInstruments.h"
#include "Interpreter/GitsRng.h"

namespace
{
	double Round(double V, int32 Places = 2)
	{
		const double F = FMath::Pow(10.0, Places);
		return FMath::RoundToDouble(V * F) / F;
	}

	FGitsScale MakeScale(const TCHAR* Id, std::initializer_list<const TCHAR*> Labels, int32 First)
	{
		FGitsScale S;
		S.Id = Id;
		for (const TCHAR* L : Labels) { S.Labels.Add(L); }
		S.FirstValue = First;
		return S;
	}

	FGitsLikertItem L(const TCHAR* Id, const TCHAR* Text, const TCHAR* Dimension, bool bReversed)
	{
		FGitsLikertItem I; I.Id = Id; I.Text = Text; I.Dimension = Dimension; I.bReversed = bReversed; return I;
	}

	TSharedPtr<FJsonObject> DimensionRows(const FGitsInstrument& In, const FGitsResponses& R, int32& OutAnswered, TArray<TSharedPtr<FJsonValue>>& OutRows)
	{
		OutAnswered = 0;
		for (const FString& D : In.Dimensions)
		{
			TArray<double> Values;
			int32 Total = 0;
			for (const FGitsLikertItem& I : In.Items)
			{
				if (I.Dimension != D) { continue; }
				++Total;
				double V;
				if (GitsInstruments::ReadLikert(In.Scale, I, R, V)) { Values.Add(V); }
			}
			OutAnswered += Values.Num();
			TSharedPtr<FJsonObject> Row = MakeShared<FJsonObject>();
			Row->SetStringField(TEXT("dimension"), D);
			double M, Md;
			if (GitsInstruments::Mean(Values, M)) { Row->SetNumberField(TEXT("mean"), Round(M)); } else { Row->SetField(TEXT("mean"), MakeShared<FJsonValueNull>()); }
			if (GitsInstruments::Median(Values, Md)) { Row->SetNumberField(TEXT("median"), Round(Md)); } else { Row->SetField(TEXT("median"), MakeShared<FJsonValueNull>()); }
			Row->SetNumberField(TEXT("answered"), Values.Num());
			Row->SetNumberField(TEXT("total"), Total);
			OutRows.Add(MakeShared<FJsonValueObject>(Row));
		}
		return nullptr;
	}
}

namespace GitsInstruments
{
	bool Mean(const TArray<double>& Values, double& Out)
	{
		if (Values.Num() == 0) { return false; }
		double Sum = 0; for (double V : Values) { Sum += V; }
		Out = Sum / Values.Num();
		return true;
	}

	bool Median(const TArray<double>& Values, double& Out)
	{
		if (Values.Num() == 0) { return false; }
		TArray<double> S = Values; S.Sort();
		const int32 Mid = S.Num() / 2;
		Out = (S.Num() % 2 == 1) ? S[Mid] : (S[Mid - 1] + S[Mid]) / 2.0;
		return true;
	}

	bool ReadLikert(const FGitsScale& Scale, const FGitsLikertItem& Item, const FGitsResponses& Responses, double& Out)
	{
		const double* Raw = Responses.Numbers.Find(Item.Id);
		if (!Raw || *Raw < Scale.Low() || *Raw > Scale.High()) { return false; }
		// A flip is a reflection about the midpoint: works for 1..5 and -2..2 alike.
		Out = Item.bReversed ? (Scale.Low() + Scale.High() - *Raw) : *Raw;
		return true;
	}

	TCHAR FormFor(const FString& ParticipantCode, const FString& Occasion)
	{
		const TCHAR First = (GitsRng::HashString(ParticipantCode) % 2 == 0) ? TEXT('A') : TEXT('B');
		if (Occasion == TEXT("pre")) { return First; }
		return First == TEXT('A') ? TEXT('B') : TEXT('A');
	}

	TArray<FGitsQuizOption> PresentedOptions(const FGitsQuizItem& Item, const FString& ParticipantCode)
	{
		GitsRng::FMulberry32 Rng(GitsRng::HashString(ParticipantCode + TEXT(":") + Item.Id));
		return Rng.Shuffle(Item.Options);
	}

	// --- SUS (Brooke, 1996): "this system" rendered as "this game", nothing else changed ------
	const FGitsInstrument& Sus()
	{
		static FGitsInstrument I = []()
		{
			FGitsInstrument S;
			S.Kind = FGitsInstrument::EKind::Likert; S.Scoring = FGitsInstrument::EScoring::Sus;
			S.Id = TEXT("sus"); S.Title = TEXT("How the game was to use");
			S.Blurb = TEXT("Ten quick statements about using the game. There are no right answers and nobody is being judged. Answer with your first reaction rather than thinking hard about it.");
			S.Provenance = TEXT("Brooke, J. (1996) SUS: a quick and dirty usability scale. \"this system\" rendered as \"this game\". Wording not yet verified against the published source.");
			S.Scale = MakeScale(TEXT("sus-5"), { TEXT("Strongly disagree"), TEXT("Disagree"), TEXT("Neither"), TEXT("Agree"), TEXT("Strongly agree") }, 1);
			S.Dimensions = { TEXT("usability") };
			S.Items = {
				L(TEXT("sus-1"), TEXT("I think that I would like to play this game frequently."), TEXT("usability"), false),
				L(TEXT("sus-2"), TEXT("I found the game unnecessarily complex."), TEXT("usability"), true),
				L(TEXT("sus-3"), TEXT("I thought the game was easy to use."), TEXT("usability"), false),
				L(TEXT("sus-4"), TEXT("I think that I would need the support of a technical person to be able to play this game."), TEXT("usability"), true),
				L(TEXT("sus-5"), TEXT("I found the various functions in this game were well integrated."), TEXT("usability"), false),
				L(TEXT("sus-6"), TEXT("I thought there was too much inconsistency in this game."), TEXT("usability"), true),
				L(TEXT("sus-7"), TEXT("I would imagine that most people would learn to play this game very quickly."), TEXT("usability"), false),
				L(TEXT("sus-8"), TEXT("I found the game very awkward to use."), TEXT("usability"), true),
				L(TEXT("sus-9"), TEXT("I felt very confident playing the game."), TEXT("usability"), false),
				L(TEXT("sus-10"), TEXT("I needed to learn a lot of things before I could get going with this game."), TEXT("usability"), true),
			};
			return S;
		}();
		return I;
	}

	// --- IMI, short form: four subscales, no overall ------------------------------------------
	const FGitsInstrument& Imi()
	{
		static FGitsInstrument I = []()
		{
			FGitsInstrument S;
			S.Kind = FGitsInstrument::EKind::Likert; S.Scoring = FGitsInstrument::EScoring::Imi;
			S.Id = TEXT("imi"); S.Title = TEXT("How the game felt");
			S.Blurb = TEXT("Twenty-two statements about how playing felt. Answer with your first reaction.");
			S.Provenance = TEXT("McAuley, Duncan and Tammen (1989), Intrinsic Motivation Inventory, short form, with the activity named. Wording not yet verified against the published source.");
			S.Scale = MakeScale(TEXT("imi-7"), { TEXT("Not at all true"), TEXT("Hardly true"), TEXT("Somewhat untrue"), TEXT("Neutral"), TEXT("Somewhat true"), TEXT("Mostly true"), TEXT("Very true") }, 1);
			S.Dimensions = { TEXT("interest/enjoyment"), TEXT("perceived competence"), TEXT("effort/importance"), TEXT("pressure/tension") };
			const TCHAR* IE = TEXT("interest/enjoyment"); const TCHAR* PC = TEXT("perceived competence"); const TCHAR* EI = TEXT("effort/importance"); const TCHAR* PT = TEXT("pressure/tension");
			S.Items = {
				L(TEXT("imi-1"), TEXT("I enjoyed playing this game very much."), IE, false),
				L(TEXT("imi-2"), TEXT("Playing this game was fun to do."), IE, false),
				L(TEXT("imi-3"), TEXT("I thought this was a boring game."), IE, true),
				L(TEXT("imi-4"), TEXT("This game did not hold my attention at all."), IE, true),
				L(TEXT("imi-5"), TEXT("I would describe this game as very interesting."), IE, false),
				L(TEXT("imi-6"), TEXT("I thought this game was quite enjoyable."), IE, false),
				L(TEXT("imi-7"), TEXT("While I was playing, I was thinking about how much I was enjoying it."), IE, false),
				L(TEXT("imi-8"), TEXT("I think I am pretty good at this game."), PC, false),
				L(TEXT("imi-9"), TEXT("I think I did pretty well at this game, compared to other people."), PC, false),
				L(TEXT("imi-10"), TEXT("After playing for a while, I felt pretty competent."), PC, false),
				L(TEXT("imi-11"), TEXT("I am satisfied with how I did at this game."), PC, false),
				L(TEXT("imi-12"), TEXT("I was pretty skilled at this game."), PC, false),
				L(TEXT("imi-13"), TEXT("This was a game that I could not do very well."), PC, true),
				L(TEXT("imi-14"), TEXT("I put a lot of effort into this."), EI, false),
				L(TEXT("imi-15"), TEXT("I did not try very hard to do well at this game."), EI, true),
				L(TEXT("imi-16"), TEXT("I tried very hard while playing this game."), EI, false),
				L(TEXT("imi-17"), TEXT("It was important to me to do well at this task."), EI, false),
				L(TEXT("imi-18"), TEXT("I did not put much energy into this."), EI, true),
				L(TEXT("imi-19"), TEXT("I did not feel nervous at all while playing."), PT, true),
				L(TEXT("imi-20"), TEXT("I felt very tense while playing this game."), PT, false),
				L(TEXT("imi-21"), TEXT("I was very relaxed while playing."), PT, true),
				L(TEXT("imi-22"), TEXT("I felt pressured while playing this game."), PT, false),
			};
			return S;
		}();
		return I;
	}

	// --- MEEGA+: thirteen dimensions, -2..+2, social interaction kept in --------------------------
	const FGitsInstrument& MeegaPlus()
	{
		static FGitsInstrument I = []()
		{
			FGitsInstrument S;
			S.Kind = FGitsInstrument::EKind::Likert; S.Scoring = FGitsInstrument::EScoring::Meega;
			S.Id = TEXT("meega-plus"); S.Title = TEXT("The game as a way of learning");
			S.Blurb = TEXT("Thirty-three statements about the game as a way of learning. Answer with your first reaction; skip anything that does not apply.");
			S.Provenance = TEXT("Petri, von Wangenheim and Borgatto (2016), MEEGA+. Wording not yet verified against the published source; no alpha from the paper may be claimed.");
			S.Scale = MakeScale(TEXT("meega-5"), { TEXT("Strongly disagree"), TEXT("Disagree"), TEXT("Neither"), TEXT("Agree"), TEXT("Strongly agree") }, -2);
			S.Dimensions = { TEXT("aesthetics"), TEXT("learnability"), TEXT("operability"), TEXT("accessibility"), TEXT("user error protection"),
				TEXT("focused attention"), TEXT("fun"), TEXT("challenge"), TEXT("social interaction"), TEXT("confidence"), TEXT("relevance"), TEXT("satisfaction"), TEXT("perceived learning") };
			S.Items = {
				L(TEXT("meega-1"), TEXT("The game design is attractive: the screens, the type and the colours."), TEXT("aesthetics"), false),
				L(TEXT("meega-2"), TEXT("The text, the colours and the layout are consistent from screen to screen."), TEXT("aesthetics"), false),
				L(TEXT("meega-3"), TEXT("I needed to learn only a few things before I could start playing."), TEXT("learnability"), false),
				L(TEXT("meega-4"), TEXT("Learning to play this game was easy for me."), TEXT("learnability"), false),
				L(TEXT("meega-5"), TEXT("I think that most people would learn to play this game very quickly."), TEXT("learnability"), false),
				L(TEXT("meega-6"), TEXT("The rules of the game are clear and understandable."), TEXT("operability"), false),
				L(TEXT("meega-7"), TEXT("The fonts, sizes and colours are well chosen and easy to read."), TEXT("operability"), false),
				L(TEXT("meega-8"), TEXT("I found the game unnecessarily complex."), TEXT("operability"), true),
				L(TEXT("meega-9"), TEXT("The colours used in the game let me read everything comfortably."), TEXT("accessibility"), false),
				L(TEXT("meega-10"), TEXT("Nothing in the game relied on a sound, an animation or a colour that I could not perceive."), TEXT("accessibility"), false),
				L(TEXT("meega-11"), TEXT("The game stopped me from making mistakes that I could not undo."), TEXT("user error protection"), false),
				L(TEXT("meega-12"), TEXT("When I did something wrong, the game explained what had happened in a way I could act on."), TEXT("user error protection"), false),
				L(TEXT("meega-13"), TEXT("There were moments during the game when I was completely absorbed in it."), TEXT("focused attention"), false),
				L(TEXT("meega-14"), TEXT("I forgot about my immediate surroundings while playing."), TEXT("focused attention"), false),
				L(TEXT("meega-15"), TEXT("I lost track of time while playing."), TEXT("focused attention"), false),
				L(TEXT("meega-16"), TEXT("I had fun with the game."), TEXT("fun"), false),
				L(TEXT("meega-17"), TEXT("Something happened during the game that made me smile."), TEXT("fun"), false),
				L(TEXT("meega-18"), TEXT("The game is appropriately challenging for me."), TEXT("challenge"), false),
				L(TEXT("meega-19"), TEXT("The game gets harder at a pace that suited me."), TEXT("challenge"), false),
				L(TEXT("meega-20"), TEXT("The game does not become monotonous as it goes on."), TEXT("challenge"), false),
				L(TEXT("meega-21"), TEXT("The game promoted moments of cooperation or competition with other people."), TEXT("social interaction"), false),
				L(TEXT("meega-22"), TEXT("I felt good interacting with other people while playing."), TEXT("social interaction"), false),
				L(TEXT("meega-23"), TEXT("When I first looked at the game, I had the impression that it would be easy for me."), TEXT("confidence"), false),
				L(TEXT("meega-24"), TEXT("The content and the structure helped me become confident that I would learn from it."), TEXT("confidence"), false),
				L(TEXT("meega-25"), TEXT("Completing a shift gave me a satisfying sense of achievement."), TEXT("confidence"), false),
				L(TEXT("meega-26"), TEXT("The content of the game is relevant to things I want to learn about programming."), TEXT("relevance"), false),
				L(TEXT("meega-27"), TEXT("It is clear to me how this game relates to learning to program."), TEXT("relevance"), false),
				L(TEXT("meega-28"), TEXT("This game is an appropriate way to learn this subject."), TEXT("relevance"), false),
				L(TEXT("meega-29"), TEXT("I would recommend this game to a friend learning to program."), TEXT("satisfaction"), false),
				L(TEXT("meega-30"), TEXT("I would like to keep playing beyond the part I was given."), TEXT("satisfaction"), false),
				L(TEXT("meega-31"), TEXT("The game contributed to my learning in this subject."), TEXT("perceived learning"), false),
				L(TEXT("meega-32"), TEXT("The game let me practise reading code, not only writing it."), TEXT("perceived learning"), false),
				L(TEXT("meega-33"), TEXT("I am better at predicting what a program will do than I was before playing."), TEXT("perceived learning"), false),
			};
			return S;
		}();
		return I;
	}

	// --- the tracing test: eleven pairs, two isomorphic forms, every distractor tagged ---------
	namespace
	{
		struct FVariant { const TCHAR* Source; const TCHAR* Correct; TArray<TPair<const TCHAR*, const TCHAR*>> Distractors; };
		struct FPairSpec { const TCHAR* PairId; const TCHAR* Concept; int32 Tier; const TCHAR* Prompt; const TCHAR* Expected; FVariant A; FVariant B; };

		const TCHAR* PRINTS = TEXT("What does this program print?");
		const TCHAR* HAPPENS = TEXT("What happens when this program runs?");

		const TArray<FPairSpec>& Pairs()
		{
			static const TArray<FPairSpec> P = {
				{ TEXT("seq"), TEXT("reassignment"), 1, PRINTS, TEXT("output"),
					{ TEXT("x = 4\nx = 7\nprint(x)\n"), TEXT("7"), { {TEXT("4"), TEXT("sequence-ignored")}, {TEXT("11"), TEXT("assignment-as-equality")}, {TEXT("47"), TEXT("type-confusion")} } },
					{ TEXT("n = 6\nn = 2\nprint(n)\n"), TEXT("2"), { {TEXT("6"), TEXT("sequence-ignored")}, {TEXT("8"), TEXT("assignment-as-equality")}, {TEXT("62"), TEXT("type-confusion")} } } },
				{ TEXT("copy"), TEXT("variable-assignment"), 1, PRINTS, TEXT("output"),
					{ TEXT("a = 3\nb = a\na = 10\nprint(b)\n"), TEXT("3"), { {TEXT("10"), TEXT("parallel-assignment")}, {TEXT("13"), TEXT("assignment-as-equality")} } },
					{ TEXT("p = 5\nq = p\np = 1\nprint(q)\n"), TEXT("5"), { {TEXT("1"), TEXT("parallel-assignment")}, {TEXT("6"), TEXT("assignment-as-equality")} } } },
				{ TEXT("accumulate"), TEXT("accumulator"), 1, PRINTS, TEXT("output"),
					{ TEXT("total = 2\ntotal = total + 3\ntotal = total + 3\nprint(total)\n"), TEXT("8"), { {TEXT("5"), TEXT("sequence-ignored")}, {TEXT("3"), TEXT("accumulator-reset")} } },
					{ TEXT("count = 1\ncount = count + 3\ncount = count + 3\nprint(count)\n"), TEXT("7"), { {TEXT("4"), TEXT("sequence-ignored")}, {TEXT("3"), TEXT("accumulator-reset")} } } },
				{ TEXT("subtract"), TEXT("arithmetic"), 1, PRINTS, TEXT("output"),
					{ TEXT("base = 10\nstep = 4\nbase = base - step\nprint(base)\n"), TEXT("6"), { {TEXT("14"), TEXT("operator-confusion")}, {TEXT("10"), TEXT("sequence-ignored")} } },
					{ TEXT("level = 9\ndrop = 2\nlevel = level - drop\nprint(level)\n"), TEXT("7"), { {TEXT("11"), TEXT("operator-confusion")}, {TEXT("9"), TEXT("sequence-ignored")} } } },
				{ TEXT("floordiv"), TEXT("type-coercion"), 1, PRINTS, TEXT("output"),
					{ TEXT("print(7 // 2)\n"), TEXT("3"), { {TEXT("3.5"), TEXT("type-confusion")}, {TEXT("4"), TEXT("fencepost")}, {TEXT("1"), TEXT("operator-confusion")} } },
					{ TEXT("print(9 // 4)\n"), TEXT("2"), { {TEXT("2.25"), TEXT("type-confusion")}, {TEXT("3"), TEXT("fencepost")}, {TEXT("1"), TEXT("operator-confusion")} } } },
				{ TEXT("concat"), TEXT("string-ops"), 1, PRINTS, TEXT("output"),
					{ TEXT("a = \"2\"\nb = \"3\"\nprint(a + b)\n"), TEXT("23"), { {TEXT("5"), TEXT("type-confusion")}, {TEXT("2+3"), TEXT("operator-confusion")} } },
					{ TEXT("x = \"4\"\ny = \"1\"\nprint(x + y)\n"), TEXT("41"), { {TEXT("5"), TEXT("type-confusion")}, {TEXT("4+1"), TEXT("operator-confusion")} } } },
				{ TEXT("branch"), TEXT("conditional"), 2, PRINTS, TEXT("output"),
					{ TEXT("temp = 3\nif temp > 5:\n    print(\"high\")\nelse:\n    print(\"low\")\n"), TEXT("low"), { {TEXT("high"), TEXT("inverted-comparison")}, {TEXT("high\nlow"), TEXT("branch-both")} } },
					{ TEXT("depth = 8\nif depth > 2:\n    print(\"deep\")\nelse:\n    print(\"shallow\")\n"), TEXT("deep"), { {TEXT("shallow"), TEXT("inverted-comparison")}, {TEXT("deep\nshallow"), TEXT("branch-both")} } } },
				{ TEXT("range"), TEXT("for-loop"), 3, PRINTS, TEXT("output"),
					{ TEXT("for i in range(3):\n    print(i)\n"), TEXT("0\n1\n2"), { {TEXT("1\n2\n3"), TEXT("fencepost")}, {TEXT("0"), TEXT("loop-runs-once")} } },
					{ TEXT("for k in range(4):\n    print(k)\n"), TEXT("0\n1\n2\n3"), { {TEXT("1\n2\n3\n4"), TEXT("fencepost")}, {TEXT("0"), TEXT("loop-runs-once")} } } },
				{ TEXT("loop-sum"), TEXT("accumulator"), 3, PRINTS, TEXT("output"),
					{ TEXT("total = 0\nfor i in range(4):\n    total = total + i\nprint(total)\n"), TEXT("6"), { {TEXT("10"), TEXT("fencepost")}, {TEXT("3"), TEXT("accumulator-reset")} } },
					{ TEXT("total = 0\nfor n in range(5):\n    total = total + n\nprint(total)\n"), TEXT("10"), { {TEXT("15"), TEXT("fencepost")}, {TEXT("4"), TEXT("accumulator-reset")} } } },
				{ TEXT("return"), TEXT("return-value"), 4, PRINTS, TEXT("output"),
					{ TEXT("def double(n):\n    return n * 2\n\nvalue = double(5)\nprint(value)\n"), TEXT("10"), { {TEXT("None"), TEXT("return-vs-print")}, {TEXT("5"), TEXT("arg-param-identity")} } },
					{ TEXT("def add_one(x):\n    return x + 1\n\nresult = add_one(7)\nprint(result)\n"), TEXT("8"), { {TEXT("None"), TEXT("return-vs-print")}, {TEXT("7"), TEXT("arg-param-identity")} } } },
				{ TEXT("scope"), TEXT("local-scope"), 4, HAPPENS, TEXT("error"),
					{ TEXT("def setup():\n    limit = 5\n    return limit\n\nsetup()\nprint(limit)\n"), TEXT("The program stops with an error: there is no name \"limit\" out here."), { {TEXT("It prints 5"), TEXT("scope-leak")}, {TEXT("It prints None"), TEXT("return-vs-print")} } },
					{ TEXT("def measure():\n    depth = 3\n    return depth\n\nmeasure()\nprint(depth)\n"), TEXT("The program stops with an error: there is no name \"depth\" out here."), { {TEXT("It prints 3"), TEXT("scope-leak")}, {TEXT("It prints None"), TEXT("return-vs-print")} } } },
			};
			return P;
		}

		FGitsInstrument BuildTracing(TCHAR Form)
		{
			FGitsInstrument S;
			S.Kind = FGitsInstrument::EKind::Quiz; S.Scoring = FGitsInstrument::EScoring::Quiz;
			S.Form = Form;
			S.Id = FString::Printf(TEXT("tracing-%c"), Form);
			S.Title = TEXT("Reading short programs");
			S.Blurb = TEXT("Eleven short programs. For each one, say what it does without running it; there is nothing to run it on. If you are not sure, skip it rather than guessing; a skipped question tells us something a guess does not.");
			S.Provenance = TEXT("Authored for this study: matched pairs over the constructs the four tiers teach, every distractor tagged with a misconception, every item checked against the interpreter.");
			int32 Index = 0;
			for (const FPairSpec& P : Pairs())
			{
				const FVariant& V = Form == TEXT('A') ? P.A : P.B;
				FGitsQuizItem I;
				I.Id = FString::Printf(TEXT("trace-%c-%02d"), Form, Index + 1);
				I.PairId = P.PairId; I.Form = Form; I.Prompt = P.Prompt; I.Source = V.Source; I.Expected = P.Expected;
				I.Concept = P.Concept; I.Tier = P.Tier;
				FGitsQuizOption C; C.Id = I.Id + TEXT("-c"); C.Text = V.Correct; I.Options.Add(C);
				int32 D = 1;
				for (const auto& Dist : V.Distractors)
				{
					FGitsQuizOption O; O.Id = FString::Printf(TEXT("%s-d%d"), *I.Id, D++); O.Text = Dist.Key; O.Misconception = Dist.Value;
					I.Options.Add(O);
				}
				I.CorrectOptionId = C.Id;
				S.QuizItems.Add(I);
				++Index;
			}
			return S;
		}
	}

	const FGitsInstrument& TracingTest(TCHAR Form)
	{
		static FGitsInstrument A = BuildTracing(TEXT('A'));
		static FGitsInstrument B = BuildTracing(TEXT('B'));
		return Form == TEXT('A') ? A : B;
	}
}

// --- scoring -----------------------------------------------------------------------------------

int32 FGitsInstrument::Answered(const FGitsResponses& R) const
{
	int32 N = 0;
	if (Kind == EKind::Likert) { for (const FGitsLikertItem& I : Items) { double V; if (GitsInstruments::ReadLikert(Scale, I, R, V)) { ++N; } } }
	else { for (const FGitsQuizItem& I : QuizItems) { if (R.Options.Contains(I.Id)) { ++N; } } }
	return N;
}

TSharedPtr<FJsonObject> FGitsInstrument::Score(const FGitsResponses& R) const
{
	TSharedPtr<FJsonObject> S = MakeShared<FJsonObject>();
	S->SetStringField(TEXT("instrumentId"), Id);
	if (Kind == EKind::Quiz)
	{
		int32 Correct = 0, Answered = 0;
		TMap<FString, TArray<int32>> ByConcept, ByMisc; // [correct, answered, total]
		for (const FGitsQuizItem& I : QuizItems)
		{
			const FString* Chosen = R.Options.Find(I.Id);
			const FGitsQuizOption* Opt = Chosen ? I.Options.FindByPredicate([Chosen](const FGitsQuizOption& O) { return O.Id == *Chosen; }) : nullptr;
			TArray<int32>& C = ByConcept.FindOrAdd(I.Concept); if (C.Num() == 0) { C = { 0, 0, 0 }; }
			C[2]++;
			if (Opt) { C[1]++; Answered++; if (Opt->Id == I.CorrectOptionId) { C[0]++; Correct++; } }
			// misconceptions count only the items where the participant chose the distractor carrying the tag
			if (Opt && !Opt->Misconception.IsEmpty())
			{
				TArray<int32>& M = ByMisc.FindOrAdd(Opt->Misconception); if (M.Num() == 0) { M = { 0, 0, 0 }; }
				M[2]++; M[1]++;
			}
		}
		auto Rows = [](const TMap<FString, TArray<int32>>& Map)
		{
			TArray<FString> Keys; Map.GetKeys(Keys); Keys.Sort();
			TArray<TSharedPtr<FJsonValue>> Out;
			for (const FString& K : Keys)
			{
				TSharedPtr<FJsonObject> Row = MakeShared<FJsonObject>();
				Row->SetStringField(TEXT("tag"), K);
				Row->SetNumberField(TEXT("correct"), Map[K][0]); Row->SetNumberField(TEXT("answered"), Map[K][1]); Row->SetNumberField(TEXT("total"), Map[K][2]);
				Out.Add(MakeShared<FJsonValueObject>(Row));
			}
			return Out;
		};
		S->SetStringField(TEXT("form"), FString::Chr(Form));
		S->SetNumberField(TEXT("correct"), Correct);
		S->SetNumberField(TEXT("answered"), Answered);
		S->SetNumberField(TEXT("total"), QuizItems.Num());
		S->SetArrayField(TEXT("byConcept"), Rows(ByConcept));
		S->SetArrayField(TEXT("byMisconception"), Rows(ByMisc));
		return S;
	}
	TArray<TSharedPtr<FJsonValue>> Rows;
	int32 Answered = 0;
	DimensionRows(*this, R, Answered, Rows);
	S->SetArrayField(TEXT("dimensions"), Rows);
	const bool bComplete = Answered == Items.Num();
	switch (Scoring)
	{
	case EScoring::Sus:
	{
		// Corrected values contribute value - 1; the sum runs 0..40 before the x2.5. Only a full
		// ten is a SUS score; nine items is not one and is not reported against the benchmark.
		double Sum = 0;
		for (const FGitsLikertItem& I : Items) { double V; if (GitsInstruments::ReadLikert(Scale, I, R, V)) { Sum += V - 1; } }
		if (bComplete) { S->SetNumberField(TEXT("overall"), Round(Sum * 2.5, 1)); } else { S->SetField(TEXT("overall"), MakeShared<FJsonValueNull>()); }
		S->SetStringField(TEXT("overallLabel"), TEXT("SUS score, 0 to 100, benchmark mean 68"));
		break;
	}
	case EScoring::Imi:
		S->SetField(TEXT("overall"), MakeShared<FJsonValueNull>());
		S->SetStringField(TEXT("overallLabel"), TEXT("no overall score; report the four subscale means separately"));
		break;
	default:
	{
		TArray<double> All;
		for (const FGitsLikertItem& I : Items) { double V; if (GitsInstruments::ReadLikert(Scale, I, R, V)) { All.Add(V); } }
		double Md;
		if (GitsInstruments::Median(All, Md)) { S->SetNumberField(TEXT("overall"), Round(Md, 1)); } else { S->SetField(TEXT("overall"), MakeShared<FJsonValueNull>()); }
		S->SetStringField(TEXT("overallLabel"), TEXT("median item score, -2 to +2; report the per-dimension distributions too"));
		break;
	}
	}
	S->SetNumberField(TEXT("answered"), Answered);
	S->SetNumberField(TEXT("total"), Items.Num());
	S->SetBoolField(TEXT("complete"), bComplete);
	return S;
}

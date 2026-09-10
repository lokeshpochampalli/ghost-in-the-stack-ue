// Evaluator, trace, oracle, caps, diff and tier 4 acceptance tests.
#include "GitsTestUtil.h"

#if WITH_DEV_AUTOMATION_TESTS

using namespace GitsTest;

#define GITS_TEST(ClassName, PrettyName) \
	IMPLEMENT_SIMPLE_AUTOMATION_TEST(ClassName, PrettyName, GitsTest::TestFlags) \
	bool ClassName::RunTest(const FString&)

namespace
{
	FGitsValue ClockOracle(const FGitsWorldState& World, const FGitsQuery&)
	{
		const FGitsWorldValue* Clock = World.Find(GitsWorld::ClockKey);
		return FGitsValue::MakeInt((Clock && Clock->Kind == FGitsWorldValue::EKind::Number) ? (int64)Clock->Number : 0);
	}

	bool IsError(const FGitsTrace& T, EGitsDiagnosticCode Code) { return T.Outcome.Kind == EGitsOutcomeKind::Error && T.Outcome.Diagnostic.Code == Code; }
}

GITS_TEST(FGitsPythonSemantics, "GhostInTheStack.Interpreter.Evaluator.PythonSemantics")
{
	TestEqual(TEXT("4 / 2"), Shows(TEXT("4 / 2")), FString(TEXT("2.0")));
	TestEqual(TEXT("7 / 2"), Shows(TEXT("7 / 2")), FString(TEXT("3.5")));
	TestEqual(TEXT("1 / 3"), Shows(TEXT("1 / 3")), FString(TEXT("0.3333333333333333")));
	TestEqual(TEXT("7 // 2"), Shows(TEXT("7 // 2")), FString(TEXT("3")));
	TestEqual(TEXT("-7 // 2"), Shows(TEXT("-7 // 2")), FString(TEXT("-4")));
	TestEqual(TEXT("7 // -2"), Shows(TEXT("7 // -2")), FString(TEXT("-4")));
	TestEqual(TEXT("-7 // -2"), Shows(TEXT("-7 // -2")), FString(TEXT("3")));
	TestEqual(TEXT("7.0 // 2"), Shows(TEXT("7.0 // 2")), FString(TEXT("3.0")));
	TestEqual(TEXT("-7 % 3"), Shows(TEXT("-7 % 3")), FString(TEXT("2")));
	TestEqual(TEXT("7 % 3"), Shows(TEXT("7 % 3")), FString(TEXT("1")));
	TestEqual(TEXT("7 % -3"), Shows(TEXT("7 % -3")), FString(TEXT("-2")));
	TestEqual(TEXT("-7 % -3"), Shows(TEXT("-7 % -3")), FString(TEXT("-1")));
	TestEqual(TEXT("-2 ** 2"), Shows(TEXT("-2 ** 2")), FString(TEXT("-4")));
	TestEqual(TEXT("(-2) ** 2"), Shows(TEXT("(-2) ** 2")), FString(TEXT("4")));
	TestEqual(TEXT("2 ** -1"), Shows(TEXT("2 ** -1")), FString(TEXT("0.5")));
	TestEqual(TEXT("2 ** 3"), Shows(TEXT("2 ** 3")), FString(TEXT("8")));
	TestEqual(TEXT("range(1, 4)"), Join(Printed(TEXT("for i in range(1, 4):\n    print(i)\n"))), FString(TEXT("1,2,3")));
	TestEqual(TEXT("range(3)"), Join(Printed(TEXT("for i in range(3):\n    print(i)\n"))), FString(TEXT("0,1,2")));
	TestEqual(TEXT("range(0, 6, 2)"), Join(Printed(TEXT("for i in range(0, 6, 2):\n    print(i)\n"))), FString(TEXT("0,2,4")));
	TestEqual(TEXT("range(3, 0, -1)"), Join(Printed(TEXT("for i in range(3, 0, -1):\n    print(i)\n"))), FString(TEXT("3,2,1")));
	TestEqual(TEXT("strings immutable"), Join(Printed(TEXT("a = \"cold\"\nb = a + \" store\"\nprint(a)\nprint(b)\n"))), FString(TEXT("cold,cold store")));
	TestEqual(TEXT("assignment does not link"), Join(Printed(TEXT("a = 1\nb = a\na = 2\nprint(b)\n"))), FString(TEXT("1")));
	TestEqual(TEXT("list binds the same list"), Join(Printed(TEXT("a = [1]\nb = a\na.append(2)\nprint(b)\n"))), FString(TEXT("[1, 2]")));
	TestEqual(TEXT("append returns None"), Join(Printed(TEXT("xs = [1]\nprint(xs.append(2))\nprint(xs)\n"))), FString(TEXT("None,[1, 2]")));
	TestEqual(TEXT("+="), Join(Printed(TEXT("x = 1\nx += 1\nprint(x)\n"))), FString(TEXT("2")));
	TestEqual(TEXT("float prints point"), Join(Printed(TEXT("print(4 / 2)\nprint(4 // 2)\n"))), FString(TEXT("2.0,2")));
	TestEqual(TEXT("print several"), Join(Printed(TEXT("print(\"depth\", 4, True)\n"))), FString(TEXT("depth 4 True")));
	TestEqual(TEXT("print nothing"), Join(Printed(TEXT("print()\n"))), FString(TEXT("")));
	TestEqual(TEXT("quotes inside a list"), Join(Printed(TEXT("print(\"a\")\nprint([\"a\"])\n"))), FString(TEXT("a,['a']")));
	TestEqual(TEXT("None True False"), Join(Printed(TEXT("print(None)\nprint(True)\nprint(False)\n"))), FString(TEXT("None,True,False")));
	TestEqual(TEXT("concat"), Shows(TEXT("\"cold\" + \" store\"")), FString(TEXT("cold store")));
	TestEqual(TEXT("repeat"), Shows(TEXT("\"-\" * 3")), FString(TEXT("---")));
	TestEqual(TEXT("repeat reversed"), Shows(TEXT("3 * \"-\"")), FString(TEXT("---")));
	TestEqual(TEXT("True + 1"), Shows(TEXT("True + 1")), FString(TEXT("2")));
	TestEqual(TEXT("False * 5"), Shows(TEXT("False * 5")), FString(TEXT("0")));
	TestEqual(TEXT("text order"), Shows(TEXT("\"apple\" < \"beacon\"")), FString(TEXT("True")));
	TestEqual(TEXT("text order 2"), Shows(TEXT("\"b\" < \"a\"")), FString(TEXT("False")));
	TestEqual(TEXT("2 == 2.0"), Shows(TEXT("2 == 2.0")), FString(TEXT("True")));
	TestEqual(TEXT("2 == \"2\""), Shows(TEXT("2 == \"2\"")), FString(TEXT("False")));
	TestEqual(TEXT("short-circuit and"), Shows(TEXT("False and undefined_name")), FString(TEXT("False")));
	TestEqual(TEXT("short-circuit or"), Shows(TEXT("True or undefined_name")), FString(TEXT("True")));
	TestEqual(TEXT("or returns operand"), Shows(TEXT("0 or \"fallback\"")), FString(TEXT("fallback")));
	TestEqual(TEXT("and returns operand"), Shows(TEXT("\"a\" and \"b\"")), FString(TEXT("b")));
	const TCHAR* Truth[][2] = { { TEXT("0"), TEXT("False") }, { TEXT("0.0"), TEXT("False") }, { TEXT("\"\""), TEXT("False") }, { TEXT("None"), TEXT("False") },
		{ TEXT("False"), TEXT("False") }, { TEXT("[]"), TEXT("False") }, { TEXT("1"), TEXT("True") }, { TEXT("\"a\""), TEXT("True") }, { TEXT("[0]"), TEXT("True") } };
	for (const auto& T : Truth) { TestEqual(*FString::Printf(TEXT("bool(%s)"), T[0]), Shows(FString::Printf(TEXT("bool(%s)"), T[0])), FString(T[1])); }
	TestEqual(TEXT("int(3.9)"), Shows(TEXT("int(3.9)")), FString(TEXT("3")));
	TestEqual(TEXT("int(-3.9)"), Shows(TEXT("int(-3.9)")), FString(TEXT("-3")));
	TestEqual(TEXT("int(\"42\")"), Shows(TEXT("int(\"42\")")), FString(TEXT("42")));
	TestEqual(TEXT("float(\"2.5\")"), Shows(TEXT("float(\"2.5\")")), FString(TEXT("2.5")));
	{
		FGitsTrace T = Exec(TEXT("x = int(\"cold\")\n"));
		TestTrue(TEXT("int of text is a type mismatch"), IsError(T, EGitsDiagnosticCode::TypeMismatch) && T.Outcome.Diagnostic.What.Contains(TEXT("cold")));
	}
	TestEqual(TEXT("if/else a"), Join(Printed(TEXT("if 1 < 2:\n    print(\"a\")\nelse:\n    print(\"b\")\n"))), FString(TEXT("a")));
	TestEqual(TEXT("if/else b"), Join(Printed(TEXT("if 1 > 2:\n    print(\"a\")\nelse:\n    print(\"b\")\n"))), FString(TEXT("b")));
	TestEqual(TEXT("elif chain"), Join(Printed(TEXT("x = 2\nif x == 1:\n    print(\"one\")\nelif x == 2:\n    print(\"two\")\nelif x == 2:\n    print(\"again\")\nelse:\n    print(\"other\")\n"))), FString(TEXT("two")));
	TestEqual(TEXT("while"), Join(Printed(TEXT("i = 0\nwhile i < 3:\n    print(i)\n    i += 1\n"))), FString(TEXT("0,1,2")));
	TestEqual(TEXT("while false"), Join(Printed(TEXT("while False:\n    print(\"never\")\nprint(\"after\")\n"))), FString(TEXT("after")));
	TestEqual(TEXT("for list"), Join(Printed(TEXT("for name in [\"a\", \"b\"]:\n    print(name)\n"))), FString(TEXT("a,b")));
	TestEqual(TEXT("break"), Join(Printed(TEXT("for i in range(5):\n    if i == 2:\n        break\n    print(i)\n"))), FString(TEXT("0,1")));
	TestEqual(TEXT("continue"), Join(Printed(TEXT("for i in range(4):\n    if i == 1:\n        continue\n    print(i)\n"))), FString(TEXT("0,2,3")));
	TestEqual(TEXT("inner break"), Join(Printed(TEXT("for i in range(2):\n    for j in range(3):\n        if j == 1:\n            break\n        print(i)\n"))), FString(TEXT("0,1")));
	TestEqual(TEXT("nested count"), Join(Printed(TEXT("total = 0\nfor i in range(3):\n    for j in range(4):\n        total += 1\nprint(total)\n"))), FString(TEXT("12")));
	TestEqual(TEXT("index"), Join(Printed(TEXT("xs = [10, 20, 30]\nprint(xs[0])\nprint(xs[-1])\n"))), FString(TEXT("10,30")));
	{
		FGitsTrace T = Exec(TEXT("xs = [1, 2]\nprint(xs[5])\n"));
		TestTrue(TEXT("index out of range"), IsError(T, EGitsDiagnosticCode::IndexOutOfRange) && T.Outcome.Diagnostic.What.Contains(TEXT("2 items")) && T.Outcome.Diagnostic.Check.Contains(TEXT("starts at 0")));
	}
	TestEqual(TEXT("len"), Join(Printed(TEXT("print(len([1, 2, 3]))\nprint(len(\"cold\"))\n"))), FString(TEXT("3,4")));
	TestEqual(TEXT("in"), Join(Printed(TEXT("print(2 in [1, 2])\nprint(5 in [1, 2])\n"))), FString(TEXT("True,False")));
	TestEqual(TEXT("string index"), Join(Printed(TEXT("print(\"cold\"[1])\n"))), FString(TEXT("o")));
	return true;
}

GITS_TEST(FGitsRuntimeErrors, "GhostInTheStack.Interpreter.Evaluator.RuntimeErrors")
{
	{
		FGitsTrace T = Exec(TEXT("print(depth)\n"));
		TestTrue(TEXT("name not defined"), IsError(T, EGitsDiagnosticCode::NameNotDefined) && T.Outcome.Diagnostic.What.Contains(TEXT("depth")) && T.Outcome.Diagnostic.Span.Start.Line == 1);
	}
	{
		FGitsTrace T = Exec(TEXT("depth = 4\nlabel = \"cold\"\nprint(depth + label)\n"));
		TestTrue(TEXT("type mismatch"), IsError(T, EGitsDiagnosticCode::TypeMismatch) && T.Outcome.Diagnostic.What.Contains(TEXT("add")) && T.Outcome.Diagnostic.What.Contains(TEXT("a piece of text")) && !T.Outcome.Diagnostic.What.Contains(TEXT("TypeError")));
	}
	for (const TCHAR* Op : { TEXT("/"), TEXT("//"), TEXT("%") })
	{
		FGitsTrace T = Exec(FString::Printf(TEXT("print(1 %s 0)\n"), Op));
		TestTrue(*FString::Printf(TEXT("division by zero for %s"), Op), IsError(T, EGitsDiagnosticCode::DivisionByZero));
	}
	{
		FGitsTrace T = Exec(TEXT("a = 1\nb = 2\nprint(missing)\n"));
		TestTrue(TEXT("keeps steps before failure"), T.Outcome.Kind == EGitsOutcomeKind::Error && T.Output.Num() == 0 && T.StatementCount == 2 && T.Steps.Num() > 2);
	}
	TestTrue(TEXT("range outside for"), IsError(Exec(TEXT("xs = range(3)\n")), EGitsDiagnosticCode::BadRange));
	TestTrue(TEXT("range step zero"), IsError(Exec(TEXT("for i in range(0, 5, 0):\n    print(i)\n")), EGitsDiagnosticCode::BadRange));
	TestTrue(TEXT("wrong argument count"), IsError(Exec(TEXT("print(len(1, 2))\n")), EGitsDiagnosticCode::WrongArgumentCount));
	{
		FGitsTrace T = Exec(TEXT("sqrt(4)\n"));
		TestTrue(TEXT("unknown function"), IsError(T, EGitsDiagnosticCode::UnknownFunction) && T.Outcome.Diagnostic.What.Contains(TEXT("sqrt()")));
	}
	TestTrue(TEXT("calling a number"), Exec(TEXT("x = 4\nprint(x(1))\n")).Outcome.Kind == EGitsOutcomeKind::Error);
	// run never throws
	for (const TCHAR* S : { TEXT("print(missing)\n"), TEXT("print(1 / 0)\n"), TEXT("xs = [1]\nprint(xs[9])\n"), TEXT("i = 0\nwhile True:\n    i += 1\n"), TEXT("") })
	{
		Exec(S, FGitsWorldState(), nullptr, 200);
	}
	return true;
}

GITS_TEST(FGitsCaps, "GhostInTheStack.Interpreter.Evaluator.Caps")
{
	{
		FGitsTrace T = Exec(TEXT("# the pump controller Ilse never finished\ni = 0\nwhile True:\n    i = i + 1\n"), FGitsWorldState(), nullptr, MAX_int32);
		TestTrue(TEXT("safety cap kind"), T.Outcome.Kind == EGitsOutcomeKind::SafetyCapExceeded);
		TestEqual(TEXT("safety cap value"), T.Outcome.Cap, GitsLimits::SafetyStepCap);
		TestEqual(TEXT("safety cap steps"), T.Steps.Num(), GitsLimits::SafetyStepCap);
		TestEqual(TEXT("safety cap line"), T.Outcome.Diagnostic.Span.Start.Line, 3);
		TestTrue(TEXT("safety cap check names line"), T.Outcome.Diagnostic.Check.Contains(TEXT("line 3")));
	}
	{
		FGitsTrace T = Exec(TEXT("i = 0\nwhile i < 5000:\n    i += 1\n"));
		TestTrue(TEXT("statement cap kind"), T.Outcome.Kind == EGitsOutcomeKind::StatementCapExceeded);
		TestEqual(TEXT("cap"), T.Outcome.Cap, GitsLimits::DefaultStatementCap);
		TestEqual(TEXT("count"), T.StatementCount, GitsLimits::DefaultStatementCap);
		TestTrue(TEXT("far from safety"), T.Steps.Num() < GitsLimits::SafetyStepCap / 2);
		TestTrue(TEXT("code"), T.Outcome.Diagnostic.Code == EGitsDiagnosticCode::StatementCapExceeded && T.Outcome.Diagnostic.Span.Start.Line == 2);
		TestTrue(TEXT("check"), T.Outcome.Diagnostic.Check.Contains(TEXT("has to change each time")));
	}
	{
		FGitsTrace T = Exec(TEXT("total = 0\nfor i in range(20):\n    for j in range(20):\n        total += 1\nprint(total)\n"));
		TestTrue(TEXT("20x20 completes"), T.Outcome.Kind == EGitsOutcomeKind::Completed && Join(T.Output) == TEXT("400"));
		TestTrue(TEXT("20x20 inside default"), T.StatementCount < GitsLimits::DefaultStatementCap && T.StatementCount > 800);
	}
	{
		FGitsTrace T = Exec(TEXT("i = 0\nwhile i < 100:\n    i += 1\n"), FGitsWorldState(), nullptr, 50);
		TestTrue(TEXT("per-level cap"), T.Outcome.Kind == EGitsOutcomeKind::StatementCapExceeded && T.StatementCount == 50);
	}
	{
		FGitsTrace T = Exec(TEXT("base = 4\nadjust = 2\nbase = base - adjust\n"));
		TArray<FString> Kinds; TArray<FString> Depths;
		for (const FGitsStep& S : T.Steps) { if (S.StmtIndex == 2) { Kinds.Add(GitsStepKindName(S.Kind)); Depths.Add(FString::FromInt(S.Depth)); } }
		TestEqual(TEXT("one step per node"), Join(Kinds), FString(TEXT("name,name,binop,assign")));
		TestEqual(TEXT("depth nesting"), Join(Depths), FString(TEXT("2,2,1,0")));
		TestEqual(TEXT("boundaries"), GitsTrace::StatementBoundaries(T).Num(), 3);
		for (int32 i = 0; i < T.Steps.Num(); ++i) { TestEqual(TEXT("consecutive index"), T.Steps[i].Index, i); }
	}
	return true;
}

GITS_TEST(FGitsOracle, "GhostInTheStack.Interpreter.Evaluator.Oracle")
{
	{
		FGitsTrace T = Exec(TEXT("t = read_sensor(\"clock\")\n"), FGitsWorldState(), FGitsWorldOracle(&ClockOracle));
		const FGitsStep* Read = T.Steps.FindByPredicate([](const FGitsStep& S) { return S.bHasOracleRead; });
		TestTrue(TEXT("read recorded"), Read && Read->OracleRead.Query.Kind == TEXT("sensor") && Read->OracleRead.Query.Id == TEXT("clock") && Read->OracleRead.Value.Kind == EGitsValueKind::Int && Read->OracleRead.Value.Int == 0);
	}
	{
		FGitsTrace T = Exec(TEXT("print(read_sensor(\"clock\"))\nwait(5)\nprint(read_sensor(\"clock\"))\n"), FGitsWorldState(), FGitsWorldOracle(&ClockOracle));
		TestEqual(TEXT("wait is an effect"), Join(T.Output), FString(TEXT("0,5")));
		TArray<FGitsEffect> Fx = GitsTrace::AllEffects(T);
		TestTrue(TEXT("one wait effect"), Fx.Num() == 1 && Fx[0].Kind == FGitsEffect::EKind::Wait && Fx[0].Ticks == 5);
	}
	{
		const FString Source = TEXT("wait(3)\na = read_sensor(\"clock\")\nwait(4)\nb = read_sensor(\"clock\")\nprint(a + b)\n");
		FGitsParseResult P = GitsParser::Parse(Source);
		FGitsTrace Original = GitsEvaluator::Run(P.Program, FGitsWorldState(), FGitsWorldOracle(&ClockOracle));
		FGitsTrace Replayed = GitsEvaluator::Run(P.Program, FGitsWorldState(), GitsTrace::ReplayOracle(Original));
		TestEqual(TEXT("replay output"), Join(Replayed.Output), Join(Original.Output));
		TestEqual(TEXT("replay length"), Replayed.Steps.Num(), Original.Steps.Num());
		TestEqual(TEXT("replay identical"), GitsTrace::Serialise(Replayed), GitsTrace::Serialise(Original));
	}
	{
		FGitsTrace T = Exec(TEXT("wait(3)\nprint(read_sensor(\"clock\"))\nwait(4)\n"), FGitsWorldState(), FGitsWorldOracle(&ClockOracle));
		TArray<int32> B = GitsTrace::StatementBoundaries(T);
		TestEqual(TEXT("world at boundary 0"), GitsTrace::WorldAt(T, B[0]).FindRef(GitsWorld::ClockKey).Number, 3.0);
		TestEqual(TEXT("world at boundary 1"), GitsTrace::WorldAt(T, B[1]).FindRef(GitsWorld::ClockKey).Number, 3.0);
		TestEqual(TEXT("world at end"), GitsTrace::WorldAt(T, T.Steps.Num() - 1).FindRef(GitsWorld::ClockKey).Number, 7.0);
		TestEqual(TEXT("world at 0 is initial"), GitsTrace::WorldAt(T, 0).Num(), 0);
	}
	{
		FGitsTrace T = Exec(TEXT("open_valve(\"a\")\nset_heater(3)\nwait(2)\nclose_valve(\"a\")\n"));
		FGitsWorldState W = GitsTrace::WorldAt(T, T.Steps.Num() - 1);
		TestTrue(TEXT("folded world"), W.Num() == 3 && W.FindRef(TEXT("valve.a")).Bool == false && W.FindRef(TEXT("heater")).Number == 3.0 && W.FindRef(GitsWorld::ClockKey).Number == 2.0);
		FGitsTrace T2 = Exec(TEXT("open_valve(\"a\")\nclose_valve(\"a\")\n"));
		TArray<int32> B = GitsTrace::StatementBoundaries(T2);
		TestTrue(TEXT("part-way world"), GitsTrace::WorldAt(T2, B[0]).FindRef(TEXT("valve.a")).Bool == true && GitsTrace::WorldAt(T2, B[1]).FindRef(TEXT("valve.a")).Bool == false);
		FGitsWorldState Initial; Initial.Add(TEXT("coldStoreTemp"), FGitsWorldValue::MakeNumber(8));
		FGitsTrace T3 = Exec(TEXT("wait(1)\n"), Initial);
		TestTrue(TEXT("starts from the level world"), GitsTrace::WorldAt(T3, T3.Steps.Num() - 1).FindRef(TEXT("coldStoreTemp")).Number == 8.0);
	}
	{
		const FString Text = GitsTrace::Serialise(Exec(TEXT("open_valve(\"a\")\nwait(1)\nlog(\"note\")\n")));
		TestTrue(TEXT("fx set"), Text.Contains(TEXT("fx set valve.a=true")));
		TestTrue(TEXT("fx wait"), Text.Contains(TEXT("fx wait 1")));
		TestTrue(TEXT("fx log"), Text.Contains(TEXT("fx log \"note\"")));
		TestTrue(TEXT("statement cap outcome"), GitsTrace::Serialise(Exec(TEXT("i = 0\nwhile True:\n    i += 1\n"), FGitsWorldState(), nullptr, 20)).Contains(TEXT("-- outcome: statement cap 20 exceeded at line 2")));
		TestTrue(TEXT("error outcome"), GitsTrace::Serialise(Exec(TEXT("x = 1\nprint(missing)\n"))).Contains(TEXT("-- outcome: error name-not-defined at line 2")));
		TestTrue(TEXT("multi-line span"), GitsTrace::Serialise(Exec(TEXT("if True:\n    x = 1\n"))).Contains(TEXT("L1-")));
	}
	return true;
}

GITS_TEST(FGitsDiff, "GhostInTheStack.Interpreter.Trace.Diff")
{
	const FString Base = TEXT("base = 4\nadjust = 2\nbase = base - adjust\nprint(base)\n");
	auto DiffOf = [](const FString& A, const FString& B) { return GitsTrace::Diff(Exec(A), Exec(B)); };
	{
		FGitsDiffResult R = DiffOf(Base, TEXT("base = 4\nadjust = 3\nbase = base - adjust\nprint(base)\n"));
		TestTrue(TEXT("changed constant"), R.bDiverged && R.AtBoundary == 1 && R.Kinds.Contains(EGitsDivergenceKind::Bindings) && FString::Join(R.Summary, TEXT(" ")).Contains(TEXT("adjust was 2, now 3")));
	}
	TestTrue(TEXT("changed output"), DiffOf(Base, TEXT("base = 4\nadjust = 2\nbase = base - adjust\nprint(\"done\")\n")).Kinds.Contains(EGitsDivergenceKind::Output));
	{
		FGitsDiffResult R = DiffOf(TEXT("open_valve(\"a\")\n"), TEXT("close_valve(\"a\")\n"));
		TestTrue(TEXT("changed effects"), R.bDiverged && R.AtBoundary == 0);
	}
	TestTrue(TEXT("changed length"), DiffOf(TEXT("for i in range(2):\n    print(i)\n"), TEXT("for i in range(3):\n    print(i)\n")).bDiverged);
	{
		FGitsDiffResult R = DiffOf(TEXT("x = 1\nif x > 0:\n    print(\"up\")\nelse:\n    print(\"down\")\n"), TEXT("x = -1\nif x > 0:\n    print(\"up\")\nelse:\n    print(\"down\")\n"));
		TestTrue(TEXT("branch flips"), R.bDiverged && R.AtBoundary == 0 && R.Kinds.Contains(EGitsDivergenceKind::Bindings));
	}
	TestFalse(TEXT("inserted line above"), DiffOf(Base, TEXT("# Ilse: checked this on day 41, it is fine\n") + Base).bDiverged);
	TestFalse(TEXT("whitespace edit"), DiffOf(TEXT("x = 1+2\nprint(x)\n"), TEXT("x = 1 + 2\nprint(x)\n")).bDiverged);
	TestTrue(TEXT("number kind"), DiffOf(TEXT("x = 4 / 2\n"), TEXT("x = 4 // 2\n")).Kinds.Contains(EGitsDivergenceKind::Bindings));
	TestTrue(TEXT("now fails"), DiffOf(TEXT("x = 1\nprint(x)\n"), TEXT("x = 1\nprint(y)\n")).bDiverged);
	{
		FGitsDiffResult R = DiffOf(Base, TEXT("base = 5\nadjust = 2\nbase = base - adjust\nprint(base)\n"));
		for (const FString& L : R.Summary) { TestFalse(TEXT("no node ids"), L.Contains(TEXT("body.")) || L.Contains(TEXT("nodeId")) || L.Contains(TEXT("L1:"))); }
	}
	{
		FGitsTrace T = Exec(Base);
		TestFalse(TEXT("identical to itself"), GitsTrace::Diff(T, T).bDiverged);
	}
	TestTrue(TEXT("name appeared"), FString::Join(DiffOf(TEXT("a = 1\nb = 2\n"), TEXT("a = 1\nc = 2\n")).Summary, TEXT(" ")).Contains(TEXT("c now exists")));
	TestFalse(TEXT("empty programs"), DiffOf(TEXT(""), TEXT("")).bDiverged);
	TestTrue(TEXT("empty vs not"), DiffOf(TEXT(""), TEXT("x = 1\n")).bDiverged);
	return true;
}

GITS_TEST(FGitsTier4, "GhostInTheStack.Interpreter.Evaluator.Tier4")
{
	TestEqual(TEXT("call"), Join(Printed(TEXT("def double(n):\n    return n * 2\n\nprint(double(4))\n"))), FString(TEXT("8")));
	TestEqual(TEXT("falls off"), Join(Printed(TEXT("def noisy():\n    print(\"here\")\n\nprint(noisy())\n"))), FString(TEXT("here,None")));
	TestEqual(TEXT("bare return"), Join(Printed(TEXT("def f():\n    return\n\nprint(f())\n"))), FString(TEXT("None")));
	TestEqual(TEXT("stops at return"), Join(Printed(TEXT("def f():\n    return 1\n    print(\"never\")\n\nprint(f())\n"))), FString(TEXT("1")));
	TestEqual(TEXT("return out of loop"), Join(Printed(TEXT("def first_over(limit):\n    for i in range(10):\n        if i > limit:\n            return i\n    return -1\n\nprint(first_over(3))\n"))), FString(TEXT("4")));
	TestEqual(TEXT("nested calls"), Join(Printed(TEXT("def double(n):\n    return n * 2\n\ndef quad(n):\n    return double(double(n))\n\nprint(quad(3))\n"))), FString(TEXT("12")));
	{
		FGitsTrace T = Exec(TEXT("def f(a, b):\n    return a\n\nprint(f(1))\n"));
		TestTrue(TEXT("arg count by name"), IsError(T, EGitsDiagnosticCode::WrongArgumentCount) && T.Outcome.Diagnostic.What.Contains(TEXT("f()")));
	}
	TestEqual(TEXT("shadow builtin"), Join(Printed(TEXT("def len(x):\n    return 99\n\nprint(len(\"abc\"))\n"))), FString(TEXT("99")));
	{
		FGitsTrace T = Exec(TEXT("def f():\n    secret = 1\n\nf()\nprint(secret)\n"));
		TestTrue(TEXT("no leak"), IsError(T, EGitsDiagnosticCode::NameNotDefined) && T.Outcome.Diagnostic.What.Contains(TEXT("secret")));
	}
	TestEqual(TEXT("reads module name"), Join(Printed(TEXT("limit = 5\n\ndef over():\n    return limit + 1\n\nprint(over())\n"))), FString(TEXT("6")));
	TestEqual(TEXT("binds locally"), Join(Printed(TEXT("count = 1\n\ndef bump():\n    count = 99\n    return count\n\nprint(bump())\nprint(count)\n"))), FString(TEXT("99,1")));
	TestEqual(TEXT("own frame"), Join(Printed(TEXT("def tag(n):\n    label = n * 2\n    return label\n\nprint(tag(1))\nprint(tag(5))\n"))), FString(TEXT("2,10")));
	TestTrue(TEXT("no sibling locals"), IsError(Exec(TEXT("def outer():\n    here = 1\n    return inner()\n\ndef inner():\n    return here\n\nprint(outer())\n")), EGitsDiagnosticCode::NameNotDefined));
	const FString Factorial = TEXT("# ilse: the pulse ladder. it calls itself, which took me a week to believe.\ndef ladder(n):\n    if n <= 1:\n        return 1\n    return n * ladder(n - 1)\n\nprint(ladder(4))\n");
	{
		FGitsTrace T = Exec(Factorial);
		TestEqual(TEXT("recursive result"), Join(T.Output), FString(TEXT("24")));
		int32 Deepest = 0, PeakAt = 0;
		for (int32 i = 0; i < T.Steps.Num(); ++i) { if (T.Steps[i].Frames.Num() > Deepest) { Deepest = T.Steps[i].Frames.Num(); PeakAt = i; } }
		TestEqual(TEXT("deepest"), Deepest, 5);
		TestEqual(TEXT("starts at 1"), T.Steps[0].Frames.Num(), 1);
		TestEqual(TEXT("ends at 1"), T.Steps.Last().Frames.Num(), 1);
		TArray<FString> Names; for (const auto& F : T.Steps[PeakAt].Frames) { Names.Add(F.FunctionName); }
		TestEqual(TEXT("frame names"), Join(Names), FString(TEXT("<module>,ladder,ladder,ladder,ladder")));
		TArray<FString> Ns; for (int32 i = 1; i < T.Steps[PeakAt].Frames.Num(); ++i) { const FGitsValue* V = T.Steps[PeakAt].Frames[i].Find(TEXT("n")); Ns.Add(V ? FString::Printf(TEXT("%lld"), V->Int) : TEXT("?")); }
		TestEqual(TEXT("each frame its own n"), Join(Ns), FString(TEXT("4,3,2,1")));
		TestTrue(TEXT("module has no call site"), !T.Steps[PeakAt].Frames[0].bHasCallSite);
		TestEqual(TEXT("recursive call site line"), T.Steps[PeakAt].Frames[3].CallSite.Start.Line, 5);
	}
	{
		FGitsTrace T = Exec(TEXT("def forever(n):\n    return forever(n + 1)\n\nprint(forever(1))\n"), FGitsWorldState(), nullptr, MAX_int32);
		TestTrue(TEXT("call depth"), IsError(T, EGitsDiagnosticCode::CallDepthExceeded) && T.Outcome.Diagnostic.What.Contains(TEXT("100")) && T.Outcome.Diagnostic.Check.Contains(TEXT("returns without calling again")));
		TestTrue(TEXT("keeps the climb"), T.Steps.Num() > 100);
		int32 Deepest = 0; for (const auto& S : T.Steps) { Deepest = FMath::Max(Deepest, S.Frames.Num()); }
		TestEqual(TEXT("max frames"), Deepest, GitsLimits::MaxCallDepth);
	}
	TestTrue(TEXT("terminating recursion"), Exec(TEXT("def down(n):\n    if n <= 0:\n        return 0\n    return down(n - 1)\n\nprint(down(50))\n")).Outcome.Kind == EGitsOutcomeKind::Completed);
	Exec(TEXT("def forever():\n    return forever()\n\nforever()\n"), FGitsWorldState(), nullptr, MAX_int32);
	TestEqual(TEXT("function prints"), Join(Printed(TEXT("def f():\n    return 1\n\nprint(f)\n"))), FString(TEXT("<function f>")));
	{
		FGitsTrace T = Exec(TEXT("def f():\n    return 1\n\nprint(f())\n"));
		const FGitsStep& Def = GitsTrace::At(T, GitsTrace::StatementBoundaries(T)[0]);
		TestTrue(TEXT("def step"), Def.Kind == EGitsStepKind::Def && Def.Label == TEXT("def f()"));
	}
	{
		FGitsParseOptions O; O.Tier = EGitsTier::Four;
		FGitsParseResult P = GitsParser::Parse(TEXT("def f():\n    return 7\n\nprint(f())\n"), O);
		TestEqual(TEXT("tier 4 clean"), P.Diagnostics.Num(), 0);
		TestEqual(TEXT("runs"), Join(GitsEvaluator::Run(P.Program, FGitsWorldState(), FGitsWorldOracle(&GitsWorld::NullOracle)).Output), FString(TEXT("7")));
	}
	return true;
}

GITS_TEST(FGitsValues, "GhostInTheStack.Interpreter.Values")
{
	TestEqual(TEXT("float 2"), GitsValue::Display(FGitsValue::MakeFloat(2)), FString(TEXT("2.0")));
	TestEqual(TEXT("float 2.5"), GitsValue::Display(FGitsValue::MakeFloat(2.5)), FString(TEXT("2.5")));
	TestEqual(TEXT("float 1e16"), GitsValue::FormatFloat(1e16), FString(TEXT("1e+16")));
	TestEqual(TEXT("float 1e15"), GitsValue::FormatFloat(1e15), FString(TEXT("1000000000000000.0")));
	TestEqual(TEXT("float 0.0001"), GitsValue::FormatFloat(0.0001), FString(TEXT("0.0001")));
	TestEqual(TEXT("float 0.00001"), GitsValue::FormatFloat(0.00001), FString(TEXT("1e-05")));
	TestEqual(TEXT("float 0.1+0.2"), GitsValue::FormatFloat(0.1 + 0.2), FString(TEXT("0.30000000000000004")));
	TestEqual(TEXT("float -0.5"), GitsValue::FormatFloat(-0.5), FString(TEXT("-0.5")));
	TestEqual(TEXT("float 123.456"), GitsValue::FormatFloat(123.456), FString(TEXT("123.456")));
	TestEqual(TEXT("repr str"), GitsValue::Repr(FGitsValue::MakeStr(TEXT("a"))), FString(TEXT("'a'")));
	TestEqual(TEXT("display str"), GitsValue::Display(FGitsValue::MakeStr(TEXT("a"))), FString(TEXT("a")));
	TestTrue(TEXT("2 == 2.0"), GitsValue::Equal(FGitsValue::MakeInt(2), FGitsValue::MakeFloat(2)));
	TestTrue(TEXT("True == 1"), GitsValue::Equal(FGitsValue::MakeBool(true), FGitsValue::MakeInt(1)));
	TestFalse(TEXT("True != 2"), GitsValue::Equal(FGitsValue::MakeBool(true), FGitsValue::MakeInt(2)));
	TestFalse(TEXT("None != 0"), GitsValue::Equal(FGitsValue::MakeNone(), FGitsValue::MakeInt(0)));
	TestEqual(TEXT("type name"), GitsValue::TypeName(FGitsValue::MakeNone()), FString(TEXT("nothing")));
	{
		TSharedPtr<FGitsList> Inner = MakeShared<FGitsList>(); Inner->Id = 2; Inner->Items.Add(FGitsValue::MakeInt(1));
		TSharedPtr<FGitsList> Outer = MakeShared<FGitsList>(); Outer->Id = 1; Outer->Items.Add(FGitsValue::MakeList(Inner));
		FGitsValue Copy = GitsValue::Copy(FGitsValue::MakeList(Outer));
		Inner->Items.Add(FGitsValue::MakeInt(9));
		TestEqual(TEXT("deep copy"), GitsValue::Display(Copy), FString(TEXT("[[1]]")));
		TestEqual(TEXT("copy keeps id"), Copy.List->Id, 1);
	}
	{
		FGitsWorldState W;
		W = GitsWorld::Reduce(W, FGitsEffect::MakeWait(2));
		TestEqual(TEXT("clock from unset"), W.FindRef(GitsWorld::ClockKey).Number, 2.0);
		W = GitsWorld::Reduce(W, FGitsEffect::MakeWait(5));
		TestEqual(TEXT("clock adds"), W.FindRef(GitsWorld::ClockKey).Number, 7.0);
		W = GitsWorld::Reduce(W, FGitsEffect::MakeLog(TEXT("a")));
		TestEqual(TEXT("log count"), W.FindRef(GitsWorld::LogCountKey).Number, 1.0);
		FGitsWorldState S; S.Add(GitsWorld::ClockKey, FGitsWorldValue::MakeString(TEXT("stopped")));
		TestEqual(TEXT("non-number clock ignored"), GitsWorld::Reduce(S, FGitsEffect::MakeWait(1)).FindRef(GitsWorld::ClockKey).Number, 1.0);
	}
	return true;
}

GITS_TEST(FGitsStationBuiltins, "GhostInTheStack.Interpreter.Evaluator.StationBuiltins")
{
	// The Phase 2 door script: a matching sensor opens the door, an off one holds it shut. The
	// door never decides; the effect in the trace is the only thing that moves it.
	const FString Source = TEXT("pressure = read_sensor(\"airlock\")\ntarget = 4\nif pressure == target:\n    log(\"opening\")\n    open_door(\"inner\")\nelse:\n    log(\"holding\")\n");
	auto Pressure4 = [](const FGitsWorldState&, const FGitsQuery& Q) { return Q.Id == TEXT("airlock") ? FGitsValue::MakeInt(4) : FGitsValue::MakeNone(); };
	auto Pressure5 = [](const FGitsWorldState&, const FGitsQuery& Q) { return Q.Id == TEXT("airlock") ? FGitsValue::MakeInt(5) : FGitsValue::MakeNone(); };
	FGitsWorldState Initial; Initial.Add(TEXT("door.inner"), FGitsWorldValue::MakeBool(false));
	{
		FGitsTrace T = Exec(Source, Initial, FGitsWorldOracle(Pressure4));
		TestEqual(TEXT("completes"), (int32)T.Outcome.Kind, (int32)EGitsOutcomeKind::Completed);
		TArray<FGitsEffect> Fx = GitsTrace::AllEffects(T);
		TestEqual(TEXT("log then set"), Fx.Num(), 2);
		TestTrue(TEXT("open_door sets door.inner"), Fx.Num() == 2 && Fx[1] == FGitsEffect::MakeSet(TEXT("door.inner"), FGitsWorldValue::MakeBool(true)));
		TestEqual(TEXT("world folds to open"), GitsTrace::WorldAt(T, T.Steps.Num() - 1).FindRef(TEXT("door.inner")).ToText(), FString(TEXT("true")));
	}
	{
		FGitsTrace T = Exec(Source, Initial, FGitsWorldOracle(Pressure5));
		TestEqual(TEXT("completes"), (int32)T.Outcome.Kind, (int32)EGitsOutcomeKind::Completed);
		TestEqual(TEXT("only the log"), GitsTrace::AllEffects(T).Num(), 1);
		TestEqual(TEXT("door stays shut"), GitsTrace::WorldAt(T, T.Steps.Num() - 1).FindRef(TEXT("door.inner")).ToText(), FString(TEXT("false")));
	}
	{
		FGitsTrace T = Exec(TEXT("close_door(\"inner\")\nset_light(\"corridor\", 7)\n"), FGitsWorldState(), FGitsWorldOracle());
		TArray<FGitsEffect> Fx = GitsTrace::AllEffects(T);
		TestTrue(TEXT("close_door"), Fx.Num() == 2 && Fx[0] == FGitsEffect::MakeSet(TEXT("door.inner"), FGitsWorldValue::MakeBool(false)));
		TestTrue(TEXT("set_light"), Fx.Num() == 2 && Fx[1] == FGitsEffect::MakeSet(TEXT("light.corridor"), FGitsWorldValue::MakeNumber(7.0)));
	}
	return true;
}

#endif

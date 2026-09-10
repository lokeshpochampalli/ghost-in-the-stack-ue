// Parser, recognition pass and diagnostics acceptance tests.
//
// One case per enumerated construct in docs/LANGUAGE-SPEC.md. The assertion is always
// the same: a NAMED message, never a generic syntax error.
#include "GitsTestUtil.h"
#include "Interpreter/GitsLexer.h"

#if WITH_DEV_AUTOMATION_TESTS

using namespace GitsTest;

#define GITS_TEST(ClassName, PrettyName) \
	IMPLEMENT_SIMPLE_AUTOMATION_TEST(ClassName, PrettyName, GitsTest::TestFlags) \
	bool ClassName::RunTest(const FString&)

namespace
{
	bool Contains(const TArray<FString>& Codes, const TCHAR* Code) { return Codes.Contains(FString(Code)); }
}

GITS_TEST(FGitsRecognitionCases, "GhostInTheStack.Interpreter.Parser.RecognitionCases")
{
	struct FCase { const TCHAR* Source; const TCHAR* Code; EGitsTier Tier; };
	const FCase Cases[] = {
		{ TEXT("readings = {\"cold\": 4, \"beacon\": 9}\n"), TEXT("excluded-dict-literal"), EGitsTier::Four },
		{ TEXT("ids = {1, 2, 3}\n"), TEXT("excluded-set-literal"), EGitsTier::Four },
		{ TEXT("ids = {}\n"), TEXT("excluded-set-literal"), EGitsTier::Four },
		{ TEXT("print(f\"depth {depth}\")\n"), TEXT("excluded-fstring"), EGitsTier::Four },
		{ TEXT("x = F'a'\n"), TEXT("excluded-fstring"), EGitsTier::Four },
		{ TEXT("import math\n"), TEXT("excluded-import"), EGitsTier::Four },
		{ TEXT("from math import pi\n"), TEXT("excluded-import"), EGitsTier::Four },
		{ TEXT("class Pump:\n    speed = 1\n"), TEXT("excluded-class"), EGitsTier::Four },
		{ TEXT("try:\n    x = 1\n"), TEXT("excluded-exception-handling"), EGitsTier::Four },
		{ TEXT("except:\n    x = 1\n"), TEXT("excluded-exception-handling"), EGitsTier::Four },
		{ TEXT("finally:\n    x = 1\n"), TEXT("excluded-exception-handling"), EGitsTier::Four },
		{ TEXT("raise ValueError\n"), TEXT("excluded-exception-handling"), EGitsTier::Four },
		{ TEXT("with open(\"log\") as f:\n    x = 1\n"), TEXT("excluded-with"), EGitsTier::Four },
		{ TEXT("double = lambda x: x * 2\n"), TEXT("excluded-lambda"), EGitsTier::Four },
		{ TEXT("xs = [i for i in range(3)]\n"), TEXT("excluded-comprehension"), EGitsTier::Four },
		{ TEXT("xs = (i for i in range(3))\n"), TEXT("excluded-comprehension"), EGitsTier::Four },
		{ TEXT("xs = {i for i in range(3)}\n"), TEXT("excluded-comprehension"), EGitsTier::Four },
		{ TEXT("point = (1, 2)\n"), TEXT("excluded-tuple"), EGitsTier::Four },
		{ TEXT("a, b = 1, 2\n"), TEXT("excluded-tuple"), EGitsTier::Four },
		{ TEXT("empty = ()\n"), TEXT("excluded-tuple"), EGitsTier::Four },
		{ TEXT("point = 1, 2\n"), TEXT("excluded-tuple"), EGitsTier::Four },
		{ TEXT("head = readings[0:2]\n"), TEXT("excluded-slicing"), EGitsTier::Four },
		{ TEXT("global depth\n"), TEXT("excluded-scope-declaration"), EGitsTier::Four },
		{ TEXT("nonlocal depth\n"), TEXT("excluded-scope-declaration"), EGitsTier::Four },
		{ TEXT("print(\"a\", sep=\"\")\n"), TEXT("excluded-keyword-argument"), EGitsTier::Four },
		{ TEXT("def report(depth=4):\n    print(depth)\n"), TEXT("excluded-default-argument"), EGitsTier::Four },
		{ TEXT("if 1 < x < 10:\n    print(1)\n"), TEXT("excluded-comparison-chaining"), EGitsTier::Four },
		{ TEXT("ok = 1 < x < 10\n"), TEXT("excluded-comparison-chaining"), EGitsTier::Four },
		{ TEXT("while True:\n    print(1)\n"), TEXT("not-yet-unlocked"), EGitsTier::Two },
		{ TEXT("readings.append(4)\n"), TEXT("method-not-available"), EGitsTier::Two },
		{ TEXT("x = readings.length\n"), TEXT("method-not-available"), EGitsTier::Four },
		{ TEXT("if x:\n\tprint(1)\n"), TEXT("tab-indentation"), EGitsTier::Four },
		{ TEXT("a = b = 1\n"), TEXT("excluded-multiple-assignment"), EGitsTier::Four },
		{ TEXT("pass\n"), TEXT("excluded-construct"), EGitsTier::Four },
		{ TEXT("del x\n"), TEXT("excluded-construct"), EGitsTier::Four },
		{ TEXT("assert x\n"), TEXT("excluded-construct"), EGitsTier::Four },
		{ TEXT("yield x\n"), TEXT("excluded-construct"), EGitsTier::Four },
		{ TEXT("@staticmethod\nx = 1\n"), TEXT("excluded-construct"), EGitsTier::Four },
		{ TEXT("if x is None:\n    print(1)\n"), TEXT("excluded-construct"), EGitsTier::Four },
		{ TEXT("x = pass\n"), TEXT("excluded-construct"), EGitsTier::Four },
		{ TEXT("print(*readings)\n"), TEXT("excluded-construct"), EGitsTier::Four },
		{ TEXT("print(**settings)\n"), TEXT("excluded-construct"), EGitsTier::Four },
		{ TEXT("print(1); print(2)\n"), TEXT("excluded-construct"), EGitsTier::Four },
		{ TEXT("a = 1; b = 2\n"), TEXT("excluded-construct"), EGitsTier::Four },
		{ TEXT("def outer():\n    def inner():\n        print(1)\n"), TEXT("excluded-construct"), EGitsTier::Four },
		{ TEXT("def f(*args):\n    print(1)\n"), TEXT("excluded-construct"), EGitsTier::Four },
		{ TEXT("x = return\n"), TEXT("not-yet-unlocked"), EGitsTier::Three },
	};
	for (const FCase& C : Cases)
	{
		const TArray<FString> Found = Codes(C.Source, C.Tier);
		TestTrue(*FString::Printf(TEXT("expected %s from: %s (got %s)"), C.Code, C.Source, *Join(Found)), Contains(Found, C.Code));
		TestFalse(*FString::Printf(TEXT("a generic error leaked from: %s (got %s)"), C.Source, *Join(Found)), Contains(Found, TEXT("unexpected-token")));
	}
	// Precise expectations from the reference suite.
	TestFalse(TEXT("dict is not also a set"), Contains(Codes(TEXT("readings = {\"cold\": 4}\n")), TEXT("excluded-set-literal")));
	TestFalse(TEXT("set is not also a dict"), Contains(Codes(TEXT("ids = {1, 2, 3}\n")), TEXT("excluded-dict-literal")));
	TestFalse(TEXT("dict comprehension names the comprehension"), Contains(Codes(TEXT("xs = {k: 1 for k in ys}\n")), TEXT("excluded-dict-literal")));
	TestEqual(TEXT("a name ending in f is not a prefix"), Codes(TEXT("leaf = 1\nprintf = 2\n")).Num(), 0);
	TestEqual(TEXT("class consumes its body"), Join(Codes(TEXT("class Pump:\n    speed = 1\n"))), FString(TEXT("excluded-class")));
	TestEqual(TEXT("a multi-argument call is not a tuple"), Codes(TEXT("print(1, 2)\n")).Num(), 0);
	TestEqual(TEXT("grouping is not a tuple"), Codes(TEXT("x = (1 + 2) * 3\n")).Num(), 0);
	TestFalse(TEXT("slicing is not reported as locked"), Contains(Codes(TEXT("head = readings[0:2]\n")), TEXT("not-yet-unlocked")));
	TestEqual(TEXT("== inside a call is not a keyword argument"), Codes(TEXT("print(a == b)\n")).Num(), 0);
	TestEqual(TEXT("default at tier 4"), Join(Codes(TEXT("def report(depth=4):\n    print(depth)\n"), EGitsTier::Four)), FString(TEXT("excluded-default-argument")));
	{
		const TArray<FString> Found = Codes(TEXT("def report(depth=4):\n    print(depth)\n"), EGitsTier::Three);
		TestTrue(TEXT("default below tier 4 names the default"), Contains(Found, TEXT("excluded-default-argument")));
		TestTrue(TEXT("default below tier 4 names the locked def"), Contains(Found, TEXT("not-yet-unlocked")));
	}
	TestEqual(TEXT("no default, tier 4"), Codes(TEXT("def report(depth):\n    print(depth)\n"), EGitsTier::Four).Num(), 0);
	TestEqual(TEXT("no default, tier 3"), Join(Codes(TEXT("def report(depth):\n    print(depth)\n"), EGitsTier::Three)), FString(TEXT("not-yet-unlocked")));
	TestEqual(TEXT("a single comparison is fine"), Codes(TEXT("ok = 1 < x\n")).Num(), 0);
	TestEqual(TEXT("in at tier 2 is locked, not an error"), Join(Codes(TEXT("if x in readings:\n    print(1)\n"), EGitsTier::Two)), FString(TEXT("not-yet-unlocked")));
	TestEqual(TEXT("a locked block is consumed"), Join(Codes(TEXT("while x:\n    print(1)\n    print(2)\nprint(3)\n"), EGitsTier::Two)), FString(TEXT("not-yet-unlocked")));
	TestEqual(TEXT("a nested block under a locked construct is consumed"), Join(Codes(TEXT("while x:\n    if y:\n        print(1)\n    print(2)\nprint(3)\n"), EGitsTier::Two)), FString(TEXT("not-yet-unlocked")));
	return true;
}

GITS_TEST(FGitsRecognitionMessages, "GhostInTheStack.Interpreter.Parser.RecognitionMessages")
{
	{
		const TArray<FGitsDiagnostic> D = Diagnostics(TEXT("x = f\"a\"\n"));
		TestTrue(TEXT("f-string check suggests str("), D.Num() > 0 && D[0].Check.Contains(TEXT("str(")));
	}
	{
		const TArray<FGitsDiagnostic> D = Diagnostics(TEXT("global depth\n"));
		TestTrue(TEXT("global named"), D.Num() > 0 && D[0].What.Contains(TEXT("global")));
	}
	{
		const TArray<FGitsDiagnostic> D = Diagnostics(TEXT("ok = 1 < x < 10\n"));
		TestTrue(TEXT("chaining suggests and"), D.Num() > 0 && D[0].Check.Contains(TEXT("1 < x and x < 10")));
	}
	{
		const TArray<FGitsDiagnostic> D = Diagnostics(TEXT("while True:\n    print(1)\n"), EGitsTier::Two);
		TestTrue(TEXT("not-yet register"), D.Num() > 0 && D[0].What.Contains(TEXT("real Python")) && D[0].What.Contains(TEXT("tier 3")));
	}
	struct FGate { const TCHAR* Source; EGitsTier Level; const TCHAR* Tier; };
	const FGate Gates[] = {
		{ TEXT("while x:\n    print(1)\n"), EGitsTier::Two, TEXT("tier 3") },
		{ TEXT("for i in range(3):\n    print(i)\n"), EGitsTier::Two, TEXT("tier 3") },
		{ TEXT("total += 1\n"), EGitsTier::Two, TEXT("tier 3") },
		{ TEXT("xs = [1, 2]\n"), EGitsTier::Two, TEXT("tier 3") },
		{ TEXT("x = xs[0]\n"), EGitsTier::Two, TEXT("tier 3") },
		{ TEXT("x = a in xs\n"), EGitsTier::Two, TEXT("tier 3") },
		{ TEXT("def f():\n    print(1)\n"), EGitsTier::Three, TEXT("tier 4") },
		{ TEXT("return 1\n"), EGitsTier::Three, TEXT("tier 4") },
	};
	for (const FGate& G : Gates)
	{
		bool bFound = false;
		for (const FGitsDiagnostic& D : Diagnostics(G.Source, G.Level))
		{
			if (D.Code == EGitsDiagnosticCode::NotYetUnlocked) { bFound = true; TestTrue(*FString::Printf(TEXT("tier named for %s"), G.Source), D.What.Contains(G.Tier)); }
			TestFalse(TEXT("never says invalid"), D.What.ToLower().Contains(TEXT("invalid")) || D.What.ToLower().Contains(TEXT("syntax error")));
		}
		TestTrue(*FString::Printf(TEXT("locked: %s"), G.Source), bFound);
	}
	{
		bool bNamed = false;
		for (const FGitsDiagnostic& D : Diagnostics(TEXT("readings.append(4)\n"), EGitsTier::Two))
		{
			if (D.Code == EGitsDiagnosticCode::MethodNotAvailable && D.What.Contains(TEXT("append()"))) { bNamed = true; }
		}
		TestTrue(TEXT("method named with parentheses"), bNamed);
	}
	{
		const TArray<FGitsDiagnostic> D = Diagnostics(TEXT("if x:\n\tprint(1)\n"));
		TestTrue(TEXT("tab named"), D.Num() > 0 && D[0].Code == EGitsDiagnosticCode::TabIndentation && D[0].What.Contains(TEXT("tab")) && D[0].Check.Contains(TEXT("four spaces")));
	}
	return true;
}

GITS_TEST(FGitsCatalogue, "GhostInTheStack.Interpreter.Diagnostics.Catalogue")
{
	TestEqual(TEXT("46 codes"), GitsDiagnostics::CodeCount(), 46);
	const TCHAR* Jargon[] = { TEXT("syntaxerror"), TEXT("typeerror"), TEXT("nameerror"), TEXT("traceback"), TEXT("operand"), TEXT("token"), TEXT("ast"), TEXT("literal"), TEXT("identifier"), TEXT("statement expected") };
	for (int32 i = 0; i < GitsDiagnostics::CodeCount(); ++i)
	{
		const EGitsDiagnosticCode Code = (EGitsDiagnosticCode)i;
		const FGitsDiagnosticParams P(TEXT("x"), TEXT("3"));
		const FString Name = GitsDiagnostics::CodeName(Code);
		TestTrue(*(Name + TEXT(" summary")), !GitsDiagnostics::Summary(Code).IsEmpty());
		TestTrue(*(Name + TEXT(" what")), !GitsDiagnostics::What(Code, FGitsDiagnosticParams()).IsEmpty());
		TestTrue(*(Name + TEXT(" check")), !GitsDiagnostics::Check(Code, FGitsDiagnosticParams()).IsEmpty());
		const FString Text = (GitsDiagnostics::What(Code, P) + TEXT(" ") + GitsDiagnostics::Check(Code, P)).ToLower();
		for (const TCHAR* Word : Jargon)
		{
			// Whole words only: "least" contains "ast" and is not jargon.
			const FString W(Word);
			int32 At = Text.Find(W);
			bool bWhole = false;
			while (At != INDEX_NONE)
			{
				const bool bBefore = At == 0 || !FChar::IsAlnum(Text[At - 1]);
				const bool bAfter = At + W.Len() >= Text.Len() || !FChar::IsAlnum(Text[At + W.Len()]);
				if (bBefore && bAfter) { bWhole = true; break; }
				At = Text.Find(W, ESearchCase::IgnoreCase, ESearchDir::FromStart, At + 1);
			}
			TestFalse(*FString::Printf(TEXT("%s uses \"%s\""), *Name, Word), bWhole);
		}
		if (GitsDiagnostics::IsRecognitionCode(Code))
		{
			const FString What = GitsDiagnostics::What(Code, P);
			if (Code == EGitsDiagnosticCode::NotYetUnlocked) { TestTrue(*Name, What.Contains(TEXT("real Python"))); }
			else if (Code == EGitsDiagnosticCode::ExcludedComparisonChaining) { TestTrue(*Name, What.Contains(TEXT("Real Python allows that"))); }
			else if (Code != EGitsDiagnosticCode::MethodNotAvailable) { TestTrue(*Name, What.Contains(TEXT("not part of this station's system"))); }
		}
	}
	const FGitsDiagnostic D = Diagnostics(TEXT("if depth = 3:\n    print(1)\n"))[0];
	TestEqual(TEXT("formatted"), GitsDiagnostics::Format(D), FString(TEXT("Line 1: This condition uses one equals sign, which stores a value rather than comparing two.\nUse two equals signs to ask whether the values match. One equals sign means \"put this value under this name\".")));
	return true;
}

GITS_TEST(FGitsMalformedInput, "GhostInTheStack.Interpreter.Parser.MalformedInput")
{
	struct FCase { const TCHAR* Name; const TCHAR* Source; const TCHAR* Code; };
	const FCase Cases[] = {
		{ TEXT("a condition with no colon"), TEXT("if depth < 3\n    print(1)\n"), TEXT("missing-colon") },
		{ TEXT("an else with no colon"), TEXT("if x:\n    print(1)\nelse\n    print(2)\n"), TEXT("missing-colon") },
		{ TEXT("a colon with nothing indented under it"), TEXT("if depth < 3:\nprint(1)\n"), TEXT("expected-indented-block") },
		{ TEXT("an indent that opens nothing"), TEXT("base = 4\n    adjust = 2\n"), TEXT("unexpected-indent") },
		{ TEXT("a tab used to indent"), TEXT("if x:\n\tprint(1)\n"), TEXT("tab-indentation") },
		{ TEXT("indentation that lines up with nothing"), TEXT("if x:\n      print(1)\n"), TEXT("inconsistent-indentation") },
		{ TEXT("a piece of text with no closing quote"), TEXT("label = \"cold store\n"), TEXT("unterminated-string") },
		{ TEXT("an unclosed bracket"), TEXT("print(base_temp\n"), TEXT("unclosed-bracket") },
		{ TEXT("a stray character"), TEXT("depth = 4 ? 2\n"), TEXT("unexpected-character") },
		{ TEXT("digits running into letters"), TEXT("depth = 12abc\n"), TEXT("malformed-number") },
		{ TEXT("assigning to something that is not a name"), TEXT("print(x) = 4\n"), TEXT("invalid-assignment-target") },
		{ TEXT("one equals sign in a condition"), TEXT("if depth = 3:\n    print(1)\n"), TEXT("assignment-in-condition") },
		{ TEXT("a leading plus"), TEXT("depth = +4\n"), TEXT("leading-plus") },
		{ TEXT("an else with no if"), TEXT("else:\n    print(1)\n"), TEXT("elif-without-if") },
		{ TEXT("an elif with no if"), TEXT("elif x:\n    print(1)\n"), TEXT("elif-without-if") },
		{ TEXT("assigning to two names at once"), TEXT("a = b = 1\n"), TEXT("excluded-multiple-assignment") },
		{ TEXT("two statements on one line"), TEXT("a = 1; b = 2\n"), TEXT("excluded-construct") },
		{ TEXT("an operator with nothing after it"), TEXT("depth = 4 +\n"), TEXT("unexpected-token") },
		{ TEXT("a bare equals sign"), TEXT("= 5\n"), TEXT("unexpected-token") },
		{ TEXT("return outside a function"), TEXT("return 1\n"), TEXT("return-outside-function") },
		{ TEXT("a def missing its colon"), TEXT("def f()\n    return 1\n"), TEXT("missing-colon") },
	};
	for (const FCase& C : Cases)
	{
		const TArray<FString> Found = Codes(C.Source);
		TestTrue(*FString::Printf(TEXT("%s: expected %s, got %s"), C.Name, C.Code, *Join(Found)), Contains(Found, C.Code));
		for (const FGitsDiagnostic& D : Diagnostics(C.Source))
		{
			TestTrue(TEXT("what is a sentence"), D.What.Len() > 10);
			TestTrue(TEXT("check is a sentence"), D.Check.Len() > 10);
			TestTrue(TEXT("line >= 1"), D.Span.Start.Line >= 1);
			TestTrue(TEXT("check differs from what"), D.Check != D.What);
		}
	}
	TestEqual(TEXT("return outside function is the only code"), Join(Codes(TEXT("return 1\n"))), FString(TEXT("return-outside-function")));
	{
		bool bFound = false;
		for (const FGitsDiagnostic& D : Diagnostics(TEXT("base = 4\nadjust = 2\nif base = 3:\n    print(1)\n")))
		{
			if (D.Code == EGitsDiagnosticCode::AssignmentInCondition) { bFound = true; TestEqual(TEXT("span line"), D.Span.Start.Line, 3); }
		}
		TestTrue(TEXT("assignment-in-condition found"), bFound);
	}
	return true;
}

GITS_TEST(FGitsRecovery, "GhostInTheStack.Interpreter.Parser.Recovery")
{
	TestEqual(TEXT("three broken lines, three codes"), Join(Codes(TEXT("a = \"one\nb = 12abc\nc = 1 ? 2\n"))), FString(TEXT("unterminated-string,malformed-number,unexpected-character")));
	{
		FGitsParseResult R = GitsParser::Parse(TEXT("a = 1\nb = = 2\nc = 3\n"));
		TestTrue(TEXT("reports"), R.Diagnostics.Num() > 0);
		TArray<FString> Names; for (const auto& S : R.Program.Root->Body) { Names.Add(S->Kind == EGitsNodeKind::Assign ? S->Target->Text : TEXT("?")); }
		TestEqual(TEXT("sound statements survive"), Join(Names), FString(TEXT("a,c")));
	}
	TestEqual(TEXT("does not swallow the next line"), Join(Codes(TEXT("a = lambda x: x\nb = {1, 2}\n"))), FString(TEXT("excluded-lambda,excluded-set-literal")));
	{
		FGitsParseResult R = GitsParser::Parse(TEXT("if x:\n    a = = 1\n    b = 2\nc = 3\n"));
		TestTrue(TEXT("reports"), R.Diagnostics.Num() > 0);
		TestEqual(TEXT("two statements"), R.Program.Root->Body.Num(), 2);
		TestTrue(TEXT("if kept with one body statement"), R.Program.Root->Body.Num() > 0 && R.Program.Root->Body[0]->Kind == EGitsNodeKind::If && R.Program.Root->Body[0]->Body.Num() == 1);
	}
	TestEqual(TEXT("no cascade"), Join(Codes(TEXT("if x:\n    print(1)\n\nwhile y:\n    print(2)\n"), EGitsTier::Two)), FString(TEXT("not-yet-unlocked")));
	{
		const TArray<FGitsDiagnostic> D = Diagnostics(TEXT("x = {1, 2}\ny = \"unclosed\nz = 1 < 2 < 3\n"));
		TArray<FString> Lines; TArray<FString> CodesFound;
		for (const auto& X : D) { Lines.Add(FString::FromInt(X.Span.Start.Line)); CodesFound.Add(GitsDiagnostics::CodeName(X.Code)); }
		TestEqual(TEXT("sorted lines"), Join(Lines), FString(TEXT("1,2,3")));
		TestEqual(TEXT("sorted codes"), Join(CodesFound), FString(TEXT("excluded-set-literal,unterminated-string,excluded-comparison-chaining")));
	}
	// Null sub-expressions never build a broken node.
	const TCHAR* Holes[] = { TEXT("x = a and {1, 2}\n"), TEXT("x = a or {1, 2}\n"), TEXT("x = not {1, 2}\n"), TEXT("x = 1 + {1, 2}\n"),
		TEXT("x = 1 * {1, 2}\n"), TEXT("x = -{1, 2}\n"), TEXT("x = 2 ** {1, 2}\n"), TEXT("x = 1 < {1, 2}\n") };
	for (const TCHAR* S : Holes)
	{
		FGitsParseResult R = GitsParser::Parse(S);
		TestTrue(*FString::Printf(TEXT("set named in %s"), S), Contains(GitsParser::CodeNames(R), TEXT("excluded-set-literal")));
		TestEqual(*FString::Printf(TEXT("nothing half-built from %s"), S), R.Program.Root->Body.Num(), 0);
	}
	{
		FGitsParseResult R = GitsParser::Parse(TEXT("if x:\n    a = 1\n        b = 2\n"));
		TestTrue(TEXT("over-indent reported"), Contains(GitsParser::CodeNames(R), TEXT("unexpected-indent")));
		TestEqual(TEXT("block kept"), R.Program.Root->Body.Num(), 1);
	}
	TestTrue(TEXT("def alone reports"), Codes(TEXT("def\n")).Num() > 0);
	GitsParser::Parse(TEXT("def f(a, b\n"));
	GitsParser::Parse(TEXT("def f(a, b\n    return 1\n"));
	TestTrue(TEXT("unclosed call recovers"), Contains(Codes(TEXT("print(1\n")), TEXT("unclosed-bracket")));
	GitsParser::Parse(TEXT("print(1\nx = 2\n"));
	TestTrue(TEXT("unclosed brace"), Contains(Codes(TEXT("x = {1, 2\n")), TEXT("unclosed-bracket")));
	TestTrue(TEXT("unclosed square"), Contains(Codes(TEXT("x = [1, 2\n")), TEXT("unclosed-bracket")));
	TestTrue(TEXT("unclosed group"), Contains(Codes(TEXT("x = (1 + 2\n")), TEXT("unclosed-bracket")));
	// The parser is total.
	const TCHAR* Nasty[] = { TEXT(""), TEXT("\n"), TEXT("    x = 1\n"), TEXT("if\n"), TEXT("if:\n"), TEXT("= 5\n"), TEXT(")))\n"), TEXT("print(((((\n"),
		TEXT("else:\n"), TEXT("\"\n"), TEXT("x = = = =\n"), TEXT("\t\tx\n"), TEXT("if x:\n"), TEXT("x = (\n") };
	for (const TCHAR* S : Nasty) { GitsParser::Parse(S); }
	FString Deep; for (int32 i = 0; i < 500; ++i) { Deep += TEXT("("); }
	GitsParser::Parse(Deep);
	FString Braces; for (int32 i = 0; i < 200; ++i) { Braces += TEXT("{"); } for (int32 i = 0; i < 200; ++i) { Braces += TEXT("}"); }
	GitsParser::Parse(Braces);
	FString Chain; for (int32 i = 0; i < 200; ++i) { Chain += TEXT("a = "); } Chain += TEXT("1\n");
	GitsParser::Parse(Chain);
	FString Quotes; for (int32 i = 0; i < 300; ++i) { Quotes += TEXT("\""); }
	GitsParser::Parse(Quotes);
	// Whitespace and comments only.
	const TCHAR* Empty[] = { TEXT(""), TEXT("\n\n\n"), TEXT("   \n   \n"), TEXT("# just a note\n"), TEXT("\n# note\n\n") };
	for (const TCHAR* S : Empty)
	{
		FGitsParseResult R = GitsParser::Parse(S);
		TestEqual(*FString::Printf(TEXT("no diagnostics for %s"), S), R.Diagnostics.Num(), 0);
		TestEqual(*FString::Printf(TEXT("empty program for %s"), S), R.Program.Root->Body.Num(), 0);
	}
	return true;
}

GITS_TEST(FGitsParserStructure, "GhostInTheStack.Interpreter.Parser.Structure")
{
	TestEqual(TEXT("int"), RenderOne(TEXT("4\n")), FString(TEXT("4")));
	TestEqual(TEXT("float"), RenderOne(TEXT("4.5\n")), FString(TEXT("4.5f")));
	TestEqual(TEXT("str"), RenderOne(TEXT("\"cold store\"\n")), FString(TEXT("\"cold store\"")));
	TestEqual(TEXT("assign"), RenderOne(TEXT("base_temp = base_temp - adjust\n")), FString(TEXT("base_temp = (base_temp - adjust)")));
	TestEqual(TEXT("precedence"), RenderOne(TEXT("1 + 2 * 3\n")), FString(TEXT("(1 + (2 * 3))")));
	TestEqual(TEXT("left assoc"), RenderOne(TEXT("1 - 2 - 3\n")), FString(TEXT("((1 - 2) - 3)")));
	TestEqual(TEXT("left assoc //%"), RenderOne(TEXT("9 // 4 % 2\n")), FString(TEXT("((9 // 4) % 2)")));
	TestEqual(TEXT("parens"), RenderOne(TEXT("(1 + 2) * 3\n")), FString(TEXT("((1 + 2) * 3)")));
	TestEqual(TEXT("** right assoc"), RenderOne(TEXT("2 ** 3 ** 2\n")), FString(TEXT("(2 ** (3 ** 2))")));
	TestEqual(TEXT("-2 ** 2"), RenderOne(TEXT("-2 ** 2\n")), FString(TEXT("(- (2 ** 2))")));
	TestEqual(TEXT("2 ** -1"), RenderOne(TEXT("2 ** -1\n")), FString(TEXT("(2 ** (- 1))")));
	TestEqual(TEXT("--5"), RenderOne(TEXT("--5\n")), FString(TEXT("(- (- 5))")));
	TestEqual(TEXT("calls"), RenderOne(TEXT("print(str(len(\"abc\")))\n")), FString(TEXT("print(str(len(\"abc\")))")));
	TestEqual(TEXT("trailing comma"), RenderOne(TEXT("print(1,)\n")), FString(TEXT("print(1)")));
	TestEqual(TEXT("no args"), RenderOne(TEXT("print()\n")), FString(TEXT("print()")));
	TestEqual(TEXT("cmp"), RenderOne(TEXT("a + 1 < b * 2\n")), FString(TEXT("((a + 1) < (b * 2))")));
	TestEqual(TEXT("or/and"), RenderOne(TEXT("a or b and c\n")), FString(TEXT("(a or (b and c))")));
	TestEqual(TEXT("and/or"), RenderOne(TEXT("a and b or c\n")), FString(TEXT("((a and b) or c)")));
	TestEqual(TEXT("not and"), RenderOne(TEXT("not a and b\n")), FString(TEXT("((not a) and b)")));
	TestEqual(TEXT("not =="), RenderOne(TEXT("not a == b\n")), FString(TEXT("(not (a == b))")));
	TestEqual(TEXT("and left assoc"), RenderOne(TEXT("a and b and c\n")), FString(TEXT("((a and b) and c)")));
	TestEqual(TEXT("if"), RenderOne(TEXT("if flag:\n    print(1)\n")), FString(TEXT("if flag {print(1)}")));
	TestEqual(TEXT("if else"), RenderOne(TEXT("if flag:\n    print(1)\nelse:\n    print(2)\n")), FString(TEXT("if flag {print(1)} else {print(2)}")));
	TestEqual(TEXT("nested"), RenderOne(TEXT("if a:\n    if b:\n        print(1)\n")), FString(TEXT("if a {if b {print(1)}}")));
	TestEqual(TEXT("several"), RenderOne(TEXT("if a:\n    x = 1\n    y = 2\n")), FString(TEXT("if a {x = 1; y = 2}")));
	TestEqual(TEXT("no trailing newline"), RenderOne(TEXT("base = 4")), FString(TEXT("base = 4")));
	TestEqual(TEXT("block without trailing newline"), RenderOne(TEXT("if x:\n    print(1)")), FString(TEXT("if x {print(1)}")));
	{
		FGitsParseResult R = GitsParser::Parse(TEXT("if a:\n    print(1)\nelif b:\n    print(2)\nelse:\n    print(3)\n"));
		const FGitsNodePtr Outer = R.Program.Root->Body[0];
		TestTrue(TEXT("outer if"), Outer->Kind == EGitsNodeKind::If && !Outer->bIsElif);
		TestTrue(TEXT("nested elif"), Outer->OrElse.Num() == 1 && Outer->OrElse[0]->Kind == EGitsNodeKind::If && Outer->OrElse[0]->bIsElif);
		TestEqual(TEXT("elif else"), Sexpr(Outer->OrElse[0]->OrElse[0]), FString(TEXT("print(3)")));
	}
	{
		FGitsParseResult R = GitsParser::Parse(TEXT("base_temp = 4\nprint(base_temp)\n"));
		TestEqual(TEXT("span end col"), R.Program.Root->Body[0]->Span.End.Column, 13);
		TestEqual(TEXT("span line 2"), R.Program.Root->Body[1]->Span.Start.Line, 2);
		TestEqual(TEXT("span end 16"), R.Program.Root->Body[1]->Span.End.Column, 16);
	}
	{
		FGitsParseResult R = GitsParser::Parse(TEXT("# cold store setpoint report\n# ilse, day 40\n\nbase_temp = 4\nprint(base_temp)  # should read 4\n"));
		TestEqual(TEXT("two statements"), R.Program.Root->Body.Num(), 2);
		TestEqual(TEXT("three comments"), R.Program.Comments.Num(), 3);
		TestTrue(TEXT("comment text"), R.Program.Comments.Num() == 3 && R.Program.Comments[0].Text == TEXT("cold store setpoint report") && !R.Program.Comments[2].bOwnLine);
	}
	{
		FGitsParseResult R = GitsParser::Parse(TEXT("def report(depth, label):\n    print(depth)\n"));
		TestTrue(TEXT("def"), R.Program.Root->Body.Num() == 1 && R.Program.Root->Body[0]->Kind == EGitsNodeKind::FunctionDef);
		TestEqual(TEXT("params"), Join(R.Program.Root->Body[0]->Params), FString(TEXT("depth,label")));
		TestEqual(TEXT("node id"), R.Program.Root->Body[0]->Body[0]->NodeId, FString(TEXT("body.0.body.0")));
	}
	// Tier gate in expressions.
	TestEqual(TEXT("if locked at tier 1"), Join(Codes(TEXT("if x:\n    print(1)\n"), EGitsTier::One)), FString(TEXT("not-yet-unlocked")));
	TestEqual(TEXT("and locked at tier 1"), Join(Codes(TEXT("x = a and b\n"), EGitsTier::One)), FString(TEXT("not-yet-unlocked")));
	TestEqual(TEXT("or locked at tier 1"), Join(Codes(TEXT("x = a or b\n"), EGitsTier::One)), FString(TEXT("not-yet-unlocked")));
	TestEqual(TEXT("not locked at tier 1"), Join(Codes(TEXT("x = not a\n"), EGitsTier::One)), FString(TEXT("not-yet-unlocked")));
	TestEqual(TEXT("comparison fine at tier 1"), Codes(TEXT("x = a < b\n"), EGitsTier::One).Num(), 0);
	TestEqual(TEXT("bool ops at tier 2"), Codes(TEXT("x = a and not b or c\n"), EGitsTier::Two).Num(), 0);
	TestEqual(TEXT("def at tier 4"), Codes(TEXT("def f():\n    return 1\n"), EGitsTier::Four).Num(), 0);
	TestTrue(TEXT("def at tier 3 locked"), Contains(Codes(TEXT("def f():\n    return 1\n"), EGitsTier::Three), TEXT("not-yet-unlocked")));
	return true;
}

GITS_TEST(FGitsLexerTests, "GhostInTheStack.Interpreter.Lexer")
{
	{
		FGitsLexResult L = GitsLexer::Tokenize(TEXT("4\n4.0\n0.5\n"));
		TArray<FString> Nums; for (const auto& T : L.Tokens) { if (T.Type == EGitsTokenType::Number) { Nums.Add(T.bIsFloat ? TEXT("f") : TEXT("i")); } }
		TestEqual(TEXT("int/float"), Join(Nums), FString(TEXT("i,f,f")));
	}
	{
		FGitsLexResult L = GitsLexer::Tokenize(TEXT("x = 4.\n"));
		const FGitsToken* N = L.Tokens.FindByPredicate([](const FGitsToken& T) { return T.Type == EGitsTokenType::Number; });
		TestTrue(TEXT("trailing point is a float"), N && N->bIsFloat && N->FloatValue == 4.0);
	}
	{
		FGitsLexResult L = GitsLexer::Tokenize(TEXT("x = \"a\\nb\\tc\\\\d\\\"e\"\n"));
		const FGitsToken* S = L.Tokens.FindByPredicate([](const FGitsToken& T) { return T.Type == EGitsTokenType::String; });
		TestTrue(TEXT("escapes"), S && S->StringValue == TEXT("a\nb\tc\\d\"e"));
	}
	{
		FGitsLexResult L = GitsLexer::Tokenize(TEXT("x = \"a\\qb\"\n"));
		const FGitsToken* S = L.Tokens.FindByPredicate([](const FGitsToken& T) { return T.Type == EGitsTokenType::String; });
		TestTrue(TEXT("unknown escape keeps backslash"), S && S->StringValue == TEXT("a\\qb"));
	}
	{
		FGitsLexResult L = GitsLexer::Tokenize(TEXT("depth = 40\n"));
		TestTrue(TEXT("span"), L.Tokens[0].Span.Start.Line == 1 && L.Tokens[0].Span.Start.Column == 0 && L.Tokens[0].Span.End.Column == 5);
		TestTrue(TEXT("number col"), L.Tokens[2].Span.Start.Column == 8);
	}
	{
		FGitsLexResult L = GitsLexer::Tokenize(TEXT("if x:\n    y = 1\nz = 2\n"));
		TArray<FString> Types;
		for (const auto& T : L.Tokens)
		{
			switch (T.Type)
			{
			case EGitsTokenType::Keyword: Types.Add(TEXT("KEYWORD")); break; case EGitsTokenType::Name: Types.Add(TEXT("NAME")); break;
			case EGitsTokenType::Op: Types.Add(TEXT("OP")); break; case EGitsTokenType::Number: Types.Add(TEXT("NUMBER")); break;
			case EGitsTokenType::Newline: Types.Add(TEXT("NEWLINE")); break; case EGitsTokenType::Indent: Types.Add(TEXT("INDENT")); break;
			case EGitsTokenType::Dedent: Types.Add(TEXT("DEDENT")); break; default: break;
			}
		}
		TestEqual(TEXT("indent structure"), Join(Types), FString(TEXT("KEYWORD,NAME,OP,NEWLINE,INDENT,NAME,OP,NUMBER,NEWLINE,DEDENT,NAME,OP,NUMBER,NEWLINE")));
	}
	{
		FGitsLexResult L = GitsLexer::Tokenize(TEXT("if x:\n    if y:\n        z = 1\n"));
		int32 In = 0, De = 0; for (const auto& T : L.Tokens) { if (T.Type == EGitsTokenType::Indent) { ++In; } if (T.Type == EGitsTokenType::Dedent) { ++De; } }
		TestTrue(TEXT("closes every block at EOF"), In == 2 && De == 2);
	}
	{
		FGitsLexResult L = GitsLexer::Tokenize(TEXT("if x:\n      y = 1\n"));
		TestTrue(TEXT("names expected column"), L.Diagnostics.Num() > 0 && L.Diagnostics[0].Code == EGitsDiagnosticCode::InconsistentIndentation
			&& L.Diagnostics[0].What.Contains(TEXT("6 spaces")) && L.Diagnostics[0].Check.Contains(TEXT("4 spaces")));
	}
	{
		FGitsLexResult L = GitsLexer::Tokenize(TEXT("if x:\n    y = 1\n\n        # ilse: this bit still bothers me\n    z = 2\n"));
		int32 In = 0, De = 0; for (const auto& T : L.Tokens) { if (T.Type == EGitsTokenType::Indent) { ++In; } if (T.Type == EGitsTokenType::Dedent) { ++De; } }
		TestTrue(TEXT("comment lines leave the indent stack alone"), L.Diagnostics.Num() == 0 && In == 1 && De == 1);
	}
	{
		FGitsLexResult L = GitsLexer::Tokenize(TEXT("# cold store setpoint report\nbase = 4  # ilse: four is the number\n"));
		TestTrue(TEXT("comments kept out of tokens"), !L.Tokens.ContainsByPredicate([](const FGitsToken& T) { return T.Text.Contains(TEXT("#")); }));
		TestTrue(TEXT("two comments"), L.Comments.Num() == 2 && L.Comments[0].bOwnLine && !L.Comments[1].bOwnLine && L.Comments[1].Text == TEXT("ilse: four is the number") && L.Comments[1].Span.Start.Line == 2);
	}
	{
		FGitsLexResult L = GitsLexer::Tokenize(TEXT("#no space after the hash\n"));
		TestTrue(TEXT("raw and text"), L.Comments.Num() == 1 && L.Comments[0].Raw == TEXT("#no space after the hash") && L.Comments[0].Text == TEXT("no space after the hash"));
	}
	{
		FGitsLexResult L = GitsLexer::Tokenize(TEXT("label = \"cold store\n"));
		TestTrue(TEXT("unterminated still tokenises"), L.Diagnostics.Num() == 1 && L.Diagnostics[0].Code == EGitsDiagnosticCode::UnterminatedString
			&& L.Tokens.ContainsByPredicate([](const FGitsToken& T) { return T.Type == EGitsTokenType::String; }));
	}
	{
		FGitsLexResult L = GitsLexer::Tokenize(TEXT("x = 1 ? 2\ny = 3\n"));
		TestTrue(TEXT("stray char"), L.Diagnostics.Num() == 1 && L.Diagnostics[0].Code == EGitsDiagnosticCode::UnexpectedCharacter && L.Diagnostics[0].What.Contains(TEXT("?")));
	}
	{
		FGitsLexResult L = GitsLexer::Tokenize(TEXT("print(\n    1,\n    2,\n)\n"));
		int32 Nl = 0, In = 0; for (const auto& T : L.Tokens) { if (T.Type == EGitsTokenType::Newline) { ++Nl; } if (T.Type == EGitsTokenType::Indent) { ++In; } }
		TestTrue(TEXT("newlines inside brackets are not logical"), Nl == 1 && In == 0);
	}
	const TCHAR* Nasty[] = { TEXT(""), TEXT("\n\n\n"), TEXT("    "), TEXT("\""), TEXT("'''"), TEXT("(((("), TEXT("#"), TEXT("\t\t"), TEXT("\\"), TEXT("}]{[") };
	for (const TCHAR* S : Nasty) { GitsLexer::Tokenize(S); }
	return true;
}

#endif

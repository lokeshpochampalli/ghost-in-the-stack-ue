// Shared helpers for the interpreter automation tests.
#pragma once

#include "CoreMinimal.h"
#include "Misc/AutomationTest.h"
#include "Interpreter/GitsEvaluator.h"
#include "Interpreter/GitsTrace.h"
#include "Interpreter/GitsDiagnostics.h"

#if WITH_DEV_AUTOMATION_TESTS

namespace GitsTest
{
	constexpr EAutomationTestFlags TestFlags = EAutomationTestFlags::EditorContext | EAutomationTestFlags::ProductFilter;

	/** Parses and runs. A parse diagnostic is reported as an error outcome; tests check for it. */
	inline FGitsTrace Exec(const FString& Source, const FGitsWorldState& World = FGitsWorldState(), FGitsWorldOracle Oracle = nullptr, int32 Cap = GitsLimits::DefaultStatementCap)
	{
		FGitsRunOptions O; O.StatementCap = Cap;
		return GitsEvaluator::RunSource(Source, World, Oracle ? Oracle : FGitsWorldOracle(&GitsWorld::NullOracle), O);
	}

	inline TArray<FString> Printed(const FString& Source) { return Exec(Source).Output; }

	/** A one-expression program's printed value. */
	inline FString Shows(const FString& Expression) { return FString::Join(Printed(TEXT("print(") + Expression + TEXT(")\n")), TEXT("")); }

	inline TArray<FString> Codes(const FString& Source, EGitsTier Tier = EGitsTier::Four)
	{
		FGitsParseOptions O; O.Tier = Tier;
		return GitsParser::CodeNames(GitsParser::Parse(Source, O));
	}

	inline TArray<FGitsDiagnostic> Diagnostics(const FString& Source, EGitsTier Tier = EGitsTier::Four)
	{
		FGitsParseOptions O; O.Tier = Tier;
		return GitsParser::Parse(Source, O).Diagnostics;
	}

	inline FString Join(const TArray<FString>& A) { return FString::Join(A, TEXT(",")); }

	/** A compact structural rendering of an expression: 1 + 2 * 3 renders as (1 + (2 * 3)). */
	inline FString Sexpr(const FGitsNodePtr& N)
	{
		if (!N.IsValid()) { return TEXT("?"); }
		switch (N->Kind)
		{
		case EGitsNodeKind::Num: return N->bIsFloat ? GitsValue::FormatFloat(N->FloatValue) + TEXT("f") : FString::Printf(TEXT("%lld"), N->IntValue);
		case EGitsNodeKind::Str: return GitsTrace::QuoteJson(N->StrValue);
		case EGitsNodeKind::Bool: return N->bBoolValue ? TEXT("True") : TEXT("False");
		case EGitsNodeKind::NoneLit: return TEXT("None");
		case EGitsNodeKind::Name: return N->Text;
		case EGitsNodeKind::BinOp: case EGitsNodeKind::BoolOp: case EGitsNodeKind::Compare:
			return TEXT("(") + Sexpr(N->Left) + TEXT(" ") + N->Text + TEXT(" ") + Sexpr(N->Right) + TEXT(")");
		case EGitsNodeKind::UnaryOp: return TEXT("(") + N->Text + TEXT(" ") + Sexpr(N->Operand) + TEXT(")");
		case EGitsNodeKind::Call:
		{
			TArray<FString> A; for (const auto& X : N->Args) { A.Add(Sexpr(X)); }
			return Sexpr(N->Func) + TEXT("(") + FString::Join(A, TEXT(", ")) + TEXT(")");
		}
		case EGitsNodeKind::Assign: return Sexpr(N->Target) + TEXT(" = ") + Sexpr(N->Value);
		case EGitsNodeKind::ExprStmt: return Sexpr(N->Value);
		case EGitsNodeKind::If:
		{
			TArray<FString> B; for (const auto& X : N->Body) { B.Add(Sexpr(X)); }
			TArray<FString> E; for (const auto& X : N->OrElse) { E.Add(Sexpr(X)); }
			FString Tail = E.Num() ? TEXT(" else {") + FString::Join(E, TEXT("; ")) + TEXT("}") : TEXT("");
			return TEXT("if ") + Sexpr(N->Test) + TEXT(" {") + FString::Join(B, TEXT("; ")) + TEXT("}") + Tail;
		}
		default: return TEXT("?");
		}
	}

	inline FString RenderOne(const FString& Source)
	{
		FGitsParseResult R = GitsParser::Parse(Source);
		if (R.Program.Root->Body.Num() != 1) { return FString::Printf(TEXT("<%d statements>"), R.Program.Root->Body.Num()); }
		return Sexpr(R.Program.Root->Body[0]);
	}
}

#endif

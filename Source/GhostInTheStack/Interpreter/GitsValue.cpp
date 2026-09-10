#include "GitsTypes.h"
#include <cmath>
#include <cstdio>
#include <cstdlib>

// --- FGitsValue --------------------------------------------------------------------

double FGitsValue::AsDouble() const
{
	switch (Kind)
	{
	case EGitsValueKind::Int: return (double)Int;
	case EGitsValueKind::Float: return Float;
	case EGitsValueKind::Bool: return Bool ? 1.0 : 0.0;
	default: return 0.0;
	}
}

int64 FGitsValue::AsInt() const
{
	switch (Kind)
	{
	case EGitsValueKind::Int: return Int;
	case EGitsValueKind::Float: return (int64)Float;
	case EGitsValueKind::Bool: return Bool ? 1 : 0;
	default: return 0;
	}
}

namespace GitsValue
{
	FString FormatFloat(double D)
	{
		if (std::isnan(D)) { return TEXT("nan"); }
		if (std::isinf(D)) { return D < 0 ? TEXT("-inf") : TEXT("inf"); }

		// Shortest digit string that round-trips, as Python's repr does.
		char Buffer[64];
		int32 Precision = 1;
		for (; Precision <= 17; ++Precision)
		{
			std::snprintf(Buffer, sizeof(Buffer), "%.*e", Precision - 1, D);
			if (std::strtod(Buffer, nullptr) == D) { break; }
		}
		// Buffer holds d.ddddde[+-]XX. Pull the pieces apart.
		FString Sci = ANSI_TO_TCHAR(Buffer);
		bool bNegative = Sci.StartsWith(TEXT("-"));
		if (bNegative) { Sci = Sci.Mid(1); }
		int32 EPos = Sci.Find(TEXT("e"));
		FString Mantissa = Sci.Left(EPos);
		int32 Exponent = FCString::Atoi(*Sci.Mid(EPos + 1));
		FString Digits = Mantissa.Replace(TEXT("."), TEXT(""));
		// Strip trailing zeros from the digit string (keep at least one digit).
		while (Digits.Len() > 1 && Digits.EndsWith(TEXT("0"))) { Digits.LeftChopInline(1); }

		FString Out;
		if (Exponent >= -4 && Exponent < 16)
		{
			if (Exponent >= 0)
			{
				// Digits before the point: Exponent + 1
				int32 IntDigits = Exponent + 1;
				FString IntPart, FracPart;
				if (Digits.Len() <= IntDigits)
				{
					IntPart = Digits + FString::ChrN(IntDigits - Digits.Len(), TEXT('0'));
					FracPart = TEXT("0");
				}
				else
				{
					IntPart = Digits.Left(IntDigits);
					FracPart = Digits.Mid(IntDigits);
				}
				Out = IntPart + TEXT(".") + FracPart;
			}
			else
			{
				Out = TEXT("0.") + FString::ChrN(-Exponent - 1, TEXT('0')) + Digits;
			}
		}
		else
		{
			FString M = Digits.Left(1);
			if (Digits.Len() > 1) { M += TEXT(".") + Digits.Mid(1); }
			Out = FString::Printf(TEXT("%se%c%02d"), *M, Exponent < 0 ? TEXT('-') : TEXT('+'), FMath::Abs(Exponent));
		}
		return bNegative ? TEXT("-") + Out : Out;
	}

	FString Display(const FGitsValue& V)
	{
		switch (V.Kind)
		{
		case EGitsValueKind::Int: return FString::Printf(TEXT("%lld"), V.Int);
		case EGitsValueKind::Float: return FormatFloat(V.Float);
		case EGitsValueKind::Str: return V.Str;
		case EGitsValueKind::Bool: return V.Bool ? TEXT("True") : TEXT("False");
		case EGitsValueKind::None: return TEXT("None");
		case EGitsValueKind::List:
		{
			FString Out = TEXT("[");
			if (V.List.IsValid())
			{
				for (int32 i = 0; i < V.List->Items.Num(); ++i)
				{
					if (i > 0) { Out += TEXT(", "); }
					Out += Repr(V.List->Items[i]);
				}
			}
			return Out + TEXT("]");
		}
		case EGitsValueKind::Function: return FString::Printf(TEXT("<function %s>"), *V.FunctionName);
		}
		return TEXT("");
	}

	FString Repr(const FGitsValue& V)
	{
		if (V.Kind == EGitsValueKind::Str) { return TEXT("'") + V.Str + TEXT("'"); }
		return Display(V);
	}

	FString TypeName(const FGitsValue& V)
	{
		switch (V.Kind)
		{
		case EGitsValueKind::Int: return TEXT("a whole number");
		case EGitsValueKind::Float: return TEXT("a decimal number");
		case EGitsValueKind::Str: return TEXT("a piece of text");
		case EGitsValueKind::Bool: return TEXT("a true/false value");
		case EGitsValueKind::None: return TEXT("nothing");
		case EGitsValueKind::List: return TEXT("a list");
		case EGitsValueKind::Function: return TEXT("a function");
		}
		return TEXT("a value");
	}

	bool Equal(const FGitsValue& A, const FGitsValue& B)
	{
		if (A.Kind == EGitsValueKind::List && B.Kind == EGitsValueKind::List)
		{
			const int32 NA = A.List.IsValid() ? A.List->Items.Num() : 0;
			const int32 NB = B.List.IsValid() ? B.List->Items.Num() : 0;
			if (NA != NB) { return false; }
			for (int32 i = 0; i < NA; ++i) { if (!Equal(A.List->Items[i], B.List->Items[i])) { return false; } }
			return true;
		}
		if (A.IsNumeric() && B.IsNumeric())
		{
			// int, float and bool compare across kinds, as they do in Python.
			if (A.Kind == EGitsValueKind::Int && B.Kind == EGitsValueKind::Int) { return A.Int == B.Int; }
			return A.AsDouble() == B.AsDouble();
		}
		if (A.Kind != B.Kind) { return false; }
		switch (A.Kind)
		{
		case EGitsValueKind::None: return true;
		case EGitsValueKind::Str: return A.Str == B.Str;
		case EGitsValueKind::Function: return A.FunctionNodeId == B.FunctionNodeId;
		default: return false;
		}
	}

	bool Truthy(const FGitsValue& V)
	{
		switch (V.Kind)
		{
		case EGitsValueKind::Int: return V.Int != 0;
		case EGitsValueKind::Float: return V.Float != 0.0;
		case EGitsValueKind::Str: return V.Str.Len() > 0;
		case EGitsValueKind::Bool: return V.Bool;
		case EGitsValueKind::None: return false;
		case EGitsValueKind::List: return V.List.IsValid() && V.List->Items.Num() > 0;
		case EGitsValueKind::Function: return true;
		}
		return false;
	}

	FGitsValue Copy(const FGitsValue& V)
	{
		if (V.Kind != EGitsValueKind::List || !V.List.IsValid()) { return V; }
		TSharedPtr<FGitsList> L = MakeShared<FGitsList>();
		L->Id = V.List->Id;
		L->Items.Reserve(V.List->Items.Num());
		for (const FGitsValue& Item : V.List->Items) { L->Items.Add(Copy(Item)); }
		return FGitsValue::MakeList(L);
	}
}

// --- world ---------------------------------------------------------------------------

FString FGitsWorldValue::ToText() const
{
	switch (Kind)
	{
	case EKind::Number:
	{
		if (std::isfinite(Number) && Number == std::floor(Number) && FMath::Abs(Number) < 1e15)
		{
			return FString::Printf(TEXT("%lld"), (int64)Number);
		}
		return GitsValue::FormatFloat(Number);
	}
	case EKind::String: return String;
	case EKind::Bool: return Bool ? TEXT("true") : TEXT("false");
	}
	return TEXT("");
}

bool FGitsWorldValue::operator==(const FGitsWorldValue& O) const
{
	if (Kind != O.Kind) { return false; }
	switch (Kind)
	{
	case EKind::Number: return Number == O.Number;
	case EKind::String: return String == O.String;
	case EKind::Bool: return Bool == O.Bool;
	}
	return false;
}

bool FGitsEffect::operator==(const FGitsEffect& O) const
{
	if (Kind != O.Kind) { return false; }
	switch (Kind)
	{
	case EKind::Set: return Key == O.Key && Value == O.Value;
	case EKind::Log: return Message == O.Message;
	case EKind::Wait: return Ticks == O.Ticks;
	}
	return false;
}

namespace GitsWorld
{
	const TCHAR* ClockKey = TEXT("clock");
	const TCHAR* LogCountKey = TEXT("log.count");

	FGitsWorldState Reduce(const FGitsWorldState& World, const FGitsEffect& Effect)
	{
		FGitsWorldState Out = World;
		switch (Effect.Kind)
		{
		case FGitsEffect::EKind::Set:
			Out.Add(Effect.Key, Effect.Value);
			break;
		case FGitsEffect::EKind::Wait:
		{
			const FGitsWorldValue* Clock = Out.Find(ClockKey);
			double Current = (Clock && Clock->Kind == FGitsWorldValue::EKind::Number) ? Clock->Number : 0.0;
			Out.Add(ClockKey, FGitsWorldValue::MakeNumber(Current + (double)Effect.Ticks));
			break;
		}
		case FGitsEffect::EKind::Log:
		{
			const FGitsWorldValue* Count = Out.Find(LogCountKey);
			double Current = (Count && Count->Kind == FGitsWorldValue::EKind::Number) ? Count->Number : 0.0;
			Out.Add(LogCountKey, FGitsWorldValue::MakeNumber(Current + 1.0));
			break;
		}
		}
		return Out;
	}

	FGitsWorldState ReduceAll(const FGitsWorldState& World, const TArray<FGitsEffect>& Effects)
	{
		FGitsWorldState Out = World;
		for (const FGitsEffect& E : Effects) { Out = Reduce(Out, E); }
		return Out;
	}

	FGitsValue NullOracle(const FGitsWorldState&, const FGitsQuery&) { return FGitsValue::MakeNone(); }
}

const TCHAR* GitsStepKindName(EGitsStepKind Kind)
{
	switch (Kind)
	{
	case EGitsStepKind::Def: return TEXT("def");
	case EGitsStepKind::Return: return TEXT("return");
	case EGitsStepKind::Literal: return TEXT("literal");
	case EGitsStepKind::Name: return TEXT("name");
	case EGitsStepKind::BinOp: return TEXT("binop");
	case EGitsStepKind::UnaryOp: return TEXT("unaryop");
	case EGitsStepKind::BoolOp: return TEXT("boolop");
	case EGitsStepKind::Compare: return TEXT("compare");
	case EGitsStepKind::List: return TEXT("list");
	case EGitsStepKind::Subscript: return TEXT("subscript");
	case EGitsStepKind::Call: return TEXT("call");
	case EGitsStepKind::Method: return TEXT("method");
	case EGitsStepKind::Assign: return TEXT("assign");
	case EGitsStepKind::AugAssign: return TEXT("augassign");
	case EGitsStepKind::Expr: return TEXT("expr");
	case EGitsStepKind::Branch: return TEXT("branch");
	case EGitsStepKind::Iterate: return TEXT("iterate");
	case EGitsStepKind::Break: return TEXT("break");
	case EGitsStepKind::Continue: return TEXT("continue");
	}
	return TEXT("?");
}

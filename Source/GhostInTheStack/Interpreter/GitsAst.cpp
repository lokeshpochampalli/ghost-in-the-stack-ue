#include "GitsAst.h"

namespace
{
	void Assign(const FGitsNodePtr& N, const FString& Id)
	{
		if (!N.IsValid()) { return; }
		N->NodeId = Id;
		auto Child = [&](const FGitsNodePtr& C, const TCHAR* Field) { if (C.IsValid()) { Assign(C, Id.IsEmpty() ? FString(Field) : Id + TEXT(".") + Field); } };
		auto Children = [&](const TArray<FGitsNodePtr>& Cs, const TCHAR* Field)
		{
			for (int32 i = 0; i < Cs.Num(); ++i)
			{
				const FString Base = Id.IsEmpty() ? FString(Field) : Id + TEXT(".") + Field;
				Assign(Cs[i], Base + TEXT(".") + FString::FromInt(i));
			}
		};
		Child(N->Left, TEXT("left"));
		Child(N->Right, TEXT("right"));
		Child(N->Operand, TEXT("operand"));
		Child(N->Func, TEXT("func"));
		Child(N->Receiver, TEXT("receiver"));
		Child(N->Value, TEXT("value"));
		Child(N->Index, TEXT("index"));
		Child(N->Target, TEXT("target"));
		Child(N->Test, TEXT("test"));
		Child(N->Iter, TEXT("iter"));
		Children(N->Args, TEXT("args"));
		Children(N->Elts, TEXT("elts"));
		Children(N->Body, TEXT("body"));
		Children(N->OrElse, TEXT("orelse"));
	}

	FString JsonString(const FString& S)
	{
		FString Out = TEXT("\"");
		for (TCHAR C : S)
		{
			switch (C)
			{
			case '"': Out += TEXT("\\\""); break;
			case '\\': Out += TEXT("\\\\"); break;
			case '\n': Out += TEXT("\\n"); break;
			case '\t': Out += TEXT("\\t"); break;
			case '\r': Out += TEXT("\\r"); break;
			default: Out.AppendChar(C); break;
			}
		}
		return Out + TEXT("\"");
	}
}

namespace GitsAst
{
	void AssignNodeIds(const FGitsNodePtr& Root) { Assign(Root, TEXT("")); }

	FString Unparse(const FGitsNodePtr& N)
	{
		if (!N.IsValid()) { return TEXT(""); }
		switch (N->Kind)
		{
		case EGitsNodeKind::Num: return N->Text;
		case EGitsNodeKind::Str: return JsonString(N->StrValue);
		case EGitsNodeKind::Bool: return N->bBoolValue ? TEXT("True") : TEXT("False");
		case EGitsNodeKind::NoneLit: return TEXT("None");
		case EGitsNodeKind::Name: return N->Text;
		case EGitsNodeKind::BinOp:
		case EGitsNodeKind::BoolOp:
		case EGitsNodeKind::Compare:
			return Unparse(N->Left) + TEXT(" ") + N->Text + TEXT(" ") + Unparse(N->Right);
		case EGitsNodeKind::UnaryOp:
			return N->Text == TEXT("not") ? TEXT("not ") + Unparse(N->Operand) : N->Text + Unparse(N->Operand);
		case EGitsNodeKind::Call:
		{
			FString Out = Unparse(N->Func) + TEXT("(");
			for (int32 i = 0; i < N->Args.Num(); ++i) { if (i > 0) { Out += TEXT(", "); } Out += Unparse(N->Args[i]); }
			return Out + TEXT(")");
		}
		case EGitsNodeKind::Method:
		{
			FString Out = Unparse(N->Receiver) + TEXT(".") + N->Text + TEXT("(");
			for (int32 i = 0; i < N->Args.Num(); ++i) { if (i > 0) { Out += TEXT(", "); } Out += Unparse(N->Args[i]); }
			return Out + TEXT(")");
		}
		case EGitsNodeKind::Subscript: return Unparse(N->Value) + TEXT("[") + Unparse(N->Index) + TEXT("]");
		case EGitsNodeKind::List:
		{
			FString Out = TEXT("[");
			for (int32 i = 0; i < N->Elts.Num(); ++i) { if (i > 0) { Out += TEXT(", "); } Out += Unparse(N->Elts[i]); }
			return Out + TEXT("]");
		}
		default: return TEXT("");
		}
	}

	FString ParamList(const FGitsNode& Def)
	{
		return FString::Join(Def.Params, TEXT(", "));
	}
}

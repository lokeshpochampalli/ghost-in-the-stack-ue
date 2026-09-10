// Ghost in the Stack — abstract syntax tree.
//
// One node struct with a kind tag, which keeps the evaluator's dispatch in one place and
// the tree cheap to walk. NodeId is a structural path (ADR-002): "body.3.test.left".
#pragma once

#include "CoreMinimal.h"
#include "GitsTypes.h"
#include "GitsLexer.h"

enum class EGitsNodeKind : uint8
{
	Program,
	// expressions
	Num, Str, Bool, NoneLit, Name, BinOp, UnaryOp, BoolOp, Compare, Call, Method, Subscript, List,
	// statements
	Assign, AugAssign, ExprStmt, If, While, For, FunctionDef, Return, Break, Continue
};

struct FGitsNode
{
	EGitsNodeKind Kind = EGitsNodeKind::Program;
	FGitsSpan Span;
	FString NodeId;

	/** Name id, operator text, method name, function name, or a number's source text. */
	FString Text;
	bool bIsFloat = false;
	int64 IntValue = 0;
	double FloatValue = 0.0;
	FString StrValue;
	bool bBoolValue = false;
	bool bIsElif = false;

	TSharedPtr<FGitsNode> Left;       // BinOp, BoolOp, Compare
	TSharedPtr<FGitsNode> Right;
	TSharedPtr<FGitsNode> Operand;    // UnaryOp
	TSharedPtr<FGitsNode> Func;       // Call
	TSharedPtr<FGitsNode> Receiver;   // Method
	TSharedPtr<FGitsNode> Value;      // Subscript value, Assign/AugAssign/ExprStmt/Return value
	TSharedPtr<FGitsNode> Index;      // Subscript
	TSharedPtr<FGitsNode> Target;     // Assign, AugAssign, For (a Name)
	TSharedPtr<FGitsNode> Test;       // If, While
	TSharedPtr<FGitsNode> Iter;       // For
	TArray<TSharedPtr<FGitsNode>> Args;    // Call, Method
	TArray<TSharedPtr<FGitsNode>> Elts;    // List
	TArray<TSharedPtr<FGitsNode>> Body;    // Program, If, While, For, FunctionDef
	TArray<TSharedPtr<FGitsNode>> OrElse;  // If
	TArray<FString> Params;                // FunctionDef

	bool IsStatement() const { return Kind >= EGitsNodeKind::Assign; }
};

typedef TSharedPtr<FGitsNode> FGitsNodePtr;

struct FGitsProgram
{
	FGitsNodePtr Root;
	TArray<FGitsComment> Comments;
};

namespace GitsAst
{
	/** Assigns structural NodeIds to the whole tree. */
	void AssignNodeIds(const FGitsNodePtr& Root);
	/** Re-renders an expression from the tree, without parentheses, the way trace labels do. */
	FString Unparse(const FGitsNodePtr& Node);
	/** The parameter list as "def name(a, b)" shows it. */
	FString ParamList(const FGitsNode& Def);
}

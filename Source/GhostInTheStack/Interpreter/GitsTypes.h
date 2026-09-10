// Ghost in the Stack — interpreter core types.
//
// The C++ port of the reference interpreter's shared vocabulary: source positions,
// runtime values, the world record, effects, queries, diagnostics, steps and traces.
// Behaviour is decided by the thirty golden fixtures in Fixtures/ and by
// docs/LANGUAGE-SPEC.md, never by the TypeScript. Line numbers are 1-based, columns
// 0-based, spans half-open.
#pragma once

#include "CoreMinimal.h"

// --- source positions --------------------------------------------------------

struct FGitsPosition
{
	int32 Line = 1;
	int32 Column = 0;
	int32 Offset = 0;

	bool operator==(const FGitsPosition& O) const { return Line == O.Line && Column == O.Column && Offset == O.Offset; }
};

struct FGitsSpan
{
	FGitsPosition Start;
	FGitsPosition End;

	static FGitsSpan Spanning(const FGitsSpan& A, const FGitsSpan& B)
	{
		FGitsSpan S;
		S.Start = A.Start.Offset <= B.Start.Offset ? A.Start : B.Start;
		S.End = A.End.Offset >= B.End.Offset ? A.End : B.End;
		return S;
	}
};

/** Language tier as defined in docs/LANGUAGE-SPEC.md. A level declares one. */
enum class EGitsTier : uint8 { One = 1, Two = 2, Three = 3, Four = 4 };

// --- diagnostics ---------------------------------------------------------------

/** The 46 codes of the beginner-facing catalogue. Names match the reference data. */
enum class EGitsDiagnosticCode : uint8
{
	// lexical
	TabIndentation,
	InconsistentIndentation,
	UnexpectedIndent,
	UnterminatedString,
	UnexpectedCharacter,
	MalformedNumber,
	// structural
	MissingColon,
	ExpectedIndentedBlock,
	UnclosedBracket,
	UnexpectedToken,
	InvalidAssignmentTarget,
	AssignmentInCondition,
	LeadingPlus,
	ElifWithoutIf,
	ReturnOutsideFunction,
	// recognition: not here
	ExcludedDictLiteral,
	ExcludedSetLiteral,
	ExcludedFString,
	ExcludedImport,
	ExcludedClass,
	ExcludedExceptionHandling,
	ExcludedWith,
	ExcludedLambda,
	ExcludedComprehension,
	ExcludedTuple,
	ExcludedSlicing,
	ExcludedScopeDeclaration,
	ExcludedKeywordArgument,
	ExcludedDefaultArgument,
	ExcludedComparisonChaining,
	ExcludedMultipleAssignment,
	ExcludedConstruct,
	// recognition: not yet
	NotYetUnlocked,
	MethodNotAvailable,
	// runtime
	NameNotDefined,
	TypeMismatch,
	DivisionByZero,
	IndexOutOfRange,
	BadIndexType,
	NotAList,
	WrongArgumentCount,
	UnknownFunction,
	BadRange,
	StatementCapExceeded,
	SafetyCapExceeded,
	CallDepthExceeded,

	Count
};

struct FGitsDiagnostic
{
	EGitsDiagnosticCode Code = EGitsDiagnosticCode::UnexpectedToken;
	FGitsSpan Span;
	/** What happened, in plain language. */
	FString What;
	/** One concrete thing to check. */
	FString Check;
};

// --- runtime values --------------------------------------------------------------

enum class EGitsValueKind : uint8 { Int, Float, Str, Bool, None, List, Function };

struct FGitsValue;

/** Lists have reference semantics: two names can hold one list. */
struct FGitsList
{
	int32 Id = 0;
	TArray<FGitsValue> Items;
};

struct FGitsValue
{
	EGitsValueKind Kind = EGitsValueKind::None;
	int64 Int = 0;
	double Float = 0.0;
	bool Bool = false;
	FString Str;
	TSharedPtr<FGitsList> List;
	/** Function values carry name, parameter names and the definition's node id. */
	FString FunctionName;
	TArray<FString> Params;
	FString FunctionNodeId;

	static FGitsValue MakeNone() { return FGitsValue(); }
	static FGitsValue MakeInt(int64 V) { FGitsValue R; R.Kind = EGitsValueKind::Int; R.Int = V; return R; }
	static FGitsValue MakeFloat(double V) { FGitsValue R; R.Kind = EGitsValueKind::Float; R.Float = V; return R; }
	static FGitsValue MakeBool(bool V) { FGitsValue R; R.Kind = EGitsValueKind::Bool; R.Bool = V; return R; }
	static FGitsValue MakeStr(const FString& V) { FGitsValue R; R.Kind = EGitsValueKind::Str; R.Str = V; return R; }
	static FGitsValue MakeList(TSharedPtr<FGitsList> L) { FGitsValue R; R.Kind = EGitsValueKind::List; R.List = L; return R; }
	static FGitsValue MakeFunction(const FString& Name, const TArray<FString>& InParams, const FString& NodeId)
	{
		FGitsValue R; R.Kind = EGitsValueKind::Function; R.FunctionName = Name; R.Params = InParams; R.FunctionNodeId = NodeId; return R;
	}

	bool IsNumber() const { return Kind == EGitsValueKind::Int || Kind == EGitsValueKind::Float; }
	bool IsNumeric() const { return IsNumber() || Kind == EGitsValueKind::Bool; }
	/** Numeric view, with True and False as 1 and 0 as Python does. */
	double AsDouble() const;
	int64 AsInt() const;
};

namespace GitsValue
{
	/** How a value prints, matching Python's print. */
	FString Display(const FGitsValue& V);
	/** How a value appears inside a list or a trace label: strings keep their quotes. */
	FString Repr(const FGitsValue& V);
	/** The name a beginner would use for this kind of value. No Python jargon. */
	FString TypeName(const FGitsValue& V);
	/** Structural equality ignoring list identity; 2 == 2.0 and True == 1 as in Python. */
	bool Equal(const FGitsValue& A, const FGitsValue& B);
	/** Python truthiness. */
	bool Truthy(const FGitsValue& V);
	/** Deep copy preserving list ids, for snapshots. */
	FGitsValue Copy(const FGitsValue& V);
	/** Python's repr of a float: shortest round-trip, always a decimal point. */
	FString FormatFloat(double D);
}

// --- the world -------------------------------------------------------------------

struct FGitsWorldValue
{
	enum class EKind : uint8 { Number, String, Bool } Kind = EKind::Number;
	double Number = 0.0;
	FString String;
	bool Bool = false;

	static FGitsWorldValue MakeNumber(double D) { FGitsWorldValue R; R.Kind = EKind::Number; R.Number = D; return R; }
	static FGitsWorldValue MakeString(const FString& S) { FGitsWorldValue R; R.Kind = EKind::String; R.String = S; return R; }
	static FGitsWorldValue MakeBool(bool B) { FGitsWorldValue R; R.Kind = EKind::Bool; R.Bool = B; return R; }
	/** The reference formatter's String(value): numbers without a trailing .0, booleans lowercase. */
	FString ToText() const;
	bool operator==(const FGitsWorldValue& O) const;
};

/** An open record (ADR-004). Levels declare their keys; nothing here enforces that yet. */
typedef TMap<FString, FGitsWorldValue> FGitsWorldState;

struct FGitsEffect
{
	enum class EKind : uint8 { Set, Log, Wait } Kind = EKind::Set;
	FString Key;
	FGitsWorldValue Value;
	FString Message;
	int64 Ticks = 0;

	static FGitsEffect MakeSet(const FString& K, const FGitsWorldValue& V) { FGitsEffect E; E.Kind = EKind::Set; E.Key = K; E.Value = V; return E; }
	static FGitsEffect MakeLog(const FString& M) { FGitsEffect E; E.Kind = EKind::Log; E.Message = M; return E; }
	static FGitsEffect MakeWait(int64 T) { FGitsEffect E; E.Kind = EKind::Wait; E.Ticks = T; return E; }
	bool operator==(const FGitsEffect& O) const;
};

struct FGitsQuery
{
	FString Kind = TEXT("sensor");
	FString Id;
};

/** The injected reader (ADR-003). Pure, synchronous, total: must return for any query. */
typedef TFunction<FGitsValue(const FGitsWorldState&, const FGitsQuery&)> FGitsWorldOracle;

namespace GitsWorld
{
	extern const TCHAR* ClockKey;     // the tick counter wait(n) advances
	extern const TCHAR* LogCountKey;  // how many lines the station log has taken
	/** Applies one effect to a copy of the world. Never mutates its input. */
	FGitsWorldState Reduce(const FGitsWorldState& World, const FGitsEffect& Effect);
	FGitsWorldState ReduceAll(const FGitsWorldState& World, const TArray<FGitsEffect>& Effects);
	/** An oracle for programs that never read the world. */
	FGitsValue NullOracle(const FGitsWorldState&, const FGitsQuery&);
}

// --- the trace ---------------------------------------------------------------------

enum class EGitsStepKind : uint8
{
	Def, Return, Literal, Name, BinOp, UnaryOp, BoolOp, Compare, List, Subscript, Call, Method,
	Assign, AugAssign, Expr, Branch, Iterate, Break, Continue
};

const TCHAR* GitsStepKindName(EGitsStepKind Kind);

/** One call-stack entry. <module> is the top level. */
struct FGitsFrameSnapshot
{
	FString FunctionName;
	/** Bindings in insertion order, deep-copied. */
	TArray<TPair<FString, FGitsValue>> Bindings;
	/** Where the call that created this frame was written. Unset for <module>. */
	bool bHasCallSite = false;
	FGitsSpan CallSite;

	const FGitsValue* Find(const FString& Name) const
	{
		for (const auto& P : Bindings) { if (P.Key == Name) { return &P.Value; } }
		return nullptr;
	}
};

struct FGitsOracleRead
{
	FGitsQuery Query;
	FGitsValue Value;
};

/** One AST node evaluation (ADR-001). */
struct FGitsStep
{
	int32 Index = 0;
	FString NodeId;
	FGitsSpan Span;
	EGitsStepKind Kind = EGitsStepKind::Literal;
	/** Expression nesting depth within the statement. Zero at the statement itself. */
	int32 Depth = 0;
	/** Which statement execution this step belongs to. */
	int32 StmtIndex = 0;
	bool bIsStatementBoundary = false;
	/** The call stack, innermost last. Deep-copied. */
	TArray<FGitsFrameSnapshot> Frames;
	TArray<FString> Output;
	TArray<FGitsEffect> Effects;
	bool bHasOracleRead = false;
	FGitsOracleRead OracleRead;
	/** One-line summary for the golden-trace format and the runtime views. */
	FString Label;
};

enum class EGitsOutcomeKind : uint8 { Completed, Error, StatementCapExceeded, SafetyCapExceeded };

struct FGitsOutcome
{
	EGitsOutcomeKind Kind = EGitsOutcomeKind::Completed;
	FGitsDiagnostic Diagnostic;
	int32 Cap = 0;
};

struct FGitsTrace
{
	TArray<FGitsStep> Steps;
	FGitsOutcome Outcome;
	/** Everything printed, in order. */
	TArray<FString> Output;
	int32 Seed = 0;
	FGitsWorldState InitialWorld;
	/** Statement executions counted on bIsStatementBoundary (ADR-021). */
	int32 StatementCount = 0;
};

namespace GitsLimits
{
	/** Runaway protection (ADR-008). A project constant, not level-configurable. */
	constexpr int32 SafetyStepCap = 50000;
	/** The design budget (ADR-021), per-level configurable. */
	constexpr int32 DefaultStatementCap = 2000;
	/** Recursion guard, counted in frames including <module>. */
	constexpr int32 MaxCallDepth = 100;
}

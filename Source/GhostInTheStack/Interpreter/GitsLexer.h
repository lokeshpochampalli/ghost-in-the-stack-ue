// Ghost in the Stack — lexer.
//
// Turns source text into tokens with spans, INDENT/DEDENT/NEWLINE structure, preserved
// comments (they are Ilse's narrative, not noise) and lexical diagnostics. Never throws;
// a bad character or number becomes an Error token that the parser drops silently, so
// each broken line is reported exactly once.
#pragma once

#include "CoreMinimal.h"
#include "GitsTypes.h"

enum class EGitsTokenType : uint8 { Name, Keyword, Number, String, Op, Newline, Indent, Dedent, Eof, Error };

struct FGitsToken
{
	EGitsTokenType Type = EGitsTokenType::Eof;
	/** The source text of the token. For strings, the raw text including quotes. */
	FString Text;
	FGitsSpan Span;
	bool bIsFloat = false;
	int64 IntValue = 0;
	double FloatValue = 0.0;
	/** The decoded string value for String tokens. */
	FString StringValue;

	bool Is(EGitsTokenType T) const { return Type == T; }
	bool IsOp(const TCHAR* Op) const { return Type == EGitsTokenType::Op && Text == Op; }
	bool IsKeyword(const TCHAR* Kw) const { return Type == EGitsTokenType::Keyword && Text == Kw; }
};

struct FGitsComment
{
	/** The comment as written, including the hash. */
	FString Raw;
	/** The comment with the hash and one leading space removed. */
	FString Text;
	FGitsSpan Span;
	/** True when the comment is the only thing on its line. */
	bool bOwnLine = true;
};

struct FGitsLexResult
{
	TArray<FGitsToken> Tokens;
	TArray<FGitsComment> Comments;
	TArray<FGitsDiagnostic> Diagnostics;
};

namespace GitsLexer
{
	FGitsLexResult Tokenize(const FString& Source);
	bool IsKeyword(const FString& Word);
}

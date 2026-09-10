#include "GitsLexer.h"
#include "GitsDiagnostics.h"

namespace
{
	const TCHAR* Keywords[] = {
		TEXT("False"), TEXT("None"), TEXT("True"), TEXT("and"), TEXT("as"), TEXT("assert"), TEXT("async"),
		TEXT("await"), TEXT("break"), TEXT("class"), TEXT("continue"), TEXT("def"), TEXT("del"), TEXT("elif"),
		TEXT("else"), TEXT("except"), TEXT("finally"), TEXT("for"), TEXT("from"), TEXT("global"), TEXT("if"),
		TEXT("import"), TEXT("in"), TEXT("is"), TEXT("lambda"), TEXT("nonlocal"), TEXT("not"), TEXT("or"),
		TEXT("pass"), TEXT("raise"), TEXT("return"), TEXT("try"), TEXT("while"), TEXT("with"), TEXT("yield"),
	};

	bool IsIdentStart(TCHAR C) { return (C >= 'a' && C <= 'z') || (C >= 'A' && C <= 'Z') || C == '_'; }
	bool IsDigit(TCHAR C) { return C >= '0' && C <= '9'; }
	bool IsIdentChar(TCHAR C) { return IsIdentStart(C) || IsDigit(C); }

	class FLexer
	{
	public:
		explicit FLexer(const FString& InSource) : Src(InSource) {}

		FGitsLexResult Run()
		{
			IndentStack.Add(0);
			while (true)
			{
				if (bAtLineStart && Brackets.Num() == 0)
				{
					if (!BeginLine()) { break; }
					continue;
				}
				if (Pos >= Src.Len()) { break; }
				const TCHAR C = Src[Pos];
				if (C == '\n') { HandleNewline(); continue; }
				if (C == ' ' || C == '\t' || C == '\r') { Advance(); continue; }
				if (C == '#') { LexComment(false); continue; }
				if (IsIdentStart(C)) { LexIdentifier(); continue; }
				if (IsDigit(C) || (C == '.' && Pos + 1 < Src.Len() && IsDigit(Src[Pos + 1]))) { LexNumber(); continue; }
				if (C == '"' || C == '\'') { LexString(Here(), TEXT("")); continue; }
				LexOperator();
			}
			FinishFile();
			return MoveTemp(Result);
		}

	private:
		const FString& Src;
		FGitsLexResult Result;
		int32 Pos = 0;
		int32 Line = 1;
		int32 Col = 0;
		TArray<int32> IndentStack;
		struct FOpen { TCHAR Ch; FGitsSpan Span; };
		TArray<FOpen> Brackets;
		bool bAtLineStart = true;
		bool bLineHasTokens = false;
		bool bLastLogicalEndedWithColon = false;

		FGitsPosition Here() const { FGitsPosition P; P.Line = Line; P.Column = Col; P.Offset = Pos; return P; }
		void Advance() { if (Pos < Src.Len()) { ++Pos; ++Col; } }
		TCHAR Peek(int32 Ahead = 0) const { return Pos + Ahead < Src.Len() ? Src[Pos + Ahead] : TEXT('\0'); }

		int32 FirstTokenOfLine = -1;

		void Emit(EGitsTokenType Type, const FString& Text, const FGitsPosition& Start, const FGitsPosition& End)
		{
			FGitsToken T; T.Type = Type; T.Text = Text; T.Span.Start = Start; T.Span.End = End;
			Result.Tokens.Add(T);
			if (Type != EGitsTokenType::Newline && Type != EGitsTokenType::Indent && Type != EGitsTokenType::Dedent)
			{
				if (!bLineHasTokens) { FirstTokenOfLine = Result.Tokens.Num() - 1; }
				bLineHasTokens = true;
			}
		}

		bool LineStartsWithBlockKeyword() const
		{
			if (FirstTokenOfLine < 0 || FirstTokenOfLine >= Result.Tokens.Num()) { return false; }
			const FGitsToken& First = Result.Tokens[FirstTokenOfLine];
			if (First.Type != EGitsTokenType::Keyword) { return false; }
			static const TCHAR* Openers[] = { TEXT("if"), TEXT("elif"), TEXT("else"), TEXT("while"), TEXT("for"), TEXT("def"),
				TEXT("class"), TEXT("try"), TEXT("except"), TEXT("finally"), TEXT("with") };
			for (const TCHAR* K : Openers) { if (First.Text == K) { return true; } }
			return false;
		}

		void Report(EGitsDiagnosticCode Code, const FGitsSpan& Span, const FGitsDiagnosticParams& P = FGitsDiagnosticParams())
		{
			Result.Diagnostics.Add(GitsDiagnostics::Diagnose(Code, Span, P));
		}

		/** Handles indentation at the start of a physical line. Returns false at end of input. */
		bool BeginLine()
		{
			if (Pos >= Src.Len()) { return false; }
			const FGitsPosition LineStart = Here();
			int32 Indent = 0;
			bool bTabSeen = false;
			while (Pos < Src.Len() && (Src[Pos] == ' ' || Src[Pos] == '\t'))
			{
				if (Src[Pos] == '\t') { bTabSeen = true; Indent += 4; } else { Indent += 1; }
				Advance();
			}
			const TCHAR C = Peek();
			if (C == '\r') { Advance(); return true; }
			if (C == '\n') { Advance(); ++Line; Col = 0; return true; }             // blank line
			if (C == '#') { LexComment(true); return true; }                          // comment-only line
			if (Pos >= Src.Len()) { return false; }

			if (bTabSeen)
			{
				FGitsSpan S; S.Start = LineStart; S.End = Here();
				Report(EGitsDiagnosticCode::TabIndentation, S);
			}
			const int32 Top = IndentStack.Last();
			FGitsSpan S; S.Start = LineStart; S.End = Here();
			if (Indent > Top)
			{
				if (bLastLogicalEndedWithColon)
				{
					if (Indent != Top + 4)
					{
						Report(EGitsDiagnosticCode::InconsistentIndentation, S,
							FGitsDiagnosticParams(FString::FromInt(Indent), FString::FromInt(Top + 4)));
					}
					IndentStack.Add(Indent);
					Emit(EGitsTokenType::Indent, TEXT(""), LineStart, Here());
				}
				else
				{
					Report(EGitsDiagnosticCode::UnexpectedIndent, S);
				}
			}
			else if (Indent < Top)
			{
				while (IndentStack.Num() > 1 && IndentStack.Last() > Indent)
				{
					IndentStack.Pop();
					Emit(EGitsTokenType::Dedent, TEXT(""), LineStart, LineStart);
				}
				if (IndentStack.Last() != Indent)
				{
					Report(EGitsDiagnosticCode::InconsistentIndentation, S,
						FGitsDiagnosticParams(FString::FromInt(Indent), FString::FromInt(IndentStack.Last())));
				}
			}
			bAtLineStart = false;
			return true;
		}

		void HandleNewline()
		{
			const FGitsPosition Start = Here();
			Advance();
			++Line; Col = 0;
			if (Brackets.Num() == 0)
			{
				if (bLineHasTokens)
				{
					const FGitsToken& Last = Result.Tokens.Last();
					// A block opens after a colon. A header that forgot its colon still gets its
					// block, so the parser can say "missing colon" once instead of the lexer
					// also saying "unexpected indent" on the line below.
					bLastLogicalEndedWithColon = Last.IsOp(TEXT(":")) || LineStartsWithBlockKeyword();
					Emit(EGitsTokenType::Newline, TEXT("\n"), Start, Here());
					bLineHasTokens = false;
				}
				bAtLineStart = true;
			}
		}

		void LexComment(bool bOwnLine)
		{
			const FGitsPosition Start = Here();
			while (Pos < Src.Len() && Src[Pos] != '\n') { Advance(); }
			FGitsComment Cm;
			Cm.Raw = Src.Mid(Start.Offset, Pos - Start.Offset);
			if (Cm.Raw.EndsWith(TEXT("\r"))) { Cm.Raw.LeftChopInline(1); }
			Cm.Text = Cm.Raw.Mid(1);
			if (Cm.Text.StartsWith(TEXT(" "))) { Cm.Text = Cm.Text.Mid(1); }
			Cm.Span.Start = Start; Cm.Span.End = Here();
			Cm.bOwnLine = bOwnLine;
			Result.Comments.Add(Cm);
			if (bOwnLine)
			{
				// Consume the newline without touching the indent stack.
				if (Peek() == '\n') { Advance(); ++Line; Col = 0; }
			}
		}

		void LexIdentifier()
		{
			const FGitsPosition Start = Here();
			while (Pos < Src.Len() && IsIdentChar(Src[Pos])) { Advance(); }
			const FString Word = Src.Mid(Start.Offset, Pos - Start.Offset);
			const TCHAR Next = Peek();
			if ((Word == TEXT("f") || Word == TEXT("F")) && (Next == '"' || Next == '\''))
			{
				// Recognition case 3: an f-string. Consume the string so nothing cascades.
				FGitsSpan S; S.Start = Start; S.End = Here();
				const int32 Before = Result.Tokens.Num();
				LexString(Start, Word);
				Result.Tokens.SetNum(Before);
				S.End = Here();
				Report(EGitsDiagnosticCode::ExcludedFString, S);
				Emit(EGitsTokenType::Error, Word, Start, Here());
				return;
			}
			Emit(GitsLexer::IsKeyword(Word) ? EGitsTokenType::Keyword : EGitsTokenType::Name, Word, Start, Here());
		}

		void LexNumber()
		{
			const FGitsPosition Start = Here();
			bool bFloat = false;
			while (IsDigit(Peek())) { Advance(); }
			if (Peek() == '.') { bFloat = true; Advance(); while (IsDigit(Peek())) { Advance(); } }
			if ((Peek() == 'e' || Peek() == 'E') && (IsDigit(Peek(1)) || ((Peek(1) == '+' || Peek(1) == '-') && IsDigit(Peek(2)))))
			{
				bFloat = true; Advance();
				if (Peek() == '+' || Peek() == '-') { Advance(); }
				while (IsDigit(Peek())) { Advance(); }
			}
			if (IsIdentStart(Peek()))
			{
				while (IsIdentChar(Peek())) { Advance(); }
				const FString Text = Src.Mid(Start.Offset, Pos - Start.Offset);
				FGitsSpan S; S.Start = Start; S.End = Here();
				Report(EGitsDiagnosticCode::MalformedNumber, S, FGitsDiagnosticParams(Text));
				Emit(EGitsTokenType::Error, Text, Start, Here());
				return;
			}
			const FString Text = Src.Mid(Start.Offset, Pos - Start.Offset);
			FGitsToken T; T.Type = EGitsTokenType::Number; T.Text = Text; T.Span.Start = Start; T.Span.End = Here();
			T.bIsFloat = bFloat;
			if (bFloat) { T.FloatValue = FCString::Atod(*Text); }
			else { T.IntValue = FCString::Atoi64(*Text); }
			Result.Tokens.Add(T);
			bLineHasTokens = true;
		}

		void LexString(const FGitsPosition& Start, const FString& Prefix)
		{
			const TCHAR Quote = Peek();
			Advance();
			FString Value;
			bool bClosed = false;
			while (Pos < Src.Len())
			{
				const TCHAR C = Src[Pos];
				if (C == '\n') { break; }
				if (C == Quote) { Advance(); bClosed = true; break; }
				if (C == '\\' && Pos + 1 < Src.Len())
				{
					const TCHAR E = Src[Pos + 1];
					if (E == '\n') { break; }
					switch (E)
					{
					case 'n': Value.AppendChar('\n'); break;
					case 't': Value.AppendChar('\t'); break;
					case 'r': Value.AppendChar('\r'); break;
					case '\\': Value.AppendChar('\\'); break;
					case '\'': Value.AppendChar('\''); break;
					case '"': Value.AppendChar('"'); break;
					default: Value.AppendChar('\\'); Value.AppendChar(E); break;   // Python keeps the backslash
					}
					Advance(); Advance();
					continue;
				}
				Value.AppendChar(C);
				Advance();
			}
			FGitsPosition End = Here();
			if (!bClosed)
			{
				FGitsSpan S; S.Start = Start; S.End = End;
				Report(EGitsDiagnosticCode::UnterminatedString, S, FGitsDiagnosticParams(FString::Chr(Quote)));
			}
			FGitsToken T; T.Type = EGitsTokenType::String;
			T.Text = Src.Mid(Start.Offset, Pos - Start.Offset);
			T.Span.Start = Start; T.Span.End = End; T.StringValue = Value;
			Result.Tokens.Add(T);
			bLineHasTokens = true;
		}

		void LexOperator()
		{
			const FGitsPosition Start = Here();
			static const TCHAR* Three[] = { TEXT("**="), TEXT("//=") };
			static const TCHAR* Two[] = { TEXT("**"), TEXT("//"), TEXT("=="), TEXT("!="), TEXT("<="), TEXT(">="), TEXT("+="), TEXT("-="), TEXT("*="), TEXT("/="), TEXT("%=") };
			static const TCHAR One[] = { '+', '-', '*', '/', '%', '=', '<', '>', '(', ')', '[', ']', '{', '}', ',', ':', '.', ';', '@' };
			const FString Rest = Src.Mid(Pos, 3);
			FString Op;
			for (const TCHAR* Cand : Three) { if (Rest.StartsWith(Cand)) { Op = Cand; break; } }
			if (Op.IsEmpty()) { for (const TCHAR* Cand : Two) { if (Rest.StartsWith(Cand)) { Op = Cand; break; } } }
			if (Op.IsEmpty())
			{
				for (TCHAR C : One) { if (Peek() == C) { Op = FString::Chr(C); break; } }
			}
			if (Op.IsEmpty())
			{
				const FString Ch = FString::Chr(Peek());
				Advance();
				FGitsSpan S; S.Start = Start; S.End = Here();
				Report(EGitsDiagnosticCode::UnexpectedCharacter, S, FGitsDiagnosticParams(Ch));
				Emit(EGitsTokenType::Error, Ch, Start, Here());
				return;
			}
			for (int32 i = 0; i < Op.Len(); ++i) { Advance(); }
			const TCHAR C = Op[0];
			if (Op.Len() == 1 && (C == '(' || C == '[' || C == '{'))
			{
				FOpen O; O.Ch = C; O.Span.Start = Start; O.Span.End = Here();
				Brackets.Add(O);
			}
			else if (Op.Len() == 1 && (C == ')' || C == ']' || C == '}'))
			{
				if (Brackets.Num() > 0) { Brackets.Pop(); }
			}
			Emit(EGitsTokenType::Op, Op, Start, Here());
		}

		void FinishFile()
		{
			if (Brackets.Num() > 0)
			{
				const FOpen& O = Brackets.Last();
				const TCHAR Closer = O.Ch == '(' ? ')' : O.Ch == '[' ? ']' : '}';
				Report(EGitsDiagnosticCode::UnclosedBracket, O.Span, FGitsDiagnosticParams(FString::Chr(O.Ch), FString::Chr(Closer)));
				Emit(EGitsTokenType::Error, FString::Chr(O.Ch), Here(), Here());
				Brackets.Empty();
			}
			if (bLineHasTokens)
			{
				Emit(EGitsTokenType::Newline, TEXT(""), Here(), Here());
				bLineHasTokens = false;
			}
			while (IndentStack.Num() > 1)
			{
				IndentStack.Pop();
				Emit(EGitsTokenType::Dedent, TEXT(""), Here(), Here());
			}
			Emit(EGitsTokenType::Eof, TEXT(""), Here(), Here());
		}
	};
}

namespace GitsLexer
{
	bool IsKeyword(const FString& Word)
	{
		for (const TCHAR* K : Keywords) { if (Word == K) { return true; } }
		return false;
	}

	FGitsLexResult Tokenize(const FString& Source)
	{
		FString Normalised = Source.Replace(TEXT("\r\n"), TEXT("\n"));
		FLexer L(Normalised);
		return L.Run();
	}
}

#include "GitsParser.h"
#include "GitsDiagnostics.h"

namespace
{
	using FNode = FGitsNodePtr;

	FNode NewNode(EGitsNodeKind Kind, const FGitsSpan& Span)
	{
		FNode N = MakeShared<FGitsNode>();
		N->Kind = Kind;
		N->Span = Span;
		return N;
	}

	int32 TierNumber(EGitsTier T) { return (int32)T; }

	class FParser
	{
	public:
		FParser(const FGitsLexResult& InLex, EGitsTier InTier) : Lex(InLex), Tokens(InLex.Tokens), Tier(InTier) {}

		FGitsParseResult Run()
		{
			FGitsParseResult R;
			R.Program.Root = NewNode(EGitsNodeKind::Program, FGitsSpan());
			R.Program.Comments = Lex.Comments;
			while (!AtEof())
			{
				if (Cur().Is(EGitsTokenType::Newline) || Cur().Is(EGitsTokenType::Dedent) || Cur().Is(EGitsTokenType::Indent))
				{
					// Stray structure after a recovery; an Indent here has already been reported
					// by the lexer as unexpected, so consume its block quietly.
					if (Cur().Is(EGitsTokenType::Indent)) { SkipBlock(); } else { Advance(); }
					continue;
				}
				FNode S = ParseStatement();
				if (S.IsValid()) { R.Program.Root->Body.Add(S); }
			}
			if (Tokens.Num() > 0)
			{
				R.Program.Root->Span.Start = Tokens[0].Span.Start;
				R.Program.Root->Span.End = Tokens.Last().Span.End;
			}
			GitsAst::AssignNodeIds(R.Program.Root);
			R.Diagnostics = Lex.Diagnostics;
			R.Diagnostics.Append(Diagnostics);
			R.Diagnostics.StableSort([](const FGitsDiagnostic& A, const FGitsDiagnostic& B)
			{
				if (A.Span.Start.Line != B.Span.Start.Line) { return A.Span.Start.Line < B.Span.Start.Line; }
				return A.Span.Start.Column < B.Span.Start.Column;
			});
			return R;
		}

	private:
		const FGitsLexResult& Lex;
		const TArray<FGitsToken>& Tokens;
		EGitsTier Tier;
		int32 Pos = 0;
		int32 FunctionDepth = 0;
		int32 LoopDepth = 0;
		TArray<FGitsDiagnostic> Diagnostics;

		// --- token access -----------------------------------------------------------------

		const FGitsToken& Cur() const { return Tokens[FMath::Min(Pos, Tokens.Num() - 1)]; }
		const FGitsToken& PeekTok(int32 Ahead = 1) const { return Tokens[FMath::Min(Pos + Ahead, Tokens.Num() - 1)]; }
		bool AtEof() const { return Cur().Is(EGitsTokenType::Eof); }
		void Advance() { if (Pos < Tokens.Num() - 1) { ++Pos; } }
		bool AtOp(const TCHAR* Op) const { return Cur().IsOp(Op); }
		bool AtKeyword(const TCHAR* Kw) const { return Cur().IsKeyword(Kw); }
		bool AtEndOfStatement() const { return Cur().Is(EGitsTokenType::Newline) || AtEof(); }
		FGitsPosition PrevEnd() const { return Pos > 0 ? Tokens[Pos - 1].Span.End : Cur().Span.Start; }

		FString Describe(const FGitsToken& T) const
		{
			switch (T.Type)
			{
			case EGitsTokenType::Newline: return TEXT("The end of the line");
			case EGitsTokenType::Eof: return TEXT("The end of the program");
			case EGitsTokenType::Indent: return TEXT("An indented line");
			case EGitsTokenType::Dedent: return TEXT("The end of the block");
			default: return T.Text;
			}
		}

		// --- diagnostics and recovery ----------------------------------------------------------

		void Report(EGitsDiagnosticCode Code, const FGitsSpan& Span, const FGitsDiagnosticParams& P = FGitsDiagnosticParams())
		{
			Diagnostics.Add(GitsDiagnostics::Diagnose(Code, Span, P));
		}

		/** True when the rest of this logical line holds a token the lexer already reported. */
		bool LineHasErrorToken() const
		{
			for (int32 i = Pos; i < Tokens.Num(); ++i)
			{
				if (Tokens[i].Is(EGitsTokenType::Newline) || Tokens[i].Is(EGitsTokenType::Eof)) { return false; }
				if (Tokens[i].Is(EGitsTokenType::Error)) { return true; }
			}
			return false;
		}

		/** Skips to just past the end of the current statement, and past any block it opened. */
		void SkipStatement()
		{
			while (!AtEof() && !Cur().Is(EGitsTokenType::Newline)) { Advance(); }
			if (Cur().Is(EGitsTokenType::Newline)) { Advance(); }
			if (Cur().Is(EGitsTokenType::Indent)) { SkipBlock(); }
		}

		/** Consumes a balanced Indent ... Dedent region starting at an Indent. */
		void SkipBlock()
		{
			int32 Depth = 0;
			do
			{
				if (Cur().Is(EGitsTokenType::Indent)) { ++Depth; }
				else if (Cur().Is(EGitsTokenType::Dedent)) { --Depth; }
				if (AtEof()) { return; }
				Advance();
			} while (Depth > 0 && !AtEof());
		}

		/** Abandons the statement without a diagnostic. */
		FNode Abandon() { SkipStatement(); return nullptr; }

		/** Reports (unless the lexer already did for this line) and abandons the statement. */
		FNode Fail(EGitsDiagnosticCode Code, const FGitsSpan& Span, const FGitsDiagnosticParams& P = FGitsDiagnosticParams())
		{
			if (!LineHasErrorToken()) { Report(Code, Span, P); }
			return Abandon();
		}

		FNode FailHere(EGitsDiagnosticCode Code, const FGitsDiagnosticParams& P = FGitsDiagnosticParams())
		{
			return Fail(Code, Cur().Span, P);
		}

		FNode Unexpected()
		{
			if (Cur().Is(EGitsTokenType::Error)) { return Abandon(); }
			return FailHere(EGitsDiagnosticCode::UnexpectedToken, FGitsDiagnosticParams(Describe(Cur())));
		}

		/** The tier gate. Reports in the not-yet register; returns true when the construct is locked. */
		bool Locked(EGitsTier Required, const FString& Subject, const FGitsSpan& Span)
		{
			if (Tier < Required)
			{
				Report(EGitsDiagnosticCode::NotYetUnlocked, Span, FGitsDiagnosticParams(Subject, FString::FromInt(TierNumber(Required))));
				return true;
			}
			return false;
		}

		FGitsSpan SpanFrom(const FGitsPosition& Start) const { FGitsSpan S; S.Start = Start; S.End = PrevEnd(); return S; }

		// --- statements -------------------------------------------------------------------------

		FNode ParseStatement()
		{
			const FGitsToken& T = Cur();
			if (T.Is(EGitsTokenType::Error)) { return Abandon(); }
			if (T.Is(EGitsTokenType::Keyword))
			{
				const FString& K = T.Text;
				if (K == TEXT("if")) { return ParseIf(false); }
				if (K == TEXT("elif") || K == TEXT("else"))
				{
					Report(EGitsDiagnosticCode::ElifWithoutIf, T.Span, FGitsDiagnosticParams(K));
					return Abandon();
				}
				if (K == TEXT("while")) { return ParseWhile(); }
				if (K == TEXT("for")) { return ParseFor(); }
				if (K == TEXT("def")) { return ParseDef(); }
				if (K == TEXT("return")) { return ParseReturn(); }
				if (K == TEXT("break") || K == TEXT("continue")) { return ParseBreakContinue(); }
				if (K == TEXT("import") || K == TEXT("from")) { return Fail(EGitsDiagnosticCode::ExcludedImport, T.Span); }
				if (K == TEXT("class")) { return Fail(EGitsDiagnosticCode::ExcludedClass, T.Span); }
				if (K == TEXT("try") || K == TEXT("except") || K == TEXT("finally") || K == TEXT("raise"))
				{
					return Fail(EGitsDiagnosticCode::ExcludedExceptionHandling, T.Span);
				}
				if (K == TEXT("with")) { return Fail(EGitsDiagnosticCode::ExcludedWith, T.Span); }
				if (K == TEXT("global") || K == TEXT("nonlocal")) { return Fail(EGitsDiagnosticCode::ExcludedScopeDeclaration, T.Span, FGitsDiagnosticParams(K)); }
				if (K == TEXT("pass")) { return Fail(EGitsDiagnosticCode::ExcludedConstruct, T.Span, FGitsDiagnosticParams(TEXT("A pass statement"))); }
				if (K == TEXT("del")) { return Fail(EGitsDiagnosticCode::ExcludedConstruct, T.Span, FGitsDiagnosticParams(TEXT("Deleting a name with del"))); }
				if (K == TEXT("assert")) { return Fail(EGitsDiagnosticCode::ExcludedConstruct, T.Span, FGitsDiagnosticParams(TEXT("An assert"))); }
				if (K == TEXT("yield")) { return Fail(EGitsDiagnosticCode::ExcludedConstruct, T.Span, FGitsDiagnosticParams(TEXT("yield"))); }
				if (K == TEXT("async") || K == TEXT("await")) { return Fail(EGitsDiagnosticCode::ExcludedConstruct, T.Span, FGitsDiagnosticParams(K)); }
				// True, False, None, not, lambda fall through to the expression statement.
			}
			if (T.IsOp(TEXT("@"))) { return Fail(EGitsDiagnosticCode::ExcludedConstruct, T.Span, FGitsDiagnosticParams(TEXT("A decorator"))); }
			return ParseSimpleStatement();
		}

		FNode ParseSimpleStatement()
		{
			const FGitsPosition Start = Cur().Span.Start;
			if (AtOp(TEXT("="))) { return Unexpected(); }
			FNode Lhs = ParseExpr();
			if (!Lhs.IsValid()) { return nullptr; }

			if (AtOp(TEXT(",")))
			{
				return Fail(EGitsDiagnosticCode::ExcludedTuple, SpanFrom(Start));
			}
			if (AtOp(TEXT(";")))
			{
				return Fail(EGitsDiagnosticCode::ExcludedConstruct, Cur().Span, FGitsDiagnosticParams(TEXT("Two statements on one line")));
			}
			if (AtOp(TEXT("=")))
			{
				const FGitsSpan EqSpan = Cur().Span;
				Advance();
				if (Lhs->Kind != EGitsNodeKind::Name)
				{
					return Fail(EGitsDiagnosticCode::InvalidAssignmentTarget, Lhs->Span);
				}
				FNode Value = ParseExpr();
				if (!Value.IsValid()) { return nullptr; }
				if (AtOp(TEXT("=")))
				{
					return Fail(EGitsDiagnosticCode::ExcludedMultipleAssignment, SpanFrom(Start));
				}
				if (AtOp(TEXT(","))) { return Fail(EGitsDiagnosticCode::ExcludedTuple, SpanFrom(Start)); }
				if (!ExpectEndOfStatement()) { return nullptr; }
				FNode N = NewNode(EGitsNodeKind::Assign, FGitsSpan::Spanning(Lhs->Span, Value->Span));
				N->Target = Lhs;
				N->Value = Value;
				return N;
			}
			if (Cur().Is(EGitsTokenType::Op))
			{
				const FString& Op = Cur().Text;
				if (Op == TEXT("+=") || Op == TEXT("-=") || Op == TEXT("*=") || Op == TEXT("//="))
				{
					const FGitsSpan OpSpan = Cur().Span;
					const bool bLocked = Locked(EGitsTier::Three, TEXT("Augmented assignment like +="), OpSpan);
					Advance();
					if (Lhs->Kind != EGitsNodeKind::Name) { return Fail(EGitsDiagnosticCode::InvalidAssignmentTarget, Lhs->Span); }
					FNode Value = ParseExpr();
					if (!Value.IsValid()) { return nullptr; }
					if (!ExpectEndOfStatement()) { return nullptr; }
					if (bLocked) { return nullptr; }
					FNode N = NewNode(EGitsNodeKind::AugAssign, FGitsSpan::Spanning(Lhs->Span, Value->Span));
					N->Target = Lhs;
					N->Value = Value;
					N->Text = Op.LeftChop(1);
					return N;
				}
				if (Op == TEXT("/=") || Op == TEXT("%=") || Op == TEXT("**="))
				{
					return Fail(EGitsDiagnosticCode::ExcludedConstruct, Cur().Span, FGitsDiagnosticParams(FString::Printf(TEXT("The %s operator"), *Op)));
				}
			}
			if (!ExpectEndOfStatement()) { return nullptr; }
			FNode N = NewNode(EGitsNodeKind::ExprStmt, Lhs->Span);
			N->Value = Lhs;
			return N;
		}

		/** Consumes the statement's newline. On anything else, reports and abandons. */
		bool ExpectEndOfStatement()
		{
			if (Cur().Is(EGitsTokenType::Newline)) { Advance(); return true; }
			if (AtEof()) { return true; }
			if (AtOp(TEXT(";")))
			{
				Fail(EGitsDiagnosticCode::ExcludedConstruct, Cur().Span, FGitsDiagnosticParams(TEXT("Two statements on one line")));
				return false;
			}
			Unexpected();
			return false;
		}

		// --- blocks -----------------------------------------------------------------------------

		/**
		 * Parses ": NEWLINE INDENT statements DEDENT". Returns false after reporting and
		 * recovering. The block's statements go into Out.
		 */
		bool ParseBlock(const FString& Keyword, TArray<FNode>& Out, const FGitsPosition& HeaderStart)
		{
			if (!AtOp(TEXT(":")))
			{
				FGitsSpan S = SpanFrom(HeaderStart);
				if (!LineHasErrorToken()) { Report(EGitsDiagnosticCode::MissingColon, S, FGitsDiagnosticParams(Keyword)); }
				SkipStatement();
				return false;
			}
			Advance();
			if (!Cur().Is(EGitsTokenType::Newline))
			{
				Unexpected();
				return false;
			}
			Advance();
			if (!Cur().Is(EGitsTokenType::Indent))
			{
				Report(EGitsDiagnosticCode::ExpectedIndentedBlock, SpanFrom(HeaderStart), FGitsDiagnosticParams(Keyword));
				return false;
			}
			Advance();
			while (!AtEof() && !Cur().Is(EGitsTokenType::Dedent))
			{
				if (Cur().Is(EGitsTokenType::Newline)) { Advance(); continue; }
				if (Cur().Is(EGitsTokenType::Indent)) { SkipBlock(); continue; }
				FNode S = ParseStatement();
				if (S.IsValid()) { Out.Add(S); }
			}
			if (Cur().Is(EGitsTokenType::Dedent)) { Advance(); }
			return true;
		}

		/** Where a block statement ends: the start of whatever follows the block. */
		FGitsPosition BlockEnd() const { return Cur().Span.Start; }

		FNode ParseIf(bool bIsElif)
		{
			const FGitsToken& Kw = Cur();
			const FGitsPosition Start = Kw.Span.Start;
			const bool bLocked = !bIsElif && Locked(EGitsTier::Two, TEXT("An if statement"), Kw.Span);
			Advance();
			FNode Test = ParseExpr();
			if (!Test.IsValid()) { return nullptr; }
			if (AtOp(TEXT("=")))
			{
				return Fail(EGitsDiagnosticCode::AssignmentInCondition, Cur().Span);
			}
			FNode N = NewNode(EGitsNodeKind::If, FGitsSpan());
			N->Test = Test;
			N->bIsElif = bIsElif;
			if (!ParseBlock(bIsElif ? TEXT("elif") : TEXT("if"), N->Body, Start)) { return nullptr; }
			if (AtKeyword(TEXT("elif")))
			{
				FNode Nested = ParseIf(true);
				if (Nested.IsValid()) { N->OrElse.Add(Nested); }
			}
			else if (AtKeyword(TEXT("else")))
			{
				const FGitsPosition ElseStart = Cur().Span.Start;
				Advance();
				if (!ParseBlock(TEXT("else"), N->OrElse, ElseStart)) { return nullptr; }
			}
			N->Span.Start = Start;
			N->Span.End = BlockEnd();
			return bLocked ? nullptr : N;
		}

		FNode ParseWhile()
		{
			const FGitsPosition Start = Cur().Span.Start;
			const bool bLocked = Locked(EGitsTier::Three, TEXT("A while loop"), Cur().Span);
			Advance();
			FNode Test = ParseExpr();
			if (!Test.IsValid()) { return nullptr; }
			if (AtOp(TEXT("="))) { return Fail(EGitsDiagnosticCode::AssignmentInCondition, Cur().Span); }
			FNode N = NewNode(EGitsNodeKind::While, FGitsSpan());
			N->Test = Test;
			++LoopDepth;
			const bool bOk = ParseBlock(TEXT("while"), N->Body, Start);
			--LoopDepth;
			if (!bOk) { return nullptr; }
			N->Span.Start = Start;
			N->Span.End = BlockEnd();
			return bLocked ? nullptr : N;
		}

		FNode ParseFor()
		{
			const FGitsPosition Start = Cur().Span.Start;
			const bool bLocked = Locked(EGitsTier::Three, TEXT("A for loop"), Cur().Span);
			Advance();
			if (!Cur().Is(EGitsTokenType::Name))
			{
				if (AtOp(TEXT("("))) { return Fail(EGitsDiagnosticCode::ExcludedTuple, Cur().Span); }
				return Unexpected();
			}
			FNode Target = NewNode(EGitsNodeKind::Name, Cur().Span);
			Target->Text = Cur().Text;
			Advance();
			if (AtOp(TEXT(","))) { return Fail(EGitsDiagnosticCode::ExcludedTuple, SpanFrom(Start)); }
			if (!AtKeyword(TEXT("in"))) { return Unexpected(); }
			Advance();
			FNode Iter = ParseExpr();
			if (!Iter.IsValid()) { return nullptr; }
			FNode N = NewNode(EGitsNodeKind::For, FGitsSpan());
			N->Target = Target;
			N->Iter = Iter;
			++LoopDepth;
			const bool bOk = ParseBlock(TEXT("for"), N->Body, Start);
			--LoopDepth;
			if (!bOk) { return nullptr; }
			N->Span.Start = Start;
			N->Span.End = BlockEnd();
			return bLocked ? nullptr : N;
		}

		FNode ParseDef()
		{
			const FGitsPosition Start = Cur().Span.Start;
			bool bDrop = Locked(EGitsTier::Four, TEXT("A function definition"), Cur().Span);
			if (FunctionDepth > 0)
			{
				Report(EGitsDiagnosticCode::ExcludedConstruct, Cur().Span, FGitsDiagnosticParams(TEXT("A function defined inside another function")));
				bDrop = true;
			}
			Advance();
			if (!Cur().Is(EGitsTokenType::Name)) { return Unexpected(); }
			FNode N = NewNode(EGitsNodeKind::FunctionDef, FGitsSpan());
			N->Text = Cur().Text;
			Advance();
			if (!AtOp(TEXT("("))) { return Unexpected(); }
			Advance();
			while (!AtOp(TEXT(")")))
			{
				if (AtEndOfStatement() || Cur().Is(EGitsTokenType::Error)) { return Unexpected(); }
				if (AtOp(TEXT("*")) || AtOp(TEXT("**")))
				{
					return Fail(EGitsDiagnosticCode::ExcludedConstruct, Cur().Span, FGitsDiagnosticParams(FString::Printf(TEXT("Unpacking with %s"), *Cur().Text)));
				}
				if (!Cur().Is(EGitsTokenType::Name)) { return Unexpected(); }
				N->Params.Add(Cur().Text);
				Advance();
				if (AtOp(TEXT("=")))
				{
					Report(EGitsDiagnosticCode::ExcludedDefaultArgument, Cur().Span);
					bDrop = true;
					Advance();
					FNode Default = ParseExpr();
					if (!Default.IsValid()) { return nullptr; }
				}
				if (AtOp(TEXT(","))) { Advance(); continue; }
				if (!AtOp(TEXT(")"))) { return Unexpected(); }
			}
			Advance();
			++FunctionDepth;
			const int32 SavedLoopDepth = LoopDepth;
			LoopDepth = 0;
			const bool bOk = ParseBlock(TEXT("def"), N->Body, Start);
			LoopDepth = SavedLoopDepth;
			--FunctionDepth;
			if (!bOk) { return nullptr; }
			N->Span.Start = Start;
			N->Span.End = BlockEnd();
			return bDrop ? nullptr : N;
		}

		FNode ParseReturn()
		{
			const FGitsToken Kw = Cur();
			bool bDrop = Locked(EGitsTier::Four, TEXT("return"), Kw.Span);
			if (!bDrop && FunctionDepth == 0)
			{
				Report(EGitsDiagnosticCode::ReturnOutsideFunction, Kw.Span);
				bDrop = true;
			}
			Advance();
			FNode N = NewNode(EGitsNodeKind::Return, Kw.Span);
			if (!AtEndOfStatement())
			{
				FNode Value = ParseExpr();
				if (!Value.IsValid()) { return nullptr; }
				if (AtOp(TEXT(","))) { return Fail(EGitsDiagnosticCode::ExcludedTuple, SpanFrom(Kw.Span.Start)); }
				N->Value = Value;
				N->Span = FGitsSpan::Spanning(Kw.Span, Value->Span);
			}
			else
			{
				// A bare return ends where its line does, on the line below.
				N->Span.End = Cur().Is(EGitsTokenType::Newline) ? Cur().Span.End : Kw.Span.End;
			}
			if (!ExpectEndOfStatement()) { return nullptr; }
			return bDrop ? nullptr : N;
		}

		FNode ParseBreakContinue()
		{
			const FGitsToken Kw = Cur();
			const bool bBreak = Kw.Text == TEXT("break");
			bool bDrop = Locked(EGitsTier::Three, bBreak ? TEXT("break") : TEXT("continue"), Kw.Span);
			if (!bDrop && LoopDepth == 0)
			{
				Report(EGitsDiagnosticCode::UnexpectedToken, Kw.Span, FGitsDiagnosticParams(Kw.Text));
				bDrop = true;
			}
			Advance();
			if (!ExpectEndOfStatement()) { return nullptr; }
			if (bDrop) { return nullptr; }
			return NewNode(bBreak ? EGitsNodeKind::Break : EGitsNodeKind::Continue, Kw.Span);
		}

		// --- expressions ------------------------------------------------------------------------

		FNode ParseExpr() { return ParseOr(); }

		FNode ParseOr()
		{
			FNode Left = ParseAnd();
			if (!Left.IsValid()) { return nullptr; }
			while (AtKeyword(TEXT("or")))
			{
				const bool bLocked = Locked(EGitsTier::Two, TEXT("or"), Cur().Span);
				Advance();
				FNode Right = ParseAnd();
				if (!Right.IsValid()) { return nullptr; }
				if (bLocked) { return DropRest(); }
				FNode N = NewNode(EGitsNodeKind::BoolOp, FGitsSpan::Spanning(Left->Span, Right->Span));
				N->Text = TEXT("or"); N->Left = Left; N->Right = Right;
				Left = N;
			}
			return Left;
		}

		FNode ParseAnd()
		{
			FNode Left = ParseNot();
			if (!Left.IsValid()) { return nullptr; }
			while (AtKeyword(TEXT("and")))
			{
				const bool bLocked = Locked(EGitsTier::Two, TEXT("and"), Cur().Span);
				Advance();
				FNode Right = ParseNot();
				if (!Right.IsValid()) { return nullptr; }
				if (bLocked) { return DropRest(); }
				FNode N = NewNode(EGitsNodeKind::BoolOp, FGitsSpan::Spanning(Left->Span, Right->Span));
				N->Text = TEXT("and"); N->Left = Left; N->Right = Right;
				Left = N;
			}
			return Left;
		}

		FNode ParseNot()
		{
			if (AtKeyword(TEXT("not")))
			{
				const FGitsToken Kw = Cur();
				const bool bLocked = Locked(EGitsTier::Two, TEXT("not"), Kw.Span);
				Advance();
				FNode Operand = ParseNot();
				if (!Operand.IsValid()) { return nullptr; }
				if (bLocked) { return DropRest(); }
				FNode N = NewNode(EGitsNodeKind::UnaryOp, FGitsSpan::Spanning(Kw.Span, Operand->Span));
				N->Text = TEXT("not"); N->Operand = Operand;
				return N;
			}
			return ParseComparison();
		}

		bool AtComparisonOp() const
		{
			if (Cur().Is(EGitsTokenType::Op))
			{
				const FString& O = Cur().Text;
				return O == TEXT("==") || O == TEXT("!=") || O == TEXT("<") || O == TEXT(">") || O == TEXT("<=") || O == TEXT(">=");
			}
			return AtKeyword(TEXT("in")) || AtKeyword(TEXT("is"));
		}

		FNode ParseComparison()
		{
			FNode Left = ParseArith();
			if (!Left.IsValid()) { return nullptr; }
			if (!AtComparisonOp()) { return Left; }
			if (AtKeyword(TEXT("is")))
			{
				return Fail(EGitsDiagnosticCode::ExcludedConstruct, Cur().Span, FGitsDiagnosticParams(TEXT("The is operator")));
			}
			const FString Op = Cur().Text;
			const FGitsSpan OpSpan = Cur().Span;
			const bool bLocked = Op == TEXT("in") && Locked(EGitsTier::Three, TEXT("The in operator"), OpSpan);
			Advance();
			FNode Right = ParseArith();
			if (!Right.IsValid()) { return nullptr; }
			if (AtComparisonOp())
			{
				// Recognition case 15: refused, never evaluated as (a < b) < c.
				const FGitsPosition Start = Left->Span.Start;
				Advance();
				FNode Third = ParseArith();
				if (!Third.IsValid()) { return nullptr; }
				return Fail(EGitsDiagnosticCode::ExcludedComparisonChaining, SpanFrom(Start));
			}
			if (bLocked) { return DropRest(); }
			FNode N = NewNode(EGitsNodeKind::Compare, FGitsSpan::Spanning(Left->Span, Right->Span));
			N->Text = Op; N->Left = Left; N->Right = Right;
			return N;
		}

		FNode ParseArith()
		{
			FNode Left = ParseTerm();
			if (!Left.IsValid()) { return nullptr; }
			while (AtOp(TEXT("+")) || AtOp(TEXT("-")))
			{
				const FString Op = Cur().Text;
				Advance();
				FNode Right = ParseTerm();
				if (!Right.IsValid()) { return nullptr; }
				FNode N = NewNode(EGitsNodeKind::BinOp, FGitsSpan::Spanning(Left->Span, Right->Span));
				N->Text = Op; N->Left = Left; N->Right = Right;
				Left = N;
			}
			return Left;
		}

		FNode ParseTerm()
		{
			FNode Left = ParseFactor();
			if (!Left.IsValid()) { return nullptr; }
			while (AtOp(TEXT("*")) || AtOp(TEXT("/")) || AtOp(TEXT("//")) || AtOp(TEXT("%")))
			{
				const FString Op = Cur().Text;
				Advance();
				FNode Right = ParseFactor();
				if (!Right.IsValid()) { return nullptr; }
				FNode N = NewNode(EGitsNodeKind::BinOp, FGitsSpan::Spanning(Left->Span, Right->Span));
				N->Text = Op; N->Left = Left; N->Right = Right;
				Left = N;
			}
			return Left;
		}

		FNode ParseFactor()
		{
			if (AtOp(TEXT("-")))
			{
				const FGitsToken Op = Cur();
				Advance();
				FNode Operand = ParseFactor();
				if (!Operand.IsValid()) { return nullptr; }
				FNode N = NewNode(EGitsNodeKind::UnaryOp, FGitsSpan::Spanning(Op.Span, Operand->Span));
				N->Text = TEXT("-"); N->Operand = Operand;
				return N;
			}
			if (AtOp(TEXT("+"))) { return Fail(EGitsDiagnosticCode::LeadingPlus, Cur().Span); }
			return ParsePower();
		}

		FNode ParsePower()
		{
			FNode Base = ParsePostfix();
			if (!Base.IsValid()) { return nullptr; }
			if (AtOp(TEXT("**")))
			{
				Advance();
				FNode Exponent = ParseFactor();   // right-associative, and the exponent may carry a unary minus
				if (!Exponent.IsValid()) { return nullptr; }
				FNode N = NewNode(EGitsNodeKind::BinOp, FGitsSpan::Spanning(Base->Span, Exponent->Span));
				N->Text = TEXT("**"); N->Left = Base; N->Right = Exponent;
				return N;
			}
			return Base;
		}

		FNode ParsePostfix()
		{
			FNode Node = ParseAtom();
			if (!Node.IsValid()) { return nullptr; }
			while (true)
			{
				if (AtOp(TEXT("(")))
				{
					const FGitsPosition Start = Node->Span.Start;
					Advance();
					TArray<FNode> Args;
					if (!ParseArgs(Args)) { return nullptr; }
					FNode Call = NewNode(EGitsNodeKind::Call, SpanFrom(Start));
					Call->Func = Node;
					Call->Args = Args;
					Node = Call;
					continue;
				}
				if (AtOp(TEXT("[")))
				{
					const FGitsPosition Start = Node->Span.Start;
					const bool bLocked = Locked(EGitsTier::Three, TEXT("Indexing with []"), Cur().Span);
					Advance();
					if (AtOp(TEXT(":"))) { return Fail(EGitsDiagnosticCode::ExcludedSlicing, Cur().Span); }
					FNode Index = ParseExpr();
					if (!Index.IsValid()) { return nullptr; }
					if (AtOp(TEXT(":"))) { return Fail(EGitsDiagnosticCode::ExcludedSlicing, Cur().Span); }
					if (!AtOp(TEXT("]"))) { return Unexpected(); }
					Advance();
					if (bLocked) { return DropRest(); }
					FNode Sub = NewNode(EGitsNodeKind::Subscript, SpanFrom(Start));
					Sub->Value = Node;
					Sub->Index = Index;
					Node = Sub;
					continue;
				}
				if (AtOp(TEXT(".")))
				{
					const FGitsPosition Start = Node->Span.Start;
					Advance();
					if (!Cur().Is(EGitsTokenType::Name)) { return Unexpected(); }
					const FString MethodName = Cur().Text;
					const FGitsSpan NameSpan = Cur().Span;
					Advance();
					if (!AtOp(TEXT("(")))
					{
						return Fail(EGitsDiagnosticCode::MethodNotAvailable, NameSpan, FGitsDiagnosticParams(MethodName));
					}
					Advance();
					TArray<FNode> Args;
					if (!ParseArgs(Args)) { return nullptr; }
					const bool bWhitelisted = Tier >= EGitsTier::Three && MethodName == TEXT("append");
					if (!bWhitelisted)
					{
						return Fail(EGitsDiagnosticCode::MethodNotAvailable, NameSpan, FGitsDiagnosticParams(MethodName + TEXT("()")));
					}
					FNode M = NewNode(EGitsNodeKind::Method, SpanFrom(Start));
					M->Receiver = Node;
					M->Text = MethodName;
					M->Args = Args;
					Node = M;
					continue;
				}
				break;
			}
			return Node;
		}

		/** Parses the arguments after an opening parenthesis, through the closing one. */
		bool ParseArgs(TArray<FNode>& Out)
		{
			while (!AtOp(TEXT(")")))
			{
				if (AtOp(TEXT("*")) || AtOp(TEXT("**")))
				{
					Fail(EGitsDiagnosticCode::ExcludedConstruct, Cur().Span, FGitsDiagnosticParams(FString::Printf(TEXT("Unpacking with %s"), *Cur().Text)));
					return false;
				}
				if (Cur().Is(EGitsTokenType::Name) && PeekTok().IsOp(TEXT("=")))
				{
					Fail(EGitsDiagnosticCode::ExcludedKeywordArgument, FGitsSpan::Spanning(Cur().Span, PeekTok().Span));
					return false;
				}
				FNode Arg = ParseExpr();
				if (!Arg.IsValid()) { return false; }
				if (AtKeyword(TEXT("for"))) { Fail(EGitsDiagnosticCode::ExcludedComprehension, Cur().Span); return false; }
				Out.Add(Arg);
				if (AtOp(TEXT(","))) { Advance(); continue; }
				if (!AtOp(TEXT(")"))) { Unexpected(); return false; }
			}
			Advance();
			return true;
		}

		FNode ParseAtom()
		{
			const FGitsToken T = Cur();
			switch (T.Type)
			{
			case EGitsTokenType::Number:
			{
				Advance();
				FNode N = NewNode(EGitsNodeKind::Num, T.Span);
				N->Text = T.Text; N->bIsFloat = T.bIsFloat; N->IntValue = T.IntValue; N->FloatValue = T.FloatValue;
				return N;
			}
			case EGitsTokenType::String:
			{
				Advance();
				FNode N = NewNode(EGitsNodeKind::Str, T.Span);
				N->StrValue = T.StringValue;
				return N;
			}
			case EGitsTokenType::Name:
			{
				Advance();
				FNode N = NewNode(EGitsNodeKind::Name, T.Span);
				N->Text = T.Text;
				return N;
			}
			case EGitsTokenType::Keyword:
			{
				if (T.Text == TEXT("True") || T.Text == TEXT("False"))
				{
					Advance();
					FNode N = NewNode(EGitsNodeKind::Bool, T.Span);
					N->bBoolValue = T.Text == TEXT("True");
					return N;
				}
				if (T.Text == TEXT("None")) { Advance(); return NewNode(EGitsNodeKind::NoneLit, T.Span); }
				if (T.Text == TEXT("lambda")) { return Fail(EGitsDiagnosticCode::ExcludedLambda, T.Span); }
				if (T.Text == TEXT("not")) { return ParseNot(); }
				return KeywordAsValue(T);
			}
			case EGitsTokenType::Op:
				if (T.Text == TEXT("(")) { return ParseGroup(); }
				if (T.Text == TEXT("[")) { return ParseList(); }
				if (T.Text == TEXT("{")) { return ParseBraces(); }
				return Unexpected();
			case EGitsTokenType::Error:
				return Abandon();
			default:
				return Unexpected();
			}
		}

		/** A keyword where a value should be: locked if it belongs to a later tier, excluded otherwise. */
		FNode KeywordAsValue(const FGitsToken& T)
		{
			struct FGated { const TCHAR* Kw; EGitsTier Tier; const TCHAR* Subject; };
			static const FGated Gated[] = {
				{ TEXT("if"), EGitsTier::Two, TEXT("An if statement") }, { TEXT("elif"), EGitsTier::Two, TEXT("An if statement") },
				{ TEXT("else"), EGitsTier::Two, TEXT("An if statement") }, { TEXT("while"), EGitsTier::Three, TEXT("A while loop") },
				{ TEXT("for"), EGitsTier::Three, TEXT("A for loop") }, { TEXT("break"), EGitsTier::Three, TEXT("break") },
				{ TEXT("continue"), EGitsTier::Three, TEXT("continue") }, { TEXT("in"), EGitsTier::Three, TEXT("The in operator") },
				{ TEXT("def"), EGitsTier::Four, TEXT("A function definition") }, { TEXT("return"), EGitsTier::Four, TEXT("return") },
			};
			for (const FGated& G : Gated)
			{
				if (T.Text == G.Kw)
				{
					if (Locked(G.Tier, G.Subject, T.Span)) { return Abandon(); }
					return Unexpected();
				}
			}
			if (T.Text == TEXT("import") || T.Text == TEXT("from")) { return Fail(EGitsDiagnosticCode::ExcludedImport, T.Span); }
			if (T.Text == TEXT("class")) { return Fail(EGitsDiagnosticCode::ExcludedClass, T.Span); }
			if (T.Text == TEXT("global") || T.Text == TEXT("nonlocal")) { return Fail(EGitsDiagnosticCode::ExcludedScopeDeclaration, T.Span, FGitsDiagnosticParams(T.Text)); }
			if (T.Text == TEXT("try") || T.Text == TEXT("except") || T.Text == TEXT("finally") || T.Text == TEXT("raise")) { return Fail(EGitsDiagnosticCode::ExcludedExceptionHandling, T.Span); }
			if (T.Text == TEXT("with")) { return Fail(EGitsDiagnosticCode::ExcludedWith, T.Span); }
			const FString Subject = T.Text == TEXT("pass") ? TEXT("A pass statement") : T.Text == TEXT("del") ? TEXT("Deleting a name with del")
				: T.Text == TEXT("assert") ? TEXT("An assert") : T.Text == TEXT("is") ? TEXT("The is operator") : T.Text;
			return Fail(EGitsDiagnosticCode::ExcludedConstruct, T.Span, FGitsDiagnosticParams(Subject));
		}

		FNode ParseGroup()
		{
			const FGitsSpan OpenSpan = Cur().Span;
			Advance();
			if (AtOp(TEXT(")"))) { return Fail(EGitsDiagnosticCode::ExcludedTuple, FGitsSpan::Spanning(OpenSpan, Cur().Span)); }
			FNode Inner = ParseExpr();
			if (!Inner.IsValid()) { return nullptr; }
			if (AtOp(TEXT(","))) { return Fail(EGitsDiagnosticCode::ExcludedTuple, FGitsSpan::Spanning(OpenSpan, Cur().Span)); }
			if (AtKeyword(TEXT("for"))) { return Fail(EGitsDiagnosticCode::ExcludedComprehension, Cur().Span); }
			if (!AtOp(TEXT(")"))) { return Unexpected(); }
			Advance();
			// Grouping returns the inner node itself; parentheses leave no trace in spans.
			return Inner;
		}

		FNode ParseList()
		{
			const FGitsPosition Start = Cur().Span.Start;
			const bool bLocked = Locked(EGitsTier::Three, TEXT("A list"), Cur().Span);
			Advance();
			FNode N = NewNode(EGitsNodeKind::List, FGitsSpan());
			while (!AtOp(TEXT("]")))
			{
				FNode E = ParseExpr();
				if (!E.IsValid()) { return nullptr; }
				if (AtKeyword(TEXT("for"))) { return Fail(EGitsDiagnosticCode::ExcludedComprehension, Cur().Span); }
				N->Elts.Add(E);
				if (AtOp(TEXT(","))) { Advance(); continue; }
				if (!AtOp(TEXT("]"))) { return Unexpected(); }
			}
			Advance();
			N->Span = SpanFrom(Start);
			if (bLocked) { return DropRest(); }
			return N;
		}

		/** Recognition cases 1, 2 and 9 for braces. Always a diagnostic; never a node. */
		FNode ParseBraces()
		{
			const FGitsSpan OpenSpan = Cur().Span;
			int32 Depth = 0;
			bool bColon = false;
			bool bFor = false;
			int32 i = Pos;
			for (; i < Tokens.Num(); ++i)
			{
				const FGitsToken& T = Tokens[i];
				if (T.Is(EGitsTokenType::Newline) || T.Is(EGitsTokenType::Eof) || T.Is(EGitsTokenType::Error)) { break; }
				if (T.IsOp(TEXT("{")) || T.IsOp(TEXT("(")) || T.IsOp(TEXT("["))) { ++Depth; continue; }
				if (T.IsOp(TEXT("}")) || T.IsOp(TEXT(")")) || T.IsOp(TEXT("]")))
				{
					--Depth;
					if (Depth == 0) { break; }
					continue;
				}
				if (Depth == 1 && T.IsOp(TEXT(":"))) { bColon = true; }
				if (Depth == 1 && T.IsKeyword(TEXT("for"))) { bFor = true; }
			}
			if (i < Tokens.Num() && Tokens[i].Is(EGitsTokenType::Error)) { return Abandon(); }
			const EGitsDiagnosticCode Code = bFor ? EGitsDiagnosticCode::ExcludedComprehension
				: bColon ? EGitsDiagnosticCode::ExcludedDictLiteral : EGitsDiagnosticCode::ExcludedSetLiteral;
			return Fail(Code, OpenSpan);
		}

		/** After a locked construct inside an expression: consume the statement and drop it. */
		FNode DropRest() { SkipStatement(); return nullptr; }
	};
}

namespace GitsParser
{
	FGitsParseResult Parse(const FString& Source, const FGitsParseOptions& Options)
	{
		FGitsLexResult Lex = GitsLexer::Tokenize(Source);
		FParser P(Lex, Options.Tier);
		return P.Run();
	}

	TArray<FString> CodeNames(const FGitsParseResult& Result)
	{
		TArray<FString> Out;
		for (const FGitsDiagnostic& D : Result.Diagnostics) { Out.Add(GitsDiagnostics::CodeName(D.Code)); }
		return Out;
	}
}

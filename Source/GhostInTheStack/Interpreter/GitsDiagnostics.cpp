#include "GitsDiagnostics.h"

namespace
{
	FString SubjectOf(const FGitsDiagnosticParams& P) { return P.bHasSubject ? P.Subject : TEXT("this"); }
	FString DetailOf(const FGitsDiagnosticParams& P) { return P.bHasDetail ? P.Detail : TEXT(""); }

	/** The shared closing line for excluded syntax, in exactly one place. */
	FString NotHere(const FString& What)
	{
		return What + TEXT(" is not part of this station's system. Ilse never used it, so the interpreter that runs this facility does not implement it.");
	}

	struct FEntry
	{
		const TCHAR* Name;
		const TCHAR* Summary;
		TFunction<FString(const FGitsDiagnosticParams&)> What;
		TFunction<FString(const FGitsDiagnosticParams&)> Check;
	};

	const FEntry& EntryFor(EGitsDiagnosticCode Code)
	{
		static TArray<FEntry> Table;
		if (Table.Num() == 0)
		{
			Table.SetNum((int32)EGitsDiagnosticCode::Count);
			auto Set = [](EGitsDiagnosticCode C, const TCHAR* Name, const TCHAR* Summary,
				TFunction<FString(const FGitsDiagnosticParams&)> What, TFunction<FString(const FGitsDiagnosticParams&)> Check)
			{
				Table[(int32)C] = FEntry{ Name, Summary, What, Check };
			};
			using P = const FGitsDiagnosticParams&;

			// --- lexical
			Set(EGitsDiagnosticCode::TabIndentation, TEXT("tab-indentation"),
				TEXT("A tab character appears in a line's leading whitespace. Recognition case 18."),
				[](P) { return FString(TEXT("This line is indented with a tab.")); },
				[](P) { return FString(TEXT("Replace the tab with four spaces. Indentation here is always four spaces per level, and a tab is one character however wide it looks.")); });
			Set(EGitsDiagnosticCode::InconsistentIndentation, TEXT("inconsistent-indentation"),
				TEXT("A line's indentation matches no open block."),
				[](P p) { return FString::Printf(TEXT("This line is indented %s spaces, which does not line up with any block that is open here."), *SubjectOf(p)); },
				[](P p) { return FString::Printf(TEXT("Indent it %s spaces to sit inside the block above, or take it back to the left to close that block."), *DetailOf(p)); });
			Set(EGitsDiagnosticCode::UnexpectedIndent, TEXT("unexpected-indent"),
				TEXT("A line is indented but nothing above it opened a block."),
				[](P) { return FString(TEXT("This line is indented, but the line above it does not start a block.")); },
				[](P) { return FString(TEXT("Move it back to the left. Only a line ending in a colon opens a block, and only the lines inside that block are indented.")); });
			Set(EGitsDiagnosticCode::UnterminatedString, TEXT("unterminated-string"),
				TEXT("A piece of text reaches the end of the line without a closing quote."),
				[](P) { return FString(TEXT("This piece of text opens with a quote mark but never closes it.")); },
				[](P p) { return FString::Printf(TEXT("Add a matching %s at the end of the text. A piece of text has to start and finish on the same line."), *SubjectOf(p)); });
			Set(EGitsDiagnosticCode::UnexpectedCharacter, TEXT("unexpected-character"),
				TEXT("A character that means nothing in this subset."),
				[](P p) { return FString::Printf(TEXT("The character %s does not mean anything here."), *SubjectOf(p)); },
				[](P) { return FString(TEXT("Check for a typo. If you meant it as part of a piece of text, it needs to be inside quote marks.")); });
			Set(EGitsDiagnosticCode::MalformedNumber, TEXT("malformed-number"),
				TEXT("Digits run directly into letters."),
				[](P p) { return FString::Printf(TEXT("%s starts as a number and then runs into letters."), *SubjectOf(p)); },
				[](P) { return FString(TEXT("Put a space or an operator between the number and the name, or fix the spelling if it was meant to be one word.")); });

			// --- structural
			Set(EGitsDiagnosticCode::MissingColon, TEXT("missing-colon"),
				TEXT("A block-opening statement has no colon."),
				[](P p) { return FString::Printf(TEXT("This %s line needs a colon at the end."), *SubjectOf(p)); },
				[](P p) { return FString::Printf(TEXT("Add a colon after the condition, then indent the lines that belong to the %s by four spaces."), *SubjectOf(p)); });
			Set(EGitsDiagnosticCode::ExpectedIndentedBlock, TEXT("expected-indented-block"),
				TEXT("A colon opened a block and nothing was indented under it."),
				[](P p) { return FString::Printf(TEXT("The %s on this line opens a block, but no lines are indented under it."), *SubjectOf(p)); },
				[](P) { return FString(TEXT("Indent at least one line by four spaces underneath, so the machine knows what belongs inside.")); });
			Set(EGitsDiagnosticCode::UnclosedBracket, TEXT("unclosed-bracket"),
				TEXT("A bracket is opened and never closed."),
				[](P p) { return FString::Printf(TEXT("This %s is opened here and never closed."), *SubjectOf(p)); },
				[](P p) { return FString::Printf(TEXT("Add the matching %s. Count them from left to right: every one that opens has to close."), *DetailOf(p)); });
			Set(EGitsDiagnosticCode::UnexpectedToken, TEXT("unexpected-token"),
				TEXT("The generic fallback. Something appears where nothing valid could."),
				[](P p) { return FString::Printf(TEXT("%s cannot appear here."), *SubjectOf(p)); },
				[](P) { return FString(TEXT("Read the line from the left and check each part is something you can name. A missing operator or a stray bracket is the usual cause.")); });
			Set(EGitsDiagnosticCode::InvalidAssignmentTarget, TEXT("invalid-assignment-target"),
				TEXT("The left-hand side of = is not a name."),
				[](P) { return FString(TEXT("The left of the equals sign has to be a name, and this is not one.")); },
				[](P) { return FString(TEXT("Assignment stores a value under a name, so it reads name = value. Check which side you meant to be which.")); });
			Set(EGitsDiagnosticCode::AssignmentInCondition, TEXT("assignment-in-condition"),
				TEXT("A single = used where == was meant. A high-value novice error."),
				[](P) { return FString(TEXT("This condition uses one equals sign, which stores a value rather than comparing two.")); },
				[](P) { return FString(TEXT("Use two equals signs to ask whether the values match. One equals sign means \"put this value under this name\".")); });
			Set(EGitsDiagnosticCode::LeadingPlus, TEXT("leading-plus"),
				TEXT("A unary plus, which this subset does not have."),
				[](P) { return FString(TEXT("This plus sign has nothing on its left to add to.")); },
				[](P) { return FString(TEXT("Remove it. A plus needs a value on both sides, and a positive number does not need marking as positive.")); });
			Set(EGitsDiagnosticCode::ElifWithoutIf, TEXT("elif-without-if"),
				TEXT("elif or else with no matching if."),
				[](P p) { return FString::Printf(TEXT("This %s has no if above it to attach to."), *SubjectOf(p)); },
				[](P p) { return FString::Printf(TEXT("Every %s continues an if. Check the if is at the same indentation and that nothing between them broke the chain."), *SubjectOf(p)); });
			Set(EGitsDiagnosticCode::ReturnOutsideFunction, TEXT("return-outside-function"),
				TEXT("return where there is no function to return from."),
				[](P) { return FString(TEXT("This line hands a value back, but there is nothing here to hand it back to.")); },
				[](P) { return FString(TEXT("return only makes sense inside a function. If you meant to show the value, use print().")); });

			// --- recognition: not here
			Set(EGitsDiagnosticCode::ExcludedDictLiteral, TEXT("excluded-dict-literal"),
				TEXT("Recognition case 1. Braces containing a colon."),
				[](P) { return NotHere(TEXT("A dictionary")); },
				[](P) { return FString(TEXT("Store the values under separate names, or in a list once lists are available to you.")); });
			Set(EGitsDiagnosticCode::ExcludedSetLiteral, TEXT("excluded-set-literal"),
				TEXT("Recognition case 2. Braces without a colon."),
				[](P) { return NotHere(TEXT("A set")); },
				[](P) { return FString(TEXT("Use a list instead once lists are available to you. Nothing in this facility needs a set.")); });
			Set(EGitsDiagnosticCode::ExcludedFString, TEXT("excluded-fstring"),
				TEXT("Recognition case 3. An f or F immediately before a quote."),
				[](P) { return NotHere(TEXT("An f-string")); },
				[](P) { return FString(TEXT("Join the pieces with + instead, converting numbers with str() first: \"depth \" + str(depth).")); });
			Set(EGitsDiagnosticCode::ExcludedImport, TEXT("excluded-import"),
				TEXT("Recognition case 4. Leading import or from."),
				[](P) { return NotHere(TEXT("Importing a module")); },
				[](P) { return FString(TEXT("Everything this shift needs is already here: the builtins and the station functions in the manual.")); });
			Set(EGitsDiagnosticCode::ExcludedClass, TEXT("excluded-class"),
				TEXT("Recognition case 5. Leading class."),
				[](P) { return NotHere(TEXT("Defining a class")); },
				[](P) { return FString(TEXT("Use names and functions instead. Nothing in the station is built out of classes.")); });
			Set(EGitsDiagnosticCode::ExcludedExceptionHandling, TEXT("excluded-exception-handling"),
				TEXT("Recognition case 6. Leading try, except, finally or raise."),
				[](P) { return NotHere(TEXT("Catching errors")); },
				[](P) { return FString(TEXT("Check the value with an if before you use it. Failures here are meant to stop the shift and be read.")); });
			Set(EGitsDiagnosticCode::ExcludedWith, TEXT("excluded-with"),
				TEXT("Recognition case 7. Leading with."),
				[](P) { return NotHere(TEXT("A with block")); },
				[](P) { return FString(TEXT("Call the station function directly. Nothing here needs opening and closing around a block.")); });
			Set(EGitsDiagnosticCode::ExcludedLambda, TEXT("excluded-lambda"),
				TEXT("Recognition case 8. lambda in expression position."),
				[](P) { return NotHere(TEXT("A lambda")); },
				[](P) { return FString(TEXT("Write it as a named function with def once functions are available to you.")); });
			Set(EGitsDiagnosticCode::ExcludedComprehension, TEXT("excluded-comprehension"),
				TEXT("Recognition case 9. A for inside a bracket."),
				[](P) { return NotHere(TEXT("A comprehension")); },
				[](P) { return FString(TEXT("Write the loop out in full: start with an empty list, then append to it one item at a time.")); });
			Set(EGitsDiagnosticCode::ExcludedTuple, TEXT("excluded-tuple"),
				TEXT("Recognition case 10. A comma-separated value list."),
				[](P) { return NotHere(TEXT("A tuple, or several values separated by commas")); },
				[](P) { return FString(TEXT("Give each value its own line and its own name. One name holds one value here.")); });
			Set(EGitsDiagnosticCode::ExcludedSlicing, TEXT("excluded-slicing"),
				TEXT("Recognition case 11. A colon inside a subscript."),
				[](P) { return NotHere(TEXT("Slicing a range out of a list")); },
				[](P) { return FString(TEXT("Take one item at a time with a single index, or loop over the list and pick out what you need.")); });
			Set(EGitsDiagnosticCode::ExcludedScopeDeclaration, TEXT("excluded-scope-declaration"),
				TEXT("Recognition case 12. Leading global or nonlocal."),
				[](P p) { return NotHere(FString::Printf(TEXT("A %s declaration"), *SubjectOf(p))); },
				[](P) { return FString(TEXT("Pass the value into the function as an argument and hand it back with return instead.")); });
			Set(EGitsDiagnosticCode::ExcludedKeywordArgument, TEXT("excluded-keyword-argument"),
				TEXT("Recognition case 13. name= inside an argument list."),
				[](P) { return NotHere(TEXT("Naming an argument at the call")); },
				[](P) { return FString(TEXT("Pass the values in order, without the name and the equals sign, matching the order the function lists them in.")); });
			Set(EGitsDiagnosticCode::ExcludedDefaultArgument, TEXT("excluded-default-argument"),
				TEXT("Recognition case 14. = inside a parameter list."),
				[](P) { return NotHere(TEXT("A default value for a parameter")); },
				[](P) { return FString(TEXT("Require the argument every time, and pass it explicitly at each call.")); });
			Set(EGitsDiagnosticCode::ExcludedComparisonChaining, TEXT("excluded-comparison-chaining"),
				TEXT("Recognition case 15. Two comparisons at one level. Refused, never mis-evaluated."),
				[](P) { return FString(TEXT("This line compares three things in a row. Real Python allows that, but this station's interpreter does not, and it will not guess at what you meant.")); },
				[](P) { return FString(TEXT("Split it into two comparisons joined by and: instead of 1 < x < 10, write 1 < x and x < 10.")); });
			Set(EGitsDiagnosticCode::ExcludedMultipleAssignment, TEXT("excluded-multiple-assignment"),
				TEXT("Chained assignment, a = b = 1."),
				[](P) { return NotHere(TEXT("Assigning to several names at once")); },
				[](P) { return FString(TEXT("Give each name its own line, so the order the values are stored in stays visible.")); });
			Set(EGitsDiagnosticCode::ExcludedConstruct, TEXT("excluded-construct"),
				TEXT("A construct on the excluded list with no enumerated case of its own."),
				[](P p) { return NotHere(SubjectOf(p)); },
				[](P) { return FString(TEXT("Check the station manual for what this shift has available to it.")); });

			// --- recognition: not yet
			Set(EGitsDiagnosticCode::NotYetUnlocked, TEXT("not-yet-unlocked"),
				TEXT("Recognition case 16. Correct Python from a tier above the level's."),
				[](P p) { return FString::Printf(TEXT("%s is real Python and it works, but this shift does not have it yet. It arrives at tier %s."), *SubjectOf(p), *DetailOf(p)); },
				[](P) { return FString(TEXT("Solve this one with what the shift gives you. You will get this back later, and it will still be right.")); });
			Set(EGitsDiagnosticCode::MethodNotAvailable, TEXT("method-not-available"),
				TEXT("Recognition case 17. A method call outside the tier whitelist."),
				[](P p) { return FString::Printf(TEXT("There is no %s available on this shift."), *SubjectOf(p)); },
				[](P) { return FString(TEXT("Check the spelling. If it is a real Python method, it is from a later tier than this shift has unlocked.")); });

			// --- runtime
			Set(EGitsDiagnosticCode::NameNotDefined, TEXT("name-not-defined"),
				TEXT("A name is read before anything was stored under it."),
				[](P p) { return FString::Printf(TEXT("Nothing has been stored under the name %s yet."), *SubjectOf(p)); },
				[](P p) { return FString::Printf(TEXT("Check the spelling against where %s is set, and check that the line setting it runs before this one."), *SubjectOf(p)); });
			Set(EGitsDiagnosticCode::TypeMismatch, TEXT("type-mismatch"),
				TEXT("An operator applied to kinds of value it does not join."),
				[](P p) { return FString::Printf(TEXT("This line tries to %s."), *SubjectOf(p)); },
				[](P) { return FString(TEXT("Check which of the two is which. To join a number onto a piece of text, convert the number first with str().")); });
			Set(EGitsDiagnosticCode::DivisionByZero, TEXT("division-by-zero"),
				TEXT("Division or remainder with a right-hand side of zero."),
				[](P) { return FString(TEXT("This line divides by zero, and nothing can be divided into zero parts.")); },
				[](P) { return FString(TEXT("Check the value on the right of the division. If it can be zero, test for that with an if before dividing.")); });
			Set(EGitsDiagnosticCode::IndexOutOfRange, TEXT("index-out-of-range"),
				TEXT("A list index outside the list."),
				[](P p) { return FString::Printf(TEXT("This asks for item %s of a list that holds %s."), *SubjectOf(p), *DetailOf(p)); },
				[](P) { return FString(TEXT("Counting starts at 0, so the last item of a list of three is item 2. Check the length with len() before reaching in.")); });
			Set(EGitsDiagnosticCode::BadIndexType, TEXT("bad-index-type"),
				TEXT("A list indexed with something other than a whole number."),
				[](P p) { return FString::Printf(TEXT("A list can only be indexed with a whole number, and this is %s."), *SubjectOf(p)); },
				[](P) { return FString(TEXT("Check what is between the square brackets. It has to count a position, not name one.")); });
			Set(EGitsDiagnosticCode::NotAList, TEXT("not-a-list"),
				TEXT("Indexing or appending to something that is not a list."),
				[](P p) { return FString::Printf(TEXT("This treats %s as if it were a list, and it is not."), *SubjectOf(p)); },
				[](P) { return FString(TEXT("Check which name you meant. Only a list can be indexed or appended to.")); });
			Set(EGitsDiagnosticCode::WrongArgumentCount, TEXT("wrong-argument-count"),
				TEXT("A call with the wrong number of arguments."),
				[](P p) { return FString::Printf(TEXT("%s was given %s."), *SubjectOf(p), *DetailOf(p)); },
				[](P) { return FString(TEXT("Count the values inside the brackets against what the function expects.")); });
			Set(EGitsDiagnosticCode::UnknownFunction, TEXT("unknown-function"),
				TEXT("A call to a name that is not a function."),
				[](P p) { return FString::Printf(TEXT("There is no function called %s on this shift."), *SubjectOf(p)); },
				[](P) { return FString(TEXT("Check the spelling against the builtins and the station functions listed in the manual.")); });
			Set(EGitsDiagnosticCode::BadRange, TEXT("bad-range"),
				TEXT("range() called with a step of zero, or with non-integer arguments."),
				[](P p) { return FString::Printf(TEXT("range() cannot count %s."), *SubjectOf(p)); },
				[](P) { return FString(TEXT("range() takes whole numbers, and its step cannot be 0 because the count would never move.")); });
			Set(EGitsDiagnosticCode::StatementCapExceeded, TEXT("statement-cap-exceeded"),
				TEXT("The per-level design budget (ADR-021). A diagnosable outcome, not a crash."),
				[](P p) { return FString::Printf(TEXT("This program is still running after %s steps of work, which is more than this shift has power for."), *SubjectOf(p)); },
				[](P p) { return FString::Printf(TEXT("Look at the loop on line %s. Something inside it has to change each time round, or the condition that ends it will never come true."), *DetailOf(p)); });
			Set(EGitsDiagnosticCode::SafetyCapExceeded, TEXT("safety-cap-exceeded"),
				TEXT("The project-wide runaway guard (ADR-008). An error path."),
				[](P) { return FString(TEXT("This program never stops. It was cut off to keep the station responsive.")); },
				[](P p) { return FString::Printf(TEXT("Look at the loop on line %s. Check that the value its condition tests actually changes inside the loop."), *DetailOf(p)); });
			Set(EGitsDiagnosticCode::CallDepthExceeded, TEXT("call-depth-exceeded"),
				TEXT("Recursion deeper than MAX_CALL_DEPTH."),
				[](P p) { return FString::Printf(TEXT("This function has called itself %s times without finishing, and it was stopped there."), *SubjectOf(p)); },
				[](P) { return FString(TEXT("A function that calls itself needs a case that returns without calling again. Check that case is reachable.")); });
		}
		return Table[(int32)Code];
	}
}

namespace GitsDiagnostics
{
	FString CodeName(EGitsDiagnosticCode Code) { return EntryFor(Code).Name; }
	FString Summary(EGitsDiagnosticCode Code) { return EntryFor(Code).Summary; }
	FString What(EGitsDiagnosticCode Code, const FGitsDiagnosticParams& P) { return EntryFor(Code).What(P); }
	FString Check(EGitsDiagnosticCode Code, const FGitsDiagnosticParams& P) { return EntryFor(Code).Check(P); }

	FGitsDiagnostic Diagnose(EGitsDiagnosticCode Code, const FGitsSpan& Span, const FGitsDiagnosticParams& P)
	{
		FGitsDiagnostic D;
		D.Code = Code;
		D.Span = Span;
		D.What = What(Code, P);
		D.Check = Check(Code, P);
		return D;
	}

	FString Format(const FGitsDiagnostic& D)
	{
		return FString::Printf(TEXT("Line %d: %s\n%s"), D.Span.Start.Line, *D.What, *D.Check);
	}

	bool IsRuntimeCode(EGitsDiagnosticCode Code)
	{
		return Code >= EGitsDiagnosticCode::NameNotDefined && Code <= EGitsDiagnosticCode::CallDepthExceeded;
	}

	bool IsRecognitionCode(EGitsDiagnosticCode Code)
	{
		return Code >= EGitsDiagnosticCode::ExcludedDictLiteral && Code <= EGitsDiagnosticCode::MethodNotAvailable;
	}

	int32 CodeCount() { return (int32)EGitsDiagnosticCode::Count; }
}

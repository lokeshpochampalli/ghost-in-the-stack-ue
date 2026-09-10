// Ghost in the Stack — the beginner-facing diagnostic catalogue.
//
// Every message has three parts: what happened in plain language, where (the span),
// and one concrete thing to check. Recognition-pass messages come in two registers:
// NOT HERE for syntax this station's system never has, NOT YET for correct Python the
// player has not unlocked. The wording is data, taken from the reference catalogue so
// the dissertation appendix can print it from source.
#pragma once

#include "CoreMinimal.h"
#include "GitsTypes.h"

struct FGitsDiagnosticParams
{
	/** What the message is about: a method name, a keyword, an offending character. */
	FString Subject;
	/** A secondary detail: an expected indent width, a required tier. */
	FString Detail;
	bool bHasSubject = false;
	bool bHasDetail = false;

	FGitsDiagnosticParams() {}
	explicit FGitsDiagnosticParams(const FString& InSubject) : Subject(InSubject), bHasSubject(true) {}
	FGitsDiagnosticParams(const FString& InSubject, const FString& InDetail)
		: Subject(InSubject), Detail(InDetail), bHasSubject(true), bHasDetail(true) {}
};

namespace GitsDiagnostics
{
	/** The catalogue's machine-readable code name, e.g. "excluded-dict-literal". */
	FString CodeName(EGitsDiagnosticCode Code);
	/** One line describing when the code fires. For the appendix, not the player. */
	FString Summary(EGitsDiagnosticCode Code);
	FString What(EGitsDiagnosticCode Code, const FGitsDiagnosticParams& P);
	FString Check(EGitsDiagnosticCode Code, const FGitsDiagnosticParams& P);

	/** Builds a diagnostic from the catalogue. The only way diagnostics are created. */
	FGitsDiagnostic Diagnose(EGitsDiagnosticCode Code, const FGitsSpan& Span, const FGitsDiagnosticParams& P = FGitsDiagnosticParams());

	/** As the player sees it: "Line N: <what>\n<check>". */
	FString Format(const FGitsDiagnostic& D);

	bool IsRuntimeCode(EGitsDiagnosticCode Code);
	bool IsRecognitionCode(EGitsDiagnosticCode Code);
	int32 CodeCount();
}

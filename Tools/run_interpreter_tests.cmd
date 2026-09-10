@echo off
rem Runs the project automation tests (interpreter, recorder) headlessly and prints the pass/fail counts.
rem Usage: Tools\run_interpreter_tests.cmd   (from the project root; the editor may be closed)
set PROJECT=%~dp0..\GhostInTheStack.uproject
set EDITOR="C:\Program Files\Epic Games\UE_5.8\Engine\Binaries\Win64\UnrealEditor-Cmd.exe"
%EDITOR% "%PROJECT%" -ExecCmds="Automation RunTests GhostInTheStack; Quit" -unattended -nopause -nullrhi -NoSound -nosplash -log=GitsTests.log > nul 2>&1
set LOG=%~dp0..\Saved\Logs\GitsTests.log
for /f %%a in ('findstr /c:"Result={Success}" "%LOG%" ^| find /c /v ""') do set PASSED=%%a
for /f %%a in ('findstr /c:"Result={Fail}" "%LOG%" ^| find /c /v ""') do set FAILED=%%a
echo passed=%PASSED% failed=%FAILED%
findstr /c:"Result={Fail}" "%LOG%"
if "%FAILED%"=="0" (exit /b 0) else (exit /b 1)

@echo off
rem Build Announce Debugger with MSVC.
rem   build.bat          release build (build\release\announce-debugger.exe + tests)
rem   build.bat test     release build, then run the tests
rem   build.bat check    debug build with CRT leak detection, then run the tests
rem   build.bat asan     AddressSanitizer build, then run the tests
rem   build.bat clean    delete the build folder
setlocal EnableExtensions
cd /d "%~dp0"

set "MODE=%~1"
if "%MODE%"=="" set "MODE=release"
if /i "%MODE%"=="clean" (
    if exist build rmdir /s /q build
    echo Dossier build supprime.
    exit /b 0
)

rem ---- Locate cl.exe (uses vswhere if not already in a developer prompt) ----
where cl >nul 2>nul
if not errorlevel 1 goto have_cl
set "VSWHERE=%ProgramFiles(x86)%\Microsoft Visual Studio\Installer\vswhere.exe"
if not exist "%VSWHERE%" goto no_vs
set "VSPATH="
for /f "usebackq delims=" %%i in (`"%VSWHERE%" -latest -products * -requires Microsoft.VisualStudio.Component.VC.Tools.x86.x64 -property installationPath`) do set "VSPATH=%%i"
if not defined VSPATH goto no_vs
call "%VSPATH%\VC\Auxiliary\Build\vcvars64.bat" >nul 2>nul
where cl >nul 2>nul
if errorlevel 1 goto no_vs
:have_cl

set "COMMON=/nologo /std:c11 /W4 /utf-8 /D_CRT_SECURE_NO_WARNINGS /DWIN32_LEAN_AND_MEAN /D_WIN32_WINNT=0x0A00 /DWINVER=0x0A00 /Isrc"
set "RUN_TESTS=1"
if /i "%MODE%"=="release" (
    set "OUT=build\release"
    set "FLAGS=%COMMON% /O2 /MT /DNDEBUG"
    set "RUN_TESTS=0"
) else if /i "%MODE%"=="test" (
    set "OUT=build\release"
    set "FLAGS=%COMMON% /O2 /MT /DNDEBUG"
) else if /i "%MODE%"=="check" (
    set "OUT=build\debug"
    set "FLAGS=%COMMON% /Od /Zi /MTd /D_DEBUG"
) else if /i "%MODE%"=="asan" (
    set "OUT=build\asan"
    set "FLAGS=%COMMON% /Od /Zi /MT /fsanitize=address"
) else (
    echo Mode inconnu : %MODE%  ^(release, test, check, asan, clean^)
    exit /b 2
)

for %%d in (core app tests) do if not exist "%OUT%\%%d" mkdir "%OUT%\%%d"

echo === Compilation (%MODE%) ===
cl %FLAGS% /c src\util.c src\bencode.c src\torrent.c src\tracker.c src\http_tracker.c src\udp_tracker.c src\peer_probe.c src\history.c /Fo%OUT%\core\ /Fd%OUT%\core\ || goto fail
lib /nologo /out:%OUT%\trackercore.lib %OUT%\core\*.obj || goto fail
cl %FLAGS% /c src\ui.c src\main.c /Fo%OUT%\app\ /Fd%OUT%\app\ || goto fail
cl %FLAGS% %OUT%\app\ui.obj %OUT%\app\main.obj %OUT%\trackercore.lib /Fe%OUT%\announce-debugger.exe /link /SUBSYSTEM:WINDOWS || goto fail
cl %FLAGS% tests\test_unit.c %OUT%\trackercore.lib /Fo%OUT%\tests\ /Fd%OUT%\tests\ /Fe%OUT%\test_unit.exe || goto fail
cl %FLAGS% tests\test_integration.c %OUT%\trackercore.lib /Fo%OUT%\tests\ /Fd%OUT%\tests\ /Fe%OUT%\test_integration.exe || goto fail
echo.
echo Application : %OUT%\announce-debugger.exe

if "%RUN_TESTS%"=="0" exit /b 0
echo.
echo === Tests unitaires ===
"%OUT%\test_unit.exe" || goto fail
echo.
echo === Tests d'integration ===
"%OUT%\test_integration.exe" || goto fail
echo.
echo Tous les tests passent.
exit /b 0

:no_vs
echo MSVC introuvable. Installe "Build Tools pour Visual Studio" avec la charge de travail
echo "Developpement Desktop en C++", ou lance ce script depuis "Developer Command Prompt".
exit /b 1

:fail
echo.
echo ECHEC.
exit /b 1

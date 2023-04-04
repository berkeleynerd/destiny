pushd %~dp0
echo off
if not exist "%CCP_EVE_PERFORCE_BRANCH_PATH%" (
    echo Please set the CCP_EVE_PERFORCE_BRANCH_PATH environment variable to point to the branch root of the branch containing the binaries to use when running the benchmark.
    exit /B 1
)

echo Benchmarking Release version of Destiny published in branch "%CCP_EVE_PERFORCE_BRANCH_PATH%"
if not exist bin (mkdir bin)
xcopy /y /D "%CCP_EVE_PERFORCE_BRANCH_PATH%\vendor\CCP\Blue\bin\Windows\x64\v141\blue.dll" bin 1>NUL
xcopy /y /D "%CCP_EVE_PERFORCE_BRANCH_PATH%\vendor\CCP\ExefileConsole\bin\Windows\x64\v141\ExefileConsole.exe" bin 1>NUL
xcopy /y /D "%CCP_EVE_PERFORCE_BRANCH_PATH%\vendor\CCP\Exefile\bin\Windows\x64\v141\Exefile.exe" bin 1>NUL
xcopy /y /D "%CCP_EVE_PERFORCE_BRANCH_PATH%\vendor\CCP\Geo2\bin\Windows\x64\v141\_geo2.dll" bin 1>NUL
rem Always copy destiny DLL
rem (Do not rely on the /D flag because it will not copy when swapping DLL for another one that has an older timestamp.
rem This should be mostly fine for dependencies, but not for the thing we're trying to benchmark)
xcopy /y "%CCP_EVE_PERFORCE_BRANCH_PATH%\vendor\CCP\Destiny\bin\Windows\x64\v141\_destiny.dll" bin 1>NUL
xcopy /y /D "%CCP_EVE_PERFORCE_BRANCH_PATH%\vendor\python\2.7.1+ccp-stackless\bin\Windows\x64\v141\python27.dll" bin 1>NUL
.\bin\ExefileConsole.exe /buildflavor=Release /lib=%~dp0../../python;%CCP_EVE_PERFORCE_BRANCH_PATH%\\\carbon/common/stdlib;%CCP_EVE_PERFORCE_BRANCH_PATH%/packages /bin=bin /py .\run.py %*

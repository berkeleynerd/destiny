pushd %~dp0
echo off
if not exist "%CCP_EVE_PERFORCE_BRANCH_PATH%" (
    echo Please set the CCP_EVE_PERFORCE_BRANCH_PATH environment variable to point to the branch root of the branch containing the binaries to use when running the benchmark.
    exit /B 1
)
IF "%BUILDFLAVOR%"=="" (
    set BUILDFLAVOR=release
)
echo Benchmarking %BUILDFLAVOR% version of Destiny published in branch "%CCP_EVE_PERFORCE_BRANCH_PATH%"
"%CCP_EVE_PERFORCE_BRANCH_PATH%\eve\client\bin\x64\ExefileConsole.exe" /buildflavor=%BUILDFLAVOR% /lib=%~dp0../../python;%CCP_EVE_PERFORCE_BRANCH_PATH%\\\carbon/common/stdlib;%CCP_EVE_PERFORCE_BRANCH_PATH%/packages /bin="%CCP_EVE_PERFORCE_BRANCH_PATH%\eve\client\bin\x64" /py .\run.py %*

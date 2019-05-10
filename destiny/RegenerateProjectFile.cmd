@echo off
echo Checking out project and filters file
p4 edit destiny.vcxproj
echo Regenerating
..\..\..\..\..\..\shared_tools\python\27\python.exe ..\..\..\carbon\tools\ProjectFileGenerator\ProjectFileGenerator.py -i destiny.ccpproj
pause
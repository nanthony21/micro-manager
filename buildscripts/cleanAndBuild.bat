set BUILDSCRIPTPATH=%~dp0
FOR %%a IN ("%BUILDSCRIPTPATH:~0,-1%") DO SET ROOTPATH=%%~dpa
cd %ROOTPATH%


call ant clean

if errorlevel 1 (
	echo Clean Failed
	pause
	exit /B
)

call ant unstage
	
if errorlevel 1 (
	echo Unstage Failed
	pause
	exit /B
)

call ant stage
pause

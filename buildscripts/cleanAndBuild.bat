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

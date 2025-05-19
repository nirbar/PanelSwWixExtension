ECHO OFF
SET /A MY_ERR=0
CD "%~dp0"

:: Install
CALL :prepareFolders
ECHO === Testing install ===
msiexec /i FileGlobUT.msi /qn /l*v FileGlobUT.msi.log INSTALLFOLDER="%CD%\install-folder"
CALL :testInstall

:: Uninstall
CALL :prepareFolders
ECHO === Testing uninstall ===
msiexec /xFileGlobUT.msi /qn /l*v FileGlobUT.msix.log

:: Clean and exit
CALL :cleanFolders
ECHO Overall error code %MY_ERR%
EXIT /B %MY_ERR%

:prepareFolders
	CALL :cleanFolders

	MKDIR "%CD%\install-folder\AccountNamesUT"

	ECHO "test" > "%CD%\install-folder\AccountNamesUT\AccountNamesUT.wxs"
	ECHO "test" >> "%CD%\install-folder\AccountNamesUT\AccountNamesUT.wxs"

	ECHO "test" > "%CD%\install-folder\FileGlobUT.wxs"
	ECHO "test" >> "%CD%\install-folder\FileGlobUT.wxs"
EXIT /B %MY_ERR%

:cleanFolders
    RMDIR /s /q "%CD%\install-folder"
EXIT /B %MY_ERR%

:testInstall
	IF NOT EXIST "%CD%\install-folder\AccountNamesUT\AccountNamesUT.wxs" (
		ECHO File "%CD%\install-folder\AccountNamesUT\AccountNamesUT.wxs" should exist
		SET /A MY_ERR=1
	)

	FOR /F "usebackq" %%A IN ('%CD%\install-folder\AccountNamesUT\AccountNamesUT.wxs') DO set size=%%~zA
	IF %size% GTR 40 (
		ECHO File "%CD%\install-folder\AccountNamesUT\AccountNamesUT.wxs" should not have been overwritten
		SET /A MY_ERR=1
	) 

	IF NOT EXIST "%CD%\install-folder\FileGlobUT.wxs" (
		ECHO File "%CD%\install-folder\FileGlobUT.wxs" should exist
		SET /A MY_ERR=1
	)

	FOR /F "usebackq" %%A IN ('%CD%\install-folder\FileGlobUT.wxs') DO set size=%%~zA
	IF %size% LSS 40 (
		ECHO File "%CD%\install-folder\FileGlobUT.wxs" should have been overwritten
		SET /A MY_ERR=1
	) 
EXIT /B %MY_ERR%

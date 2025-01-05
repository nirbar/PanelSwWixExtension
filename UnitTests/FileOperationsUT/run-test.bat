ECHO OFF
SET /A MY_ERR=0

:: Install
CALL :prepareFolders
msiexec /i FileOperationsUT.msi /l*v FileOperationsUT.msi.log REMOVE_ON_INSTALL="%CD%\install-remove" REMOVE_ON_REPAIR="%CD%\repair-remove" REMOVE_ON_UNINSTALL="%CD%\uninstall-remove" REMOVE_NO_RECURSIVE="%CD%\remove-no-recursive" REMOVE_ON_BOTH="%CD%\both-remove" NEVER_REMOVED="%CD%\never-remove" CONDITIONED_FOLDER="%CD%\never-remove-2"
CALL :testInstall
PAUSE

:: Repair
CALL :prepareFolders
msiexec /fvamus FileOperationsUT.msi /l*v FileOperationsUT.msif.log REMOVE_ON_INSTALL="%CD%\install-remove" REMOVE_ON_REPAIR="%CD%\repair-remove" REMOVE_ON_UNINSTALL="%CD%\uninstall-remove" REMOVE_NO_RECURSIVE="%CD%\remove-no-recursive" REMOVE_ON_BOTH="%CD%\both-remove" NEVER_REMOVED="%CD%\never-remove" CONDITIONED_FOLDER="%CD%\never-remove-2"
CALL :testRepair
PAUSE

:: Uninstall
CALL :prepareFolders
msiexec /xFileOperationsUT.msi /l*v FileOperationsUT.msix.log REMOVE_ON_INSTALL="%CD%\install-remove" REMOVE_ON_REPAIR="%CD%\repair-remove" REMOVE_ON_UNINSTALL="%CD%\uninstall-remove" REMOVE_NO_RECURSIVE="%CD%\remove-no-recursive" REMOVE_ON_BOTH="%CD%\both-remove" NEVER_REMOVED="%CD%\never-remove" CONDITIONED_FOLDER="%CD%\never-remove-2"
CALL :testUninstall
PAUSE

:: Clean and exit
CALL :cleanFolders
ECHO Overall error code %MY_ERR%
EXIT /B %MY_ERR%

:prepareFolders
	CALL :cleanFolders

	MKDIR "%CD%\d-target"
	ECHO test > "%CD%\d-target\f-target.txt"
	ECHO test > "%CD%\d-target\f-temp.txt"

	:: Folder install-remove should be removed entirely on install
	MKDIR "%CD%\install-remove\2\3"
	ECHO test > "%CD%\install-remove\2\file.txt"
	ECHO test > "%CD%\install-remove\2\3\file.txt"
	mklink /D "%CD%\install-remove\d-sl-1" "%CD%\d-target"
	mklink /D "%CD%\install-remove\2\3\d-sl-2" "%CD%\d-target"
	mklink "%CD%\install-remove\f-sl-1.txt" "%CD%\d-target\f-target.txt"
	mklink "%CD%\install-remove\f-sl-dangling-1.txt" "%CD%\d-target\f-temp.txt"
	mklink "%CD%\install-remove\2\3\f-sl-2.txt" "%CD%\d-target\f-target.txt"
	mklink "%CD%\install-remove\2\3\f-sl-dangling-2.txt" "%CD%\d-target\f-temp.txt"

	:: Folder repair-remove should be removed entirely on repair
	MKDIR "%CD%\repair-remove\2\3"
	ECHO test > "%CD%\repair-remove\2\file.txt"
	ECHO test > "%CD%\repair-remove\2\3\file.txt"
	mklink /D "%CD%\repair-remove\d-sl-1" "%CD%\d-target"
	mklink /D "%CD%\repair-remove\2\3\d-sl-2" "%CD%\d-target"
	mklink "%CD%\repair-remove\f-sl-1.txt" "%CD%\d-target\f-target.txt"
	mklink "%CD%\repair-remove\f-sl-dangling-1.txt" "%CD%\d-target\f-temp.txt"
	mklink "%CD%\repair-remove\2\3\f-sl-2.txt" "%CD%\d-target\f-target.txt"
	mklink "%CD%\repair-remove\2\3\f-sl-dangling-2.txt" "%CD%\d-target\f-temp.txt"

	:: Folder uninstall-remove should be removed entirely on uninstall
	MKDIR "%CD%\uninstall-remove\2\3"
	ECHO test > "%CD%\uninstall-remove\2\file.txt"
	ECHO test > "%CD%\uninstall-remove\2\3\file.txt"
	mklink /D "%CD%\uninstall-remove\d-sl-1" "%CD%\d-target"
	mklink /D "%CD%\uninstall-remove\2\3\d-sl-2" "%CD%\d-target"
	mklink "%CD%\uninstall-remove\f-sl-1.txt" "%CD%\d-target\f-target.txt"
	mklink "%CD%\uninstall-remove\f-sl-dangling-1.txt" "%CD%\d-target\f-temp.txt"
	mklink "%CD%\uninstall-remove\2\3\f-sl-2.txt" "%CD%\d-target\f-target.txt"
	mklink "%CD%\uninstall-remove\2\3\f-sl-dangling-2.txt" "%CD%\d-target\f-temp.txt"

	:: Folder both-remove should be removed entirely on both install and uninstall
	MKDIR "%CD%\both-remove\2\3"
	ECHO test > "%CD%\both-remove\2\file.txt"
	ECHO test > "%CD%\both-remove\2\3\file.txt"
	mklink /D "%CD%\both-remove\d-sl-1" "%CD%\d-target"
	mklink /D "%CD%\both-remove\2\3\d-sl-2" "%CD%\d-target"
	mklink "%CD%\both-remove\f-sl-1.txt" "%CD%\d-target\f-target.txt"
	mklink "%CD%\both-remove\f-sl-dangling-1.txt" "%CD%\d-target\f-temp.txt"
	mklink "%CD%\both-remove\2\3\f-sl-2.txt" "%CD%\d-target\f-target.txt"
	mklink "%CD%\both-remove\2\3\f-sl-dangling-2.txt" "%CD%\d-target\f-temp.txt"

	:: Folder both-remove should be removed entirely on both install and uninstall
	MKDIR "%CD%\remove-no-recursive\2\3"
	ECHO test > "%CD%\remove-no-recursive\file.txt"
	ECHO test > "%CD%\remove-no-recursive\2\3\file.txt"
	mklink /D "%CD%\remove-no-recursive\d-sl-1" "%CD%\d-target"
	mklink /D "%CD%\remove-no-recursive\2\3\d-sl-2" "%CD%\d-target"
	mklink "%CD%\remove-no-recursive\f-sl-1.txt" "%CD%\d-target\f-target.txt"
	mklink "%CD%\remove-no-recursive\f-sl-dangling-1.txt" "%CD%\d-target\f-temp.txt"
	mklink "%CD%\remove-no-recursive\2\3\f-sl-2.txt" "%CD%\d-target\f-target.txt"
	mklink "%CD%\remove-no-recursive\2\3\f-sl-dangling-2.txt" "%CD%\d-target\f-temp.txt"

	:: Folder "%CD%\never-remove" and all content should never be removed
	MKDIR "%CD%\never-remove"
	ECHO test > "%CD%\never-remove\file.txt"
	mklink /D "%CD%\never-remove\d-sl-2" "%CD%\d-target"
	mklink "%CD%\never-remove\f-sl-1.txt" "%CD%\d-target\f-target.txt"
	mklink "%CD%\never-remove\f-sl-dangling-1.txt" "%CD%\d-target\f-temp.txt"

	:: Folder "%CD%\never-remove-2" and all content should never be removed
	MKDIR "%CD%\never-remove-2"
	ECHO test > "%CD%\never-remove-2\file.txt"
	mklink /D "%CD%\never-remove-2\d-sl-2" "%CD%\d-target"
	mklink "%CD%\never-remove-2\f-sl-1.txt" "%CD%\d-target\f-target.txt"
	mklink "%CD%\never-remove-2\f-sl-dangling-1.txt" "%CD%\d-target\f-temp.txt"

	DEL "%CD%\d-target\f-temp.txt"
EXIT /B %MY_ERR%

:cleanFolders
    RMDIR /s /q "%CD%\d-target"
    RMDIR /s /q "%CD%\install-remove"
    RMDIR /s /q "%CD%\repair-remove"
    RMDIR /s /q "%CD%\uninstall-remove"
    RMDIR /s /q "%CD%\both-remove"
    RMDIR /s /q "%CD%\remove-no-recursive"
    RMDIR /s /q "%CD%\never-remove"
    RMDIR /s /q "%CD%\never-remove-2"
EXIT /B %MY_ERR%

:testInstall
	IF EXIST "%CD%\install-remove\" (
		ECHO Folder "%CD%\install-remove\" should not exist
		SET /A MY_ERR=1
	)
	IF NOT EXIST "%CD%\repair-remove\" (
		ECHO Folder "%CD%\repair-remove\" should exist
		SET /A MY_ERR=1
	)
	IF NOT EXIST "%CD%\uninstall-remove\" (
		ECHO Folder "%CD%\uninstall-remove\" should exist
		SET /A MY_ERR=1
	)
	IF EXIST "%CD%\both-remove\2\3\file.txt" (
		ECHO Folder "%CD%\both-remove\" should not exist
		SET /A MY_ERR=1
	)
	IF NOT EXIST "%CD%\remove-no-recursive\2\3\file.txt" (
		ECHO Folder "%CD%\remove-no-recursive\2\" should exist
		SET /A MY_ERR=1
	)
	IF EXIST "%CD%\remove-no-recursive\file.txt" (
		ECHO File "%CD%\remove-no-recursive\file.txt" should not exist
		SET /A MY_ERR=1
	)
	IF NOT EXIST "%CD%\never-remove\file.txt" (
		ECHO File "%CD%\never-remove\" should exist
		SET /A MY_ERR=1
	)
	IF NOT EXIST "%CD%\never-remove\d-sl-2\" (
		ECHO Link "%CD%\never-remove\d-sl-2\" should exist
		SET /A MY_ERR=1
	)
	IF NOT EXIST "%CD%\never-remove\f-sl-1.txt" (
		ECHO Link "%CD%\never-remove\f-sl-1.txt" should exist
		SET /A MY_ERR=1
	)
	IF NOT EXIST "%CD%\never-remove\" (
		ECHO Folder "%CD%\both-remove\" should exist
		SET /A MY_ERR=1
	)
	IF NOT EXIST "%CD%\never-remove-2\" (
		ECHO Folder "%CD%\both-remove-2\" should exist
		SET /A MY_ERR=1
	)
EXIT /B %MY_ERR%

:testRepair
	IF EXIST "%CD%\install-remove\" (
		ECHO Folder "%CD%\install-remove\" should not exist
		SET /A MY_ERR=1
	)
	IF EXIST "%CD%\repair-remove\" (
		ECHO Folder "%CD%\repair-remove\" should not exist
		SET /A MY_ERR=1
	)
	IF NOT EXIST "%CD%\uninstall-remove\2\3\file.txt" (
		ECHO Folder "%CD%\uninstall-remove\" should exist
		SET /A MY_ERR=1
	)
	IF EXIST "%CD%\both-remove\" (
		ECHO Folder "%CD%\both-remove\" should not exist
		SET /A MY_ERR=1
	)
	IF NOT EXIST "%CD%\remove-no-recursive\2\3\file.txt" (
		ECHO Folder "%CD%\remove-no-recursive\2\" should exist
		SET /A MY_ERR=1
	)
	IF EXIST "%CD%\remove-no-recursive\file.txt" (
		ECHO File "%CD%\remove-no-recursive\file.txt" should not exist
		SET /A MY_ERR=1
	)
	IF NOT EXIST "%CD%\never-remove\file.txt" (
		ECHO File "%CD%\never-remove\" should exist
		SET /A MY_ERR=1
	)
	IF NOT EXIST "%CD%\never-remove\d-sl-2\" (
		ECHO Link "%CD%\never-remove\d-sl-2\" should exist
		SET /A MY_ERR=1
	)
	IF NOT EXIST "%CD%\never-remove\f-sl-1.txt" (
		ECHO Link "%CD%\never-remove\f-sl-1.txt" should exist
		SET /A MY_ERR=1
	)
	IF NOT EXIST "%CD%\never-remove\" (
		ECHO Folder "%CD%\both-remove\" should exist
		SET /A MY_ERR=1
	)
	IF NOT EXIST "%CD%\never-remove-2\" (
		ECHO Folder "%CD%\both-remove-2\" should exist
		SET /A MY_ERR=1
	)
EXIT /B %MY_ERR%

:testUninstall
	IF NOT EXIST "%CD%\install-remove\2\3\file.txt" (
		ECHO Folder "%CD%\install-remove\" should exist
		SET /A MY_ERR=1
	)
	IF NOT EXIST "%CD%\repair-remove\2\3\file.txt" (
		ECHO Folder "%CD%\repair-remove\" should exist
		SET /A MY_ERR=1
	)
	IF EXIST "%CD%\uninstall-remove\" (
		ECHO Folder "%CD%\uninstall-remove\" should not exist
		SET /A MY_ERR=1
	)
	IF EXIST "%CD%\both-remove\" (
		ECHO Folder "%CD%\both-remove\" should not exist
		SET /A MY_ERR=1
	)
	IF NOT EXIST "%CD%\remove-no-recursive\2\3\file.txt" (
		ECHO Folder "%CD%\remove-no-recursive\2\" should exist
		SET /A MY_ERR=1
	)
	IF EXIST "%CD%\remove-no-recursive\file.txt" (
		ECHO File "%CD%\remove-no-recursive\file.txt" should not exist
		SET /A MY_ERR=1
	)
	IF NOT EXIST "%CD%\never-remove\file.txt" (
		ECHO File "%CD%\never-remove\" should exist
		SET /A MY_ERR=1
	)
	IF NOT EXIST "%CD%\never-remove\d-sl-2\" (
		ECHO Link "%CD%\never-remove\d-sl-2\" should exist
		SET /A MY_ERR=1
	)
	IF NOT EXIST "%CD%\never-remove\f-sl-1.txt" (
		ECHO Link "%CD%\never-remove\f-sl-1.txt" should exist
		SET /A MY_ERR=1
	)
	IF NOT EXIST "%CD%\never-remove\" (
		ECHO Folder "%CD%\both-remove\" should exist
		SET /A MY_ERR=1
	)
	IF NOT EXIST "%CD%\never-remove-2\" (
		ECHO Folder "%CD%\both-remove-2\" should exist
		SET /A MY_ERR=1
	)
EXIT /B %MY_ERR%

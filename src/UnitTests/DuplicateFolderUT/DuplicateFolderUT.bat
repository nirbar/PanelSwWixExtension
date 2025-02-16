ECHO OFF
SET /A MY_ERR=0

:: Install
CALL :prepareFolders
msiexec /i DuplicateFolderUT.msi /l*v DuplicateFolderUT.msi.log INSTALLFOLDER="%CD%\\INSTALLFOLDER" INSTALLFOLDER_COPY="%CD%\\INSTALLFOLDER_COPY" INSTALLFOLDER_COPY_2="%CD%\\INSTALLFOLDER_COPY_2"
CALL :testInstall
PAUSE

:: Uninstall
msiexec /xDuplicateFolderUT.msi /l*v DuplicateFolderUT.msix.log INSTALLFOLDER="%CD%\\INSTALLFOLDER" INSTALLFOLDER_COPY="%CD%\\INSTALLFOLDER_COPY" INSTALLFOLDER_COPY_2="%CD%\\INSTALLFOLDER_COPY_2"
CALL :testUninstall
PAUSE

:: Clean and exit
CALL :cleanFolders
ECHO Overall error code %MY_ERR%
EXIT /B %MY_ERR%

:prepareFolders
	CALL :cleanFolders

	MKDIR "%CD%\INSTALLFOLDER\sub\sub2"
	ECHO test > "%CD%\INSTALLFOLDER\a.txt"
	ECHO test > "%CD%\INSTALLFOLDER\sub\b.txt"
	ECHO test > "%CD%\INSTALLFOLDER\sub\sub2\c.txt"
EXIT /B %MY_ERR%

:cleanFolders
    RMDIR /s /q "%CD%\INSTALLFOLDER"
    RMDIR /s /q "%CD%\INSTALLFOLDER_COPY"
    RMDIR /s /q "%CD%\INSTALLFOLDER_COPY_2"
EXIT /B %MY_ERR%

:testInstall
	CALL :testCommon
	IF NOT EXIST "%CD%\INSTALLFOLDER_COPY\DuplicateFolderUT\DuplicateFolderUT.wxs" (
		ECHO File "%CD%\INSTALLFOLDER_COPY\DuplicateFolderUT\DuplicateFolderUT.wxs" should exist
		SET /A MY_ERR=1
	)
	IF NOT EXIST "%CD%\INSTALLFOLDER_COPY_2\copysub\DuplicateFolderUT\DuplicateFolderUT.wxs" (
		ECHO File "%CD%\INSTALLFOLDER_COPY_2\copysub\DuplicateFolderUT\DuplicateFolderUT.wxs" should exist
		SET /A MY_ERR=1
	)
EXIT /B %MY_ERR%

:testUninstall
	CALL :testCommon
	IF EXIST "%CD%\INSTALLFOLDER_COPY\DuplicateFolderUT\DuplicateFolderUT.wxs" (
		ECHO File "%CD%\INSTALLFOLDER_COPY\DuplicateFolderUT\DuplicateFolderUT.wxs" should not exist
		SET /A MY_ERR=1
	)
	IF EXIST "%CD%\INSTALLFOLDER_COPY_2\copysub\DuplicateFolderUT\DuplicateFolderUT.wxs" (
		ECHO File "%CD%\INSTALLFOLDER_COPY_2\copysub\DuplicateFolderUT\DuplicateFolderUT.wxs" should not exist
		SET /A MY_ERR=1
	)
EXIT /B %MY_ERR%

:testCommon
	IF EXIST "%CD%\INSTALLFOLDER_COPY\a.txt" (
		ECHO File "%CD%\INSTALLFOLDER_COPY\a.txt" should not exist
		SET /A MY_ERR=1
	)
	IF EXIST "%CD%\INSTALLFOLDER_COPY\sub\b.txt" (
		ECHO File "%CD%\INSTALLFOLDER_COPY\sub\b.txt" should not exist
		SET /A MY_ERR=1
	)
	IF EXIST "%CD%\INSTALLFOLDER_COPY\sub\sub2\c.txt" (
		ECHO File "%CD%\INSTALLFOLDER_COPY\sub\sub2\c.txt" should not exist
		SET /A MY_ERR=1
	)

	IF NOT EXIST "%CD%\INSTALLFOLDER_COPY_2\a.txt" (
		ECHO File "%CD%\INSTALLFOLDER_COPY_2\a.txt" should exist
		SET /A MY_ERR=1
	)
	IF NOT EXIST "%CD%\INSTALLFOLDER_COPY_2\sub\b.txt" (
		ECHO File "%CD%\INSTALLFOLDER_COPY_2\sub\b.txt" should exist
		SET /A MY_ERR=1
	)
	IF NOT EXIST "%CD%\INSTALLFOLDER_COPY_2\sub\sub2\c.txt" (
		ECHO File "%CD%\INSTALLFOLDER_COPY_2\sub\sub\c.txt" should exist
		SET /A MY_ERR=1
	)

	IF NOT EXIST "%CD%\INSTALLFOLDER_COPY_2\copysub\a.txt" (
		ECHO File "%CD%\INSTALLFOLDER_COPY_2\copysub\a.txt" should exist
		SET /A MY_ERR=1
	)
	IF NOT EXIST "%CD%\INSTALLFOLDER_COPY_2\copysub\sub\b.txt" (
		ECHO File "%CD%\INSTALLFOLDER_COPY_2\copysub\sub\b.txt" should exist
		SET /A MY_ERR=1
	)
	IF NOT EXIST "%CD%\INSTALLFOLDER_COPY_2\copysub\sub\sub2\c.txt" (
		ECHO File "%CD%\INSTALLFOLDER_COPY_2\copysub\sub\sub2\c.txt" should exist
		SET /A MY_ERR=1
	)

	IF NOT EXIST "%CD%\INSTALLFOLDER_COPY_2\DuplicateFolderUT\DuplicateFolderUT.wxs" (
		ECHO File "%CD%\INSTALLFOLDER_COPY_2\DuplicateFolderUT\DuplicateFolderUT.wxs" should exist
		SET /A MY_ERR=1
	)
EXIT /B %MY_ERR%

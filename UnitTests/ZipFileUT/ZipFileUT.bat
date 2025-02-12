ECHO OFF
SET /A MY_ERR=0

:: Install
CALL :prepareFolders
msiexec /i ZipFileUT.msi /l*v ZipFileUT.msi.log MY_DIR="%CD%\\"
CALL :testInstall
PAUSE

:: Uninstall
msiexec /xZipFileUT.msi /l*v ZipFileUT.msix.log

:: Clean and exit
CALL :cleanFolders
ECHO Overall error code %MY_ERR%
EXIT /B %MY_ERR%

:prepareFolders
	CALL :cleanFolders

	MKDIR "%CD%\src\sub"
	ECHO test > "%CD%\src\f.txt"
	ECHO test > "%CD%\src\f.log"
	ECHO test > "%CD%\src\sub\f.txt"
	ECHO test > "%CD%\src\sub\f.log"
EXIT /B %MY_ERR%

:cleanFolders
	DEL *.zip
    RMDIR /s /q "%CD%\src"
    RMDIR /s /q "%CD%\root-logs"
    RMDIR /s /q "%CD%\no-root-logs"
    RMDIR /s /q "%CD%\txt"
    RMDIR /s /q "%CD%\txt2"
    RMDIR /s /q "%CD%\all"
    RMDIR /s /q "%CD%\all2"
    RMDIR /s /q "%CD%\none"
    RMDIR /s /q "%CD%\none2"
    RMDIR /s /q "%CD%\group-all"
    RMDIR /s /q "%CD%\group-none"
    RMDIR /s /q "%CD%\ungroup-none"
    RMDIR /s /q "%CD%\group-logs"
    RMDIR /s /q "%CD%\range-logs"
    RMDIR /s /q "%CD%\range-none"
EXIT /B %MY_ERR%

:testInstall
	:: root-logs
	IF EXIST "%CD%\root-logs\*.txt" (
		ECHO Folder "%CD%\root-logs\" should not contain .txt files
		SET /A MY_ERR=1
	)
	IF EXIST "%CD%\root-logs\sub\*.*" (
		ECHO Folder "%CD%\root-logs\sub" should not contain files
		SET /A MY_ERR=1
	)
	IF NOT EXIST "%CD%\root-logs\*.log" (
		ECHO Folder "%CD%\root-logs\" should contain .log files
		SET /A MY_ERR=1
	)

	:: no-root-logs
	IF NOT EXIST "%CD%\no-root-logs\*.txt" (
		ECHO Folder "%CD%\no-root-logs\" should contain .txt files
		SET /A MY_ERR=1
	)
	IF EXIST "%CD%\no-root-logs\*.log" (
		ECHO Folder "%CD%\no-root-logs\" should not contain .log files
		SET /A MY_ERR=1
	)
	IF NOT EXIST "%CD%\no-root-logs\sub\*.log" (
		ECHO Folder "%CD%\no-root-logs\sub" should contain log files
		SET /A MY_ERR=1
	)
	IF NOT EXIST "%CD%\no-root-logs\sub\*.txt" (
		ECHO Folder "%CD%\no-root-logs\sub" should contain txt files
		SET /A MY_ERR=1
	)

	:: txt
	IF NOT EXIST "%CD%\txt\*.txt" (
		ECHO Folder "%CD%\txt\" should contain .txt files
		SET /A MY_ERR=1
	)
	IF NOT EXIST "%CD%\txt\sub\*.txt" (
		ECHO Folder "%CD%\txt\sub\" should contain .txt files
		SET /A MY_ERR=1
	)
	IF EXIST "%CD%\txt\*.log" (
		ECHO Folder "%CD%\txt\" should not contain .log files
		SET /A MY_ERR=1
	)
	IF EXIST "%CD%\txt\sub\*.log" (
		ECHO Folder "%CD%\txt\sub\" should not contain .log files
		SET /A MY_ERR=1
	)

	:: txt2
	IF NOT EXIST "%CD%\txt2\*.txt" (
		ECHO Folder "%CD%\txt2\" should contain .txt files
		SET /A MY_ERR=1
	)
	IF NOT EXIST "%CD%\txt2\sub\*.txt" (
		ECHO Folder "%CD%\txt2\sub\" should contain .txt files
		SET /A MY_ERR=1
	)
	IF EXIST "%CD%\txt2\*.log" (
		ECHO Folder "%CD%\txt2\" should not contain .log files
		SET /A MY_ERR=1
	)
	IF EXIST "%CD%\txt2\sub\*.log" (
		ECHO Folder "%CD%\txt2\sub\" should not contain .log files
		SET /A MY_ERR=1
	)

	:: all
	IF NOT EXIST "%CD%\all\*.txt" (
		ECHO Folder "%CD%\all\" should contain .txt files
		SET /A MY_ERR=1
	)
	IF NOT EXIST "%CD%\all\sub\*.txt" (
		ECHO Folder "%CD%\all\sub\" should contain .txt files
		SET /A MY_ERR=1
	)
	IF NOT EXIST "%CD%\all\*.log" (
		ECHO Folder "%CD%\all\" should contain .log files
		SET /A MY_ERR=1
	)
	IF NOT EXIST "%CD%\all\sub\*.log" (
		ECHO Folder "%CD%\all\sub\" should contain .log files
		SET /A MY_ERR=1
	)

	:: all2
	IF NOT EXIST "%CD%\all2\*.txt" (
		ECHO Folder "%CD%\all2\" should contain .txt files
		SET /A MY_ERR=1
	)
	IF NOT EXIST "%CD%\all2\sub\*.txt" (
		ECHO Folder "%CD%\all2\sub\" should contain .txt files
		SET /A MY_ERR=1
	)
	IF NOT EXIST "%CD%\all2\*.log" (
		ECHO Folder "%CD%\all2\" should contain .log files
		SET /A MY_ERR=1
	)
	IF NOT EXIST "%CD%\all2\sub\*.log" (
		ECHO Folder "%CD%\all2\sub\" should contain .log files
		SET /A MY_ERR=1
	)

	:: none
	IF EXIST "%CD%\none\*.*" (
		ECHO Folder "%CD%\none\" should not contain files
		SET /A MY_ERR=1
	)
	IF EXIST "%CD%\none\sub\*.*" (
		ECHO Folder "%CD%\none\sub\" should not contain files
		SET /A MY_ERR=1
	)

	:: none2
	IF EXIST "%CD%\none2\*.*" (
		ECHO Folder "%CD%\none2\" should not contain files
		SET /A MY_ERR=1
	)
	IF EXIST "%CD%\none2\sub\*.*" (
		ECHO Folder "%CD%\none2\sub\" should not contain files
		SET /A MY_ERR=1
	)

	:: group-all
	IF NOT EXIST "%CD%\group-all\*.*" (
		ECHO Folder "%CD%\group-all\" should contain files
		SET /A MY_ERR=1
	)
	IF NOT EXIST "%CD%\group-all\sub\*.*" (
		ECHO Folder "%CD%\group-all\sub" should contain files
		SET /A MY_ERR=1
	)

	:: group-none
	IF EXIST "%CD%\group-none\*.*" (
		ECHO Folder "%CD%\group-none\" should not contain files
		SET /A MY_ERR=1
	)
	IF EXIST "%CD%\group-none\sub\*.*" (
		ECHO Folder "%CD%\group-none\sub\" should not contain files
		SET /A MY_ERR=1
	)

	:: ungroup-none
	IF EXIST "%CD%\ungroup-none\*.*" (
		ECHO Folder "%CD%\ungroup-none\" should not contain files
		SET /A MY_ERR=1
	)
	IF EXIST "%CD%\ungroup-none\sub\*.*" (
		ECHO Folder "%CD%\ungroup-none\sub\" should not contain files
		SET /A MY_ERR=1
	)

	:: group-logs
	IF EXIST "%CD%\group-logs\*.txt" (
		ECHO Folder "%CD%\group-logs\" should not contain .txt files
		SET /A MY_ERR=1
	)
	IF EXIST "%CD%\group-logs\sub\*.txt" (
		ECHO Folder "%CD%\group-logs\sub\" should not contain .txt files
		SET /A MY_ERR=1
	)
	IF NOT EXIST "%CD%\group-logs\*.log" (
		ECHO Folder "%CD%\group-logs\" should contain .log files
		SET /A MY_ERR=1
	)
	IF NOT EXIST "%CD%\group-logs\sub\*.log" (
		ECHO Folder "%CD%\group-logs\sub\" should contain .log files
		SET /A MY_ERR=1
	)

	::range-logs
	IF EXIST "%CD%\range-logs\*.txt" (
		ECHO Folder "%CD%\range-logs\" should not contain .txt files
		SET /A MY_ERR=1
	)
	IF EXIST "%CD%\range-logs\sub\*.txt" (
		ECHO Folder "%CD%\range-logs\sub\" should not contain .txt files
		SET /A MY_ERR=1
	)
	IF NOT EXIST "%CD%\range-logs\*.log" (
		ECHO Folder "%CD%\range-logs\" should contain .log files
		SET /A MY_ERR=1
	)
	IF NOT EXIST "%CD%\range-logs\sub\*.log" (
		ECHO Folder "%CD%\range-logs\sub" should contain .log files
		SET /A MY_ERR=1
	)

	:: range-none
	IF EXIST "%CD%\range-none\*.*" (
		ECHO Folder "%CD%\range-none\" should not contain files
		SET /A MY_ERR=1
	)
	IF EXIST "%CD%\range-none\sub\*.*" (
		ECHO Folder "%CD%\range-none\sub\" should not contain files
		SET /A MY_ERR=1
	)
EXIT /B %MY_ERR%

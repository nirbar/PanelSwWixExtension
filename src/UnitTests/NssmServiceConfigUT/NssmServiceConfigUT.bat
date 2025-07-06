ECHO OFF
SET /A MY_ERR=0
CD "%~dp0"

:: Install + rollback
CALL :prepare
ECHO === Testing install and rollback ===
msiexec /i NssmServiceConfigUT.msi /qn /l*v NssmServiceConfigUT.msir.log DO_ROLLBACK=1 INSTALLFOLDER="%CD%\install-dir"
CALL :testRollback

:: Install
ECHO === Testing install ===
msiexec /i NssmServiceConfigUT.msi /qn /l*v NssmServiceConfigUT.msi.log INSTALLFOLDER="%CD%\install-dir"
CALL :testInstall

:: Uninstall + rollback
ECHO === Testing uninstall and rollback ===
DEL "%CD%\install-dir\NssmServiceConfigUT?.txt"
msiexec /xNssmServiceConfigUT.msi /qn /l*v NssmServiceConfigUT.msixr.log DO_ROLLBACK=1
CALL :testInstall

:: Uninstall
ECHO === Testing uninstall ===
msiexec /xNssmServiceConfigUT.msi /qn /l*v NssmServiceConfigUT.msix.log

:: Clean and exit
CALL :clean
ECHO Overall error code %MY_ERR%
EXIT /B %MY_ERR%

:prepare
	CALL :clean

	REG add "HKLM\System\CurrentControlSet\Services\NssmServiceConfigUT2\Parameters" /v "Application" /t REG_EXPAND_SZ /d "%ProgramFiles%\test.exe" /reg:64 /f
	REG add "HKLM\System\CurrentControlSet\Services\NssmServiceConfigUT2\Parameters" /v "Useless" /t REG_EXPAND_SZ /d "%ProgramFiles%\Useless.exe" /reg:64 /f
	REG add "HKLM\System\CurrentControlSet\Services\NssmServiceConfigUT2\Parameters\AppExit" /v "" /t REG_SZ /d "Invalid" /reg:64 /f
EXIT /B %MY_ERR%

:clean
    RMDIR /s /q "%CD%\install-dir"
	REG delete "HKLM\System\CurrentControlSet\Services\NssmServiceConfigUT1" /reg:64 /f
	REG delete "HKLM\System\CurrentControlSet\Services\NssmServiceConfigUT2" /reg:64 /f
EXIT /B %MY_ERR%

:testRollback
	REG query "HKLM\System\CurrentControlSet\Services\NssmServiceConfigUT1" /reg:64
	IF %ERRORLEVEL% EQU 0 (
		ECHO Registry "HKLM\System\CurrentControlSet\Services\NssmServiceConfigUT1" should not exist
		SET /A MY_ERR=1
	)
	REG query "HKLM\System\CurrentControlSet\Services\NssmServiceConfigUT2\Parameters\AppExit" /reg:64
	IF %ERRORLEVEL% NEQ 0 (
		ECHO Registry "HKLM\System\CurrentControlSet\Services\NssmServiceConfigUT2\Parameters\AppExit" should exist
		SET /A MY_ERR=1
	)
	REG query "HKLM\System\CurrentControlSet\Services\NssmServiceConfigUT2\Parameters" /v "Application" /reg:64
	IF %ERRORLEVEL% NEQ 0 (
		ECHO Registry "HKLM\System\CurrentControlSet\Services\NssmServiceConfigUT2\Parameters\@Application" should exist
		SET /A MY_ERR=1
	)
	REG query "HKLM\System\CurrentControlSet\Services\NssmServiceConfigUT2\Parameters" /v "AppParameters" /reg:64
	IF %ERRORLEVEL% EQU 0 (
		ECHO Registry "HKLM\System\CurrentControlSet\Services\NssmServiceConfigUT2\Parameters\@AppParameters" should not exist
		SET /A MY_ERR=1
	)
	REG query "HKLM\System\CurrentControlSet\Services\NssmServiceConfigUT2\Parameters" /v "Useless" /reg:64
	IF %ERRORLEVEL% NEQ 0 (
		ECHO Registry "HKLM\System\CurrentControlSet\Services\NssmServiceConfigUT2\Parameters\@Useless" should exist
		SET /A MY_ERR=1
	)
EXIT /B %MY_ERR%

:testInstall
	IF NOT EXIST "%CD%\install-dir\NssmServiceConfigUT1.txt" (
		ECHO File "%CD%\install-dir\NssmServiceConfigUT1.txt" should exist
		SET /A MY_ERR=1
	)
	IF NOT EXIST "%CD%\install-dir\NssmServiceConfigUT2.txt" (
		ECHO File "%CD%\install-dir\NssmServiceConfigUT2.txt" should exist
		SET /A MY_ERR=1
	)
	REG query "HKLM\System\CurrentControlSet\Services\NssmServiceConfigUT2\Parameters" /v "Useless" /reg:64
	IF %ERRORLEVEL% NEQ 0 (
		ECHO Registry "HKLM\System\CurrentControlSet\Services\NssmServiceConfigUT2\Parameters\@Useless" should exist
		SET /A MY_ERR=1
	)
	REG query "HKLM\System\CurrentControlSet\Services\NssmServiceConfigUT2\Parameters" /v "AppNoConsole" /reg:64
	IF %ERRORLEVEL% NEQ 0 (
		ECHO Registry "HKLM\System\CurrentControlSet\Services\NssmServiceConfigUT2\Parameters\@AppNoConsole" should exist
		SET /A MY_ERR=1
	)
	REG query "HKLM\System\CurrentControlSet\Services\NssmServiceConfigUT2\Parameters" /v "AppAffinity" /reg:64
	IF %ERRORLEVEL% NEQ 0 (
		ECHO Registry "HKLM\System\CurrentControlSet\Services\NssmServiceConfigUT2\Parameters\@AppAffinity" should exist
		SET /A MY_ERR=1
	)
	REG query "HKLM\System\CurrentControlSet\Services\NssmServiceConfigUT2\Parameters" /v "Test" /reg:64
	IF %ERRORLEVEL% NEQ 0 (
		ECHO Registry "HKLM\System\CurrentControlSet\Services\NssmServiceConfigUT2\Parameters\@Test" should exist
		SET /A MY_ERR=1
	)
EXIT /B %MY_ERR%

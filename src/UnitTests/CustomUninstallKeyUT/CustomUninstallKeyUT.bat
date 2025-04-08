:: Test designed for x64 package!

ECHO OFF
SET /A MY_ERR=0
CD "%~dp0"

:: Install + rollback
CALL :prepare
ECHO === Testing install and rollback ===
msiexec /i CustomUninstallKeyUT.msi /qn /l*v CustomUninstallKeyUT.msir.log DO_ROLLBACK=1
CALL :testRollback

:: Install
CALL :prepare
ECHO === Testing install ===
msiexec /i CustomUninstallKeyUT.msi /qn /l*v CustomUninstallKeyUT.msi.log
CALL :testInstall

:: Uninstall
ECHO === Testing uninstall ===
msiexec /xCustomUninstallKeyUT.msi /qn /l*v CustomUninstallKeyUT.msix.log
CALL :testUninstall

:: Clean and exit
CALL :clean
ECHO Overall error code %MY_ERR%
EXIT /B %MY_ERR%

:prepare
	CALL :clean

	REG add "HKLM\Software\Microsoft\Windows\CurrentVersion\Uninstall\CustomUninstallKeyUT" /v "existingVal" /t REG_SZ /d "existing" /reg:64 /f
EXIT /B %MY_ERR%

:clean
	REG delete "HKLM\Software\Microsoft\Windows\CurrentVersion\Uninstall\CustomUninstallKeyUT" /reg:64 /f
EXIT /B %MY_ERR%

:testRollback
	REG query "HKLM\Software\Microsoft\Windows\CurrentVersion\Uninstall\CustomUninstallKeyUT" /v "existingVal" /reg:64
	IF %ERRORLEVEL% NEQ 0 (
		ECHO Registry "HKLM\Software\Microsoft\Windows\CurrentVersion\Uninstall\CustomUninstallKeyUT\@existingVal" should exist
		SET /A MY_ERR=1
	)
	REG query "HKLM\Software\Microsoft\Windows\CurrentVersion\Uninstall\CustomUninstallKeyUT" /v "newVal" /reg:64
	IF %ERRORLEVEL% EQU 0 (
		ECHO Registry "HKLM\Software\Microsoft\Windows\CurrentVersion\Uninstall\CustomUninstallKeyUT\@newVal" should not exist
		SET /A MY_ERR=1
	)
	REG query "HKLM\Software\Microsoft\Windows\CurrentVersion\Uninstall\{9972FF2F-0828-4B45-81A0-9E71B07DFC83}" /v "newVal" /reg:64
	IF %ERRORLEVEL% EQU 0 (
		ECHO Registry "HKLM\Software\Microsoft\Windows\CurrentVersion\Uninstall\CustomUninstallKeyUT\@newVal" should not exist
		SET /A MY_ERR=1
	)
EXIT /B %MY_ERR%

:testInstall
	REG query "HKLM\Software\Microsoft\Windows\CurrentVersion\Uninstall\CustomUninstallKeyUT" /v "existingVal" /reg:64
	IF %ERRORLEVEL% NEQ 0 (
		ECHO Registry "HKLM\Software\Microsoft\Windows\CurrentVersion\Uninstall\CustomUninstallKeyUT\@existingVal" should exist
		SET /A MY_ERR=1
	)
	REG query "HKLM\Software\Microsoft\Windows\CurrentVersion\Uninstall\CustomUninstallKeyUT" /v "newVal" /reg:64
	IF %ERRORLEVEL% NEQ 0 (
		ECHO Registry "HKLM\Software\Microsoft\Windows\CurrentVersion\Uninstall\CustomUninstallKeyUT\@newVal" should exist
		SET /A MY_ERR=1
	)
	REG query "HKLM\Software\Microsoft\Windows\CurrentVersion\Uninstall\{9972FF2F-0828-4B45-81A0-9E71B07DFC83}" /v "newVal" /reg:64
	IF %ERRORLEVEL% NEQ 0 (
		ECHO Registry "HKLM\Software\Microsoft\Windows\CurrentVersion\Uninstall\CustomUninstallKeyUT\@newVal" should exist
		SET /A MY_ERR=1
	)
EXIT /B %MY_ERR%

:testUninstall
	REG query "HKLM\Software\Microsoft\Windows\CurrentVersion\Uninstall\CustomUninstallKeyUT" /v "existingVal" /reg:64
	IF %ERRORLEVEL% NEQ 0 (
		ECHO Registry "HKLM\Software\Microsoft\Windows\CurrentVersion\Uninstall\CustomUninstallKeyUT\@existingVal" should exist
		SET /A MY_ERR=1
	)
	REG query "HKLM\Software\Microsoft\Windows\CurrentVersion\Uninstall\CustomUninstallKeyUT" /v "newVal" /reg:64
	IF %ERRORLEVEL% NEQ 0 (
		ECHO Registry "HKLM\Software\Microsoft\Windows\CurrentVersion\Uninstall\CustomUninstallKeyUT\@newVal" should exist
		SET /A MY_ERR=1
	)
	REG query "HKLM\Software\Microsoft\Windows\CurrentVersion\Uninstall\{9972FF2F-0828-4B45-81A0-9E71B07DFC83}" /v "newVal" /reg:64
	IF %ERRORLEVEL% EQU 0 (
		ECHO Registry "HKLM\Software\Microsoft\Windows\CurrentVersion\Uninstall\CustomUninstallKeyUT\@newVal" should not exist
		SET /A MY_ERR=1
	)
EXIT /B %MY_ERR%

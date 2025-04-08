:: Test designed for x64 package!

ECHO OFF
SET /A MY_ERR=0
CD "%~dp0"

:: Install
CALL :prepare
ECHO === Testing install ===
msiexec /i RemoveRegistryValueUT.msi /qn /l*v RemoveRegistryValueUT.msi.log
CALL :testInstall

:: Uninstall + rollback
CALL :prepare
ECHO === Testing uninstall and rollback ===
msiexec /xRemoveRegistryValueUT.msi /qn /l*v RemoveRegistryValueUT.msir.log DO_ROLLBACK=1
CALL :testRollback

:: Uninstall
CALL :prepare
ECHO === Testing uninstall ===
msiexec /xRemoveRegistryValueUT.msi /qn /l*v RemoveRegistryValueUT.msix.log
CALL :testUninstall

:: Clean and exit
CALL :clean
ECHO Overall error code %MY_ERR%
EXIT /B %MY_ERR%

:prepare
	CALL :clean

	REG add "HKLM\SOFTWARE\RemoveRegistryValueUT" /v "hklm-32" /t REG_SZ /d "1" /reg:32 /f
	REG add "HKLM\SOFTWARE\RemoveRegistryValueUT" /v "hklm-64" /t REG_DWORD /d "64" /reg:64 /f
	REG add "HKLM\SOFTWARE\RemoveRegistryValueUT" /v "hklm-default" /t REG_EXPAND_SZ /d "%ProgramFiles%" /reg:64 /f
	REG add "HKLM\SOFTWARE\RemoveRegistryValueUT" /v "comp-hklm-permanent" /t REG_SZ /d "1" /reg:64 /f
	REG add "HKLM\SOFTWARE\RemoveRegistryValueUT" /v "comp32-hkmu-default" /t REG_SZ /d "1" /reg:32 /f
	REG add "HKLM\SOFTWARE\RemoveRegistryValueUT" /v "comp32-hkmu-64" /t REG_SZ /d "1" /reg:64 /f
	REG add "HKLM\SOFTWARE\RemoveRegistryValueUT" /v "comp64-hklm-default" /t REG_SZ /d "1" /reg:64 /f
	REG add "HKLM\SOFTWARE\RemoveRegistryValueUT" /v "comp64-hklm-32" /t REG_SZ /d "1" /reg:32 /f
	REG add "HKCR\SOFTWARE\RemoveRegistryValueUT" /v "comp64-hkcr" /t REG_SZ /d "1" /reg:64 /f
	REG add "HKCU\SOFTWARE\RemoveRegistryValueUT" /v "comp64-hkcu" /t REG_SZ /d "1" /reg:64 /f
	REG add "HKU\.DEFAULT\SOFTWARE\RemoveRegistryValueUT" /v "comp64-hku" /t REG_SZ /d "1" /reg:64 /f
EXIT /B %MY_ERR%

:clean
	REG delete "HKLM\SOFTWARE\RemoveRegistryValueUT" /reg:64 /f
	REG delete "HKCU\SOFTWARE\RemoveRegistryValueUT" /reg:64 /f
	REG delete "HKCR\SOFTWARE\RemoveRegistryValueUT" /reg:64 /f
	REG delete "HKU\.DEFAULT\SOFTWARE\RemoveRegistryValueUT" /reg:64 /f
	REG delete "HKLM\SOFTWARE\RemoveRegistryValueUT" /reg:32 /f
	REG delete "HKCU\SOFTWARE\RemoveRegistryValueUT" /reg:32 /f
	REG delete "HKCR\SOFTWARE\RemoveRegistryValueUT" /reg:32 /f
	REG delete "HKU\.DEFAULT\SOFTWARE\RemoveRegistryValueUT" /reg:32 /f
EXIT /B %MY_ERR%

:testInstall
	REG query "HKLM\SOFTWARE\RemoveRegistryValueUT" /v "hklm-32" /reg:32
	IF %ERRORLEVEL% EQU 0 (
		ECHO Registry "HKLM\SOFTWARE\RemoveRegistryValueUT\@hklm-32" should not exist
		SET /A MY_ERR=1
	)
	REG query "HKLM\SOFTWARE\RemoveRegistryValueUT" /v "hklm-64" /reg:64
	IF %ERRORLEVEL% EQU 0 (
		ECHO Registry "HKLM\SOFTWARE\RemoveRegistryValueUT\@hklm-64" should not exist
		SET /A MY_ERR=1
	)
	REG query "HKLM\SOFTWARE\RemoveRegistryValueUT" /v "hklm-default" /reg:64
	IF %ERRORLEVEL% EQU 0 (
		ECHO Registry "HKLM\SOFTWARE\RemoveRegistryValueUT\@hklm-default" should not exist
		SET /A MY_ERR=1
	)
	REG query "HKLM\SOFTWARE\RemoveRegistryValueUT" /v "comp-hklm-permanent" /reg:64
	IF %ERRORLEVEL% NEQ 0 (
		ECHO Registry "HKLM\SOFTWARE\RemoveRegistryValueUT\@comp-hklm-permanent" should exist
		SET /A MY_ERR=1
	)
	REG query "HKLM\SOFTWARE\RemoveRegistryValueUT" /v "comp32-hkmu-default" /reg:32
	IF %ERRORLEVEL% NEQ 0 (
		ECHO Registry "HKLM\SOFTWARE\RemoveRegistryValueUT\@comp32-hkmu-default" should exist
		SET /A MY_ERR=1
	)
	REG query "HKLM\SOFTWARE\RemoveRegistryValueUT" /v "comp32-hkmu-64" /reg:64
	IF %ERRORLEVEL% NEQ 0 (
		ECHO Registry "HKLM\SOFTWARE\RemoveRegistryValueUT\@comp32-hkmu-64" should exist
		SET /A MY_ERR=1
	)
	REG query "HKLM\SOFTWARE\RemoveRegistryValueUT" /v "comp64-hklm-default" /reg:64
	IF %ERRORLEVEL% NEQ 0 (
		ECHO Registry "HKLM\SOFTWARE\RemoveRegistryValueUT\@comp64-hklm-default" should exist
		SET /A MY_ERR=1
	)
	REG query "HKLM\SOFTWARE\RemoveRegistryValueUT" /v "comp64-hklm-32" /reg:32
	IF %ERRORLEVEL% NEQ 0 (
		ECHO Registry "HKLM\SOFTWARE\RemoveRegistryValueUT\@comp64-hklm-32" should exist
		SET /A MY_ERR=1
	)
	REG query "HKCR\SOFTWARE\RemoveRegistryValueUT" /v "comp64-hkcr" /reg:64
	IF %ERRORLEVEL% NEQ 0 (
		ECHO Registry "HKCR\SOFTWARE\RemoveRegistryValueUT\@comp64-hkcr" should exist
		SET /A MY_ERR=1
	)
	REG query "HKCU\SOFTWARE\RemoveRegistryValueUT" /v "comp64-hkcu" /reg:64
	IF %ERRORLEVEL% NEQ 0 (
		ECHO Registry "HKCU\SOFTWARE\RemoveRegistryValueUT\@comp64-hkcu" should exist
		SET /A MY_ERR=1
	)
	REG query "HKU\.DEFAULT\SOFTWARE\RemoveRegistryValueUT" /v "comp64-hku" /reg:64
	IF %ERRORLEVEL% NEQ 0 (
		ECHO Registry "HKU\.DEFAULT\SOFTWARE\RemoveRegistryValueUT\@comp64-hku" should exist
		SET /A MY_ERR=1
	)
EXIT /B %MY_ERR%

:testRollback
	REG query "HKLM\SOFTWARE\RemoveRegistryValueUT" /v "hklm-32" /reg:32
	IF %ERRORLEVEL% NEQ 0 (
		ECHO Registry "HKLM\SOFTWARE\RemoveRegistryValueUT\@hklm-32" should exist
		SET /A MY_ERR=1
	)
	REG query "HKLM\SOFTWARE\RemoveRegistryValueUT" /v "hklm-64" /reg:64
	IF %ERRORLEVEL% NEQ 0 (
		ECHO Registry "HKLM\SOFTWARE\RemoveRegistryValueUT\@hklm-64" should exist
		SET /A MY_ERR=1
	)
	REG query "HKLM\SOFTWARE\RemoveRegistryValueUT" /v "hklm-default" /reg:64
	IF %ERRORLEVEL% NEQ 0 (
		ECHO Registry "HKLM\SOFTWARE\RemoveRegistryValueUT\@hklm-default" should exist
		SET /A MY_ERR=1
	)
	REG query "HKLM\SOFTWARE\RemoveRegistryValueUT" /v "comp-hklm-permanent" /reg:64
	IF %ERRORLEVEL% NEQ 0 (
		ECHO Registry "HKLM\SOFTWARE\RemoveRegistryValueUT\@comp-hklm-permanent" should exist
		SET /A MY_ERR=1
	)
	REG query "HKLM\SOFTWARE\RemoveRegistryValueUT" /v "comp32-hkmu-default" /reg:32
	IF %ERRORLEVEL% NEQ 0 (
		ECHO Registry "HKLM\SOFTWARE\RemoveRegistryValueUT\@comp32-hkmu-default" should exist
		SET /A MY_ERR=1
	)
	REG query "HKLM\SOFTWARE\RemoveRegistryValueUT" /v "comp32-hkmu-64" /reg:64
	IF %ERRORLEVEL% NEQ 0 (
		ECHO Registry "HKLM\SOFTWARE\RemoveRegistryValueUT\@comp32-hkmu-64" should exist
		SET /A MY_ERR=1
	)
	REG query "HKLM\SOFTWARE\RemoveRegistryValueUT" /v "comp64-hklm-default" /reg:64
	IF %ERRORLEVEL% NEQ 0 (
		ECHO Registry "HKLM\SOFTWARE\RemoveRegistryValueUT\@comp64-hklm-default" should exist
		SET /A MY_ERR=1
	)
	REG query "HKLM\SOFTWARE\RemoveRegistryValueUT" /v "comp64-hklm-32" /reg:32
	IF %ERRORLEVEL% NEQ 0 (
		ECHO Registry "HKLM\SOFTWARE\RemoveRegistryValueUT\@comp64-hklm-32" should exist
		SET /A MY_ERR=1
	)
	REG query "HKCR\SOFTWARE\RemoveRegistryValueUT" /v "comp64-hkcr" /reg:64
	IF %ERRORLEVEL% NEQ 0 (
		ECHO Registry "HKCR\SOFTWARE\RemoveRegistryValueUT\@comp64-hkcr" should exist
		SET /A MY_ERR=1
	)
	REG query "HKCU\SOFTWARE\RemoveRegistryValueUT" /v "comp64-hkcu" /reg:64
	IF %ERRORLEVEL% NEQ 0 (
		ECHO Registry "HKCU\SOFTWARE\RemoveRegistryValueUT\@comp64-hkcu" should exist
		SET /A MY_ERR=1
	)
	REG query "HKU\.DEFAULT\SOFTWARE\RemoveRegistryValueUT" /v "comp64-hku" /reg:64
	IF %ERRORLEVEL% NEQ 0 (
		ECHO Registry "HKU\.DEFAULT\SOFTWARE\RemoveRegistryValueUT\@comp64-hku" should exist
		SET /A MY_ERR=1
	)
EXIT /B %MY_ERR%

:testUninstall
	REG query "HKLM\SOFTWARE\RemoveRegistryValueUT" /v "hklm-32" /reg:32
	IF %ERRORLEVEL% EQU 0 (
		ECHO Registry "HKLM\SOFTWARE\RemoveRegistryValueUT\@hklm-32" should not exist
		SET /A MY_ERR=1
	)
	REG query "HKLM\SOFTWARE\RemoveRegistryValueUT" /v "hklm-64" /reg:64
	IF %ERRORLEVEL% EQU 0 (
		ECHO Registry "HKLM\SOFTWARE\RemoveRegistryValueUT\@hklm-64" should not exist
		SET /A MY_ERR=1
	)
	REG query "HKLM\SOFTWARE\RemoveRegistryValueUT" /v "hklm-default" /reg:64
	IF %ERRORLEVEL% EQU 0 (
		ECHO Registry "HKLM\SOFTWARE\RemoveRegistryValueUT\@hklm-default" should not exist
		SET /A MY_ERR=1
	)
	REG query "HKLM\SOFTWARE\RemoveRegistryValueUT" /v "comp-hklm-permanent" /reg:64
	IF %ERRORLEVEL% NEQ 0 (
		ECHO Registry "HKLM\SOFTWARE\RemoveRegistryValueUT\@comp-hklm-permanent" should exist
		SET /A MY_ERR=1
	)
	REG query "HKLM\SOFTWARE\RemoveRegistryValueUT" /v "comp32-hkmu-default" /reg:32
	IF %ERRORLEVEL% EQU 0 (
		ECHO Registry "HKLM\SOFTWARE\RemoveRegistryValueUT\@comp32-hkmu-default" should not exist
		SET /A MY_ERR=1
	)
	REG query "HKLM\SOFTWARE\RemoveRegistryValueUT" /v "comp32-hkmu-64" /reg:64
	IF %ERRORLEVEL% EQU 0 (
		ECHO Registry "HKLM\SOFTWARE\RemoveRegistryValueUT\@comp32-hkmu-64" should not exist
		SET /A MY_ERR=1
	)
	REG query "HKLM\SOFTWARE\RemoveRegistryValueUT" /v "comp64-hklm-default" /reg:64
	IF %ERRORLEVEL% EQU 0 (
		ECHO Registry "HKLM\SOFTWARE\RemoveRegistryValueUT\@comp64-hklm-default" should not exist
		SET /A MY_ERR=1
	)
	REG query "HKLM\SOFTWARE\RemoveRegistryValueUT" /v "comp64-hklm-32" /reg:32
	IF %ERRORLEVEL% EQU 0 (
		ECHO Registry "HKLM\SOFTWARE\RemoveRegistryValueUT\@comp64-hklm-32" should not exist
		SET /A MY_ERR=1
	)
	REG query "HKCR\SOFTWARE\RemoveRegistryValueUT" /v "comp64-hkcr" /reg:64
	IF %ERRORLEVEL% EQU 0 (
		ECHO Registry "HKCR\SOFTWARE\RemoveRegistryValueUT\@comp64-hkcr" should not exist
		SET /A MY_ERR=1
	)
	REG query "HKCU\SOFTWARE\RemoveRegistryValueUT" /v "comp64-hkcu" /reg:64
	IF %ERRORLEVEL% EQU 0 (
		ECHO Registry "HKCU\SOFTWARE\RemoveRegistryValueUT\@comp64-hkcu" should not exist
		SET /A MY_ERR=1
	)
	REG query "HKU\.DEFAULT\SOFTWARE\RemoveRegistryValueUT" /v "comp64-hku" /reg:64
	IF %ERRORLEVEL% EQU 0 (
		ECHO Registry "HKU\.DEFAULT\SOFTWARE\RemoveRegistryValueUT\@comp64-hku" should not exist
		SET /A MY_ERR=1
	)
EXIT /B %MY_ERR%

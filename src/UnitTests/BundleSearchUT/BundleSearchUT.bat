ECHO OFF

:: Install
CALL :prepare
BundleSearchUT.exe /install CANT_OVRD_ON_COMMAND_LINE=2
PAUSE

:: Uninstall
BundleSearchUT.exe /uninstall ovrd_on_command_line=1 CANT_OVRD_ON_COMMAND_LINE=2
PAUSE
CALL :clean

:prepare
	CALL :clean

	REG add "HKLM\SOFTWARE\Microsoft\Windows\CurrentVersion\Uninstall\BundleSearchUT-BadVersion" /v "DisplayVersion" /t REG_SZ /d "abcd" /reg:32 /f
	REG add "HKLM\SOFTWARE\Microsoft\Windows\CurrentVersion\Uninstall\NotBundleSearchUT-Version9999" /v "DisplayVersion" /t REG_SZ /d "9.9.9.9" /reg:32 /f
	REG add "HKLM\SOFTWARE\Microsoft\Windows\CurrentVersion\Uninstall\BundleSearchUT-Version1111" /v "DisplayVersion" /t REG_SZ /d "1.1.1.1" /reg:32 /f
	REG add "HKLM\SOFTWARE\Microsoft\Windows\CurrentVersion\Uninstall\BundleSearchUT-Version1234" /v "DisplayVersion" /t REG_SZ /d "1.2.3.4" /reg:32 /f
	REG add "HKLM\SOFTWARE\Microsoft\Windows\CurrentVersion\Uninstall\BundleSearchUT-Version2345" /v "DisplayVersion" /t REG_SZ /d "2.3.4.5" /reg:64 /f
	REG add "HKCU\SOFTWARE\Microsoft\Windows\CurrentVersion\Uninstall\BundleSearchUT-Version3456" /v "DisplayVersion" /t REG_SZ /d "3.4.5.6" /reg:64 /f
EXIT /B %MY_ERR%

:clean
	REG delete "HKLM\SOFTWARE\WOW6432Node\Microsoft\Windows\CurrentVersion\Uninstall\BundleSearchUT-BadVersion" /f
	REG delete "HKLM\SOFTWARE\WOW6432Node\Microsoft\Windows\CurrentVersion\Uninstall\NotBundleSearchUT-Version1111" /f
	REG delete "HKLM\SOFTWARE\WOW6432Node\Microsoft\Windows\CurrentVersion\Uninstall\BundleSearchUT-Version1111" /f
	REG delete "HKLM\SOFTWARE\WOW6432Node\Microsoft\Windows\CurrentVersion\Uninstall\BundleSearchUT-Version1234" /f
	REG delete "HKLM\SOFTWARE\Microsoft\Windows\CurrentVersion\Uninstall\BundleSearchUT-Version2345" /f
	REG delete "HKCU\SOFTWARE\Microsoft\Windows\CurrentVersion\Uninstall\BundleSearchUT-Version3456" /f
EXIT /B %MY_ERR%

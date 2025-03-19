ECHO OFF

:: Install
BundleVariableSearchUT.exe /install CANT_OVRD_ON_COMMAND_LINE=2
PAUSE

:: Uninstall
BundleVariableSearchUT.exe /uninstall ovrd_on_command_line=1 CANT_OVRD_ON_COMMAND_LINE=2
PAUSE

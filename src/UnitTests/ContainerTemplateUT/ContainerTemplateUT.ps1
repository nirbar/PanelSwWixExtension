$MY_ERR=0
$MY_DIR = (Split-Path $MyInvocation.MyCommand.Path)
Write-Host "Folder $MY_DIR"
CD $MY_DIR

# Install
Write-Host "=== Testing install ==="
$prc = Start-Process "ContainerTemplateUT.exe" -ArgumentList ("/install", "/silent", "/norestart", "/log", "ContainerTemplateUT.i.log") -Wait -PassThru
if ($prc.ExitCode -ne 0){
	Write-Host "install failed"
	$Script:MY_ERR=$prc.ExitCode
}

# Uninstall
Write-Host "=== Testing uninstall ==="
$prc = Start-Process "ContainerTemplateUT.exe" -ArgumentList ("/uninstall", "/silent", "/norestart", "/log", "ContainerTemplateUT.x.log") -Wait -PassThru
if ($prc.ExitCode -ne 0){
	Write-Host "uninstall failed"
	$Script:MY_ERR=$prc.ExitCode
}

# Clean and exit
Write-Host "Overall error code $MY_ERR"
exit $MY_ERR

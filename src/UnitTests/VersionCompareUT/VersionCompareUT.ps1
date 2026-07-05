$MY_ERR=0
$MY_DIR = (Split-Path $MyInvocation.MyCommand.Path)
Write-Host "Folder $MY_DIR"
CD $MY_DIR

# Install
Write-Host "=== Testing install ==="
$prc = Start-Process msiexec -ArgumentList ("/i", "VersionCompareUT.msi", "/qn", "/l*v", "VersionCompareUT.msi.log", "MY_DIR=""$MY_DIR""") -Wait -PassThru
if ($prc.ExitCode -ne 0){
	Write-Host "msiexec failed"
	$Script:MY_ERR=$prc.ExitCode
}

# Clean and exit
Write-Host "Overall error code $MY_ERR"
exit $MY_ERR

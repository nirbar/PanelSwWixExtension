$MY_ERR=0
$MY_DIR = (Split-Path $MyInvocation.MyCommand.Path)
CD $MY_DIR

function cleanFiles{
    Remove-Item -Path "$Script:MY_DIR\INSTALLFOLDER" -Force -ErrorAction Ignore
}

function testInstallFolder{
	if (Test-Path -Path "$Script:MY_DIR\INSTALLFOLDER") {
		Write-Host "$Script:MY_DIR\INSTALLFOLDER should not exist"
		$Script:MY_ERR=1
		return
	}
}

# Install + rollback
Write-Host "=== Testing rollback ==="
cleanFiles
$prc = Start-Process msiexec -ArgumentList ("/i", "RestartLocalResourcesUT.msi", "/qn", "/l*v", "RestartLocalResourcesUT.msi-r.log", "INSTALLFOLDER=""$MY_DIR\INSTALLFOLDER""", "DO_ROLLBACK=1") -Wait -PassThru
if ($prc.ExitCode -eq 0){
	Write-Host "msiexec was expected to fail"
	$Script:MY_ERR=1
}
testInstallFolder
cleanFiles

# Install
Write-Host "=== Testing install ==="
$prc = Start-Process msiexec -ArgumentList ("/i", "RestartLocalResourcesUT.msi", "/qn", "/l*v", "RestartLocalResourcesUT.msi-i.log", "INSTALLFOLDER=""$MY_DIR\INSTALLFOLDER""") -Wait -PassThru
if ($prc.ExitCode -ne 0){
	Write-Host "msiexec failed"
	$Script:MY_ERR=$prc.ExitCode
}

# Uninstall
Write-Host "=== Testing uninstall ==="
$prc = Start-Process msiexec -ArgumentList ("/xRestartLocalResourcesUT.msi", "/qn", "/l*v", "RestartLocalResourcesUT.msi-x.log") -Wait -PassThru
if ($prc.ExitCode -ne 0){
	Write-Host "msiexec failed"
	$Script:MY_ERR=$prc.ExitCode
}
testInstallFolder
cleanFiles

# Clean and exit
Write-Host "Overall error code $MY_ERR"
exit $MY_ERR

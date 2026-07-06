$MY_ERR=0
$MY_DIR = (Split-Path $MyInvocation.MyCommand.Path)
Write-Host "Folder $MY_DIR"
CD $MY_DIR

function cleanFiles{
    Remove-Item -Path "$Script:MY_DIR\install-test.txt" -Force -ErrorAction Ignore
    Remove-Item -Path "$Script:MY_DIR\commit-test.txt" -Force -ErrorAction Ignore
    Remove-Item -Path "$Script:MY_DIR\rollback-test.txt" -Force -ErrorAction Ignore
}

function testInstall{
	if (-NOT (Test-Path -Path "$Script:MY_DIR\install-test.txt" -PathType Leaf)) {
		Write-Host "$Script:MY_DIR\install-test.txt should exist"
		$Script:MY_ERR=1
		return
	}
	if (-NOT (Test-Path -Path "$Script:MY_DIR\commit-test.txt" -PathType Leaf)) {
		Write-Host "$Script:MY_DIR\commit-test.txt should exist"
		$Script:MY_ERR=1
		return
	}
	if (Test-Path -Path "$Script:MY_DIR\rollback-test.txt" -PathType Leaf) {
		Write-Host "$Script:MY_DIR\rollback-test.txt should not exist"
		$Script:MY_ERR=1
		return
	}
}

function testUninstall{
	if (-NOT (Test-Path -Path "$Script:MY_DIR\install-test.txt" -PathType Leaf)) {
		Write-Host "$Script:MY_DIR\install-test.txt should exist"
		$Script:MY_ERR=1
		return
	}
	if (-NOT (Test-Path -Path "$Script:MY_DIR\commit-test.txt" -PathType Leaf)) {
		Write-Host "$Script:MY_DIR\commit-test.txt should exist"
		$Script:MY_ERR=1
		return
	}
	if (Test-Path -Path "$Script:MY_DIR\rollback-test.txt" -PathType Leaf) {
		Write-Host "$Script:MY_DIR\rollback-test.txt should not exist"
		$Script:MY_ERR=1
		return
	}
}

function testInstallRollback{
	if (-NOT (Test-Path -Path "$Script:MY_DIR\install-test.txt" -PathType Leaf)) {
		Write-Host "$Script:MY_DIR\install-test.txt should exist"
		$Script:MY_ERR=1
		return
	}
	if (-NOT (Test-Path -Path "$Script:MY_DIR\rollback-test.txt" -PathType Leaf)) {
		Write-Host "$Script:MY_DIR\rollback-test.txt should exist"
		$Script:MY_ERR=1
		return
	}
	if (Test-Path -Path "$Script:MY_DIR\commit-test.txt" -PathType Leaf) {
		Write-Host "$Script:MY_DIR\commit-test.txt should not exist"
		$Script:MY_ERR=1
		return
	}
}

# Install + rollback
Write-Host "=== Testing rollback ==="
cleanFiles
$prc = Start-Process msiexec -ArgumentList ("/i", "ExecuteCommandUT.msi", "/qn", "/l*v", "ExecuteCommandUT.msi-r.log", "MY_DIR=""$MY_DIR""", "DO_ROLLBACK=1") -Wait -PassThru
if ($prc.ExitCode -eq 0){
	Write-Host "msiexec was expected to fail"
	$Script:MY_ERR=1
}
testInstallRollback
cleanFiles

# Install
Write-Host "=== Testing install ==="
$prc = Start-Process msiexec -ArgumentList ("/i", "ExecuteCommandUT.msi", "/qn", "/l*v", "ExecuteCommandUT.msi-i.log", "MY_DIR=""$MY_DIR""") -Wait -PassThru
if ($prc.ExitCode -ne 0){
	Write-Host "msiexec failed"
	$Script:MY_ERR=$prc.ExitCode
}
testInstall
cleanFiles

# Uninstall
Write-Host "=== Testing uninstall ==="
$prc = Start-Process msiexec -ArgumentList ("/xExecuteCommandUT.msi", "/qn", "/l*v", "ExecuteCommandUT.msi-x.log", "MY_DIR=""$MY_DIR""") -Wait -PassThru
if ($prc.ExitCode -ne 0){
	Write-Host "msiexec failed"
	$Script:MY_ERR=$prc.ExitCode
}
testUninstall
cleanFiles

# Clean and exit
Write-Host "Overall error code $MY_ERR"
exit $MY_ERR

$MY_ERR=0
$MY_DIR = (Split-Path $MyInvocation.MyCommand.Path)
Write-Host "Folder $MY_DIR"
CD $MY_DIR

function cleanFolders{
    Remove-Item -Path "$Script:MY_DIR\install-folder" -Recurse -Force -ErrorAction Ignore
}

function testInstall{
	if (-NOT (Test-Path -Path "$Script:MY_DIR\install-folder\Json-Object.json" -PathType Leaf)) {
		Write-Host "$Script:MY_DIR\install-folder\Json-Object.json should exist"
		$Script:MY_ERR=1
		return
	}

    $json = Get-Content -Path "$Script:MY_DIR\install-folder\Json-Object.json" -Raw | ConvertFrom-Json
	if ($($json.Manufacturers[1].Products[0].Price) -ne 99.95){
		Write-Host "Elbow Grease price should be 99.95"
		$Script:MY_ERR=1
	}
	if ($($json.Manufacturers[1].Products[1].Price) -ne 10){
		Write-Host "Headlight Fluid price should be 10"
		$Script:MY_ERR=1
	}
	if ($($json.TestAdd) -ne 10){
		Write-Host "TestAdd should be 10"
		$Script:MY_ERR=1
	}
	if ($json.PSObject.Properties.Name -contains 'TestDontAdd'){
		Write-Host "TestDontAdd should not exist"
		$Script:MY_ERR=1
	}
}

function testUninstall{
	if (Test-Path -Path "$Script:MY_DIR\install-folder\Json-Object.json" -PathType Leaf) {
		Write-Host "$Script:MY_DIR\install-folder\Json-Object.json should not exist"
		$Script:MY_ERR=1
	}
}

# Install
Write-Host "=== Testing install ==="
cleanFolders
Start-Process msiexec -ArgumentList ("/i", "JsonJPathUT.msi", "/qn", "/l*v", "JsonJPathUT.msi.log", "INSTALLFOLDER=""$MY_DIR\install-folder""") -Wait
testInstall

# Uninstall
Write-Host "=== Testing uninstall ==="
Start-Process msiexec -ArgumentList ("/xJsonJPathUT.msi", "/qn", "/l*v", "JsonJPathUT.msix.log", "INSTALLFOLDER=""$MY_DIR\install-folder""") -Wait
testUninstall

# Clean and exit
Write-Host "Overall error code $MY_ERR"
exit $MY_ERR

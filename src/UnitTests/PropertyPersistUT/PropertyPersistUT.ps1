$MY_ERR=0
$MY_DIR = (Split-Path $MyInvocation.MyCommand.Path)
Write-Host "Folder $MY_DIR"
CD $MY_DIR

$RegPath = "HKLM:\SOFTWARE\PanelSwWixExtension\{B997AF76-8F98-40D1-B123-552C82C40B37}\Properties"
function cleanFolders{
    Remove-Item -Path "$Script:MY_DIR\install-folder" -Recurse -Force -ErrorAction Ignore
	Remove-Item -Path "HKLM:\SOFTWARE\PanelSwWixExtension" -Recurse -Force -ErrorAction Ignore
}

function prepare{
	New-Item -Path $RegPath -Force
	New-ItemProperty -Path $RegPath -Name "empty_value_test" -PropertyType "String"
}

function check{
	$value = Get-ItemPropertyValue -Path $RegPath -Name "empty_value_test"
	if ($value){
		Write-Host "Expected empty_value_test to be empty"
		$Script:MY_ERR=1
	}
	$value = Get-ItemPropertyValue -Path $RegPath -Name "no_value_test"
	if ($value -ne 'no_value_test'){
		Write-Host "Expected no_value_test to be 'no_value_test'"
		$Script:MY_ERR=1
	}
}

$HostArchitecture = $env:PROCESSOR_ARCHITEW6432
if ($HostArchitecture -eq "AMD64") {
    Write-Host "Skipping test because we're running x86 on a x64 machine'"
	exit 0
}

# Install
Write-Host "=== Testing install ==="
cleanFolders
prepare
$prc = Start-Process msiexec -ArgumentList ("/i", "PropertyPersistUT.msi", "/qn", "/l*v", "PropertyPersistUT.msi.log", "INSTALLFOLDER=""$MY_DIR\install-folder""") -Wait -PassThru
if ($prc.ExitCode -ne 0){
	Write-Host "msiexec failed"
	$Script:MY_ERR=1
}
check

# Uninstall
Write-Host "=== Testing uninstall ==="
$prc = Start-Process msiexec -ArgumentList ("/xPropertyPersistUT.msi", "/qn", "/l*v", "PropertyPersistUT.msix.log", "INSTALLFOLDER=""$MY_DIR\install-folder""") -Wait -PassThru
if ($prc.ExitCode -ne 0){
	Write-Host "msiexec failed"
	$Script:MY_ERR=1
}
cleanFolders

# Clean and exit
Write-Host "Overall error code $MY_ERR"
exit $MY_ERR

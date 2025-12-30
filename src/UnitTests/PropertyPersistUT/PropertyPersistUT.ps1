$MY_ERR=0
$MY_DIR = (Split-Path $MyInvocation.MyCommand.Path)
Write-Host "Folder $MY_DIR"
CD $MY_DIR

function cleanFolders{
    Remove-Item -Path "$Script:MY_DIR\install-folder" -Recurse -Force -ErrorAction Ignore
	Remove-Item -Path "HKLM:\SOFTWARE\PanelSwWixExtension" -Recurse -Force -ErrorAction Ignore
}

# Install
Write-Host "=== Testing install ==="
cleanFolders
$prc = Start-Process msiexec -ArgumentList ("/i", "PropertyPersistUT.msi", "/qn", "/l*v", "PropertyPersistUT.msi.log", "INSTALLFOLDER=""$MY_DIR\install-folder""") -Wait -PassThru
if ($prc.ExitCode -ne 0){
	Write-Host "msiexec failed"
	$Script:MY_ERR=1
}

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

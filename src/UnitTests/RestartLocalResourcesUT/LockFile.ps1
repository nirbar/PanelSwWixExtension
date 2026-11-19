$filePath = $MyInvocation.MyCommand.Path
$mode   = [System.IO.FileMode]::Open
$access = [System.IO.FileAccess]::ReadWrite
$share  = [System.IO.FileShare]::None
$stream = New-Object System.IO.FileStream -ArgumentList @($filePath, $mode, $access, $share)

try {
    Start-Sleep -Seconds 60
}
finally {
    $stream.Dispose()   # releases the lock
    Write-Host "Lock released."
}

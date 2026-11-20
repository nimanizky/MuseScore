param([string]$AppDirectory = "$env:ProgramFiles\Sandikala-MuseScore")
$ErrorActionPreference = 'Stop'
try {
    $identity = [Security.Principal.WindowsIdentity]::GetCurrent()
    $principal = [Security.Principal.WindowsPrincipal]::new($identity)
    if (!$principal.IsInRole([Security.Principal.WindowsBuiltInRole]::Administrator)) {
        throw 'Run Install-Sound-Pack.cmd to grant administrator access.'
    }
    if (!(Test-Path $AppDirectory)) { throw "Install the Sandikala MSI first. Application directory not found: $AppDirectory" }
    if (!(Get-ChildItem $AppDirectory -Filter '*.exe' -Recurse | Select-Object -First 1)) { throw 'No application executable found in the selected directory.' }
    $manifest = Get-Content "$PSScriptRoot\manifest.json" -Raw | ConvertFrom-Json
    $target = Join-Path $AppDirectory 'sound'
    New-Item -ItemType Directory -Force $target | Out-Null
    foreach ($entry in $manifest.files) {
        $archive = Join-Path $PSScriptRoot $entry.archive
        if (!(Test-Path $archive)) { throw "Missing package: $($entry.archive). Download all ZIP files into the same folder." }
        Write-Host "Checking $($entry.archive)..."
        if ((Get-FileHash $archive -Algorithm SHA256).Hash.ToLowerInvariant() -ne $entry.archive_sha256) { throw "Download damaged: $($entry.archive)" }
    }
    foreach ($entry in $manifest.files) {
        Write-Host "Installing $($entry.filename)..."
        Expand-Archive -LiteralPath (Join-Path $PSScriptRoot $entry.archive) -DestinationPath $target -Force
        if ((Get-FileHash (Join-Path $target $entry.filename) -Algorithm SHA256).Hash.ToLowerInvariant() -ne $entry.sha256) { throw "Installed file verification failed: $($entry.filename)" }
    }
    Write-Host 'Sound pack installed successfully. Restart Sandikala MuseScore.' -ForegroundColor Green
} catch {
    Write-Host $_.Exception.Message -ForegroundColor Red
    exit 1
} finally {
    Read-Host 'Press Enter to close'
}

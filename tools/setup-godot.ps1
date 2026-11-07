$ErrorActionPreference = 'Stop'
$ProgressPreference = 'SilentlyContinue'
$projectRoot = Split-Path -Parent $PSScriptRoot
$engineDirectory = Join-Path $projectRoot '.tools/godot'
$releaseTag = '4.7.2-stable'
$release = Invoke-RestMethod -Uri "https://api.github.com/repos/godotengine/godot-builds/releases/tags/$releaseTag"
$asset = $release.assets | Where-Object { $_.name -eq "Godot_v${releaseTag}_win64.exe.zip" } | Select-Object -First 1
if (-not $asset) { throw 'Official Windows engine asset was not found.' }
New-Item -ItemType Directory -Force -Path $engineDirectory | Out-Null
$archive = Join-Path $engineDirectory 'engine.zip'
Invoke-WebRequest -Uri $asset.browser_download_url -OutFile $archive
if ($asset.digest -and $asset.digest.StartsWith('sha256:')) {
    $expectedHash = $asset.digest.Substring(7)
    $actualHash = (Get-FileHash -LiteralPath $archive -Algorithm SHA256).Hash
    if ($actualHash -ne $expectedHash) { throw 'Engine archive checksum does not match the release.' }
}
Expand-Archive -LiteralPath $archive -DestinationPath $engineDirectory -Force
# Keep editor configuration beside this portable engine.
New-Item -ItemType File -Force -Path (Join-Path $engineDirectory '_sc_') | Out-Null
Write-Output "Godot $releaseTag is ready. Run tools/run.ps1."

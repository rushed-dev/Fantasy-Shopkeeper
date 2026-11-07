param([switch]$Editor, [switch]$Test, [switch]$Smoke)
$ErrorActionPreference = 'Stop'
$projectRoot = Split-Path -Parent $PSScriptRoot
$enginePath = Join-Path $projectRoot '.tools/godot/Godot_v4.7.2-stable_win64_console.exe'
if (-not (Test-Path -LiteralPath $enginePath)) {
    throw 'Godot is missing. Run tools/setup-godot.ps1 first, or import project.godot in Godot 4.7.2.'
}
if ($Test) {
    & $enginePath --headless --editor --path $projectRoot --import
    if ($LASTEXITCODE -ne 0) { exit $LASTEXITCODE }
    & $enginePath --headless --path $projectRoot --script res://tests/item_actions_test.gd
} elseif ($Smoke) {
    & $enginePath --headless --path $projectRoot -- --smoke-test
} elseif ($Editor) {
    & $enginePath --editor --path $projectRoot
} else {
    & $enginePath --path $projectRoot
}
exit $LASTEXITCODE

param([switch]$Editor, [switch]$Test, [switch]$Smoke)
$ErrorActionPreference = 'Stop'
$projectRoot = Split-Path -Parent $PSScriptRoot
$enginePath = Join-Path $projectRoot '.tools/godot/Godot_v4.7.2-stable_win64_console.exe'
if (-not (Test-Path -LiteralPath $enginePath)) {
    throw 'Godot is missing. Run tools/setup-godot.ps1 first, or import project.godot in Godot 4.7.2.'
}
function Invoke-GodotCheck {
    param([string[]]$EngineArguments)
    $output = & $enginePath @EngineArguments 2>&1
    $status = $LASTEXITCODE
    $output | ForEach-Object { Write-Output $_ }
    if ($status -ne 0) { exit $status }
    if ($output | Select-String -Pattern 'SCRIPT ERROR:|ERROR:') { exit 1 }
}
if ($Test) {
    Invoke-GodotCheck -EngineArguments @('--headless', '--editor', '--path', $projectRoot, '--import', '--quit')
    Invoke-GodotCheck -EngineArguments @('--headless', '--path', $projectRoot, '--script', 'res://tests/item_actions_test.gd')
    Invoke-GodotCheck -EngineArguments @('--headless', '--path', $projectRoot, '--script', 'res://tests/shop_scene_test.gd')
    Invoke-GodotCheck -EngineArguments @('--headless', '--fixed-fps', '60', '--path', $projectRoot, '--script', 'res://tests/checkout_test.gd')
    Invoke-GodotCheck -EngineArguments @('--headless', '--fixed-fps', '60', '--path', $projectRoot, '--script', 'res://tests/day_loop_test.gd')
    exit 0
} elseif ($Smoke) {
    Invoke-GodotCheck -EngineArguments @('--headless', '--path', $projectRoot, '--', '--smoke-test')
    exit 0
} elseif ($Editor) {
    & $enginePath --editor --path $projectRoot
} else {
    & $enginePath --path $projectRoot
}
exit $LASTEXITCODE

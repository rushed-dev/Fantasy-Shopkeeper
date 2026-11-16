param(
    [string]$EngineRoot = 'C:/Program Files/Epic Games/UE_5.8',
    [switch]$SkipBuild,
    [ValidateSet('Gameplay', 'Rendered', 'Assets', 'Build', 'Package', 'Editor')]
    [string]$Action = 'Gameplay',
    [string]$PackageDirectory,
    [string[]]$EditorArguments = @()
)

$ErrorActionPreference = 'Stop'
$projectRoot = Split-Path -Parent $PSScriptRoot
$projectFile = Join-Path $projectRoot 'FantasyShopkeeper.uproject'
$buildTool = Join-Path $EngineRoot 'Engine/Build/BatchFiles/Build.bat'
$editor = Join-Path $EngineRoot 'Engine/Binaries/Win64/UnrealEditor-Cmd.exe'
$runId = Get-Date -Format 'yyyyMMdd-HHmmss-fff'
$reportPath = Join-Path $projectRoot "Saved/Automation/$Action-$runId"
$logPath = Join-Path $projectRoot "Saved/Logs/$Action-$runId.log"

# UE 5.8 always uses UBA internally. NoUBA disables detours and its remote service.
$buildArguments = @('-NoUBA', '-NoXGE', '-NoFASTBuild', '-NoSNDBS')
# Keep the in-process message bus: automation needs it, but no network transport.
# These flags also apply to commandlets and the packaging cook process.
$localEditorArguments = @(
    '-notraceserver', '-traceautostart=0',
    '-UDPMESSAGING_TRANSPORT_ENABLE=0',
    '-DisablePlugins=UdpMessaging,TcpMessaging,TraceUtilities,CmdLinkServer',
    '-ini:Engine:[Zen.AutoLaunch]:RemoteNetworkService=None',
    '-ini:Engine:[ConsoleVariables]:r.ShaderCompiler.AllowDistributedCompilation=0',
    '-DDC=NoShared', '-NoSourceControl'
)
# UE 5.8's precompiled TraceLog still opens a wildcard trace-control listener.
# -notraceserver disables the separate server, NOT that embedded socket.
Write-Host 'Distributed builds and optional network services disabled. Stock UE 5.8 trace-control listener remains; see docs/local-tooling.md.'

if (-not $SkipBuild) {
    & $buildTool FantasyShopkeeperEditor Win64 Development "-Project=$projectFile" -WaitMutex -NoHotReloadFromIDE @buildArguments
    if ($LASTEXITCODE -ne 0) { throw "Editor build failed with exit code $LASTEXITCODE" }
}
if ($Action -eq 'Build') { return }

if ($Action -eq 'Package') {
    if (-not $PackageDirectory) { throw 'Package requires -PackageDirectory; use a new directory to preserve earlier builds.' }
    $uat = Join-Path $EngineRoot 'Engine/Build/BatchFiles/RunUAT.bat'
    & $uat BuildCookRun "-project=$projectFile" -noP4 -platform=Win64 -clientconfig=Development -build -cook -stage -pak -archive "-archivedirectory=$PackageDirectory" -nocompileeditor -skipbuildeditor -unattended -utf8output "-UbtArgs=$($buildArguments -join ' ')" "-AdditionalCookerOptions=$($localEditorArguments -join ' ')"
    if ($LASTEXITCODE -ne 0) { throw "Packaging failed with exit code $LASTEXITCODE" }
    return
}
if ($Action -eq 'Assets') {
    & $editor $projectFile -run=pythonscript "-script=$PSScriptRoot/check_shop_art.py" '-EnablePlugins=PythonScriptPlugin,EditorScriptingUtilities' @localEditorArguments -unattended -nullrhi -nosound -nosplash "-abslog=$logPath"
    if ($LASTEXITCODE -ne 0) { throw "Asset checks failed. See $logPath" }
    if (-not (Select-String -LiteralPath $logPath -SimpleMatch 'SHOP_ART_CHECK_PASSED' -Quiet)) { throw "Asset report missing. See $logPath" }
    Write-Host "Asset checks passed. Log at $logPath"
    return
}
if ($Action -eq 'Editor') {
    & $editor $projectFile @EditorArguments @localEditorArguments "-abslog=$logPath"
    if ($LASTEXITCODE -ne 0) { throw "Editor task failed. See $logPath" }
    return
}

$testFilter = 'FantasyShopkeeper.'
$expectedTests = 38
$renderArguments = @('-nullrhi')
if ($Action -eq 'Rendered') {
    $testFilter = 'FantasyShopkeeper.Interaction.ShopMapKeyboardWorkflow'
    $expectedTests = 1
    $reviewPath = Join-Path $projectRoot "Saved/ArtReview/$runId"
    New-Item -ItemType Directory -Force -Path $reviewPath | Out-Null
    $renderArguments = @('-RenderOffscreen', "-ShopStorageScreenshot=$reviewPath/storage.png", "-ShopSignScreenshot=$reviewPath/sign.png", "-ShopTillScreenshot=$reviewPath/counter.png", "-ShopPaidScreenshot=$reviewPath/paid.png", "-ShopEveningScreenshot=$reviewPath/evening.png", "-ShopMorningScreenshot=$reviewPath/morning.png", "-ShopNightScreenshot=$reviewPath/night.png")
}
& $editor $projectFile "-ExecCmds=Automation RunTests $testFilter" '-TestExit=Automation Test Queue Empty' "-ReportExportPath=$reportPath" @localEditorArguments @renderArguments -unattended -nosound -nosplash "-abslog=$logPath"
if ($LASTEXITCODE -ne 0) { throw "Unreal test process failed with exit code $LASTEXITCODE. See $logPath" }

# Unreal may exit zero even when individual automation tests fail.
$report = Get-Content -LiteralPath (Join-Path $reportPath 'index.json') -Raw | ConvertFrom-Json
$passed = $report.succeeded + $report.succeededWithWarnings
if ($report.failed -gt 0 -or $report.notRun -gt 0 -or $passed -ne $expectedTests) {
    throw "Interaction tests incomplete or failed. See $reportPath and $logPath"
}
Write-Host "All $passed shop automation tests passed. Report at $reportPath"
if ($report.succeededWithWarnings -gt 0) {
    Write-Warning "$($report.succeededWithWarnings) passing test(s) logged warnings. Review the report."
}

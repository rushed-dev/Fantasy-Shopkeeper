# Local build and test tooling

Use `unreal/FantasyShopkeeper/Tools/check.ps1` for builds, automation, asset checks and packaging. All actions share the same local execution settings. No firewall rules, engine binaries, user-wide build configuration or security settings are changed.

```powershell
# Build and run all 49 gameplay tests (default).
./unreal/FantasyShopkeeper/Tools/check.ps1

# After building, run the rendered workflow or asset validation.
./unreal/FantasyShopkeeper/Tools/check.ps1 -SkipBuild -Action Rendered
./unreal/FantasyShopkeeper/Tools/check.ps1 -SkipBuild -Action Assets

# Compile only.
./unreal/FantasyShopkeeper/Tools/check.ps1 -Action Build

# Package to a new directory, retaining previous builds.
./unreal/FantasyShopkeeper/Tools/check.ps1 -Action Package -PackageDirectory "$PWD/builds/unreal-local-tooling"
```

`-EngineRoot` selects another engine installation. Gameplay and rendered actions validate the automation report as well as the exit code; rendered checks expect one test and gameplay expects 49. Rendered screenshots go to a timestamped `Saved/ArtReview` directory. Reports and logs go to `Saved/Automation` and `Saved/Logs`.

For maintained editor-Python tools, use the same runner rather than launching an unconfigured editor process:

```powershell
$script = (Resolve-Path unreal/FantasyShopkeeper/Tools/install_shop_art.py).Path
./unreal/FantasyShopkeeper/Tools/check.ps1 -SkipBuild -Action Editor -EditorArguments @(
    '-run=pythonscript', "-script=$script",
    '-EnablePlugins=PythonScriptPlugin,EditorScriptingUtilities',
    '-unattended', '-nullrhi', '-nosound'
)
```

Close the interactive editor before builds or saved-asset imports. A static rendered Python review can use `-RenderOffscreen` and `-ExecCmds=py <absolute script path>` instead of the commandlet arguments. Existing `SHOP_ART_ASSETS` selection still works. The runner does not change or save assets unless the selected tool does so.

## Settings and limits

- UBT receives `-NoUBA -NoXGE -NoFASTBuild -NoSNDBS`, including the packaging build through `-UbtArgs`. UE 5.8 always instantiates its UBA executor internally: `-NoUBA` disables detouring and remote execution, so the log can still say “Unreal Build Accelerator local executor”. Actual compile/link actions were verified with `[NoUba]`, without the UBA listening service.
- UDP/TCP messaging transports are disabled, while the in-process message bus remains available for automation. The runner also disables UdpMessaging, TcpMessaging, TraceUtilities and CmdLinkServer plugins for that process. Project defaults disable UDP/TCP transport for normal launches too.
- `-notraceserver` prevents the separate Unreal Trace Server from starting; `-traceautostart=0` disables automatic trace connections. These do **not** disable the embedded trace-control listener.
- Zen's `RemoteNetworkService=None` selects `--http-forceloopback`; project defaults and runner overrides agree. Project configuration clears Zen's port-forwarding plugin array with `!Plugins=ClearArray`, and the runner uses `-DDC=NoShared` (project/engine pak caches and local Zen only). Measured Zen listeners were `127.0.0.1:8558` and `[::1]:8558`.
- Distributed shader compilation is explicitly disabled with `r.ShaderCompiler.AllowDistributedCompilation=0`, both in project defaults and process overrides. Local ShaderCompileWorker processes remain available.
- Package cooks receive the same editor settings through `-AdditionalCookerOptions`. Custom `Editor` actions also receive them. Direct Build.bat/RunUAT/UnrealEditor invocations do not inherit runner flags; use this entry point for routine work.

**Remaining stock-engine limitation:** UE 5.8's precompiled TraceLog opens a TCP control socket on `0.0.0.0:1985`, or a fallback port if that is occupied. The installed `TraceLog/Private/Trace/Control.cpp` and `Detail/Windows/WindowsTrace.cpp` show an unconditional control update and wildcard bind. There is no runtime bind-address/disable option in this implementation; `UE_TRACE_ALLOW_TCP_CONTROL` is a compile-time setting. Disabling the trace worker merely moves updates onto the frame loop and does not remove the listener. A separate source-built engine would be required to change that behavior. The user chose to retain the built-in engine behavior rather than pursue that disproportionate change.

These settings remove optional network services and distributed builds; they are **not** a guarantee of zero listeners or zero firewall prompts from the stock engine. No existing firewall permission is broadened or bypassed. Socket snapshots are observations of running processes, not a packet capture or continuous audit.

## Packaged performance checks

Use an isolated `-UserDir` with `-SaveToUserDir` for packaged verification. A normal offscreen 1080p benchmark exits cleanly. On this installed engine, CSV profiling captures valid frames but the process exits with code 777003 during shutdown (the engine crash-reporting thread failure code), even when capture finishes before exit. Do not count that profiling run as a clean runtime pass or suppress the exit code. See [validation](validation.md) for measurements and the separate normal-render result. Fresh installs use High graphics; existing player settings remain intact.

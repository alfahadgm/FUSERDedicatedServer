# FDS: FUSER Dedicated Server

FDS is a dedicated server for the online multiplayer of **FUSER** (Harmonix, 2020). It is an Unreal Engine 4.24.3 project that serves unmodified FUSER clients (EGS v1.4.2, network version `10402`). It reproduces the server side of FUSER's freestyle sessions and Main Stage events. It contains no FUSER content.

## Features

- **Freestyle sessions:**
  - lobby (teams, DJ queue, readiness, leader, an optional public lobby timer)
  - sets: music clock, stage driver, DJ actions replayed to every listener, live requests
  - results, the hottest mix and the XP payout
  - late joiners get the current mix
- **Main Stage mode:** one long-running server that plays a schedule of events. Each event's booked headliners DJ in turn, and everyone else watches and reacts.
- **fuserd integration:** FDS works with fuserd, the companion matchmaking and backend service.
  - It registers with fuserd and sends status heartbeats.
  - Players are admitted with fuserd-signed Ed25519 session tokens.
  - It answers FUSER's session-browser beacon.
- **Venues:** each of FUSER's six venues has its own empty server world, and the session's time of day can be set.
- **Optional FUSER install:** a licensed install can be mounted read-only to read the venue catalogue. FDS never redistributes or loads FUSER's assets. Content that clients reference is replaced on the server by empty placeholder objects at the same paths, and each client resolves the path to its own copy.

## Repository layout

| Path | Contents |
|---|---|
| `Fuser/Fuser.uproject` | the UE 4.24.3 project |
| `Fuser/Source/FuserServer` | the server: game mode, session authority (`FDSLobbyAuthority`), Main Stage, fuserd registration, admission and session tokens, beacon, venues, install mounting, content placeholders |
| `Fuser/Source/Hype` | FUSER's network-visible types (`/Script/Hype`) as FDS needs them: generated headers and reflection (do not edit by hand; see `generated-files.txt`), plus hand-written serializers and RPC bodies |
| `Fuser/Source/OnlineSubsystemHype` | the FUSER enum that lives in `/Script/OnlineSubsystemHype` |
| `Fuser/Source/FDSEditor` | editor-only commandlet that creates FDS's Blueprint stubs |
| `Fuser/Content` | FDS's own assets: Blueprint stubs at FUSER's class paths and empty venue worlds and sublevels |
| `Fuser/Config` | engine, game and network settings; the map lists cooked into the server |
| `EnginePatches/` | patches FDS needs in the UE 4.24.3 source (see [Engine patches](#engine-patches)) |
| `docker/` | Dockerfile, entrypoint and run reference for the Linux server ([docker/README.md](docker/README.md)) |

## Requirements

- **Windows 10 or 11 x64.** Every build, including the Linux server, runs on Windows.
- **The Unreal Engine 4.24.3 source.** You need an Epic Games account linked to GitHub to see [EpicGames/UnrealEngine](https://github.com/EpicGames/UnrealEngine). The Launcher's binary engine cannot build dedicated servers.
- **Visual Studio 2019 (16.11) or its Build Tools**, with:
  - the *Desktop development with C++* workload and MSVC v142 (14.29)
  - the Windows 10 SDK 10.0.19041.0
  - the .NET Framework 4.6.2 targeting pack (UnrealBuildTool and AutomationTool)
- **Git**, for applying the patches.
- **Disk space:** about 150 GB. The engine's dependencies alone are about 11 GB to download, and a full engine build is large.
- **For the Linux server:**
  - Epic's cross-compile toolchain `v15_clang-8.0.1-centos7`
  - an x86_64 Linux host, WSL2 or Docker to run it

## Building

The paths below are examples. `C:\UE-4.24.3\UnrealEngine-4.24.3-release` is the engine root (the folder holding `Engine\` and `Setup.bat`), and `C:\src\FDS` is this repository.

### 1. Get the engine source

Download the `4.24.3-release` tag, either as a zip from [its release page](https://github.com/EpicGames/UnrealEngine/releases/tag/4.24.3-release) or with Git:

```powershell
git clone --depth 1 --branch 4.24.3-release https://github.com/EpicGames/UnrealEngine.git C:\UE-4.24.3\UnrealEngine-4.24.3-release
```

Keep the path short: the engine build hits Windows path-length limits under deep folders.

### 2. Fetch the engine dependencies

1. **Replace the dependency manifest.** The `Commit.gitdeps.xml` in the 4.24.3 tag points at download locations that no longer work. Download the updated `Commit.gitdeps.xml` attached to the [4.24.3-release release page](https://github.com/EpicGames/UnrealEngine/releases/tag/4.24.3-release) and save it over `Engine\Build\Commit.gitdeps.xml`.
2. **If the download fails with TLS or connection errors:** the dependency tool targets an old .NET runtime. Let it use the operating system's TLS settings by adding this element inside `<configuration>` in `Engine\Binaries\DotNET\GitDependencies.exe.config`:

   ```xml
   <runtime><AppContextSwitchOverrides value="Switch.System.Net.DontEnableSystemDefaultTlsVersions=false;Switch.System.Net.DontEnableSchUseStrongCrypto=false" /></runtime>
   ```

3. **Run setup** from the engine root:

   ```powershell
   .\Setup.bat
   ```

### 3. Apply the engine patches

Run these from the engine root. Git applies the patches whether or not the engine is a Git checkout. `--ignore-whitespace` handles either line-ending style.

```powershell
git apply --ignore-whitespace C:\src\FDS\EnginePatches\fds-engine.patch
git apply --ignore-whitespace C:\src\FDS\EnginePatches\msvc-build-fixes.patch
```

`fds-engine.patch` is required. `msvc-build-fixes.patch` is needed with recent MSVC 14.29 releases, and harmless otherwise. To check before changing anything, run the same commands with `--check`. See [Engine patches](#engine-patches) for what each patch changes.

### 4. Pin the compiler

UE 4.24 can pick up a newer Visual Studio installed on the same machine, or a Windows SDK it can't compile against. Pin both in `Engine\Saved\UnrealBuildTool\BuildConfiguration.xml`:

```xml
<?xml version="1.0" encoding="utf-8"?>
<Configuration xmlns="https://www.unrealengine.com/BuildConfiguration">
  <WindowsPlatform>
    <Compiler>VisualStudio2019</Compiler>
    <CompilerVersion>14.29.30159</CompilerVersion>
    <WindowsSdkVersion>10.0.19041.0</WindowsSdkVersion>
  </WindowsPlatform>
</Configuration>
```

`CompilerVersion` is the *product* version of your MSVC 14.29 toolset, which can differ from its folder name under `VC\Tools\MSVC`. If yours differs, use the version UnrealBuildTool reports, or remove the element.

### 5. Build UnrealBuildTool

From the engine root:

```powershell
.\GenerateProjectFiles.bat
```

This compiles UnrealBuildTool and writes `UE4.sln`. You don't need to open the solution.

### 6. Build the editor and the server

The first build compiles the whole engine and can take an hour or more.

```powershell
$engine  = 'C:\UE-4.24.3\UnrealEngine-4.24.3-release'
$project = 'C:\src\FDS\Fuser\Fuser.uproject'
& "$engine\Engine\Build\BatchFiles\Build.bat" FuserServerEditor Win64 Development "-Project=$project" -WaitMutex
& "$engine\Engine\Build\BatchFiles\Build.bat" FuserServerServer Win64 Development "-Project=$project" -WaitMutex
```

- `FuserServerEditor` is needed to cook content and run FDS's commandlets.
- `FuserServerServer` is the dedicated server.
- The project also has a `FuserServer` game target. It's only for test clients built from this project.

### 7. Cook and package

The cooker skips maps in `DirectoriesToAlwaysCook`, so every map FDS serves is passed explicitly. The lists are `Config\FDSVenueMaps.txt` (one empty world per venue) and `Config\FDSVenueSublevels-*.txt` (the streamed sublevel stubs).

```powershell
$fuser = Split-Path $project
$maps  = @('/Game/Maps/Venue01/Venue01', '/Game/Maps/Server/ServerDefaultMap')
Get-ChildItem "$fuser\Config" -Filter 'FDSVenue*.txt' | ForEach-Object { $maps += Get-Content $_.FullName | Where-Object { $_.Trim() } }
$mapArg = '-map=' + (($maps | Select-Object -Unique) -join '+')

& "$engine\Engine\Build\BatchFiles\RunUAT.bat" BuildCookRun "-project=$project" -noP4 -utf8output `
    -server -noclient -serverplatform=Win64 -serverconfig=Development `
    -build -cook -stage -pak -archive $mapArg "-archivedirectory=C:\src\FDS\build\package"
```

The packaged server is `build\package\WindowsServer\Fuser\Binaries\Win64\FuserServerServer.exe`. `build\` is git-ignored.

If `RunUAT.bat` fails with *'AutomationToolLauncher.exe' is not recognized*, clear `NoDefaultCurrentDirectoryInExePath` for that shell (`Remove-Item Env:NoDefaultCurrentDirectoryInExePath`).

### 8. Linux server (optional)

1. Install Epic's toolchain `v15_clang-8.0.1-centos7`, from `https://cdn.unrealengine.com/CrossToolchain_Linux/v15_clang-8.0.1-centos7.exe`.
2. Set `LINUX_MULTIARCH_ROOT` to its folder; the installer sets it machine-wide.
3. Run setup and generate project files again (steps 2 and 5), so the engine picks up its Linux dependencies.
4. Build and package. The editor stays Win64 because cooking runs on Windows.

   ```powershell
   $env:LINUX_MULTIARCH_ROOT = 'C:\UE-4.24.3\LinuxToolchain\v15_clang-8.0.1-centos7\'
   & "$engine\Engine\Build\BatchFiles\Build.bat" FuserServerServer Linux Development "-Project=$project" -WaitMutex
   & "$engine\Engine\Build\BatchFiles\RunUAT.bat" BuildCookRun "-project=$project" -noP4 -utf8output `
       -server -noclient -serverplatform=Linux -serverconfig=Development `
       -build -cook -stage -pak -archive $mapArg "-archivedirectory=C:\src\FDS\build\package-linux"
   ```

The staged server is `build\package-linux\LinuxServer\`. Inside it, `FuserServerServer.sh` launches `Fuser/Binaries/Linux/FuserServerServer`. The binary links glibc and runs on any x86_64 distribution from CentOS 7 onwards. It needs no display or audio device.

### 9. Docker image (optional)

The image is built from `docker/` plus a tarball of the staged Linux server. Make the build context in a git-ignored folder. On Linux or in WSL, from the repository root:

```sh
mkdir -p build/docker
cp docker/Dockerfile docker/entrypoint.sh docker/README.md build/docker/
cd build/package-linux/LinuxServer
chmod 755 FuserServerServer.sh Fuser/Binaries/Linux/FuserServerServer
tar --exclude=./Fuser/Saved --exclude='*.debug' --exclude='*.sym' --owner=0 --group=0 \
    -czf ../../docker/fds-server-linux.tar.gz .
cd ../../docker
docker build -t fds-server:latest .
```

- **Version reporting (optional):** FDS reports the `version` field of a `MANIFEST.json` at the tarball's root (for example `{"version": "1.0.0"}`) to fuserd. Without the file it reports `dev`.
- **Debug symbols:** excluding the `.debug` / `.sym` files keeps the image small.
- **Running the image:** see [docker/README.md](docker/README.md) for the ports, environment variables and mounting a FUSER install.

## Running

Windows, from the package:

```powershell
build\package\WindowsServer\Fuser\Binaries\Win64\FuserServerServer.exe -port=7777 -log
```

Linux:

```sh
build/package-linux/LinuxServer/FuserServerServer.sh -port=7777 -log
```

- **Ports:** clients join UDP `7777`. The session-browser beacon answers on the game port + 100.
- **Settings:** the server is configured through environment variables, listed in [docker/README.md](docker/README.md), and through `Fuser/Config/DefaultGame.ini`, section `[/Script/FuserServer.FDSLobby]` (set length, DJ slots, lobby timer, results behaviour). Any ini value can be overridden at launch, for example `-ini:Game:[/Script/FuserServer.FDSLobby]:ResultsXPPayout=False`.
- **Venue:** `-FDSVenue=` / `FDS_VENUE` picks the venue. `-FDSMainStage` / `FDS_MAIN_STAGE=1` selects the Main Stage mode.
- **FUSER install:** to mount one, set `FDS_FUSER_ROOT` to the folder holding `Fuser\Content\Paks`, and `FDS_PAK_KEY` to FUSER's AES pak key (64 hex digits or base64). The key is read from the environment only and never logged. Without an install, FDS runs on its own content. Only Venue01 then works, because FDS has stubs for Venue01's streamed sublevels only.

FUSER clients reach FDS through fuserd's matchmaking. A client set up to join an address directly can join it too.

## Engine patches

FDS needs two behaviour changes in the engine. Each changed block is fenced with `// FDS PATCH BEGIN` / `// FDS PATCH END` (or ends with `// FDS PATCH` on a one-line change), so `grep -rn "FDS PATCH" Engine/Source` lists them all.

### `fds-engine.patch` (required)

- **Server missing-object resolver.** `Engine/Classes/Engine/PackageMapClient.h`, `Engine/Private/PackageMapClient.cpp`.
  - **Problem.** Clients refer to their own content (crate instruments, audio effects) by path. A stock server loads that path, fails because FDS has no FUSER content, logs an error and replicates `null`.
  - **Change.** The patch adds `FNetGUIDCache::ServerMissingObjectResolver`, a static hook that is unset by default. It is consulted on the server before any load, and returns one of:
    - `Declined`: stock behaviour;
    - `Created`: use the returned object;
    - `Deferred`: leave the reference unresolved, without loading or logging.
  - **FDS's use.** FDS binds the hook (`FDSAssetPlaceholders`) and creates empty placeholders at the referenced paths.
  - **Compatibility.** With no resolver bound, the engine behaves as stock. Clients never reach this code, and replication layouts and checksums are unchanged.
- **Login-token redaction.** `Engine/Private/World.cpp`.
  - **Problem.** The server logs every login URL, which carries the player's fuserd session token (`playerSessionId=`).
  - **Change.** The three login and join log lines now print that value as `<redacted>`. What the game mode receives is unchanged.

### `msvc-build-fixes.patch` (build environment)

`Experimental/Voronoi/Private/Voronoi/Voronoi.cpp` and `RenderCore/Private/RenderGraphBuilder.cpp`. Recent MSVC 14.29 toolsets reject `HUGE_VALF` / `INFINITY` there with warning C4756 (overflow in constant arithmetic), and UE builds treat it as an error. The patch uses `std::numeric_limits<float>::infinity()` instead. The behaviour is the same.

Changing these files rebuilds the Engine module for every target.

## Regenerating FDS's content (optional)

The assets in `Fuser/Content` are checked in. To recreate them, run the editor commandlets. They write new, empty assets, and nothing is copied from FUSER.

```powershell
$cmd = "$engine\Engine\Binaries\Win64\UE4Editor-Cmd.exe"
& $cmd $project -run=FDSCreateMaps -mapsfile="$fuser\Config\FDSVenueMaps.txt"
& $cmd $project -run=FDSCreateMaps -mapsfile="$fuser\Config\FDSVenueSublevels-Venue01.txt"
& $cmd $project -run=FDSCreateMaps -maps=/Game/Maps/Server/ServerDefaultMap
& $cmd $project -run=FDSCreateBlueprints
```

- `FDSCreateMaps` creates the empty worlds and sublevel stubs under the package names FUSER clients travel to.
- `FDSCreateBlueprints` creates the Blueprint stubs at FUSER's class paths, with the parents and network-relevant defaults FUSER's own Blueprints have.

Other commandlets:

- `-run=FDSNetLayout -classes=<file> -out=<json>` writes FDS's replication layout and class checksums.
- `-run=FDSTokenSelfTest -vectors=<json>` checks the session-token verifier against Ed25519 test vectors.

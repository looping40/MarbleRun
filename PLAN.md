# Marble rolling game prototype in UE 5.5 (C++)

## Context
The user wants a marble-rolling game in Unreal Engine. We are building an **original** game in that genre: a physics ball on an isometric track, racing a countdown to a goal. It uses no names, levels, art or enemies from Marble Madness. The scope is a playable prototype. Target engine: **UE 5.8** (the user is installing it in place of the old version; expected at `C:\Program Files\Epic Games\UE_5.8`, check this before building). VS Community 2022 is installed; its MSVC toolchain must be one that 5.8 supports. Working name: **MarbleRun**.

## Location
New folder `D:\Projets\MarbleRun` (moved into via `change_directory` and approved by the user). `git init` with a UE `.gitignore`. First action after approval: copy this plan to `D:\Projets\MarbleRun\PLAN.md`.

## Before building
Check that `C:\Program Files\Epic Games\UE_5.8\Engine\Build\BatchFiles\Build.bat` exists, and use the 5.8 APIs (Enhanced Input, Chaos physics).

## Project skeleton (written as text, no editor wizard needed)
- `MarbleRun.uproject`: EngineAssociation "5.8", one Runtime module `MarbleRun`, Enhanced Input plugin.
- `Source/MarbleRun.Target.cs`, `Source/MarbleRunEditor.Target.cs`
- `Source/MarbleRun/MarbleRun.Build.cs`: Core, CoreUObject, Engine, InputCore, EnhancedInput, UMG, Slate, SlateCore.
- `Config/DefaultEngine.ini`: GameDefaultMap / EditorStartupMap = `/Game/Maps/Track01`, GlobalDefaultGameMode = `AMarbleGameMode`.
- `Config/DefaultInput.ini`: Enhanced Input as the default input classes.

## C++ classes (`Source/MarbleRun/`)
| Class | Role |
|---|---|
| `AMarblePawn` (APawn) | Root `UStaticMeshComponent` sphere (engine `/Engine/BasicShapes/Sphere`) with physics simulated, angular damping and a physics material. A `USpringArmComponent` with absolute rotation at -45° pitch / 45° yaw (fixed isometric view, no camera lag on rotation) and a `UCameraComponent`. Input: a 2D move action applies torque/force in camera-relative screen directions (so "up" goes up-screen diagonally), plus an optional brake. The input actions and mapping context are **created in C++** (`NewObject<UInputAction>`) so no .uasset is needed. Tracks fall speed: a hard landing over a threshold counts as a "shatter" and respawns the ball. |
| `AMarbleGameMode` | Default pawn = AMarblePawn, HUD = AMarbleHUD. Owns the countdown (e.g. 60 s), the respawn location (last checkpoint) and the state Playing / Won / TimeUp. Restarts the level on R. |
| `ATrackBuilder` (AActor) | Builds the greybox level **procedurally** from a tile array set in C++ (editable in the Details panel), using `UInstancedStaticMeshComponent`s of engine cubes. Tile types: flat, ramp down (slope), narrow bridge, gap. Spawns the start, checkpoints, hazards and goal at tile positions, which means the map asset only needs this one actor plus lights. |
| `AKillZone` (box trigger) | Below the track: on overlap, respawn at the checkpoint with a time penalty. |
| `ACheckpoint` / `AGoalZone` (box triggers) | Checkpoint updates the respawn point and adds bonus seconds. Goal sets the Won state. |
| `ASweeperHazard` | A rotating bar that knocks the ball (kinematic, sweeps with physics push). |
| `AMovingPlatform` | A platform that slides back and forth along a path (`InterpToMovement` or tick sine) and carries the ball. |
| `AMarbleHUD` (AHUD) | Draws timer, state text ("FINISH!", "TIME UP – press R") and speed with `DrawText`. No UMG asset needed. |

## Only editor step: the map
Maps are binary, so the one manual step is: open the editor, File > New Level > Basic, place `ATrackBuilder`, save as `Content/Maps/Track01`. Fallback to avoid even that: `AMarbleGameMode::StartPlay` spawns an `ATrackBuilder` if none exists, so any empty map with a light works. I'll also try to create the map automatically via a commandlet/Python (`UnrealEditor-Cmd.exe ... -run=pythonscript`, with the Python Editor Script plugin enabled) and fall back to the manual step if that fails.

## Build
```
"C:\Program Files\Epic Games\UE_5.8\Engine\Build\BatchFiles\Build.bat" MarbleRunEditor Win64 Development -Project="...\MarbleRun.uproject" -WaitMutex
```
Run in the background, then fix any compile errors and repeat.

## Verification
1. The Build.bat compile finishes with no errors.
2. The user opens `MarbleRun.uproject`, presses Play and checks that:
   - WASD/stick rolls the ball in screen-relative directions with an isometric camera
   - the ball respawns at the last checkpoint after falling off
   - the sweeper knocks the ball and the moving platform carries it
   - reaching the goal shows FINISH, and the timer running out shows TIME UP
   - R restarts the level
3. Tune force, damping and timer values from the feedback.

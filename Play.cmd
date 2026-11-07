@echo off
cd /d "%~dp0"
if not exist ".tools\godot\Godot_v4.7.2-stable_win64.exe" (
  echo Godot is missing. Run tools\setup-godot.ps1 first.
  pause
  exit /b 1
)
start "Fantasy Shopkeeper" ".tools\godot\Godot_v4.7.2-stable_win64.exe" --path "%~dp0."

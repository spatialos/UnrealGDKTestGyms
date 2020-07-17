@echo off
call "%~dp0FindEngine.bat"
call "%~dp0ProjectPaths.bat"
"%UNREAL_ENGINE:"=%\Engine\Binaries\Win64\UE4Editor.exe" "%~dp0%PROJECT_PATH%\%GAME_NAME%.uproject" /Game/Maps/benchmarkgym -ReadFromCommandLine -TotalPlayers=3 -PlayerDensity=3 -TotalNPCs=12 -MaxRoundTrip=150 -MaxLateness=150 -server -log -workerType UnrealWorker -stdout -nowrite -unattended -nologtimes -nopause -noin -messaging -SaveToUserDir -NoVerifyGC -windowed -resX=400 -resY=300

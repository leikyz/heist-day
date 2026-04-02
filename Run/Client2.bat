@echo off
set GAME_EXE="C:\Users\leiky\Desktop\Heist Day\HeistDay\Build\Windows\FinalGame.exe"

start "" %GAME_EXE% -log -EpicApp=MainArtifact -AUTH_LOGIN=DevUser2 -windowed -ResX=1280 -ResY=720

exit
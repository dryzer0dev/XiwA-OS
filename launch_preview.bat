@echo off
echo Lancement de la preview XiwA OS...

REM Vérifier si WSL est installé
wsl --status >nul 2>nul
if %errorlevel% neq 0 (
    echo Installation de WSL...
    wsl --install
    echo Redemarrage necessaire. Relancez le script apres le redemarrage.
    pause
    exit
)

REM Vérifier si l'ISO existe
if not exist "xiwa.iso" (
    echo Creation de l'ISO...
    wsl bash create_iso.sh
)

REM Lancer la VM
echo Lancement de la machine virtuelle...
call vm_launcher.bat

echo Preview terminee. 
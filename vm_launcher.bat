@echo off
echo Creation de la machine virtuelle XiwA OS...

REM Vérifier si QEMU est installé
where qemu-system-x86_64 >nul 2>nul
if %errorlevel% neq 0 (
    echo Installation de QEMU...
    winget install QEMU.QEMU
)

REM Créer le dossier pour la VM
if not exist "vm" mkdir vm
cd vm

REM Créer le disque virtuel s'il n'existe pas
if not exist "xiwa.qcow2" (
    echo Creation du disque virtuel...
    qemu-img create -f qcow2 xiwa.qcow2 10G
)

REM Créer le fichier de configuration de la VM
echo [virtio]
echo driver=virtio
echo [drive]
echo file=xiwa.qcow2
echo format=qcow2
echo [memory]
echo size=2048
echo [display]
echo type=gtk
echo [network]
echo type=user
echo [boot]
echo order=cd
echo [cdrom]
echo file=../xiwa.iso > vm_config.ini

REM Lancer la VM
echo Lancement de XiwA OS...
qemu-system-x86_64 ^
    -m 2048 ^
    -smp 2 ^
    -drive file=xiwa.qcow2,format=qcow2 ^
    -cdrom ../xiwa.iso ^
    -boot d ^
    -display gtk ^
    -enable-kvm ^
    -vga virtio ^
    -net nic,model=virtio ^
    -net user ^
    -name "XiwA OS Preview"

echo VM arretee. 
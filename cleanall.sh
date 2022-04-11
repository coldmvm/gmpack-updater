#/bin/bash
clear
echo ""
echo "****************************************************************"
echo "*                        SCRIPT STARTED                        *"
echo "****************************************************************"
echo ""
echo "******************* Cleaning environment..."
make clean

echo "cleaning gmpack-forwarder..."
cd gmpack-forwarder
rm -rf build
rm gmpack-forwarder.elf
rm gmpack-forwarder.nacp
rm gmpack-forwarder.nro
cd ..

echo "cleaning gmpack-rcm..."
cd gmpack-rcm
rm -rf build
rm -rf output

echo "finishing..."
rm gmpack-updater.zip

echo ""
echo "****************************************************************"
echo "*                       SCRIPT TERMINATED                      *"
echo "****************************************************************"

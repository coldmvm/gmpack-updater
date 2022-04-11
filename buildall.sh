#/bin/bash
clear
echo ""
echo "Updating from GitHub..."
git pull origin main
echo ""

echo "Building gmpack-forwarder..."
cd gmpack-forwarder
make
cd ..
echo ""

echo "Building gmpack-rcm..."
cd gmpack-rcm
make
cd ..
echo ""

echo "Building gmpack-updater..."
make
echo ""

echo "DONE!"

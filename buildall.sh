#/bin/bash
clear
echo ""
echo "****************************************************************"
echo "*                        SCRIPT STARTED                        *"
echo "****************************************************************"
echo ""
echo "******************* Updating from GitHub (DISCARDING ALL LOCAL CHANGES)..."
git fetch --all
git reset --hard origin/main
git pull origin main
echo ""

echo "******************* Building gmpack-forwarder..."
cd gmpack-forwarder
make
cd ..
echo ""

#echo "******************* Building gmpack-rcm..."
#cd gmpack-rcm
#make
#cd ..
#echo ""

echo "******************* Building gmpack-updater..."
make
echo ""

echo "******************* Creating the release ZIP..."
mkdir switch
mkdir ./switch/gmpack-updater
cp gmpack-updater.nro ./switch/gmpack-updater
zip -r -9 ./gmpack-updater.zip ./switch/
rm -rf switch

echo ""

echo "****************************************************************"
echo "*                       SCRIPT TERMINATED                      *"
echo "****************************************************************"

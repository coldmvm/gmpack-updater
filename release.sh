#/bin/bash
echo ""
echo "******************* Creating the release ZIP..."
mkdir switch
mkdir ./switch/gmpack-updater
cp gmpack-updater.nro ./switch/gmpack-updater
zip -r -9 ./gmpack-updater.zip ./switch/
rm -rf switch

echo ""
echo "******************* Done!"
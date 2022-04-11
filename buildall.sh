#/bin/bash
clear
git pull origin main
cd gmpack-forwarder
make
cd ..
cd gmpack-rcm
make
cd ..
make

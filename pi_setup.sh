#!/bin/bash

# Update package list and install git
sudo apt update
sudo apt full-upgrade -y
sudo apt install git -y

# Clone and install daqhats
cd ~
git clone https://github.com/mccdaq/daqhats.git
cd ~/daqhats
sudo ./install.sh

# Install libi2c-dev
sudo apt-get install libi2c-dev -y

# Move daqhats_utils.h to the include directory
sudo cp ~/daqhats/examples/c/daqhats_utils.h /usr/local/include

# Clone and install WiringPi
cd ~
git clone https://github.com/WiringPi/WiringPi.git
cd ~/WiringPi
./build debian
mv debian-template/*.deb .
sudo apt install ./* -y

# cp -r ~/media/pi/USB DRIVE/MUD_Code ~

sudo rfkill block 0
sudo rfkill block 1

echo "Installation complete!"

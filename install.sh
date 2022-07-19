#!/bin/bash
set -e

# Setup
sudo mkdir -p /home/$USER/.m2/repository/com/google/code/gson/gson/2.2.4
sudo apt-get update
sudo apt-get install -y git python3-pip cmake openjdk-8-jdk ant libboost-all-dev libgoogle-gson-java default-jdk default-jre
sudo update-java-alternatives --jre-headless --jre --set java-1.8.0-openjdk-amd64
pip3 install setuptools pyyaml

mkdir -p build
cd build
if [[ "$OSTYPE" == "darwin"* ]]; then
  cmake .. -DCMAKE_CXX_COMPILER=g++ $@
else
  cmake .. $@
fi

# Limit number of cores to use to avoid running out of memory
cores_available=$(nproc)
eval "make -j $(($cores_available-1))"

START=$(date +%s)
sudo make install
END=$(date +%s)
echo "Total install time (real) = $(( $END - $START )) seconds"
cd ..

# Install Confluo libraries / subcomponents
cd libutils
sudo make install
sudo cp libconfluoutils.a /usr/local/lib/
cd ..

cd lz4-prefix/src/lz4
sudo make install
cd ..

cd pyclient
pip3 install -e . --user --verbose
cd ..

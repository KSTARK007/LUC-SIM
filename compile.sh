# sudo apt install cmake nlohmann-json3-dev libtbb-dev screen btop -y
mkdir -p build
cd build
cmake -DCMAKE_BUILD_TYPE=Debug ..
make -j
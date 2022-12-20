rm -rf ./build
cmake -S . -B build
cmake --build build
cp -r ~/workspace/sample/* ./build/bin/
cd ./build/bin; gdb -ex run disk_based_db

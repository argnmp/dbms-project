rm -rf ./build
cmake -S . -B build
cmake --build build
cp -r ~/workspace/sample/* ./build/bin/
cd ./build/bin; gdb disk_based_db

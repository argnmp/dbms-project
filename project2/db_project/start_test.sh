rm -rf ./build
cmake -S . -B build
cmake --build build
cd ./build/bin; ./db_test

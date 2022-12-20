rm -rf ./build
cmake -S . -B build
cmake --build build
cp ./build/lib/libdb.a ~/workspace/2022_ite2038_2019097210/project6_sample_test/marker_project/libdb.a
cd ~/workspace/2022_ite2038_2019097210/project6_sample_test/marker_project/
make testall

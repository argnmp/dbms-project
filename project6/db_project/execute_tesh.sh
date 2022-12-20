#cd ./build/bin
#rm -rf dbrandtest.db logmsg.txt log.data saved_test.cc
cp -r ~/workspace/sample/* ./build/bin/
cd ./build/bin
gdb -ex run disk_based_db


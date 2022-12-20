cd ./build/bin
rm -rf dbrandtest.db logmsg.txt log.data saved_test.cc
cp -r ~/workspace/sample/* ./
gdb -ex run disk_based_db


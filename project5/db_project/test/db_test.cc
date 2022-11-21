#include "db.h"
#include "dbpt.h"
#include <gtest/gtest.h>
#include <string>
#include <random>
#include <algorithm>
#include <sstream>
#include <fstream>

using namespace std;

#define GTEST_COUT(args) std::cerr << "[ RUNNING  ] " args << std::endl;

#define DbAllRandomInsertDeleteTest 0
#define DbScanTest 0

// these should be set exclusively.
#define customTest 0
#define DbSequentialInsertDeleteSet 0
#define DbRandomInsertDeleteSet 1
#define DbBufferSequentialSet 0
#define DbBufferRandomSet 0
#define DbMultipleTableSet 0

class DbTest : public ::testing::Test {
    protected:
        int64_t table_id;
        string pathname;
        DbTest() {
            //remove existing test db file
            remove(pathname.c_str());
            pathname = "a"; 
            table_id = open_table(pathname.c_str()); 
            init_db(1000);
        }
        ~DbTest() {
            shutdown_db();
        }

};
#if customTest
class DbBufferSeqTest : public ::testing::TestWithParam<int> {
    protected:
        int64_t table_id;
        string pathname;
        int sample;
        DbBufferSeqTest() {
            pathname = "dbseqtest.db"; 
            sample = 1000;
            table_id = open_table(pathname.c_str()); 
            init_db(2000);
        }
        ~DbBufferSeqTest() {
            shutdown_db();
        }

};
TEST_P(DbBufferSeqTest, SequentialInsertDeleteBufferTest){
    for(int i = 1; i<=1000; i++){
        string value = "thisisvalue" + to_string(i);
        int result = db_insert(table_id, GetParam()*10000+i, value.c_str(), value.length());
        ASSERT_EQ(result, 0);
    }
}
INSTANTIATE_TEST_SUITE_P(BufSizeTest, DbBufferSeqTest, testing::Range(1, 10, 1));

#endif

#if DbAllRandomInsertDeleteTest
TEST_F(DbTest, AllRandomInsertDeleteTest){

    bool global_procedure_success = true;

    random_device rd;
    mt19937 mt(rd());
    uniform_int_distribution<int> insert_range(0, 10000000);
    uniform_int_distribution<int> delete_range(0, 100);
    uniform_int_distribution<int> value_multiplied_range(1, 10);
    
    /*
     * Insert 10000 random numbers in range (0~100000)
     * For checking the value inserted is same as the found value by db_find, store keys and values in "rands" and "inserted_values"
     */
    set<int> rands;
    map<int, string> inserted_values;
    for(int i = 0; i<=10000; i++){
        int64_t val = insert_range(mt);
        rands.insert(val);
        string value = "thisisvalue";
        for(int k = 0; k<value_multiplied_range(mt); k++){
            value += to_string(val);
        }
        if(inserted_values.find(val)==inserted_values.end()){
            inserted_values.insert({val, value}); 
        }

        //GTEST_COUT(<<"Inserting: "<<val);
        int result = db_insert(table_id, val, value.c_str(), value.length());
    }
        string value = "thisisvalueaaaaaaa=a==a==++++aaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaa";

    /*
     * Create vector of inserted keys and randomly shuffle the keys for testing deletion
     */
    std::vector<int> shuffled_inserted_keys;
    for(auto i: rands){
        shuffled_inserted_keys.push_back(i); 
    }
    shuffle(shuffled_inserted_keys.begin(), shuffled_inserted_keys.end(), mt);
    
    /*
     * Delete the keys and the values, in the order of shuffled inserted values.
     * In the sequence of deletion, this function do not delete key in a probability of one-hundredth (1/100) 
     */
    vector<int> not_deleted_keys;
    for(auto i: shuffled_inserted_keys){
        char ret_val[120]; 
        uint16_t val_size;
        db_find(table_id, i, ret_val, &val_size); 
        /*
         * First find the value using db_find
         * If the value is not same, test fail
         */
        if(memcmp(ret_val, inserted_values[i].c_str(), val_size) != 0){
            global_procedure_success = false;
        }
        
        /*
         * With the probability of one-hundredth, do not delete key. Store the not-deleted key to check again after deletion procedure.
         */
        if(delete_range(mt)==0){
            not_deleted_keys.push_back(i);
            continue;
        }
        //GTEST_COUT(<<"Deleting: "<<i);
        if(db_delete(table_id, i)!=0){
            global_procedure_success = false; 
        }
    }
    /*
     * Checking the existance of not-deleted keys in the db
     */
    for(auto i: not_deleted_keys){
        char ret_val[120]; 
        uint16_t val_size;
        db_find(table_id, i, ret_val, &val_size); 
        if(memcmp(ret_val, inserted_values[i].c_str(), val_size) != 0){
            global_procedure_success = false;
        }
        //GTEST_COUT(<<"Leftover Deleting: "<<i);
        if(db_delete(table_id, i)!=0){
            global_procedure_success = false;
        }
    }
    ASSERT_TRUE(global_procedure_success);
}
#endif
#if DbScanTest
TEST_F(DbTest, ScanTest){
    bool global_procedure_success = true;

    random_device rd;
    mt19937 mt(rd());
    uniform_int_distribution<int> value_multiplied_range(1, 10);
    
    /*
     * Insert 10000 random numbers in range (0~100000)
     * For checking the value inserted is same as the found value by db_find, store keys and values in "rands" and "inserted_values"
     */
    map<int, string> inserted_values;
    for(int i = 0; i<=10000; i++){
        string value = "thisisvalue";
        for(int k = 0; k<value_multiplied_range(mt); k++){
            value += to_string(i);
        }
        int result = db_insert(table_id, i, value.c_str(), value.length());
        inserted_values.insert({i, value});
        ASSERT_EQ(result, 0);
    }

    int64_t begin_key = 1000;
    int64_t end_key = 6000;
    vector<int64_t> keys;
    vector<char*> values;
    vector<uint16_t> val_sizes;
    db_scan(table_id, begin_key, end_key, &keys, &values, &val_sizes);
    ASSERT_EQ(keys.size(), end_key - begin_key + 1);
    for(int i = 0; i<keys.size(); i++){
        int result = memcmp(values[i], inserted_values[keys[i]].c_str(), val_sizes[i]);
        ASSERT_EQ(result, 0);
    }

}
#endif
#if DbSequentialInsertDeleteSet
class DbSeqTest : public ::testing::Test {
    protected:
        int64_t table_id;
        string pathname;
        int sample;
        DbSeqTest() {
            pathname = "dbseqtest.db"; 
            sample = 1000;
            table_id = open_table(pathname.c_str()); 
            init_db(1000);
        }
        ~DbSeqTest() {
            shutdown_db();
        }

};
TEST_F(DbSeqTest, SequentialInsertTest){
    for(int i = 0; i<=sample; i++){
        string value = "thisisvalue" + to_string(i);
        int result = db_insert(table_id, i, value.c_str(), value.length());
        
        // insert should always be successful
        ASSERT_EQ(result, 0);
    }
}
TEST_F(DbSeqTest, SequentialDeleteTest){
    for(int i = 0; i<=sample; i++){
        //printf("inserting: %d\n",i);
        string value = "thisisvalue" + to_string(i);
        int result = db_delete(table_id, i);
        //buf_print();

        // insert should always be successful
        ASSERT_EQ(result, 0);
    }
}
#endif

#if DbRandomInsertDeleteSet

// It takes a lot of time extending the file size
class DbRandTest : public ::testing::Test {
    protected:
        int64_t table_id;
        string pathname;
        int sample;
        
        vector<int> insert_keys;
        vector<int> delete_keys;

        DbRandTest() {
            sample = 100000;
            /*
            //initialize sample
            for(int64_t i = 1; i<=sample; i++){
                insert_keys.push_back(i);
                delete_keys.push_back(i);
            }
            random_device rd;
            mt19937 mt(rd());
            shuffle(insert_keys.begin(), insert_keys.end(), mt);
            shuffle(delete_keys.begin(), delete_keys.end(), mt);
            
            for(int i = 0; i<sample; i++){
                printf("%d ",insert_keys[i]);
            }
            printf("\n");
            for(int i = 0; i<sample; i++){
                printf("%d ",delete_keys[i]);
            }
            printf("\n");
            */

            ifstream insert_keys_s("../../test/insert_keys_100000.txt");
            ifstream delete_keys_s("../../test/delete_keys_100000.txt");
            int elem;
            while(insert_keys_s >> elem){
                insert_keys.push_back(elem);
            }
            while(delete_keys_s >> elem){
                delete_keys.push_back(elem);
            }
            
            pathname = "dbrandtest.db"; 

            table_id = open_table(pathname.c_str()); 
            init_db(1000);
        }
        ~DbRandTest() {
            shutdown_db();
        }

};
TEST_F(DbRandTest, RandomInsertTest){
    //printf("start test\n");
    for(auto i: insert_keys){
        //printf("inserting %d\n",i);
        string value = "thisisvalueaaaaaaa=a==a==++++aaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaa";
        int result = db_insert(table_id, i, value.c_str(), value.length());
        ASSERT_EQ(result, 0);
    }
}
/*
TEST_F(DbRandTest, RandomDeleteTest){
    for(auto i: delete_keys){
        //printf("deleting %d\n",i);
        int result = db_delete(table_id, i); 
        ASSERT_EQ(result, 0);
    }
}
*/
#endif
#if DbBufferSequentialSet
class DbBufferSeqTest : public ::testing::TestWithParam<int> {
    protected:
        int64_t table_id;
        string pathname;
        int sample;
        DbBufferTest() {
            pathname = "dbseqtest.db"; 
            sample = 100000;
            table_id = open_table(pathname.c_str()); 
            init_db(GetParam());
        }
        ~DbBufferTest() {
            shutdown_db();
        }

};
TEST_P(DbBufferTest, SequentialInsertDeleteBufferTest){
    for(int i = 0; i<=sample; i++){
        string value = "thisisvalue" + to_string(i);
        int result = db_insert(table_id, i, value.c_str(), value.length());
        ASSERT_EQ(result, 0);
    }
    for(int i = 0; i<=sample; i++){
        string value = "thisisvalue" + to_string(i);
        int result = db_delete(table_id, i);
        ASSERT_EQ(result, 0);
    }
}
INSTANTIATE_TEST_SUITE_P(BufSizeTest, DbBufferTest, testing::Range(2000, 3000, 10));
#endif

#if DbBufferRandomSet

// It takes a lot of time extending the file size
class DbBufferRandTest : public ::testing::TestWithParam<int> {
    protected:
        int64_t table_id;
        string pathname;
        int sample;
        
        vector<int> insert_keys;
        vector<int> delete_keys;

        DbBufferRandTest() {
            sample = 10000;
            /*
            //initialize sample
            for(int64_t i = 1; i<=sample; i++){
                insert_keys.push_back(i);
                delete_keys.push_back(i);
            }
            random_device rd;
            mt19937 mt(rd());
            shuffle(insert_keys.begin(), insert_keys.end(), mt);
            shuffle(delete_keys.begin(), delete_keys.end(), mt);
            */

            ifstream insert_keys_s("../../test/insert_keys_10000.txt");
            ifstream delete_keys_s("../../test/delete_keys_10000.txt");
            int elem;
            while(insert_keys_s >> elem){
                insert_keys.push_back(elem);
            }
            while(delete_keys_s >> elem){
                delete_keys.push_back(elem);
            }

            pathname = "dbrandtest.db"; 
            remove(pathname.c_str());

            table_id = open_table(pathname.c_str()); 
            init_db(GetParam());
            GTEST_COUT(<<"Working on : "<<GetParam());
        }
        ~DbBufferRandTest() {
            shutdown_db();
        }

};
TEST_P(DbBufferRandTest, RandomInsertDeleteBufferTest){
    for(auto i: insert_keys){
        string value = "thisisvalueaaaaaaa=a==a==++++aaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaa";
        int result = db_insert(table_id, i, value.c_str(), value.length());
        
        ASSERT_EQ(result, 0);
    }
    for(auto i: delete_keys){
        int result = db_delete(table_id, i); 
        ASSERT_EQ(result, 0);
    }
}
INSTANTIATE_TEST_SUITE_P(BufSizeTest, DbBufferRandTest, testing::Range(100, 300, 10));
#endif

#if DbMultipleTableSet
class DbMultipleTableTest : public ::testing::Test {
    protected:
        int64_t tid1;
        int64_t tid2;
        int64_t tid3;
        string pathname;
        int sample;
        vector<int64_t> insert_keys;
        vector<int64_t> delete_keys;
        vector<int64_t> insert_keys1;
        vector<int64_t> delete_keys1;
        vector<int64_t> insert_keys2;
        vector<int64_t> delete_keys2;

        DbMultipleTableTest() {
            sample = 100000;
            pathname = "a"; 
            tid1 = open_table(pathname.c_str()); 
            pathname = "b";
            tid2 = open_table(pathname.c_str()); 
            pathname = "c";
            tid3 = open_table(pathname.c_str()); 

            int elem;

            ifstream insert_keys_s("../../test/insert_keys_100000.txt");
            ifstream delete_keys_s("../../test/delete_keys_100000.txt");
            while(insert_keys_s >> elem){
                insert_keys.push_back(elem);
            }
            while(delete_keys_s >> elem){
                delete_keys.push_back(elem);
            }

            ifstream insert_keys_s1("../../test/insert_keys_100000_1.txt");
            ifstream delete_keys_s1("../../test/delete_keys_100000_1.txt");
            while(insert_keys_s1 >> elem){
                insert_keys1.push_back(elem);
            }
            while(delete_keys_s1 >> elem){
                delete_keys1.push_back(elem);
            }

            ifstream insert_keys_s2("../../test/insert_keys_100000_2.txt");
            ifstream delete_keys_s2("../../test/delete_keys_100000_2.txt");
            while(insert_keys_s2 >> elem){
                insert_keys2.push_back(elem);
            }
            while(delete_keys_s2 >> elem){
                delete_keys2.push_back(elem);
            }

            init_db(10000);
        }
        ~DbMultipleTableTest() {
            shutdown_db();
        }

};
TEST_F(DbMultipleTableTest, MultipleTableTest){
    string value; 
    int result;
    
    for(int i = 0; i<sample; i++){
        value =  "tid1: thisisvalue" + to_string(insert_keys[i]);
        result = db_insert(tid1, insert_keys[i], value.c_str(), value.length());
        ASSERT_EQ(result, 0);
        value =  "tid2: thisisvalue" + to_string(insert_keys1[i]);
        result = db_insert(tid2, insert_keys1[i], value.c_str(), value.length());
        ASSERT_EQ(result, 0);
        value =  "tid3: thisisvalue" + to_string(insert_keys2[i]);
        result = db_insert(tid3, insert_keys2[i], value.c_str(), value.length());
        ASSERT_EQ(result, 0);
        
    }
    /*
    for(int i = 0; i<sample; i++){
        value =  "tid1: thisisvalue" + to_string(delete_keys[i]);
        result = db_insert(tid1, delete_keys[i], value.c_str(), value.length());
        ASSERT_EQ(result, 0);
        value =  "tid2: thisisvalue" + to_string(delete_keys1[i]);
        result = db_insert(tid2, delete_keys1[i], value.c_str(), value.length());
        ASSERT_EQ(result, 0);
        value =  "tid3: thisisvalue" + to_string(delete_keys2[i]);
        result = db_insert(tid3, delete_keys2[i], value.c_str(), value.length());
        ASSERT_EQ(result, 0);
    }
    */
}

#endif

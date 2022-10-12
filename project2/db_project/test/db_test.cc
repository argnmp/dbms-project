#include "db.h"
#include "dbpt.h"
#include <gtest/gtest.h>
#include <string>
#include <random>
#include <algorithm>
#include <sstream>

using namespace std;

#define GTEST_COUT(args) std::cerr << "[          ] [ DEBUG ] " args << std::endl;

class DbTest : public ::testing::Test {
    protected:
        int64_t table_id;
        string pathname;
        DbTest() {
            pathname = "db_test.db"; 
            table_id = open_table(pathname.c_str()); 
            init_db();
        }
        ~DbTest() {
            shutdown_db();
            //remove test db file
            remove(pathname.c_str());
        }

};
TEST_F(DbTest, RandomInsertionDeletionTest){

    bool global_procedure_success = true;

    random_device rd;
    mt19937 mt(rd());
    uniform_int_distribution<int> insert_range(0, 100000);
    uniform_int_distribution<int> delete_range(0, 100);
    uniform_int_distribution<int> value_multiplied_range(1, 10);
    
    /*
     * Insert 10000 random numbers in range (0~100000)
     * For checking the value inserted is same as the found value by db_find, store keys and values in "rands" and "inserted_values"
     */
    set<int> rands;
    map<int, string> inserted_values;
    for(int i = 0; i<=1000; i++){
        int64_t val = insert_range(mt);
        rands.insert(val);
        string value = "thisisvalue";
        for(int k = 0; k<value_multiplied_range(mt); k++){
            value += to_string(val);
        }
        if(inserted_values.find(val)==inserted_values.end()){
            inserted_values.insert({val, value}); 
        }

        int result = db_insert(table_id, val, value.c_str(), value.length());
    }

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
        if(db_delete(table_id, i)!=0){
            global_procedure_success = false;
        }
    }
    ASSERT_TRUE(global_procedure_success);
}
/*
void random_test(int64_t table_id){
    printf("------RANDOM_TEST------\n");
    bool global_procedure_success = true;

    random_device rd;
    mt19937 mt(rd());
    uniform_int_distribution<int> insert_range(0, 10000);
    uniform_int_distribution<int> delete_range(0, 1000);
    uniform_int_distribution<int> value_multiplied_range(1, 20);
    
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
        int result = db_insert(table_id, val, value.c_str(), value.length());
    }
    cout<< "free pages: " << free_page_count(table_id) << endl;
    print_tree(table_id, false);
    print_leaves(table_id);
    std::vector<int> shuffled_inserted_keys;
    for(auto i: rands){
        shuffled_inserted_keys.push_back(i); 
    }
    shuffle(shuffled_inserted_keys.begin(), shuffled_inserted_keys.end(), mt);
    
    vector<int> not_deleted_keys;
    for(auto i: shuffled_inserted_keys){
        char ret_val[120]; 
        uint16_t val_size;
        db_find(table_id, i, ret_val, &val_size); 
        if(memcmp(ret_val, inserted_values[i].c_str(), val_size) != 0){
            global_procedure_success = false;
        }
        
        if(delete_range(mt)==0){
            //printf(" do not delete\n");
            not_deleted_keys.push_back(i);
            continue;
        }
        if(db_delete(table_id, i)!=0){
            global_procedure_success = false; 
        }
    }
    cout<< "free pages: " << free_page_count(table_id) << endl;
    print_tree(table_id, false);
    print_leaves(table_id);
    
    print_tree(table_id, true);
    cout<< "<not_deleted_keys>" << endl;
    for(auto i: not_deleted_keys){
        char ret_val[120]; 
        uint16_t val_size;
        db_find(table_id, i, ret_val, &val_size); 
        printf("key: %d, value: ",i);
        for(int i = 0; i<val_size; i++){
            printf("%c",ret_val[i]);
        }
        if(memcmp(ret_val, inserted_values[i].c_str(), val_size) != 0){
            global_procedure_success = false;
        }
        if(db_delete(table_id, i)!=0){
            global_procedure_success = false;
        }
        cout << endl;
    }

    printf("GLOBAL_PROCEDURE_SUCCESS: %d\n",global_procedure_success);
    printf("--------------END------\n");
}
void scan_test(int64_t table_id){
    random_device rd;
    mt19937 mt(rd());
    uniform_int_distribution<int> insert_range(0, 10000);
    uniform_int_distribution<int> value_multiplied_range(1, 5);
    
    for(int i = 0; i<=100; i++){
        int64_t val = insert_range(mt);
        string value = "thisisvalue";
        for(int k = 0; k<value_multiplied_range(mt); k++){
            value += to_string(val);
        }
        int result = db_insert(table_id, val, value.c_str(), value.length());
    }
    print_tree(table_id, false);

    vector<int64_t> keys;
    vector<char*> values;
    vector<uint16_t> val_sizes;

    db_scan(table_id, 8000, 100000, &keys, &values, &val_sizes);
    printf("total-keys-size: %lu, total-values-size: %lu, total-val_sizes-size: %lu\n", keys.size(), values.size(), val_sizes.size());
    for(int i = 0; i<keys.size(); i++){
        printf("key: %ld | value: ",keys[i]);
        for(int k = 0; k<val_sizes[i]; k++){
            printf("%c",values[i][k]); 
        }
        printf(" | val_size: %hu\n",val_sizes[i]);
    }
    printf("\n");
    shutdown_db();

}
*/

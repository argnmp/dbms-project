//#define buffer_test 123
#ifdef buffer_test

#include "dbpt.h"
#include "db.h"
#include "buffer.h"
#include "trx.h"
#include "log.h"
#include <random>
#include <algorithm>
#include <sstream>
#include <set>
#include <fcntl.h>
#include <fstream>
#include <chrono>
#include <thread>
#include <time.h>
#include <pthread.h>

int64_t table_id;
void* find_thread(void* arg){
    int trx_id = trx_begin();
    for(int k = 1; k<10; k++){
        int i = rand() % 100;
        printf("trx_id: %d, find %d\n",trx_id, i);
        char ret_val[150];
        uint16_t val_size;
        int result = db_find(table_id, i, ret_val, &val_size, trx_id);
        if(result==-1) return NULL;

        /*
        printf("key %d : ",i);
        for(int i = 0; i<val_size; i++){
            printf("%c",ret_val[i]);
        } 
        printf("\n");
        */

    }
    trx_commit(trx_id);
    return NULL;
}
void* update_thread(void* arg){
    int trx_id = trx_begin();
    int result;
    for(int k = 1; k<10; k++){
        int i = rand() % 89 + 10;
        /*
        printf("trx_id: %d, update %d\n",trx_id, i);
        string value = "helloworld";
        uint16_t old_val_size;
        int result = db_update(table_id, i, (char*) value.c_str(), value.length(), &old_val_size, trx_id);
        if(result==-1) return NULL;
        */
        
        printf("trx_id: %d, update %d\n",trx_id, i);
        string value = "value";
        /*
        char ret_val[150];
        uint16_t val_size;
        result = db_find(table_id, i, ret_val, &val_size, trx_id);
        string ret_value(ret_val+5, ret_val+val_size);


        int converted = stoi(ret_value);
        value += to_string(converted + 1);
        

        */
        uint16_t old_val_size;
        result = db_update(table_id, i, (char*) value.c_str(), value.length(), &old_val_size, trx_id);
        if(result==-1) return NULL;
    }
    trx_commit(trx_id);
}
int main(){

    string pathname = "dbrandtest.db"; 

    table_id = open_table(pathname.c_str()); 
    init_db(1000, -1, 30, (char*)"log.data", (char*)"logmsg.txt");

    /*
    for(int i = 1; i<=10000; i++){
        string value = "value" + to_string(i);
        db_insert(table_id, i, value.c_str(), value.length());
    }
    */

    int finder_num = 5;
    int updater_num = 5;

    pthread_t finder[finder_num];
    pthread_t updater[updater_num];

    for(int i = 0; i<finder_num; i++){
        pthread_create(&finder[i], 0, find_thread, NULL);
    }
    for(int i = 0; i<updater_num; i++){
        pthread_create(&updater[i], 0, update_thread, NULL);
    }
	for (int i = 0; i < finder_num; i++) {
		pthread_join(finder[i], NULL);
	}
	for (int i = 0; i < updater_num; i++) {
		pthread_join(updater[i], NULL);
	}
    log_manager.show_lb_buffer();

    shutdown_db();
}

#endif 


#define buffer_test_2 123
#ifdef buffer_test_2

#include "dbpt.h"
#include "db.h"
#include "buffer.h"
#include "trx.h"
#include "log.h"
#include <random>
#include <algorithm>
#include <sstream>
#include <set>
#include <fcntl.h>
#include <fstream>
#include <chrono>
#include <thread>
#include <time.h>
#include <pthread.h>
#include <stdlib.h>

int64_t table_id;
void sequence(){
    int result;
    int trx_id = trx_begin();
    int trx_id_for_commit = trx_begin();

    printf("before start, verify 20\n");
    char ret_val[150];
    uint16_t val_size;
    result = db_find(table_id, 20, ret_val, &val_size, trx_id);
    string ret_value(ret_val, ret_val+val_size);

    printf("value: %s\n", ret_value.c_str());

    for(int k = 1; k<=20; k++){
        int i = 20;
        printf("trx_id: %d, update %d\n",trx_id, i);
        string value = "value";
        char ret_val[150];
        uint16_t val_size;
        result = db_find(table_id, i, ret_val, &val_size, trx_id);

        string ret_value(ret_val+5, ret_val+val_size);

        int converted = stoi(ret_value);
        value += to_string(converted + 1);
        
        if(k==10) {
            trx_commit(trx_id_for_commit);
            log_manager.show_lb_buffer();
            exit(1);
        }

        uint16_t old_val_size;
        result = db_update(table_id, i, (char*) value.c_str(), value.length(), &old_val_size, trx_id);
        
    }
    trx_commit(trx_id);
    
}
void* test_thread(void* arg){
    log_manager.show_lb_buffer();
    sequence();
    return nullptr;
}
int main(){

    string pathname = "dbrandtest.db"; 

    table_id = open_table(pathname.c_str()); 
    init_db(1000, -1, 30, (char*)"log.data", (char*)"logmsg.txt");

    /*
    for(int i = 1; i<=10000; i++){
        string value = "value" + to_string(i);
        db_insert(table_id, i, value.c_str(), value.length());
    }
    */

    int test_num = 1;

    pthread_t tester[test_num];

    for(int i = 0; i<test_num; i++){
        pthread_create(&tester[i], 0, test_thread, NULL);
    }
	for (int i = 0; i < test_num; i++) {
		pthread_join(tester[i], NULL);
	}

    log_manager.show_lb_buffer();

    shutdown_db();
}

#endif 


































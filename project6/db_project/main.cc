#define buffer_test 123
#ifdef buffer_test

#include "dbpt.h"
#include "db.h"
#include "buffer.h"
#include "trx.h"
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
    for(int k = 1; k<100; k++){
        int i = rand() % 100;
        //printf("trx_id: %d, find %d\n",trx_id, i);
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
    for(int k = 1; k<10000; k++){
        int i = rand() % 100;
        /*
        printf("trx_id: %d, update %d\n",trx_id, i);
        char ret_val[150];
        uint16_t val_size;
        int result = db_update(table_id, i, ret_val, val_size, &val_size, trx_id);
        if(result==-1) return NULL;
        */
        printf("trx_id: %d, update %d\n",trx_id, i);
        string value = "helloworld";
        uint16_t old_val_size;
        int result = db_update(table_id, i, (char*) value.c_str(), value.length(), &old_val_size, trx_id);
        if(result==-1) return NULL;
        
    }
    trx_commit(trx_id);
}
int main(){

    string pathname = "dbrandtest.db"; 

    table_id = open_table(pathname.c_str()); 
    init_db(1000, 0, 0, (char*)"log.data", (char*)"logmsg.txt");

    for(int i = 1; i<=10000; i++){
        string value = "thisisvalueaaaaaaa=a==a==++++aaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaa" + to_string(i);
        db_insert(table_id, i, value.c_str(), value.length());
    }

    int finder_num = 100;
    int updater_num = 100;

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

    shutdown_db();
}

#endif 




































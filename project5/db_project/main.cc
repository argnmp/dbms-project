
//#define main_test 123
#ifdef main_test

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

using namespace std;
void* get_trx_id(void* arg){
    int trx_id = trx_begin();
    //printf("trx_id: %d\n",trx_id);
    for(int i = 0; i<100; i++){
        //printf("working: %d\n",i);
        lock_t* lock_obj = lock_acquire(1, 1, 1, trx_id, EXCLUSIVE);        
        if(lock_obj==nullptr){
            printf("abort!");
            trx_commit(trx_id);
            return NULL;
        }
        //lock_obj = lock_acquire(1, 2, trx_id*1000+i, trx_id, EXCLUSIVE);        
        
    }
    trx_commit(trx_id);
}
int main(){
    pthread_t workers[2];
    for(int i = 0; i<2; i++){
        pthread_create(&workers[i], 0, get_trx_id, NULL);
    }
	for (int i = 0; i < 2; i++) {
		pthread_join(workers[i], NULL);
	}
    printf("%d", trx_table.trx_map.size());
    /*
    for (int i = 1; i<=1000000; i++){
        lock_t* cursor = trx_table.trx_map[i].head;
        int counter = 0;
        while(cursor != nullptr){
            counter++;
            cursor = cursor -> next_lock;
        }
        if(counter != 20){
            printf("false\n");
            return 0;
        }

        
    }
    printf("success");
    */
    return 0;
}
#endif 

//#define temp_test 123
#ifdef temp_test
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
using namespace std;

#define CUBE_SIZE 10

int64_t cube[CUBE_SIZE][CUBE_SIZE][CUBE_SIZE] = {0,};

void* inc_number(void* arg){
    int trx_id  = trx_begin();
    for(int i = 0; i<1000; i++){
        lock_t* lock_obj = lock_acquire(1,1,i%10, trx_id, EXCLUSIVE);
        lock_obj = lock_acquire(1,2,i%30, trx_id, EXCLUSIVE);
    }
    trx_commit(trx_id);
    return NULL;
}
void* scan_number(void* arg){
    int trx_id  = trx_begin();
    for(int i = 0; i<1000; i++){
        lock_t* lock_obj = lock_acquire(1,1,i%10, trx_id, SHARED);
        lock_obj = lock_acquire(1,2,i%30, trx_id, SHARED);
    }
    trx_commit(trx_id);
    return NULL;
}

void cube_test(){
    pthread_t workers[100];
    pthread_t scanners[10];
    for(int i = 0; i<100; i++){
        pthread_create(&workers[i], 0, inc_number, NULL);
    }
    for(int i = 0; i<10; i++){
        pthread_create(&scanners[i], 0, scan_number, NULL);
    }
    
    for (int i = 0; i <100; i++) {
        pthread_join(workers[i], NULL);
    }
    for(int i = 0; i<10; i++){
        pthread_join(scanners[i], NULL);
    }
    
}
int main(){
    cube_test(); 
}
#endif

//#define lock_table_test 100
#ifdef lock_table_test
#include "trx.h"

#include <stdio.h>
#include <pthread.h>
#include <unistd.h>
#include <stdlib.h>
#include <time.h>

#define TRANSFER_THREAD_NUMBER	(100)
#define SCAN_THREAD_NUMBER		(0)

#define TRANSFER_COUNT			(1000000)
#define SCAN_COUNT				(10000)

#define TABLE_NUMBER			(3)
#define RECORD_NUMBER			(5)
#define INITIAL_MONEY			(100000)
#define MAX_MONEY_TRANSFERRED	(100)
#define SUM_MONEY				(TABLE_NUMBER * RECORD_NUMBER * INITIAL_MONEY)

/* This is shared data pretected by your lock table. */
int accounts[TABLE_NUMBER][RECORD_NUMBER];

/*
 * This thread repeatedly transfers some money between accounts randomly.
 */
void*
transfer_thread_func(void* arg)
{
    int trx_id = trx_begin();
	lock_t*			source_lock;
	lock_t*			destination_lock;
	int				source_table_id;
	int				source_record_id;
	int				destination_table_id;
	int				destination_record_id;
	int				money_transferred;

	for (int i = 0; i < TRANSFER_COUNT; i++) {
        //printf("working %d\n",i);
		/* Decide the source account and destination account for transferring. */
		source_table_id = rand() % TABLE_NUMBER;
		source_record_id = rand() % RECORD_NUMBER;
		destination_table_id = rand() % TABLE_NUMBER;
		destination_record_id = rand() % RECORD_NUMBER;

		if ((source_table_id > destination_table_id) ||
				(source_table_id == destination_table_id &&
				 source_record_id >= destination_record_id)) {
			/* Descending order may invoke deadlock conditions, so avoid it. */
			continue;
		}
		
		/* Decide the amount of money transferred. */
		money_transferred = rand() % MAX_MONEY_TRANSFERRED;
		money_transferred = rand() % 2 == 0 ?
			(-1) * money_transferred : money_transferred;
		
		/* Acquire lock!! */
        char value[130];
        uint16_t val_size = 0;
		source_lock = lock_acquire(1, source_table_id, source_record_id, trx_id, EXCLUSIVE, value, val_size);
        if(source_lock == nullptr) {
            printf("abort!\n");
            trx_commit(trx_id);
                return NULL;
        }

		/* withdraw */
		accounts[source_table_id][source_record_id] -= money_transferred;

		/* Acquire lock!! */
		destination_lock =
			lock_acquire(1, destination_table_id, destination_record_id, trx_id, EXCLUSIVE, value, val_size);
        if(destination_lock == nullptr) {
            printf("abort!\n");
            trx_commit(trx_id);
                return NULL;
        }

		/* deposit */
		accounts[destination_table_id][destination_record_id]
			+= money_transferred;

		/* Release lock!! */
		//lock_release(destination_lock);
		//lock_release(source_lock);
	}
    trx_commit(trx_id);


	printf("Transfer thread is done.\n");

	return NULL;
}

/*
 * This thread repeatedly check the summation of all accounts.
 * Because the locking strategy is 2PL (2 Phase Locking), the summation must
 * always be consistent.
 */
void*
scan_thread_func(void* arg)
{
    int trx_id = trx_begin();
	int				sum_money;
	lock_t*			lock_array[TABLE_NUMBER][RECORD_NUMBER];

	for (int i = 0; i < SCAN_COUNT; i++) {
        printf("scanning %d\n",i);
		sum_money = 0;

		/* Iterate all accounts and summate the amount of money. */
		for (int table_id = 0; table_id < TABLE_NUMBER; table_id++) {
			for (int record_id = 0; record_id < RECORD_NUMBER; record_id++) {
				/* Acquire lock!! */
				lock_array[table_id][record_id] =
					lock_acquire(1, table_id, record_id, trx_id, SHARED, nullptr, 0);
                if(lock_array[table_id][record_id] == nullptr) {
                    printf("abort!\n");
                    trx_commit(trx_id);
                    return NULL;
                }

				/* Summation. */
				sum_money += accounts[table_id][record_id];
			}
		}

		for (int table_id = 0; table_id < TABLE_NUMBER; table_id++) {
			for (int record_id = 0; record_id < RECORD_NUMBER; record_id++) {
				/* Release lock!! */
				//lock_release(lock_array[table_id][record_id]);
			}
		}

        
		/* Check consistency. */
        
		if (sum_money != SUM_MONEY) {
            /*
			printf("Inconsistent state is detected!!!!!\n");
			printf("sum_money : %d\n", sum_money);
			printf("SUM_MONEY : %d\n", SUM_MONEY);
            trx_commit(trx_id);
            return NULL;
            */
		}
	}

    trx_commit(trx_id);
	printf("Scan thread is done.\n");

	return NULL;
}

int main()
{
	pthread_t	transfer_threads[TRANSFER_THREAD_NUMBER];
	pthread_t	scan_threads[SCAN_THREAD_NUMBER];

	srand(time(NULL));

	/* Initialize accounts. */
	for (int table_id = 0; table_id < TABLE_NUMBER; table_id++) {
		for (int record_id = 0; record_id < RECORD_NUMBER; record_id++) {
			accounts[table_id][record_id] = INITIAL_MONEY;
		}
	}

	/* Initialize your lock table. */
	init_lock_table();

	/* thread create */
	for (int i = 0; i < TRANSFER_THREAD_NUMBER; i++) {
		pthread_create(&transfer_threads[i], 0, transfer_thread_func, NULL);
	}
	for (int i = 0; i < SCAN_THREAD_NUMBER; i++) {
		pthread_create(&scan_threads[i], 0, scan_thread_func, NULL);
	}

	/* thread join */
	for (int i = 0; i < TRANSFER_THREAD_NUMBER; i++) {
		pthread_join(transfer_threads[i], NULL);
	}
	for (int i = 0; i < SCAN_THREAD_NUMBER; i++) {
		pthread_join(scan_threads[i], NULL);
	}

    /*
     * check connected transaction table objects 
     * this test is valid when delete lock_obj is disabled in lock_release api
     */
    
    /*
    for (int i = 1; i < trx_table.g_trx_id; i++){
        lock_t* cursor = trx_table.trx_map[i].head;
        int counter = 0;
        while(cursor != nullptr){
            counter++;
            cursor = cursor -> next_lock;
        }
        printf("trx id: %d, counter %d\n",i, counter);
    }
    */
    printf("trx_table_size: %d", trx_table.trx_map.size());
    

	return 0;
}

#endif

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
    init_db(1000);

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


































//#define transaction_test 100
#ifdef transaction_test
#include "trx.h"
#include "db.h"

#include <stdio.h>
#include <pthread.h>
#include <unistd.h>
#include <stdlib.h>
#include <time.h>

#define TRANSFER_THREAD_NUMBER	(10)
#define SCAN_THREAD_NUMBER		(2)

#define TRANSFER_COUNT			(1000000)
#define SCAN_COUNT				(10000)

#define TABLE_NUMBER			(3)
#define RECORD_NUMBER			(5)
#define INITIAL_MONEY			(100000)
#define MAX_MONEY_TRANSFERRED	(100)
#define SUM_MONEY				(TABLE_NUMBER * RECORD_NUMBER * INITIAL_MONEY)

/* This is shared data pretected by your lock table. */
int accounts[TABLE_NUMBER][RECORD_NUMBER];

/*
 * This thread repeatedly transfers some money between accounts randomly.
 */
void*
transfer_thread_func(void* arg)
{
    int trx_id = trx_begin();
	lock_t*			source_lock;
	lock_t*			destination_lock;
	int				source_table_id;
	int				source_record_id;
	int				destination_table_id;
	int				destination_record_id;
	int				money_transferred;

	for (int i = 0; i < TRANSFER_COUNT; i++) {
        printf("working %d\n",i);
		/* Decide the source account and destination account for transferring. */
		source_table_id = rand() % TABLE_NUMBER;
		source_record_id = rand() % RECORD_NUMBER;
		destination_table_id = rand() % TABLE_NUMBER;
		destination_record_id = rand() % RECORD_NUMBER;

		if ((source_table_id > destination_table_id) ||
				(source_table_id == destination_table_id &&
				 source_record_id >= destination_record_id)) {
			/* Descending order may invoke deadlock conditions, so avoid it. */
			continue;
		}
		
		/* Decide the amount of money transferred. */
		money_transferred = rand() % MAX_MONEY_TRANSFERRED;
		money_transferred = rand() % 2 == 0 ?
			(-1) * money_transferred : money_transferred;
        
        int result;
        char ret_val[120];
        uint16_t val_size;
        uint16_t old_val_size;
        string num_str;
        int num_int;

		/* withdraw */
        result = db_find(source_table_id, source_record_id, ret_val, &val_size, trx_id);
        if(result==-1) return NULL;
        
        num_str = "";
        for(int i = 0; i<val_size; i++) num_str += ret_val[i];
        num_int = stoi(num_str);
        num_int -= money_transferred;
        num_str = to_string(num_int);  

        result = db_update(source_table_id, source_record_id, (char*) num_str.c_str(), num_str.length(),&old_val_size, trx_id);
        if(result==-1) return NULL;

		/* deposit */
        result = db_find(destination_table_id, destination_record_id, ret_val, &val_size, trx_id);
        if(result==-1) return NULL;
        
        num_str = "";
        for(int i = 0; i<val_size; i++) num_str += ret_val[i];
        num_int = stoi(num_str);
        num_int += money_transferred;
        num_str = to_string(num_int);  

        result = db_update(destination_table_id, destination_record_id, (char*) num_str.c_str(), num_str.length(),&old_val_size, trx_id);
        if(result==-1) return NULL;

	}
    trx_commit(trx_id);


	printf("Transfer thread is done.\n");

	return NULL;
}

/*
 * This thread repeatedly check the summation of all accounts.
 * Because the locking strategy is 2PL (2 Phase Locking), the summation must
 * always be consistent.
 */
void*
scan_thread_func(void* arg)
{
    int trx_id = trx_begin();
	int				sum_money;
	lock_t*			lock_array[TABLE_NUMBER][RECORD_NUMBER];

	for (int i = 0; i < SCAN_COUNT; i++) {
        printf("scanning %d\n",i);
		sum_money = 0;

		/* Iterate all accounts and summate the amount of money. */
		for (int table_id = 0; table_id < TABLE_NUMBER; table_id++) {
			for (int record_id = 0; record_id < RECORD_NUMBER; record_id++) {
                int result;
                char ret_val[120];
                uint16_t val_size;
                uint16_t old_val_size;
                string num_str;
                int num_int;

                /* withdraw */
                result = db_find(table_id, record_id, ret_val, &val_size, trx_id);
                if(result==-1) return NULL;

                num_str = "";
                for(int i = 0; i<val_size; i++) num_str += ret_val[i];
                num_int = stoi(num_str);

				/* Summation. */
				sum_money += num_int;
			}
		}
        
		/* Check consistency. */
        
		if (sum_money != SUM_MONEY) {
			printf("Inconsistent state is detected!!!!!\n");
			printf("sum_money : %d\n", sum_money);
			printf("SUM_MONEY : %d\n", SUM_MONEY);
            trx_commit(trx_id);
            return NULL;
		}
	}

    trx_commit(trx_id);
	printf("Scan thread is done.\n");

	return NULL;
}

int main()
{
	pthread_t	transfer_threads[TRANSFER_THREAD_NUMBER];
	pthread_t	scan_threads[SCAN_THREAD_NUMBER];

	srand(time(NULL));

	/* Initialize accounts. */
	for (int64_t table_id = 0; table_id < TABLE_NUMBER; table_id++) {
		for (int64_t record_id = 0; record_id < RECORD_NUMBER; record_id++) {
            printf("initializing\n");
            string value = to_string(INITIAL_MONEY);
            db_insert(table_id, record_id, value.c_str(), value.length());
		}
	}

    init_db(1000);

	/* Initialize your lock table. */

	/* thread create */
	for (int i = 0; i < TRANSFER_THREAD_NUMBER; i++) {
		pthread_create(&transfer_threads[i], 0, transfer_thread_func, NULL);
	}
	for (int i = 0; i < SCAN_THREAD_NUMBER; i++) {
		pthread_create(&scan_threads[i], 0, scan_thread_func, NULL);
	}

	/* thread join */
	for (int i = 0; i < TRANSFER_THREAD_NUMBER; i++) {
		pthread_join(transfer_threads[i], NULL);
	}
	for (int i = 0; i < SCAN_THREAD_NUMBER; i++) {
		pthread_join(scan_threads[i], NULL);
	}

    /*
     * check connected transaction table objects 
     * this test is valid when delete lock_obj is disabled in lock_release api
     */
    
    /*
    for (int i = 1; i < trx_table.g_trx_id; i++){
        lock_t* cursor = trx_table.trx_map[i].head;
        int counter = 0;
        while(cursor != nullptr){
            counter++;
            cursor = cursor -> next_lock;
        }
        printf("trx id: %d, counter %d\n",i, counter);
    }
    */
    

	return 0;
}
#endif

//#define single_thread_test 100
#ifdef single_thread_test
#include "trx.h"
#include "db.h"

#include <stdio.h>
#include <pthread.h>
#include <unistd.h>
#include <stdlib.h>
#include <time.h>

int64_t table_id;
int result;

void test(){
    int trx_id = trx_begin();
    //printf("trx_id: %d\n",trx_id);
    char ret_val[130];
    uint16_t val_size;
    string new_val = to_string(trx_id);
    uint16_t old_val_size;
    
    result = db_update(table_id, 1000, (char*)new_val.c_str(), new_val.length(), &old_val_size, trx_id ); 
    result = db_update(table_id, 1000, (char*)new_val.c_str(), new_val.length(), &old_val_size, trx_id ); 
    result = db_find(table_id, 1000, ret_val, &val_size, trx_id); 
    /*
    for(int i = 0; i<val_size; i++){
        printf("%c",ret_val[i]);
    }
    printf("\n");
    */

    result = trx_commit(trx_id);
    //printf("result: %d\n",result);

}
int main(){
    string pathname = "dbrandtest.db"; 

    table_id = open_table(pathname.c_str()); 
    init_db(1000);
    
    for(int i = 1; i<=1000; i++){
        string value = to_string(i);
        db_insert(table_id, i, value.c_str(), value.length());
    }
    for(int i = 0; i<1000; i++){
        test();
    }

    shutdown_db();
}
#endif

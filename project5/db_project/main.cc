
#define main_test 123
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
		source_lock = lock_acquire(1, source_table_id, source_record_id, trx_id, EXCLUSIVE);
        if(source_lock == nullptr) {
            printf("abort!\n");
            trx_commit(trx_id);
                return NULL;
        }

		/* withdraw */
		accounts[source_table_id][source_record_id] -= money_transferred;

		/* Acquire lock!! */
		destination_lock =
			lock_acquire(1, destination_table_id, destination_record_id, trx_id, EXCLUSIVE);
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
					lock_acquire(1, table_id, record_id, trx_id, SHARED);
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

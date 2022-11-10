#include "lock_table.h"

struct lock_t {
    lock_t* prev;
    lock_t* next;
    hash_table_entry* sentinel;
    pthread_cond_t cond;
};

typedef struct lock_t lock_t;

pthread_mutex_t lock_table_latch;
unordered_map<pair<int64_t,int64_t>, hash_table_entry, pair_for_hash> hash_table;

int init_lock_table() {
    int result = 0;

    pthread_mutexattr_t mtx_attribute;
    result = pthread_mutexattr_init(&mtx_attribute);
    if(result!=0) return -1;
    result = pthread_mutexattr_settype(&mtx_attribute, PTHREAD_MUTEX_NORMAL);
    if(result!=0) return -1;
    result = pthread_mutex_init(&lock_table_latch, &mtx_attribute);
    if(result!=0) return -1;
    return 0;
}

lock_t* lock_acquire(int64_t table_id, int64_t key) {
    int result = 0;

    result = pthread_mutex_lock(&lock_table_latch); 
    if(result!=0) return nullptr;

    bool is_immediate;
    if(hash_table.find({table_id, key})==hash_table.end()){
        hash_table_entry hte;
        hte.table_id = table_id;
        hte.key = key;
        hte.head = nullptr;
        hte.tail = nullptr;
        hash_table.insert({{table_id, key}, hte}); 
    }
    hash_table_entry* target = &hash_table[{table_id, key}]; 

    is_immediate = (target->head == nullptr);
    lock_t* lck = new lock_t;
    lck->next = nullptr;
    lck->prev = nullptr;
    lck->sentinel = &hash_table[{table_id, key}];
    //lck->cond = PTHREAD_COND_INITIALIZER;
    result = pthread_cond_init(&(lck->cond), NULL);
    if(result!=0) return nullptr;

    if(is_immediate){
        target->head = lck;
        target->tail = lck;
    }
    else {
        target->tail->next = lck;
        lck->prev = target->tail;
        target->tail = lck; 
    }
    if(!is_immediate){
        result = pthread_cond_wait(&lck->cond, &lock_table_latch);
        if(result!=0) return nullptr;
    }

    result = pthread_mutex_unlock(&lock_table_latch); 
    if(result!=0) return nullptr;

    return lck;
};

int lock_release(lock_t* lock_obj) {
    int result = 0;

    result = pthread_mutex_lock(&lock_table_latch); 
    if(result!=0) return -1;

    bool is_immediate;
    hash_table_entry* target = lock_obj->sentinel;

    is_immediate = (lock_obj->next == nullptr);

    if(is_immediate){
        target->head = nullptr;
        target->tail = nullptr;
    }
    else {
        target->head = lock_obj->next;
        lock_obj->next->prev = nullptr;
    }

    //!
    if(!is_immediate){
        result = pthread_cond_signal(&lock_obj->next->cond);
        if(result!=0) return -1;
    }

    delete lock_obj;

    result = pthread_mutex_unlock(&lock_table_latch); 
    if(result!=0) return -1;

    return 0;
}

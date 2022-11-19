#ifndef TRX_H_
#define TRX_H_

#include <stdint.h>
#include <unordered_map>
#include <limits.h>
#include <pthread.h>
#include "file.h"

typedef struct lock_t lock_t;

/*
 * TRX_TABLE_MODULE_BEGIN
 */
struct trx_map_entry_t {
    lock_t* head;
    lock_t* tail;
};
class TRX_Table {
public:
    unordered_map<int, trx_map_entry_t> trx_map;    
    int g_trx_id = 1;
    pthread_mutex_t trx_table_latch;    
    
    // trx_id: success, 0: fail
    int create_entry();

    // 0: success, -1: fail
    int connect_lock_obj(int trx_id, lock_t* lock_obj); 

    // trx_id: success, 0: fail
    int release_trx_lock_obj(int trx_id);
};
extern TRX_Table trx_table;

int trx_begin();
int trx_commit(int trx_id);

/*
 * TRX_TABLE_MODULE_END
 */

/*
 * LOCK_TABLE_MODULE_BEGIN
 */
using namespace std;

#define SHARED 0
#define EXCLUSIVE 1

// This is the hash combine function originated from boost::hash_combine
// https://www.boost.org/doc/libs/1_55_0/doc/html/hash/reference.html#boost.hash_combine
// https://www.boost.org/doc/libs/1_55_0/doc/html/hash/combine.html](https://www.boost.org/doc/libs/1_55_0/doc/html/hash/combine.html
// https://stackoverflow.com/questions/7222143/unordered-map-hash-function-c](https://stackoverflow.com/questions/7222143/unordered-map-hash-function-c

template <class T>
inline void boost_hash_combine(std::size_t& seed, const T& v){
    std::hash<T> hasher;
    seed ^= hasher(v) + 0x9e3779b9 + (seed << 6) + (seed >> 2);
}

struct pair_for_hash {
    template <class T1, class T2>
        std::size_t operator () (const std::pair<T1, T2> &p) const {
            std::size_t seed = 0;
            boost_hash_combine(seed, p.first);
            boost_hash_combine(seed, p.second);
            return seed;
        }
};


struct hash_table_entry {
    int64_t table_id;
    int64_t page_id;
    lock_t* head;
    lock_t* tail;
};

struct lock_t {
    lock_t* prev;
    lock_t* next;
    hash_table_entry* sentinel;
    pthread_cond_t cond;
    int lock_mode;
    int64_t record_id;
    lock_t* next_lock;
    int trx_id;
};


extern pthread_mutex_t lock_table_latch;
extern unordered_map<pair<int64_t,pagenum_t>, hash_table_entry, pair_for_hash> hash_table;


/* APIs for lock table */
int init_lock_table();
lock_t* lock_acquire(int64_t table_id, pagenum_t page_id, int64_t key, int trx_id, int lock_mode);
int lock_release(lock_t* lock_obj);

/*
 * LOCK_TABLE_MODULE_END
 */


#endif


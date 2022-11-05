#ifndef __LOCK_TABLE_H__
#define __LOCK_TABLE_H__

#include <stdint.h>
#include <unordered_map>
#include <map>
#include <iostream>
using namespace std;

// https://gist.github.com/eugene-malashkin/884e225ff57aca1b9cbe
// This is the hash combine function originated from boost::hash_combine
template <class T>
inline void boost_hash_combine(std::size_t& seed, const T& v){
    std::hash<T> hasher;
    seed ^= hasher(v) + 0x9e3779b9 + (seed << 6) + (seed >> 2);
}

typedef struct lock_t lock_t;
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
    int64_t key;
    lock_t* head;
    lock_t* tail;
};

extern pthread_mutex_t lock_table_latch;
extern unordered_map<pair<int64_t,int64_t>, hash_table_entry, pair_for_hash> hash_table;


/* APIs for lock table */
int init_lock_table();
lock_t* lock_acquire(int64_t table_id, int64_t key);
int lock_release(lock_t* lock_obj);

#endif /* __LOCK_TABLE_H__ */

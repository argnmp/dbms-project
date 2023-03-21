# Project4
## Introduction
This Project4 wiki page describes the design and implementation of lock table.

## Design
### Hash table
To implement lock table, I used c++ stl data structure, **unordered_map** with **boost hash combine function**.

**boost** is library set for c++. Boost library defines custom **hash library** which is `boost::hash`, and also provides `boost::hash_combine` function for combining hashed keys in a way to reduce hash collisions.

`unordered_map` c++ hash function does not support for **paired keys**. To store paired keys in `unordered_map`, both hashed keys should be combined to one hashed key. In this process, I used boost's hash combine function.

```cpp
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

extern unordered_map<pair<int64_t,int64_t>, hash_table_entry, pair_for_hash> hash_table;
```

The source of this hash combine code is as follows:
- [https://www.boost.org/doc/libs/1_55_0/doc/html/hash/reference.html#boost.hash_combine](https://www.boost.org/doc/libs/1_55_0/doc/html/hash/reference.html#boost.hash_combine)
- [https://www.boost.org/doc/libs/1_55_0/doc/html/hash/combine.html](https://www.boost.org/doc/libs/1_55_0/doc/html/hash/combine.html)

implementation:
- [https://stackoverflow.com/questions/7222143/unordered-map-hash-function-c](https://stackoverflow.com/questions/7222143/unordered-map-hash-function-c)

`hash_table` unordered map uses a pair of table id and record id as key and stores `hash_table_entry` struct as value.

```cpp
struct hash_table_entry {
    int64_t table_id;
    int64_t key;
    lock_t* head;
    lock_t* tail;
};
```
Each entry maintains the list of **lock objects** which haven't returned to threads (so that they are in waiting state), because one other thread is using the **record**. 
- `lock_t* head` points to the front of this list and `lock_t* tail` points to the end of this list.

### Lock object
```cpp
struct lock_t {
    lock_t* prev;
    lock_t* next;
    hash_table_entry* sentinel;
    pthread_cond_t cond;
};
```
This is the lock object that is connected as a list in the `hash_table_entry`. 
- `lock_t* prev` and `lock_t* next` point the previous and next lock object in the **lock object list** of  `hash_table_entry`.
- `hash_table_entry* sentinel` points to the `hash_table_entry` which the lock object is connected to as a list.
- `pthread_cont_t cond` is **conditional variable** that is used to awake the thread associated with this lock object.

### Apis

#### `int init_lock_table();`
In this implementation, when multiple threads are trying to access lock table, only one thread should access to it at a time. This can be done by wrapping the accessing part of `lock_acquire` and `lock_release` function with mutex. This mutex is defined as `pthread_mutex_t lock_table_latch` and it is initialized in `init_lock_table` function.

```cpp
extern pthread_mutex_t lock_table_latch;
```

#### `lock_t* lock_acquire(int64_t table_id, int64_t key)`
This function is invoked by multiple threads when they attempt to lock specific record of a table. 

Sequence of this function is as follows:
1. Acquire `lock_table_latch`. 
2. If the entry according to table_id and key does not exist, create new entry using `hash_table_entry`.
3. If `lock_t* head` is null, this is the case that no thread currently uses the target record of a table, so set `is_immediate` to true. Otherwise, set `is_immediate` to false.
4. a) If `is_immediate` is true, the thread that invoked this function does not go into the blocking state. Create new lock object and connect to the **lock object list** of `hash_table_entry`. And then, release `lock_table_latch`, return lock object immediately to the thread.
4. b) If `lock_t* head` is false, the thread that invoked this function goes in to the blocking state because one other thread is locking this entry. In this case, create new lock object and connect to the existing **lock object list** of `hash_table_entry`. And then go into blocking state(the thread called this function is blocked) using the conditional variable of new lock object. Most important part is that `lock_table_latch` is given to `pthread_cond_wait` because it has to release `lock_table_latch` before going into blocking state, otherwise other threads can't acquire `lock_table_latch` and eventually wait infinitely. The thread in blocking state is awakened by the thread which lock object is right in front of the blocked thread's lock object. When it is awakened, it again acquires `lock_table_latch`, so that it is able to do further processing. And then, release `lock_table_latch` and return lock object.

#### `int lock_release(lock_t* lock_obj)`
This function is invoked by multiple threads when each thread finished accessing specific record of a table that they locked using `lock_acquire` api.

Sequence of this function is as follows:
1. Acquire `lock_table_latch`.
2. If `lock_obj->next`(given `lock_obj`) is null, this is the case that no other threads are currently waiting(blocked), so set `is_immediate` to true. Otherwise, set `is_immediate` to false.
3. a) If `is_immediate` is true, there's no need to signal because there are no waiting threads. Remove lock object(given `lock_obj`) from **lock object list** of `hash_table_entry` using `lock_obj` and release `lock_table_latch`.
3. b) If `is_immediate` is false, remove lock object from the list and generate signal using conditional variable of next lock object. With this signal, waiting(blocked) thread is awakened and tries to acquire `lock_table_latch`. Next lock object can be found using given `lock_obj`. Finally release `lock_table_latch`.




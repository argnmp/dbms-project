# project5 : milestone1
## Introduction
This Project 5 milestone 1 wiki page describes lock manager module. In this project, transaction concept is applied to my dbms.

## Design
### Transaction Table
Each client(thread) starts their action using `trx_begin` function call and finishes ther action using `trx_end`. So, module for managing each transactions is needed.

```cpp
extern pthread_mutex_t trx_table_latch;    
class TRX_Table {
public:
    unordered_map<int, trx_map_entry_t> trx_map;    
    int g_trx_id = 1;
    int create_entry();
    int connect_lock_obj(int trx_id, lock_t* lock_obj); 
    int release_trx_lock_obj(int trx_id);
    int abort_trx_lock_obj(int trx_id);
    int push_undo_value(int trx_id, char* value, uint16_t old_val_size);
};
extern TRX_Table trx_table;

int trx_begin();
int trx_commit(int trx_id);
```
`TRX_Table` class contains all function that are used for managing transactions. Because multiple threads concurrently access to transaction table, protect this table with `trx_table_latch` mutex.

`trx_begin` api, it returns unique transaction id increasing `g_trx_id`. Allocated transaction id is stored in `trx_map` with `trx_map_entry_t`.

```cpp
struct trx_map_entry_t {
    lock_t* head;
    lock_t* tail;
    queue<pair<char*, uint16_t>> undo_values; 
};
```
Main function that `trx_map_entry_t` does is to connect all the lock objects created by this `transaction`(trx_id). `trx_commit` function which internally uses `TRX_Table::release_trx_lock_obj` uses this entry to release all the acquired lock objects by the transaction(S2PL).

`trx_commit` api, it internally calls `TRX_Table::release_trx_lock_obj`. `TRX_Table::release_trx_lock_obj` releases all the lock objects that is linked in transaction table using `lock_release`. 

According to Strict two-phase lock(S2PL), when creating lock object to the lock table, there can be deadlock situation. In this case, abort sequence restor all the modified by the transaction. When the transaction updates record, stores before value in `undo_values`. `abort_trx_lock_obj` which aborts the transaction, uses `undo_values` to restore all the modified values.


### Buffer Manager
To implement Lock Manager, `is_pinned` value that is used for not evicting the buffer page is replaced with `pthread_mutex_t page_latch`.
```cpp
struct buf_block_t{
    int64_t table_id;
    pagenum_t pagenum; 
    int is_dirty;
    //int is_pinned;
    pthread_mutex_t page_latch;
    frame_t* frame;  
    buf_block_t* next;
    buf_block_t* prev;
};
```
When a specific page is needed on page buffer by a transaction, first lock the page using `page_latch`. Other transaction should wait until the transaction using the page calls `buf_unpin` function which internally unlock the `page_latch`.

To evict a buffer page, buffer manager calls `pthread_mutex_trylock` to check if a buffer frame is in use. If it is not used, buffer manager successfully acquires lock on the page. Now it can safely evict the buffer page.

### Lock Table
Lock table is the main module to guarantee **conflict serializable schedule**. This implementation uses Strict Two Phase Locking to satisfy that condition.

There are two types of lock mode, **Shared** and **Exclusive**. Shared lock can be concurrently acquired with same Shared locks. But only one Exclusive lock can be acquired at a time.

s: shared lock, x: exclusive lock

|   | s | x |
|---|---|---|
| s | V | - |
| x | - | - | 

This is how each lock is acquired.



#### structure
```cpp
#define SHARED 0
#define v
    bitset<50> key_set;
};

extern pthread_mutex_t lock_table_latch;
extern unordered_map<pair<int64_t,pagenum_t>, hash_table_entry, pair_for_hash> hash_table;

/* APIs for lock table */
lock_t* lock_acquire(int64_t table_id, pagenum_t page_id, int64_t key, int trx_id, int lock_mode);
int lock_release(lock_t* lock_obj);
```
`hash_table` is `lock table`. `lock_table_latch` protects `lock table` from concurrent access from different threads.

Each entry in lock table is defined for `table_id` and `page_id` pair. So, records that belongs to same `leaf page` is linked to the same `hash_table_entry`. 

`trx_id` is added to `lock_t` which is lock object linked to `hash_table_entry` compared to `project4` lock table implementation.

`key_map`, `key_map_reverse`, `key_set` is used for optimizations in **Project5 milestone2**, description will be in that wiki page.

#### `lock_acquire`
There are 3 main phase in `lock_acquire` api. 
1. Implicit lock phase
2. Checking already acquired lock phase
3. Deadlock checking phase

First phase is used for optimizations in **Project5 milestone2**, so description will be in that wiki page.

In second phase, this api checks if the transaction has already acquired lock object. A transaction has already acquired shared lock and if the same transaction requests for exclusive lock, it internally upgrade shared lock to exclusive lock, which means that it detaches shared lock of the transaction and attach exclusive lock.

After second phase if `lock_acquire` function is not returned, new lock object is attached at the end of lock table entry. 

In third phase, it performs deadlock checking. In Strict Two Phase Locking, before finishing to attach lock object, it needs to check for any deadlock situation if it attaches new lock object.  If deadlock is detected, abort sequence is required.

In this checking sequence,
1. First, it search any other lock object which new lock object should wait for(which has same record_id). That is, searching for **dependent lock object**. And then check if the dependent lock object has the same **transaction id** which new lock object belongs to. 
2. a) If dependent lock object has same transaction id, this means that the transactions form cycle in waiting. This is the deadlock case. b) If dependent lock object does not have same transaction id, now it needs to check for dependency of that **dependent lock object**. This is recursive job, so `bool lock_acquire_deadlock_detection(lock_t* dependency, int trx_id)` does this action.
3. `lock_acquire_deadlock_detection` first moves to **acquired or not acquired lock object** in the transaction which dependent lock object is belonged to using transaction table. Because all the lock objects that are previous than the lock object is already acquired, so only the last object in the transaction lock object list is involved in dependency. And then, search for dependent lock object and check transaction id. (sequence 1, 2). This action is **recursively** done to all transactions that have dependency to each other.

Shared lock is dependent to one Exclusive lock, but not dependent to Shared lock. Exclusive lock is dependent to one Exclusive lock and can be dependent to **multiple Shared lock**. So, when deadlock check in this situation requires, checking for all shared lock dependencies.

#### `lock_release`
The main job of this api is to detach lock object and signal to waiting lock object. In the specific case, multiple shared lock can be acquired to one record and an exclusive lock is waiting for these all shared lock to be finished. In this case, for each releasing these multiple shared lock, it should check if any other shared lock is concurrently acquiring the same record. 

## Apis
### `trx_begin`
By calling this function, start new transaction. This api creates entry in transaction table with transaction id of g_trx_id and increases g_trx_id. This returns unique transaction id.

### `trx_commit`
When all `db_find` and `db_update` actions end, this api is called. Deallocate all the previously allocated resources, and erase entry in transaction table. 

### `db_find`
Check if the record id exists in the database, and attempt to acquire shared lock. If deadlock occurs, start abort sequence and rollback all the changes of this transaction. When shared lock is successfully acquired, return data of the record and return 0 if successful.

### `db_update`
Check if the record id exists in the database, and attemp to acquire exclusive lock. If deadlock occurs, start abort sequence and rollback all the changes of this transaction. During the lock_acquire sequence, there are different cases whether undo_value needs to be inserted into `undo_values` of `trx_map_entry_t`. `add_undo_value` is given to `lock_acquire` function, and this works as a flag to insert undo value in `undo_values` or not. When exclusive lock is successfully acquired, update the record and return 0 if successful.

# project5 : milestone2
## Introduction
This Project5 milestone 2 wiki page describes two optimization for lock manager. **Implicit locking** reduces space overhead for exclusive locks, and **Lock compression** reduces space overhead for shared locks.

## Implicit Locking
To apply implicit locking, previously implemented `slot_t` structure.
```cpp
class slot_t{
public:
    uint8_t data[16] = {0};
    int64_t get_key();
    uint16_t get_size();
    uint16_t get_offset();
    int get_trx();
    void set_key(int64_t key);
    void set_size(uint16_t size);
    void set_offset(uint16_t offset);
    void set_trx(int trx);
};
```
This is the class for managing slot space. Size of data increased to 16 bytes, and last 4bytes are used to store transaction id.

As mentioned in project5 milestone1 wiki page, implicit locking is applied as the first phase of `lock_acquire` function. Because implicit locking works when explicit lock can be acquired immeidately, it is positioned to be check first in `lock_acquire` function.

First, it checks if there is any lock object that has the same record id. If it does exist, there is nothing to do with implicit locking, so goes to next phase. However, if there is no same record id lock object, transaction id of **slot** of record should be checked. 

Next action is differs accoding the the transaction id of slot and transaction state.

1. transaction id of slot == 0
- No implicit lock is acquired on this record. If lock object to be acquired is Shared, it goes to next phase. Otherwise(exclusive lock), acquires implicit lock and return.

2. transaction id of slot == transaction id of lock object to be acquired
- This case means that transaction id of lock object to be acquired is currently alive, and already acquired exclusive lock. No action is needed, so return.

3. else
- In this case, it's time to convert implicit lock to explicit lock. Because lock object to be acquired is created and attached in the next phase, just change implicit lock to explicit lock.

## Lock Compression
To use bitmap for representing shared lock objects of same transaction id, mapping each bit with record id in a page is needed.

```cpp
struct hash_table_entry {
    int64_t table_id;
    int64_t page_id;
    lock_t* head;
    lock_t* tail;
    unordered_map<int64_t, int> key_map;
    unordered_map<int, int64_t> key_map_reverse;
};
```
`key_map` is used to map record id to bit index, and `key_map_reverse` is used to map bit index to record id. By using this mapping, each 

```cpp
    if(hash_table.find({table_id, page_id})==hash_table.end()){
        ...
        for(int i = 0; i<leaf.leaf_ptr->number_of_keys; i++){
            slot_t tmp; memcpy(&tmp.data, leaf.leaf_ptr->data + i*SLOT_SIZE, sizeof(tmp.data)); 
            hte.key_map.insert({tmp.get_key(), i});
            hte.key_map_reverse.insert({i, tmp.get_key()});
        }   
        buf_unpin(table_id, leaf.pn);

        hash_table.insert({{table_id, page_id}, hte}); 
    }
```
When hash table entry is created, mapping information is stored in `key_map` and `key_map_reverse` structure. Because this project doesn't consider index structure modified case, this implementation is possible.

To implement bitmap, I used `bitset` stl in cpp. 
```cpp
struct lock_t {
    ...
    bitset<50> key_set;
};
```
Each lock object has `key_set` which is a bitmap which individual bit marks the mapped record id acquired to that transaction. Shared lock uses `key_set` to compress shared lock objects.

Changes for implementing lock compression affects almost entire part of `lock_acquire`, `lock_acquire_deadlock_detection`, `lock_release`.

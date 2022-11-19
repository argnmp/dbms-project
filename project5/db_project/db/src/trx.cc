#include "trx.h"

/*
 * TRX_TABLE_MODULE_BEGIN
 */

int TRX_Table::create_entry(){
    int result;
    result = pthread_mutex_lock(&trx_table_latch); 
    if(result != 0) return 0;
    
    while(trx_map.find(g_trx_id)!=trx_map.end()){
        if(g_trx_id == INT_MAX){
            g_trx_id = 1;
        }
        g_trx_id += 1;
    }
    trx_map_entry_t tmet = {nullptr, nullptr};
    trx_map.insert({g_trx_id, tmet});

    int return_value = g_trx_id;
    g_trx_id += 1;
    if(g_trx_id == INT_MAX){
        g_trx_id = 1;
    }

    result = pthread_mutex_unlock(&trx_table_latch);
    if(result != 0) return 0;
    
    return return_value;
}

int TRX_Table::connect_lock_obj(int trx_id, lock_t* lock_obj){

    int result;
    result = pthread_mutex_lock(&trx_table_latch); 
    if(result != 0) return 0;

    // no entry exists
    if(trx_map.find(trx_id)==trx_map.end()){
        pthread_mutex_unlock(&trx_table_latch);
        return -1;
    }
    
    if(trx_map[trx_id].head == nullptr){
        trx_map[trx_id].head = lock_obj;
        trx_map[trx_id].tail = lock_obj;
    }
    else {
        trx_map[trx_id].tail->next_lock = lock_obj;
        trx_map[trx_id].tail = lock_obj;
    }

    result = pthread_mutex_unlock(&trx_table_latch);
    if(result != 0) return 0;

    return 0;
}

TRX_Table trx_table;

// not yet implemented
int TRX_Table::release_trx_lock_obj(int trx_id){
    int result;
    result = pthread_mutex_lock(&trx_table_latch); 
    if(result != 0) return 0;

    printf("release trx: %d\n",trx_id);

    // no entry exists
    if(trx_map.find(trx_id)==trx_map.end()){
        pthread_mutex_unlock(&trx_table_latch);
        return 0;
    }
    
    lock_t* cursor = trx_map[trx_id].head;
    while(cursor != nullptr){
        printf("record_id: %d\n",cursor->record_id);
        printf("release\n");
        lock_t* next = cursor -> next_lock;
        lock_release(cursor);
        printf("after release\n");
        cursor = next;
    }

    trx_table.trx_map.erase(trx_id);

    result = pthread_mutex_unlock(&trx_table_latch);
    if(result != 0) return 0;
    
    return trx_id; 
}

int trx_begin(){
    return trx_table.create_entry(); 
}
int trx_commit(int trx_id){
    return trx_table.release_trx_lock_obj(trx_id); 
}



/*
 * TRX_TABLE_MODULE_END
 */

/*
 * LOCK_TABLE_MODULE_BEGIN
 */

int debug_count = 0;

pthread_mutex_t lock_table_latch;
unordered_map<pair<int64_t,pagenum_t>, hash_table_entry, pair_for_hash> hash_table;

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

/*
bool lock_acquire_deadlock_detection(){

}
*/
lock_t* lock_acquire(int64_t table_id, pagenum_t page_id, int64_t key, int trx_id, int lock_mode) {
    //printf("lock_acquire %d, %d, %d\n",table_id, page_id, key);
    int result = 0;

    result = pthread_mutex_lock(&lock_table_latch); 
    if(result!=0) return nullptr;

    if(hash_table.find({table_id, page_id})==hash_table.end()){
        hash_table_entry hte;
        hte.table_id = table_id;
        hte.page_id = page_id;
        hte.head = nullptr;
        hte.tail = nullptr;
        hash_table.insert({{table_id, page_id}, hte}); 
    }
    hash_table_entry* target = &hash_table[{table_id, page_id}]; 

    lock_t* lck = new lock_t;
    lck->next = nullptr;
    lck->prev = nullptr;
    lck->sentinel = &hash_table[{table_id, page_id}];
    lck->lock_mode = lock_mode;
    lck->record_id = key;
    lck->next_lock = nullptr;
    lck->trx_id = trx_id;
    //lck->cond = PTHREAD_COND_INITIALIZER;
    result = pthread_cond_init(&(lck->cond), NULL);
    if(result!=0) return nullptr;

    //connect lock object at the end of list
    bool is_immediate;
    is_immediate = (target->head == nullptr);
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
        lock_t* cursor = lck->prev;
        bool is_prev_xlock_exist = false;
        bool is_record_id_exist = false;
        while(cursor != nullptr){
            //printf("debug_count lock_acquire %d\n", debug_count++);
            if(cursor->record_id != key){
                cursor = cursor -> prev;
                continue;
            }
            is_record_id_exist = true;
            if(cursor->lock_mode == EXCLUSIVE){
                is_prev_xlock_exist = true;
                break;
            }     
            cursor = cursor -> prev;
        }
        if(is_record_id_exist == true){
            if(lck->lock_mode == EXCLUSIVE){
                result = pthread_cond_wait(&lck->cond, &lock_table_latch);
                if(result!=0) return nullptr;
            }
            else if(lck->lock_mode == SHARED){
                if(is_prev_xlock_exist == true){
                    result = pthread_cond_wait(&lck->cond, &lock_table_latch);
                    if(result!=0) return nullptr;
                }
            }
            
        }

        /*
        result = pthread_cond_wait(&lck->cond, &lock_table_latch);
        if(result!=0) return nullptr;
        */
    }

    result = pthread_mutex_unlock(&lock_table_latch); 
    if(result!=0) return nullptr;

    //trx_table connection
    trx_table.connect_lock_obj(trx_id, lck);

    return lck;
};

int lock_release(lock_t* lock_obj) {

    int result = 0;

    printf("lock_release start\n");
    result = pthread_mutex_lock(&lock_table_latch); 
    //printf("lock_release start\n");
    //printf("    lock_release %d, %d, %d\n",lock_obj->sentinel->table_id, lock_obj->sentinel->page_id, lock_obj->record_id);
    if(result!=0) return -1;

    bool is_immediate;
    hash_table_entry* target = lock_obj->sentinel;

    is_immediate = ((lock_obj->next == nullptr) && (lock_obj->prev == nullptr));

    if(is_immediate){
        //this is the case that only one lock object exists
        target->head = nullptr;
        target->tail = nullptr;
    }
    else {
        //this is the case that lock objects are more than 2
        if(lock_obj->lock_mode == SHARED){
            //printf("shared case\n");
            int slock_count = 0;
            bool is_xlock_on_right_side = false;
            lock_t* xlock_cursor = lock_obj->next;

            while(xlock_cursor != nullptr){
                //printf("debug_count on shared%d\n", debug_count++);
                if(xlock_cursor->record_id != lock_obj->record_id) {
                    xlock_cursor = xlock_cursor->next;
                    continue;
                }
                if(xlock_cursor->lock_mode == EXCLUSIVE){
                    is_xlock_on_right_side = true;
                    break;
                }
                slock_count += 1;
                xlock_cursor = xlock_cursor->next;
            } 

            if(is_xlock_on_right_side == false){
                //printf("no rightside xlock case\n");
                if(lock_obj->prev == nullptr){
                    //case lock_obj is leftmost
                    target->head = lock_obj->next;
                    lock_obj->next->prev = nullptr;
                }
                else if(lock_obj->next == nullptr){
                    //case lock_obj is rightmost
                    target->tail = lock_obj->prev;
                    lock_obj->prev->next = nullptr;
                }
                else {
                    lock_obj->prev->next = lock_obj->next;
                    lock_obj->next->prev = lock_obj->prev;
                }
            }
            else if(is_xlock_on_right_side == true){
                //printf("rightside xlock case\n");
                lock_t* cursor = lock_obj->prev;
                while(cursor != nullptr){
                    //printf("debug_count on shared xlock rightside %d\n", debug_count++);
                    if(cursor->record_id != lock_obj->record_id){
                        cursor = cursor->prev;
                        continue;
                    }
                    slock_count += 1;
                    cursor = cursor->prev;
                } 

                if(lock_obj->prev == nullptr){
                    //case lock_obj is leftmost
                    target->head = lock_obj->next;
                    lock_obj->next->prev = nullptr;
                }
                else if(lock_obj->next == nullptr){
                    //case lock_obj is rightmost
                    target->tail = lock_obj->prev;
                    lock_obj->prev->next = nullptr;
                }
                else {
                    lock_obj->prev->next = lock_obj->next;
                    lock_obj->next->prev = lock_obj->prev;
                }

                if(slock_count > 0){
                    //just release
                }
                else {
                    //signal xlock
                    //printf("signal xlock!\n");
                    result = pthread_cond_signal(&xlock_cursor->cond);
                    if(result!=0) return -1;
                }
            }
        }
        else if(lock_obj->lock_mode == EXCLUSIVE){
            lock_t* cursor = lock_obj->next;
            bool is_slock_acquired = false;
            while(cursor!=nullptr){
                //printf("debug_count on exclusive %d\n", debug_count++);
                if(cursor->record_id != lock_obj->record_id){
                    cursor = cursor->next;
                    continue;
                }
                if(cursor->lock_mode == EXCLUSIVE){
                    if(!is_slock_acquired){
                        result = pthread_cond_signal(&cursor->cond);
                        if(result!=0) return -1;
                    }
                    break;
                } 
                else {
                    is_slock_acquired = true;
                }
                result = pthread_cond_signal(&cursor->cond);
                if(result!=0) return -1;

                cursor = cursor -> next;
            }

            if(lock_obj->prev == nullptr){
                //case lock_obj is leftmost
                target->head = lock_obj->next;
                lock_obj->next->prev = nullptr;
            }
            else if(lock_obj->next == nullptr){
                //case lock_obj is rightmost
                target->tail = lock_obj->prev;
                lock_obj->prev->next = nullptr;
            }
            else {
                lock_obj->prev->next = lock_obj->next;
                lock_obj->next->prev = lock_obj->prev;
            }
            
        }
    }

    delete lock_obj;

    //printf("lock_release end\n");
    result = pthread_mutex_unlock(&lock_table_latch); 
    if(result!=0) return -1;

    return 0;
}

/*
 * LOCK_TABLE_MODULE_END
 */



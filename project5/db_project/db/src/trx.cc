#include "trx.h"

/*
 * TRX_TABLE_MODULE_BEGIN
 */

pthread_mutex_t trx_table_latch;

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

    // save undo_values
    
    if(trx_map[trx_id].head == nullptr){
        trx_map[trx_id].head = lock_obj;
        trx_map[trx_id].tail = lock_obj;
    }
    else if(trx_map[trx_id].tail == lock_obj){
        //do nothing
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

int TRX_Table::release_trx_lock_obj(int trx_id){
   //printf("release sequence start\n");
    int result;
    result = pthread_mutex_lock(&trx_table_latch); 
    if(result != 0) return 0;

    //printf("release trx: %d\n",trx_id);

    // no entry exists
    if(trx_map.find(trx_id)==trx_map.end()){
        pthread_mutex_unlock(&trx_table_latch);
        return 0;
    }
    
    lock_t* cursor = trx_map[trx_id].head;
    queue<pair<char*, uint16_t>> restored_queue = trx_map[trx_id].undo_values;

    trx_table.trx_map.erase(trx_id);

    result = pthread_mutex_unlock(&trx_table_latch);
    if(result != 0) return 0;

    result = pthread_mutex_lock(&lock_table_latch); 
    if(result!=0) return 0;

    while(cursor != nullptr){
        lock_t* next = cursor -> next_lock;
        lock_release(cursor);
        cursor = next;
    }

    result = pthread_mutex_unlock(&lock_table_latch); 
    if(result!=0) return 0;

    while(!restored_queue.empty()){
        auto i = restored_queue.front();
        restored_queue.pop();
        delete i.first;
    }
    
    return trx_id; 
}
int TRX_Table::abort_trx_lock_obj(int trx_id){
    printf("abort sequence start\n");
    int result;
    result = pthread_mutex_lock(&trx_table_latch); 
    if(result != 0) return 0;

    //printf("release trx: %d\n",trx_id);

    // no entry exists
    if(trx_map.find(trx_id)==trx_map.end()){
        pthread_mutex_unlock(&trx_table_latch);
        return 0;
    }
    
    lock_t* cursor = trx_map[trx_id].head;
    queue<pair<char*, uint16_t>> restored_queue = trx_map[trx_id].undo_values;

    trx_table.trx_map.erase(trx_id);

    result = pthread_mutex_unlock(&trx_table_latch);
    if(result != 0) return 0;

    result = pthread_mutex_lock(&lock_table_latch); 
    if(result!=0) return 0;

    while(!restored_queue.empty()){
        auto restored_item =  restored_queue.front();
        restored_queue.pop();

        h_page_t header_node;
        buf_read_page(cursor->sentinel->table_id, 0, (page_t*) &header_node);      

        Node acquired_leaf = find_leaf(cursor->sentinel->table_id, header_node.root_page_number, cursor->record_id);
        uint16_t dummy;
        acquired_leaf.leaf_update(cursor->record_id, restored_item.first, restored_item.second, &dummy);
        acquired_leaf.write_to_disk(); 

        buf_unpin(cursor->sentinel->table_id, acquired_leaf.pn);
        buf_unpin(cursor->sentinel->table_id, 0);

        delete restored_item.first;

    }

    while(cursor != nullptr){
        //printf("record_id: %d\n",cursor->record_id);
        //printf("release\n");
        lock_t* next = cursor -> next_lock;
        lock_release(cursor);
        //printf("after release\n");
        cursor = next;
    }

    result = pthread_mutex_unlock(&lock_table_latch); 
    if(result!=0) return 0;
    
    return trx_id; 
}
int TRX_Table::push_undo_value(int trx_id, char *value, uint16_t old_val_size){
    int result;
    result = pthread_mutex_lock(&trx_table_latch); 
    if(result != 0) return 0;

    if(trx_map.find(trx_id)==trx_map.end()){
        pthread_mutex_unlock(&trx_table_latch);
        return 0;
    }

    trx_map[trx_id].undo_values.push({value, old_val_size});

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

int init_trx() {
    int result = 0;

    pthread_mutexattr_t mtx_attribute;
    result = pthread_mutexattr_init(&mtx_attribute);
    if(result!=0) return -1;
    result = pthread_mutexattr_settype(&mtx_attribute, PTHREAD_MUTEX_NORMAL);
    if(result!=0) return -1;
    result = pthread_mutex_init(&lock_table_latch, &mtx_attribute);
    if(result!=0) return -1;

    result = pthread_mutex_init(&trx_table_latch, &mtx_attribute);
    if(result!=0) return -1;
    return 0;
}
/*
 * LOCK_TABLE_MODULE_BEGIN
 */

int debug_count = 0;

pthread_mutex_t lock_table_latch;
unordered_map<pair<int64_t,pagenum_t>, hash_table_entry, pair_for_hash> hash_table;


bool lock_acquire_deadlock_detection(lock_t* dependency, int trx_id){
    //printf("lock_acquire_deadlock_detection %d\n", trx_id);
    int debug_int = 0;
    //printf("deadlock detection dependency trx_id: %d, dependency record_id %d, compared trx_id: %d\n",dependency->trx_id, dependency->record_id, trx_id);
    while(dependency->next_lock != nullptr){
        //printf("dependency looping: %d\n",debug_int++);
        dependency = dependency -> next_lock;
    }

    if(dependency->trx_id == trx_id){
        //printf("detected\n");
        return true;
    }

    int dependency_key_bit = dependency->sentinel->key_map[dependency->record_id];
    lock_t* cursor = dependency->prev;
    while(cursor != nullptr){
        //printf("cursor looping,finding dependency: %d\n",debug_int++);
        //printf("debug_count lock_acquire %d\n", debug_count++);
        if(cursor->record_id != dependency->record_id && !cursor->key_set.test(dependency_key_bit)){
            cursor = cursor -> prev;
            continue;
        }
        if(dependency->lock_mode==EXCLUSIVE){
            break;
        }
        else if(dependency->lock_mode==SHARED){
            if(cursor->lock_mode == EXCLUSIVE){
                break;
            }     
        }
        cursor = cursor -> prev;
    }
    // now cursor points to the lock that occurs dependency. if it is nullptr there is no dependency.
    if(cursor!=nullptr){
        if(dependency->lock_mode == EXCLUSIVE){
            if(cursor->lock_mode==EXCLUSIVE){
                //deadlock check
                return lock_acquire_deadlock_detection(cursor, trx_id);
            }
            else if(cursor->lock_mode==SHARED){
                //printf("deadlock check\n");
                //deadlock check
                while(cursor!=nullptr){
        //printf("cursor looping: %d\n",debug_int++);
                    if(cursor->record_id != dependency->record_id && !cursor->key_set.test(dependency_key_bit)){
                        cursor = cursor -> prev;
                        continue;
                    }
                    if(cursor->lock_mode != SHARED){
                        break;
                    }
                    if(lock_acquire_deadlock_detection(cursor, trx_id)){
                        return true;
                    }
                    cursor = cursor->prev;             
                }
                //printf("no deadlock");
                return false;
            }
        }
        else if(dependency->lock_mode == SHARED){
            //printf("deadlock check\n");
            //deadlock check
            return lock_acquire_deadlock_detection(cursor, trx_id);
        }

    }
    else {
        //printf("no deadlock");
    }
    //else case/ because of warning changed this way
    return false;
}
void lock_detach(hash_table_entry* target, lock_t* lock_obj){
    if(lock_obj->prev == nullptr && lock_obj->next == nullptr){
        target->head = nullptr;
        target->tail = nullptr;
    }
    else if(lock_obj->prev == nullptr){
        //case lock_obj is leftmost
        target->head = lock_obj->next;
        lock_obj->next->prev = nullptr;
        lock_obj->next = nullptr;
    }
    else if(lock_obj->next == nullptr){
        //case lock_obj is rightmost
        target->tail = lock_obj->prev;
        lock_obj->prev->next = nullptr;
        lock_obj->prev = nullptr;
    }
    else {
        lock_obj->prev->next = lock_obj->next;
        lock_obj->next->prev = lock_obj->prev;
        lock_obj->next = nullptr;
        lock_obj->prev = nullptr;
    }

}

//deadlock or inner error: -1, success: 0
int lock_acquire(int64_t table_id, pagenum_t page_id, int64_t key, int trx_id, int lock_mode, bool* add_undo_value) {
    //printf("trx_id: %d, lock_acquire\n",trx_id);
    int result = 0;

    result = pthread_mutex_lock(&lock_table_latch); 
    if(result!=0) return -1;
    //printf("lock_acquire trx: %d, table_id: %d, page_id: %d, record_id: %d\n", trx_id, table_id, page_id, key);

    if(hash_table.find({table_id, page_id})==hash_table.end()){
        hash_table_entry hte;
        hte.table_id = table_id;
        hte.page_id = page_id;
        hte.head = nullptr;
        hte.tail = nullptr;
        //store all keys in the leaf
        Node leaf(table_id, page_id);
        for(int i = 0; i<leaf.leaf_ptr->number_of_keys; i++){
            slot_t tmp;
            memcpy(&tmp.data, leaf.leaf_ptr->data + i*SLOT_SIZE, sizeof(tmp.data)); 
            hte.key_map.insert({tmp.get_key(), i});
            hte.key_map_reverse.insert({i, tmp.get_key()});
        }   
        buf_unpin(table_id, leaf.pn);

        hash_table.insert({{table_id, page_id}, hte}); 


    }
    hash_table_entry* target = &hash_table[{table_id, page_id}]; 

    
    int key_bit = target->key_map[key]; 

    /*
     * implicit lock sequence start
     */

    //printf("implicit lock sequence start\n");
    lock_t* same_record_lock_obj = target->tail;
    while(same_record_lock_obj != nullptr){
        if(same_record_lock_obj->lock_mode==EXCLUSIVE){
            if(same_record_lock_obj->record_id != key){
                same_record_lock_obj = same_record_lock_obj -> prev;
                continue;
            }
        }
        else if (same_record_lock_obj->lock_mode == SHARED){
            if(same_record_lock_obj->record_id != key && !same_record_lock_obj->key_set.test(key_bit)){
                same_record_lock_obj = same_record_lock_obj -> prev;
                continue;
            }
        }
        break;
    }
    if(same_record_lock_obj == nullptr){
        //printf("dbg1\n");
        h_page_t header_node;
        buf_read_page(table_id, 0, (page_t*) &header_node);      
        buf_unpin(table_id, 0);

        Node leaf = find_leaf(table_id, header_node.root_page_number, key);
        slot_t slot;
        int slotnum;
        int result = leaf.leaf_find_slot_ret(key, &slot, &slotnum);
        if(slot.get_trx()==0){
        //printf("dbg2\n");
            if(lock_mode==SHARED){
                buf_unpin(table_id, leaf.pn);
            }
            else {
                slot.set_trx(trx_id);
                leaf.leaf_set_slot(&slot, slotnum);
                leaf.write_to_disk();
                buf_unpin(table_id, leaf.pn);

                result = pthread_mutex_unlock(&lock_table_latch); 
                if(result!=0) return -1;

                return 0;
            }
        }
        else if(slot.get_trx()==trx_id){
        //printf("dbg3\n");
            buf_unpin(table_id, leaf.pn);

            *add_undo_value = false;

            result = pthread_mutex_unlock(&lock_table_latch); 
            if(result!=0) return -1;

            //need to be modified. need to return lock_obj pointer which is not nullptr
            return 0;
        }
        else {
             
        //printf("dbg4\n");
            int result;
            result = pthread_mutex_lock(&trx_table_latch); 
            if(result != 0) return -1;
            //printf("trx_table_latch acquire\n");

            //printf("slot trx: %d\n", slot.get_trx());
            if(trx_table.trx_map.find(slot.get_trx())!=trx_table.trx_map.end()){
        //printf("dbg5\n");
                lock_t* exp = new lock_t;
                exp->next = nullptr;
                exp->prev = nullptr;
                exp->sentinel = &hash_table[{table_id, page_id}];
                exp->lock_mode = EXCLUSIVE;
                exp->record_id = slot.get_key();
                exp->next_lock = nullptr;
                exp->trx_id = slot.get_trx();
                result = pthread_cond_init(&(exp->cond), NULL);
                if(result!=0) {
                    result = pthread_mutex_unlock(&trx_table_latch);
                    if(result != 0) return -1;
                    result = pthread_mutex_unlock(&lock_table_latch); 
                    if(result!=0) return -1;

                    return -1;
                };

                bool is_immediate;
                is_immediate = (target->head == nullptr);
                if(is_immediate){
                    target->head = exp;
                    target->tail = exp;
                }
                else {
                    target->tail->next = exp;
                    exp->prev = target->tail;
                    target->tail = exp; 
                }

                if(trx_table.trx_map[exp->trx_id].head == nullptr){
                    trx_table.trx_map[exp->trx_id].head = exp;
                    trx_table.trx_map[exp->trx_id].tail = exp;
                }
                else if(trx_table.trx_map[exp->trx_id].tail == exp){
                    //do nothing
                }
                else {
                    //trx_table.trx_map[exp->trx_id].tail->next_lock = exp;
                    //trx_table.trx_map[exp->trx_id].tail = exp;

                    exp->next_lock = trx_table.trx_map[exp->trx_id].head;
                    trx_table.trx_map[exp->trx_id].head = exp;
                    //need to be changed because this just attatch explict lock to the head of trx lock list.
                }
                result = pthread_mutex_unlock(&trx_table_latch);
                if(result != 0) return -1;

                buf_unpin(table_id, leaf.pn);

            //printf("trx_table_latch release\n");

            }
            else {
        //printf("dbg6\n");
                if(lock_mode==SHARED){
        //printf("dbg7\n");
                    buf_unpin(table_id, leaf.pn);

            //printf("trx_table_latch release\n");
                    result = pthread_mutex_unlock(&trx_table_latch);
                    if(result != 0) return -1;
                } 
                else{
        //printf("dbg8\n");
                    slot.set_trx(trx_id);
                    leaf.leaf_set_slot(&slot, slotnum);
                    leaf.write_to_disk();
                    buf_unpin(table_id, leaf.pn);

            //printf("trx_table_latch release\n");
                    result = pthread_mutex_unlock(&trx_table_latch);
                    if(result != 0) return -1;

                    result = pthread_mutex_unlock(&lock_table_latch); 
                    if(result!=0) return -1;
                    //need to be modified. need to return lock_obj pointer which is not nullptr
                    return 0;

                }

            }



        }
        
    }
    //printf("implicit lock sequence end\n");

    /*
     * implicit lock sequence end
     */

    lock_t* lck = new lock_t;
    lck->next = nullptr;
    lck->prev = nullptr;
    lck->sentinel = &hash_table[{table_id, page_id}];
    lck->lock_mode = lock_mode;
    lck->record_id = key;
    lck->next_lock = nullptr;
    lck->trx_id = trx_id;
    lck->key_set.set(lck->sentinel->key_map[key], true);
    //lck->cond = PTHREAD_COND_INITIALIZER;
    result = pthread_cond_init(&(lck->cond), NULL);
    if(result!=0) return -1;

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


    //first check if trx already acquired lock. one or zero same trx lock_obj
    lock_t* same_trx_lock_obj = lck->prev;
    while(same_trx_lock_obj != nullptr){
        //printf("debug_count lock_acquire %d\n", debug_count++);
        if(same_trx_lock_obj->record_id != key && !same_trx_lock_obj->key_set.test(key_bit)){
            same_trx_lock_obj = same_trx_lock_obj -> prev;
            continue;
        }
        if(same_trx_lock_obj->trx_id == lck->trx_id){
            break; 
        }
        same_trx_lock_obj = same_trx_lock_obj -> prev;
    }
    if(same_trx_lock_obj != nullptr){
        if(same_trx_lock_obj->lock_mode == lck->lock_mode){
            lock_detach(target, lck);
            delete lck;
            //delete value;

            result = pthread_mutex_unlock(&lock_table_latch); 
            if(result!=0) return -1;
            
            *add_undo_value = false;

            return 0;
        }
        else {
            if(lck->lock_mode == EXCLUSIVE){
                same_trx_lock_obj->key_set.set(key_bit, false);
                if(same_trx_lock_obj->key_set.none()){
                    lock_detach(target, same_trx_lock_obj); 
                    same_trx_lock_obj->lock_mode = EXCLUSIVE;

                    lock_detach(target, lck);

                    if(target->head == nullptr){
                        target->head = same_trx_lock_obj;
                        target->tail = same_trx_lock_obj;
                        same_trx_lock_obj->next = nullptr;
                        same_trx_lock_obj->prev = nullptr;
                    }
                    else {
                        target->tail->next = same_trx_lock_obj;
                        same_trx_lock_obj->prev = target->tail;
                        target->tail = same_trx_lock_obj; 
                    }

                    delete lck;

                    lck = same_trx_lock_obj;

                    *add_undo_value = true;
                }
                else {

                }
                //do not return for further processing
            }
            else {
                lck->prev->next = nullptr; target->tail = lck->prev; delete lck; 

                result = pthread_mutex_unlock(&lock_table_latch); 
                if(result!=0) return -1;

                *add_undo_value = false;

                return 0;
            }
        }
    }

    if(!is_immediate){
        lock_t* cursor = lck->prev;

        lock_t* shared_same_trx_lock_obj = nullptr;

        while(cursor != nullptr){
            //printf("debug_count lock_acquire %d\n", debug_count++);
            if(cursor->record_id != key && !cursor->key_set.test(key_bit)){
                if(cursor->lock_mode == SHARED && cursor->trx_id == trx_id){
                    shared_same_trx_lock_obj = cursor;
                }
                cursor = cursor -> prev;
                continue;
            }
            
            if(lck->lock_mode==EXCLUSIVE){
                break;
            }
            else if(lck->lock_mode==SHARED){
                if(cursor->lock_mode == EXCLUSIVE){
                    break;
                }     
            }
            cursor = cursor -> prev;
        }


        if(cursor!=nullptr){
            if(lck->lock_mode == EXCLUSIVE){
                if(cursor->lock_mode==EXCLUSIVE){
                    //printf("current: exclusive, traget: exclusive\n");
                    //deadlock check
                    bool is_deadlock = lock_acquire_deadlock_detection(cursor, trx_id);
                    if(is_deadlock){
                        //printf("deadlock!\n");
                        lck->prev->next = nullptr;
                        target->tail = lck->prev;

                        delete lck; 

                        result = pthread_mutex_unlock(&lock_table_latch); 
                        if(result!=0) return -1;

                        return -1;
                    }
                }
                else if(cursor->lock_mode==SHARED){
                    while(cursor!=nullptr){
                        if(cursor->record_id != key && !cursor->key_set.test(key_bit)){
                            cursor = cursor -> prev;
                            continue;
                        }
                        if(cursor->lock_mode != SHARED){
                            break;
                        } 
                        bool is_deadlock = lock_acquire_deadlock_detection(cursor, trx_id);
                        //printf("current: exclusive, traget: shared, %d",is_deadlock);
                        if(is_deadlock){
                            //printf("deadlock!\n");
                            lck->prev->next = nullptr;
                            target->tail = lck->prev;

                            delete lck; 

                            result = pthread_mutex_unlock(&lock_table_latch); 
                            if(result!=0) return -1;

                            return -1;
                        }
                        cursor = cursor->prev;             
                    }
                }
                //trx_table connection
                trx_table.connect_lock_obj(trx_id, lck);

                result = pthread_cond_wait(&lck->cond, &lock_table_latch);
                if(result!=0) return -1;
            }
            else if(lck->lock_mode == SHARED){
                bool is_deadlock = lock_acquire_deadlock_detection(cursor, trx_id);
                //printf("current: shared, traget: exclusive, %d",is_deadlock);
                if(is_deadlock){
                    //printf("deadlock!\n");
                    lck->prev->next = nullptr;
                    target->tail = lck->prev;

                    delete lck; 

                    result = pthread_mutex_unlock(&lock_table_latch); 
                    if(result!=0) return -1;
                    
                    return -1;
                }

                //trx_table connection
                trx_table.connect_lock_obj(trx_id, lck);

                result = pthread_cond_wait(&lck->cond, &lock_table_latch);
                if(result!=0) return -1;
            }
            
        }
        else if(lck->lock_mode == SHARED && shared_same_trx_lock_obj != nullptr){
           //printf("lock compression!\n");
            //no dependency

            /*
             * lock compression
             */

            // in this case, shared_same_trx_lock_obj is acquired lock obj because this is next operation in the same trx // so it is safe to insert

            // detach lck, delete lck, and push key to shared_same_trx_lock_obj
            lock_detach(target, lck);
            delete lck;
            shared_same_trx_lock_obj->key_set.set(key_bit, true);
            
        }
        else {
            //no dependecy
            //trx_table connection
            trx_table.connect_lock_obj(trx_id, lck);
        }
    }
    else {
        //trx_table connection
        trx_table.connect_lock_obj(trx_id, lck);
    }



    result = pthread_mutex_unlock(&lock_table_latch); 
    if(result!=0) return -1;


    return 0;
};

// wrapper function release_trx_lock_obj acquires lock table latch
int lock_release(lock_t* lock_obj) {

    int result = 0;

    //printf("lock_release start\n");
    //printf("lock_release start\n");
    //printf("    lock_release %d, %d, %d\n",lock_obj->sentinel->table_id, lock_obj->sentinel->page_id, lock_obj->record_id);

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

            for(int i = 0; i<lock_obj->sentinel->key_map.size(); i++){
                if(!lock_obj->key_set.test(i)){
                    int slock_count = 0;
                    bool is_xlock_on_right_side = false;
                    lock_t* xlock_cursor = lock_obj->next;

                    int64_t key_target = lock_obj->sentinel->key_map_reverse[i];
                    //no need to check slock
                    while(xlock_cursor != nullptr){
                        //printf("debug_count on shared%d\n", debug_count++);
                        if(xlock_cursor->record_id != key_target && !xlock_cursor->key_set.test(i)) {
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

                    if(is_xlock_on_right_side == true){
                        //printf("rightside xlock case\n");
                        lock_t* cursor = lock_obj->prev;
                        while(cursor != nullptr){
                            //this search shared slock from other trx
                            if(cursor->record_id != key_target && !cursor->key_set.test(i)){
                                cursor = cursor->prev;
                                continue;
                            }
                            //this is not needed because release is occured in lock acquired status
                            if(cursor->lock_mode == EXCLUSIVE){
                                break;
                            }
                            slock_count += 1;
                            cursor = cursor->prev;
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

            }
            lock_detach(target, lock_obj);


        }
        else if(lock_obj->lock_mode == EXCLUSIVE){
            lock_t* cursor = lock_obj->next;
            bool is_slock_acquired = false;
            while(cursor!=nullptr){
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
            
            lock_detach(target, lock_obj); 
        }
    }

    delete lock_obj;

    //printf("lock_release end\n");

    return 0;
}

/*
 * LOCK_TABLE_MODULE_END
 */


/*
 * TRX_API_BEGIN
 */

// 0: success, non-zero: failed
int db_find(int64_t table_id, int64_t key, char* ret_val, uint16_t* val_size, int trx_id){
    h_page_t header_node;
    buf_read_page(table_id, 0, (page_t*) &header_node);      
    buf_unpin(table_id, 0);

    if(header_node.root_page_number==0){
        return 1;
    }
    Node leaf = find_leaf(table_id, header_node.root_page_number, key);
    int result = leaf.leaf_find_slot(key);

    if(result == -1){
        buf_unpin(table_id, leaf.pn);
        return 1;
    }
    buf_unpin(table_id, leaf.pn);

    bool dummy;
    int res = lock_acquire(table_id, leaf.pn, key, trx_id, SHARED, &dummy);        
    if(res==-1){
        //printf("abort!");
        trx_table.abort_trx_lock_obj(trx_id);
        return -1;
    }

    buf_read_page(table_id, 0, (page_t*) &header_node);      
    buf_unpin(table_id, 0);

    Node acquired_leaf = find_leaf(table_id, header_node.root_page_number, key);
    acquired_leaf.leaf_find(key, ret_val, val_size);
    buf_unpin(table_id, acquired_leaf.pn);

    return 0;
}

// 0: success: non-zero: failed
int db_update(int64_t table_id, int64_t key, char* value, uint16_t new_val_size, uint16_t* old_val_size, int trx_id){
    //printf("trx_id: %d, db_update\n",trx_id);
    h_page_t header_node;
    buf_read_page(table_id, 0, (page_t*) &header_node);      
    buf_unpin(table_id, 0);
    if(header_node.root_page_number==0){
        return 1;
    }
    //printf("update db_update dbg\n");
    Node leaf = find_leaf(table_id, header_node.root_page_number, key);
    int result = leaf.leaf_find_slot(key);

    if(result == -1){
        buf_unpin(table_id, leaf.pn);
        return 1;
    }
    buf_unpin(table_id, leaf.pn);

    bool add_undo_value = true;
    int res = lock_acquire(table_id, leaf.pn, key, trx_id, EXCLUSIVE, &add_undo_value);        
    if(res==-1){
        //printf("abort! %d\n",trx_id);
        trx_table.abort_trx_lock_obj(trx_id);
        return -1;
    }

    buf_read_page(table_id, 0, (page_t*) &header_node);      
    buf_unpin(table_id, 0);

    Node acquired_leaf = find_leaf(table_id, header_node.root_page_number, key);
    if(add_undo_value){
        char* ret_val  = new char[130];
        result = leaf.leaf_find(key, ret_val, old_val_size);
        trx_table.push_undo_value(trx_id, ret_val, *old_val_size);
    }
    
    acquired_leaf.leaf_update(key, value, new_val_size, old_val_size);
    acquired_leaf.write_to_disk(); 

    buf_unpin(table_id, acquired_leaf.pn);

    return 0;
}

/*
 * TRX_API_END
 */

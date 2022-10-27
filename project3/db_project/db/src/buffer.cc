#include "buffer.h"

int file_io = 0;
int cache_hit = 0;
int eviction_write = 0;
int alloc_io = 0;
int read_page_io = 0;
int read_cache_hit = 0;
int eviction_with_write = 0;
LRU_Buffer Buffer;

buf_key_t tidpn_to_key(pair<int64_t, pagenum_t> tidpn){
    return tidpn.second*100 + (uint64_t)tidpn.first; 
}
pair<int64_t, pagenum_t> key_to_tidpn(buf_key_t key){
    return {key%100, key/100};
}


buf_block_t* alloc_buf_block_t(bool create_frame, int64_t tid, pagenum_t pn){
    buf_block_t* ctrl_block = (buf_block_t*)malloc(sizeof(buf_block_t));
    if(create_frame){
        ctrl_block->frame = (frame_t*)malloc(sizeof(frame_t)); 
        ctrl_block->is_dirty = 0;
        ctrl_block->is_pinned = 0;
    } 
    else {
        ctrl_block->frame = nullptr;
        //-1 means not using this value
        ctrl_block->is_dirty = -1;
        ctrl_block->is_pinned = -1;
    }
    ctrl_block->table_id = tid;
    ctrl_block->pagenum = pn;
    return ctrl_block;
}
void clear_buf_block_t(buf_block_t* buf_block){
    buf_block->table_id = -1;
    buf_block->pagenum = -1;
    buf_block->is_pinned = 0;
    buf_block->is_dirty = 0; 
}

LRU_Buffer::LRU_Buffer(){
    frame_in_use = 0;
    //buf_block of tid -2 is head and tail.
    head = alloc_buf_block_t(false, -2, 0);
    tail = alloc_buf_block_t(false, -2, 0);
    head->next = tail;
    tail->prev = head;
}

void LRU_Buffer::add_frame_front(buf_block_t* target){
    buf_block_t* temp = head->next;
    target->next = head->next;
    head->next = target;
    target->prev = temp->prev;
    temp->prev = target;
}
void LRU_Buffer::remove_frame(buf_block_t* target){
    target->prev->next = target->next;  
    target->next->prev = target->prev;
}
void buf_print(){
    Buffer.print_buf_block();
}

int init_buffer(int num_buf){
    Buffer = LRU_Buffer();
    Buffer.frame_total = num_buf;
    Buffer.print_buf_block();
    return 0;
}

int buf_read_page(int64_t table_id, pagenum_t pagenum, struct page_t* dest){
    //cache hit!
    if(Buffer.page_buf_block_map.find(tidpn_to_key({table_id, pagenum}))!=Buffer.page_buf_block_map.end()){
        //printf("Cache hit\n");
        cache_hit += 1;
        read_cache_hit += 1;
        memcpy(dest, Buffer.page_buf_block_map[tidpn_to_key({table_id,pagenum})]->frame, PAGE_SIZE);
        Buffer.page_buf_block_map[tidpn_to_key({table_id, pagenum})]->is_pinned += 1;
        buf_block_t* target = Buffer.page_buf_block_map[tidpn_to_key({table_id, pagenum})];
        Buffer.remove_frame(target);
        Buffer.add_frame_front(target);
        return 0;      
    }
    
    //Buffer is not full 
    if(Buffer.frame_total > Buffer.frame_in_use){
        //printf("Buf control block not full, so insert\n");

        // new buf_block initialize
        buf_block_t* new_buf_block = alloc_buf_block_t(true, table_id, pagenum); 
        new_buf_block->is_pinned += 1;
        file_read_page(table_id, pagenum, (page_t*)new_buf_block->frame);
        file_io+=1;
        read_page_io+=1;

        // return cached page
        memcpy(dest, new_buf_block->frame, PAGE_SIZE);

        // update Buffer 
        Buffer.add_frame_front(new_buf_block); 
        Buffer.frame_in_use++;
        Buffer.page_buf_block_map.insert({tidpn_to_key({table_id, pagenum}),new_buf_block});

        return 0;
    } 
    if(Buffer.frame_total < Buffer.frame_in_use){
        //printf("Must not be occured case occured!\n");
        return -1;
    } 

    /*
     * eviction phase
     */
    //printf("Buf Eviction\n");

    // Buffer is full, evict the victim page, traversing from tail.
    buf_block_t* cur = Buffer.tail->prev; 
    // header frame has table id of -2
    // select victim 
    while(cur->table_id!=-2){
        if(cur->is_pinned == 0){
            break; 
        }
        cur = cur->prev;
    }
    /*
    if(cur->table_id==-2){
        cur = Buffer.tail->prev; 
        while(cur->table_id!=-2){
            if(cur->is_pinned == 0){
                break; 
            }
            cur = cur->prev;
        }
        
    }
    */
    // cannot find victim
    if(cur->table_id==-2){
        //printf("ERROR: BUFFER is full and all frames are in use\n");
        return -1;
    }
    // evict the victim and load page from disk 
    if(cur->is_dirty==1){
        file_write_page(cur->table_id, cur->pagenum, (page_t*)cur->frame);
        file_io+=1;
        read_page_io+=1;
        eviction_write += 1;
        cur->is_dirty=0;
    } 
    Buffer.remove_frame(cur);
    //printf("find: %d\n", cur->pagenum);

    cur->is_pinned += 1;
    file_read_page(table_id, pagenum, (page_t*)cur->frame);
        file_io+=1;
        read_page_io+=1;
    Buffer.page_buf_block_map.erase(tidpn_to_key({cur->table_id, cur->pagenum}));
    cur->table_id = table_id;
    cur->pagenum = pagenum;
    Buffer.page_buf_block_map.insert({tidpn_to_key({cur->table_id, cur->pagenum}), cur});

    // return cached page
    memcpy(dest, cur->frame, PAGE_SIZE);

    // update Buffer
    Buffer.add_frame_front(cur);
    
    //printf("Buf Eviction END!!\n");
    return 0;
}
int alloc_frame(int64_t table_id, pagenum_t pagenum){
    //cache hit!
    if(Buffer.page_buf_block_map.find(tidpn_to_key({table_id, pagenum}))!=Buffer.page_buf_block_map.end()){
        printf("ERROR: should never be the case!\n");
        return -1;
    }
    
    //Buffer is not full 
    if(Buffer.frame_total > Buffer.frame_in_use){
        //printf("Buf control block not full, so insert\n");

        // new buf_block initialize
        buf_block_t* new_buf_block = alloc_buf_block_t(true, table_id, pagenum); 
        new_buf_block->is_pinned += 1;

        // update Buffer 
        Buffer.add_frame_front(new_buf_block); 
        Buffer.frame_in_use++;
        Buffer.page_buf_block_map.insert({tidpn_to_key({table_id, pagenum}),new_buf_block});

        return 0;
    } 
    if(Buffer.frame_total < Buffer.frame_in_use){
        printf("Must not be occured case occured!\n");
        return -1;
    } 

    /*
     * eviction phase
     */
    //printf("Buf Eviction\n");

    // Buffer is full, evict the victim page, traversing from tail.
    buf_block_t* cur = Buffer.tail->prev; 
    // header frame has table id of -2
    // select victim 
    while(cur->table_id!=-2){
        if(cur->is_pinned == 0){
            break; 
        }
        cur = cur->prev;
    }
    /*
    if(cur->table_id==-2){
        cur = Buffer.tail->prev; 
        while(cur->table_id!=-2){
            if(cur->is_pinned == 0){
                break; 
            }
            cur = cur->prev;
        }
        
    }
    */
    // cannot find victim
    if(cur->table_id==-2){
        //printf("ERROR: BUFFER is full and all frames are in use\n");
        return -1;
    }
    // evict the victim and load page from disk 
    if(cur->is_dirty==1){
        file_write_page(cur->table_id, cur->pagenum, (page_t*)cur->frame);
        file_io+=1;
        eviction_write += 1;
        cur->is_dirty=0;
    } 
    Buffer.remove_frame(cur);

    cur->is_pinned += 1;
    Buffer.page_buf_block_map.erase(tidpn_to_key({cur->table_id, cur->pagenum}));
    cur->table_id = table_id;
    cur->pagenum = pagenum;
    Buffer.page_buf_block_map.insert({tidpn_to_key({cur->table_id, cur->pagenum}), cur});

    // update Buffer
    Buffer.add_frame_front(cur);
    
    return 0;
}

/*
//allocate resource
pagenum_t buf_alloc_page(int64_t table_id){
    int cost = 0;
    if(Buffer.page_buf_block_map.find({table_id, 0})!=Buffer.page_buf_block_map.end() && Buffer.page_buf_block_map[{table_id, 0}]->is_dirty == 1){
        //? pin unpin think
        file_write_page(table_id, 0, (page_t*) Buffer.page_buf_block_map[{table_id, 0}]->frame);
        file_io+=1;
        cost+=1;
        Buffer.page_buf_block_map[{table_id, 0}]->is_dirty = 0;
    }
    pagenum_t allocated_pagenum = file_alloc_page(table_id); 
        file_io+=3;
        cost+=3;
    if(Buffer.page_buf_block_map.find({table_id, 0})!=Buffer.page_buf_block_map.end()){
        //? pin unpin think
        file_read_page(table_id, 0, (page_t*) Buffer.page_buf_block_map[{table_id, 0}]->frame);
        file_io+=1;
        cost+=1;
    }
    
    //printf("allocated_pagenum: %d\n",allocated_pagenum);
    int result = alloc_frame(table_id, allocated_pagenum);
    if(result == -1) {
        printf("alloc page failed\n");
        file_free_page(table_id, allocated_pagenum); 
        return -1;
    }
    alloc_io += cost;
    return allocated_pagenum;
}
*/
pagenum_t buf_alloc_page(int64_t table_id){
    int cost = 0;
    pagenum_t allocated_pagenum;

    if(Buffer.page_buf_block_map.find(tidpn_to_key({table_id, 0}))!=Buffer.page_buf_block_map.end()){
        h_page_t* target = (h_page_t*)(Buffer.page_buf_block_map[tidpn_to_key({table_id, 0})]->frame);
        if(target->free_page_number==0){
            if(Buffer.page_buf_block_map[tidpn_to_key({table_id, 0})]->is_dirty == 1){
                //? pin unpin think
                file_write_page(table_id, 0, (page_t*) Buffer.page_buf_block_map[tidpn_to_key({table_id, 0})]->frame);
                file_io+=1;
                Buffer.page_buf_block_map[tidpn_to_key({table_id, 0})]->is_dirty = 0;
            }
            allocated_pagenum = file_alloc_page(table_id); 
            file_io+=3;
            file_read_page(table_id, 0, (page_t*) Buffer.page_buf_block_map[tidpn_to_key({table_id, 0})]->frame);
            file_io+=1;
        }
        else {
            f_page_t free_page_buf;
            allocated_pagenum = target->free_page_number;
            pread(fd_mapper(table_id), &free_page_buf, PAGE_SIZE, allocated_pagenum * PAGE_SIZE);
            file_io+=1;

            target->free_page_number = free_page_buf.next_free_page_number; 
            Buffer.page_buf_block_map[tidpn_to_key({table_id, 0})]->is_dirty = 1;
        }
    } else {
        allocated_pagenum = file_alloc_page(table_id);
    }
    int result = alloc_frame(table_id, allocated_pagenum);
    if(result == -1) {
        printf("alloc page failed\n");
        file_free_page(table_id, allocated_pagenum); 
        return -1;
    }
    return allocated_pagenum;
}

// think about pin count!! // this should be called only when the resource is allocated
// think about synchronization, this function should be pending if other is using page.
int buf_write_page(int64_t table_id, pagenum_t pagenum, const struct page_t* src){
    if(Buffer.page_buf_block_map.find(tidpn_to_key({table_id, pagenum}))!=Buffer.page_buf_block_map.end()){
        //printf("Cache hit should be occured");
        memcpy(Buffer.page_buf_block_map[tidpn_to_key({table_id,pagenum})]->frame, src, PAGE_SIZE);
        Buffer.page_buf_block_map[tidpn_to_key({table_id, pagenum})]->is_dirty = 1;
        return 0;      
    }
    else {
        printf("This should not be the case!\n");
        return -1;
    }
    /*
    // If not in Buffer, read
    if(alloc_frame(table_id, pagenum)==-1) return -1;
    memcpy(Buffer.page_buf_block_map[{table_id, pagenum}]->frame, src, PAGE_SIZE);
    Buffer.page_buf_block_map[{table_id, pagenum}]->is_dirty = 1;
    buf_unpin(table_id, pagenum);
    return 0;
    */
}
/*
void buf_free_page(int64_t table_id, pagenum_t pagenum){
    if(Buffer.page_buf_block_map.find({table_id, pagenum})!=Buffer.page_buf_block_map.end()){
        free(Buffer.page_buf_block_map[{table_id, pagenum}]->frame);
        Buffer.remove_frame(Buffer.page_buf_block_map[{table_id, pagenum}]);
        free(Buffer.page_buf_block_map[{table_id, pagenum}]);

        Buffer.page_buf_block_map.erase({table_id, pagenum});
    }
    file_free_page(table_id, pagenum);
}
*/

// if the target page is on cache, delete it. if not, just call file_free_page
// this should be called when the resource is allocated
void buf_free_page(int64_t table_id, pagenum_t pagenum){
    if(Buffer.page_buf_block_map.find(tidpn_to_key({table_id, 0}))!=Buffer.page_buf_block_map.end()){
        f_page_t free_page_buf;
        h_page_t* target = (h_page_t*)(Buffer.page_buf_block_map[tidpn_to_key({table_id, 0})]->frame);
        pagenum_t temp = target->free_page_number;
        target->free_page_number = pagenum;
        Buffer.page_buf_block_map[tidpn_to_key({table_id, 0})]->is_dirty = 1;

        free_page_buf.next_free_page_number = temp;  
        file_write_page(table_id, pagenum, (page_t*)&free_page_buf);
    }
    else {
        file_free_page(table_id, pagenum);
    }
    free(Buffer.page_buf_block_map[tidpn_to_key({table_id, pagenum})]->frame);
    Buffer.remove_frame(Buffer.page_buf_block_map[tidpn_to_key({table_id, pagenum})]);
    Buffer.frame_in_use -= 1;
    free(Buffer.page_buf_block_map[tidpn_to_key({table_id, pagenum})]);

    Buffer.page_buf_block_map.erase(tidpn_to_key({table_id, pagenum}));
}

int buf_unpin(int64_t table_id, pagenum_t pagenum){
    if(Buffer.page_buf_block_map.find(tidpn_to_key({table_id, pagenum}))!=Buffer.page_buf_block_map.end()){
        Buffer.page_buf_block_map[tidpn_to_key({table_id, pagenum})]->is_pinned -= 1;
        return 0;
    }
    //printf("unpin failed\n");
    return -1;
}

void fini_buffer(){
    printf("fini_buffer, file_io: %d, cache hit: %d, eviction: %d, alloc_io: %d, read_page_io: %d, read_cache_hit: %d\n",file_io,cache_hit,eviction_write, alloc_io, read_page_io, read_cache_hit);
    buf_block_t* cur = Buffer.head->next;
    while(cur != Buffer.tail){
        if(cur->is_dirty==1){
            file_write_page(cur->table_id, cur->pagenum, (page_t*)cur->frame);
        file_io+=1;
            cur->is_dirty=0;
        }  
        //printf("freeing : ctrl_block | tid: %ld, pn: %lu, pin_count: %d, is_dirty: %d, frame_ptr: %p\n", cur->table_id, cur->pagenum, cur->is_pinned, cur->is_dirty, cur->frame);
        buf_block_t* nextptr = cur->next;
        free(cur->frame);
        free(cur);
        cur = nextptr;
    }
    free(Buffer.head);
    free(Buffer.tail);
}

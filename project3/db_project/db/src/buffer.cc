#include "buffer.h"

LRU_Buffer Buffer;

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
    if(Buffer.page_buf_block_map.find({table_id, pagenum})!=Buffer.page_buf_block_map.end()){
        //printf("Cache hit\n");
        memcpy(dest, Buffer.page_buf_block_map[{table_id,pagenum}]->frame, PAGE_SIZE);
        Buffer.page_buf_block_map[{table_id, pagenum}]->is_pinned += 1;
        buf_block_t* target = Buffer.page_buf_block_map[{table_id, pagenum}];
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

        // return cached page
        memcpy(dest, new_buf_block->frame, PAGE_SIZE);

        // update Buffer 
        Buffer.add_frame_front(new_buf_block); 
        Buffer.frame_in_use++;
        Buffer.page_buf_block_map.insert({{table_id, pagenum},new_buf_block});

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
    // cannot find victim
    if(cur->table_id==-2){
        //printf("ERROR: BUFFER is full and all frames are in use\n");
        return -1;
    }
    // evict the victim and load page from disk 
    if(cur->is_dirty==1){
        file_write_page(cur->table_id, cur->pagenum, (page_t*)cur->frame);
        cur->is_dirty=0;
    } 
    Buffer.remove_frame(cur);

    cur->is_pinned += 1;
    file_read_page(table_id, pagenum, (page_t*)cur->frame);
    Buffer.page_buf_block_map.erase({cur->table_id, cur->pagenum});
    cur->table_id = table_id;
    cur->pagenum = pagenum;
    Buffer.page_buf_block_map.insert({{cur->table_id, cur->pagenum}, cur});

    // return cached page
    memcpy(dest, cur->frame, PAGE_SIZE);

    // update Buffer
    Buffer.add_frame_front(cur);
    
    return 0;
}
int alloc_frame(int64_t table_id, pagenum_t pagenum){
    //cache hit!
    if(Buffer.page_buf_block_map.find({table_id, pagenum})!=Buffer.page_buf_block_map.end()){
        //printf("ERROR: should never be the case!\n");
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
        Buffer.page_buf_block_map.insert({{table_id, pagenum},new_buf_block});

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
    // cannot find victim
    if(cur->table_id==-2){
        //printf("ERROR: BUFFER is full and all frames are in use\n");
        return -1;
    }
    // evict the victim and load page from disk 
    if(cur->is_dirty==1){
        file_write_page(cur->table_id, cur->pagenum, (page_t*)cur->frame);
        cur->is_dirty=0;
    } 
    Buffer.remove_frame(cur);

    cur->is_pinned += 1;
    Buffer.page_buf_block_map.erase({cur->table_id, cur->pagenum});
    cur->table_id = table_id;
    cur->pagenum = pagenum;
    Buffer.page_buf_block_map.insert({{cur->table_id, cur->pagenum}, cur});

    // update Buffer
    Buffer.add_frame_front(cur);
    
    return 0;
}

pagenum_t buf_alloc_page(int64_t table_id){
    if(Buffer.page_buf_block_map.find({table_id, 0})!=Buffer.page_buf_block_map.end() && Buffer.page_buf_block_map[{table_id, 0}]->is_dirty == 1){
        //? pin unpin think
        file_write_page(table_id, 0, (page_t*) Buffer.page_buf_block_map[{table_id, 0}]->frame);
        Buffer.page_buf_block_map[{table_id, 0}]->is_dirty = 0;
    }
    pagenum_t allocated_pagenum = file_alloc_page(table_id); 
    if(Buffer.page_buf_block_map.find({table_id, 0})!=Buffer.page_buf_block_map.end()){
        //? pin unpin think
        file_read_page(table_id, 0, (page_t*) Buffer.page_buf_block_map[{table_id, 0}]->frame);
    }
    //printf("allocated_pagenum: %d\n",allocated_pagenum);
    int result = alloc_frame(table_id, allocated_pagenum);
    if(result == -1) {
        file_free_page(table_id, allocated_pagenum); 
        return -1;
    }
    return allocated_pagenum;
}

// think about pin count!!
int buf_write_page(int64_t table_id, pagenum_t pagenum, const struct page_t* src){
    if(Buffer.page_buf_block_map.find({table_id, pagenum})!=Buffer.page_buf_block_map.end()){
        //printf("Cache hit from buf_write_page\n");
        memcpy(Buffer.page_buf_block_map[{table_id,pagenum}]->frame, src, PAGE_SIZE);
        Buffer.page_buf_block_map[{table_id, pagenum}]->is_dirty = 1;
        //Buffer.page_buf_block_map[{table_id, pagenum}]->is_pinned += 1;
        return 0;      
    }
    // If not in Buffer, read
    if(alloc_frame(table_id, pagenum)==-1) return -1;
    memcpy(Buffer.page_buf_block_map[{table_id, pagenum}]->frame, src, PAGE_SIZE);
    Buffer.page_buf_block_map[{table_id, pagenum}]->is_dirty = 1;
    buf_unpin(table_id, pagenum);
    return 0;
}

// if the target page is on cache, delete it. if not, just call file_free_page
void buf_free_page(int64_t table_id, pagenum_t pagenum){
    if(Buffer.page_buf_block_map.find({table_id, pagenum})!=Buffer.page_buf_block_map.end()){
        free(Buffer.page_buf_block_map[{table_id, pagenum}]);
        Buffer.remove_frame(Buffer.page_buf_block_map[{table_id, pagenum}]);
        Buffer.page_buf_block_map.erase({table_id, pagenum});
    }
    file_free_page(table_id, pagenum);
}

int buf_unpin(int64_t table_id, pagenum_t pagenum){
    if(Buffer.page_buf_block_map.find({table_id, pagenum})!=Buffer.page_buf_block_map.end()){
        Buffer.page_buf_block_map[{table_id, pagenum}]->is_pinned -= 1;
        return 0;
    }
    //printf("unpin failed\n");
    return -1;
}

void fini_buffer(){
    printf("fini_buffer\n");
    buf_block_t* cur = Buffer.head->next;
    while(cur != Buffer.tail){
        if(cur->is_dirty==1){
            file_write_page(cur->table_id, cur->pagenum, (page_t*)cur->frame);
            cur->is_dirty=0;
        }  
        printf("freeing : ctrl_block | tid: %ld, pn: %lu, pin_count: %d, is_dirty: %d, frame_ptr: %p\n", cur->table_id, cur->pagenum, cur->is_pinned, cur->is_dirty, cur->frame);
        buf_block_t* nextptr = cur->next;
        free(cur->frame);
        free(cur);
        cur = nextptr;
    }
    free(Buffer.head);
    free(Buffer.tail);
}

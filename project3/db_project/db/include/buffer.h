#ifndef BUFFER_H_
#define BUFFER_H_
#include <stdint.h>
#include "page.h"
#include "file.h"
#include <unordered_map>

struct frame_t{
    uint8_t data[PAGE_SIZE];
};
struct buf_block_t{
    int64_t table_id;
    pagenum_t pagenum; 
    int is_dirty;
    int is_pinned;
    frame_t* frame;  
    buf_block_t* next;
    buf_block_t* prev;
};
buf_block_t* alloc_buf_block_t(bool creat_frame, int64_t tid, pagenum_t pn);
void clear_buf_block_t(buf_block_t* buf_block);

class LRU_Buffer {
public:
    int frame_total;
    int frame_in_use; 
    buf_block_t* head; 
    buf_block_t* tail;
    map<pair<int64_t, pagenum_t>, buf_block_t*> page_buf_block_map;

    LRU_Buffer();
    
    void add_frame_front(buf_block_t* target);
    void remove_frame(buf_block_t* target);

    void print_buf_block(){
        buf_block_t* cur = head->next;
        while(cur != tail){
            printf("ctrl_block | tid: %ld, pn: %lu, pin_count: %d, is_dirty: %d, frame_ptr: %p\n", cur->table_id, cur->pagenum, cur->is_pinned, cur->is_dirty, cur->frame);
            cur = cur->next;
        }
    }
};

extern int file_io;
extern int cache_hit;
extern LRU_Buffer Buffer;

void buf_print();
int init_buffer(int num_buf);

//0: success, -1: failed
int buf_read_page(int64_t table_id, pagenum_t pagenum, struct page_t* dest);
pagenum_t buf_alloc_page(int64_t table_id);
int buf_write_page(int64_t table_id, pagenum_t pagenum, const struct page_t* src);
void buf_free_page(int64_t table_id, pagenum_t pagenum);
//0: success, -1: {table_id, pagenum} not in the buffer.
int buf_unpin(int64_t table_id, pagenum_t pagenum);

void fini_buffer();

#endif

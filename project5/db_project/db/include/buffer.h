#ifndef BUFFER_H_
#define BUFFER_H_
#include <stdint.h>
#include <pthread.h>
#include "page.h"
#include "file.h"
#include <unordered_map>

typedef uint64_t buf_key_t;

struct frame_t{
    uint8_t data[PAGE_SIZE];
};
struct buf_block_t{
    int64_t table_id;
    pagenum_t pagenum; 
    int is_dirty;
    //int is_pinned;
    pthread_rwlock_t page_latch;
    frame_t* frame;  
    buf_block_t* next;
    buf_block_t* prev;
};
buf_block_t* alloc_buf_block_t(bool create_frame, int64_t tid, pagenum_t pn);
void clear_buf_block_t(buf_block_t* buf_block);

extern pthread_mutex_t buffer_latch;    
class LRU_Buffer {
public:
    int frame_total;
    int frame_in_use; 
    buf_block_t* head; 
    buf_block_t* tail;
    unordered_map<buf_key_t, buf_block_t*> page_buf_block_map;

    LRU_Buffer();
    
    void add_frame_front(buf_block_t* target);
    void remove_frame(buf_block_t* target);

    void print_buf_block();
};

extern int file_io;
extern int cache_hit;
extern LRU_Buffer Buffer;

void buf_print();
int init_buffer(int num_buf);

int64_t buf_open_table_file(const char* pathname);

//0: success, -1: failed
int buf_read_page(int64_t table_id, pagenum_t pagenum, struct page_t* dest);
pagenum_t buf_alloc_page(int64_t table_id, bool isLeaf);
int buf_write_page(int64_t table_id, pagenum_t pagenum, const struct page_t* src);
void buf_free_page(int64_t table_id, pagenum_t pagenum);
//0: success, -1: {table_id, pagenum} not in the buffer.
int buf_unpin(int64_t table_id, pagenum_t pagenum);

void fini_buffer();

#endif

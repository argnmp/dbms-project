# Project3
## Introduction
This **Project3** wiki page describes the design and implementation of buffer manager.
## Design
### Overall Design
The buffer manager in this project consists of `LRU_Buffer` class which connects all buffer control blocks and serveral **apis** that utilizes the linked list of buffer control blocks. In the instance of `LRU_Buffer` class, all the buffer control blocks, which are represented as `buf_block_t` struct, are connected in a **doubly linked list** method. The buffer frame is dynamically allocated and its pointer is stored in buffer control block, which is `buf_block_t`.

### Minimum buffer size
**The minimum buffer size of this design is 3**. During the redistribution procedure in `db_delete` operation, 3 buffer frames corresponding to 3 nodes(parent node, neighbor node, target node) are pinned. Therefore, at least 3 buffer frames are needed. So in this design, I pre-allocated 5 buffer frames.

### `struct buf_block_t`
This struct represents buffer control block which is needed to define and manage dynamically allocated buffer frames.

```cpp
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
```

- `struct frame_t` is needed for pointing dynamically allocated buffer frames.
- `int64_t table_id` and `pagenum_t pagenum` indicate which page of a table is currently managed by the buffer control block.
- `int is_dirty` represents whether the buffer frame is modified or not. 
- `int is_pinned` represents the number of workers currently using this buffer frame. 
- `frame_t* frame` points the **dynamically allocated memory space** which is used as **buffer frame**. 
- `buf_block_t* next` and `buf_block_t* prev` are used to link all the other buffer control blocks in a **doubly linked list** manner.

buffer frame(`frame_t* frame`) is dynamically allocated, when the buffer block(`buf_block_t`) is dynamically allocated and linked to the **buffer control block doubly linked list**. 

### `class LRU_Buffer`
The instance from `LRU_Buffer` class connects all buffer control blocks in a doubly-linked-list method, and is involved in inserting and deleting the buffer control blocks. 

The buffer control block(`buf_block_t`) is dynamically allocated in memory, and also the buffer frame which `frame_t* frame` points to in buffer control block(`buf_block_t`) is dynamically allocated in memory.

Because all buffer control blocks are connected as doubly linked list method, searching a certain buffer control block is done sequentially from the front to the end, which has time complexity of O(n) in average. To avoid this, **unordered_map** data structure is used to find the target buffer control block. **unordered_map** uses hash function internally so it takes O(1) time.

```cpp
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
```
- `int frame_total` is used to store the **buffer size** that is given from `init_db` api. 
- `int frame_in_use` is used to store the number of currently connected buffer control blocks. `frame_in_use` increases by 1 each time new buffer control block is dynamically allocated until it reaches frame_in_use. After that, **buffer frame eviction** occurs when it is needed to read page frame from file. 
- `buf_block_t* head` and `buf_block_t* tail` are used for pointing the start and the end of doubly-linked buffer control block list.
- `unordered_map<buf_key_t, buf_block_t*> page_buf_block_map` is used to find and access buffer control block in O(1) time. This can be done using in pair of **table id and page number**. But in c++, it is impossible to set the key of `unordered_map` as pair. There are several other methods to use pair as key in `unordered_map`, but they have a lot of hash collisions or use exteranl libraries. So I decided to combine **table id** and **page number** in one number. In the specification of **project2**, the max number of opened table is 20. **In the file manager layer(file.h), I defined table id for these 20 tables as 0~19, and mapped them to each file descriptors of opened table file.** So it is possible to set the key of `unordered_map` as **((page number)*100 + (table id))**, and it is guaranteed that all representation of the combination of table id and page num is unique. 

- `add_frame_front` function inserts the buffer control block(target) to the front (near to `head`) in the doubly-linked list.
- `remove_frame` detaches the buffer control block(target)

### Diagram

![diagram](https://user-images.githubusercontent.com/52111798/226518582-c4c5910f-fc05-43c0-8a5e-210d76f847a6.png)

## Cases 
There are 7 apis to replace the function of file manager (file.h from project1).
```cpp
int init_buffer(int num_buf);
int buf_read_page(int64_t table_id, pagenum_t pagenum, struct page_t* dest);
pagenum_t buf_alloc_page(int64_t table_id);
int buf_write_page(int64_t table_id, pagenum_t pagenum, const struct page_t* src);
void buf_free_page(int64_t table_id, pagenum_t pagenum);
int buf_unpin(int64_t table_id, pagenum_t pagenum);
void fini_buffer();
```

### pin and unpin
Every time a page is read or new page is allocated from upper layer(index manager), buffer manager set a pin to the buffer control block(increase the pin) and returns the buffer frame data. Because the buffer control block is pinned, now the upper layer (index manager) takes the resource of the buffer frame. So it should return the resource by calling `int buf_unpin(int64_t table_id, pagenum_t pagenum)`. If that api is called, buffer manager decrease the pin of buffer control block.

### read page using buffer
- `int buf_read_page(int64_t table_id, pagenum_t pagenum, struct page_t* dest);`
- Using the instance of `LRU_Buffer`, buffer manager first checks the requested page is in the buffer. If it is in the buffer(cache hit), set pin to the buffer control block, move it to the front(near to head) and return buffer frame. If it is not in the buffer, create buffer control block with buffer frame(frame_in_use < frame_total) to the front or select victim. Victim selection is done according to LRU policy. If the victim is selected, eviction occurs and if `is_dirty` is set, write to disk. After eviction, target buffer control block is moved to front. Then read page from file using file manager api and store in the buffer frame. After that, set pin and return the page.

### allocate page using buffer
- `pagenum_t buf_alloc_page(int64_t table_id);`
- With buffer implemented, header page of a table can exist in the buffer and can be in the modified state(not written to disk yet). The existing `file_alloc_page` api can't be used in this case, because it directly reads the header page from disk(file). So the existing api cannot be used immediately. In the modified version, it eventually creates an entry for newly allocated page at the front of buffer control block list and set pin to it.


### write to page using buffer
- `int buf_write_page(int64_t table_id, pagenum_t pagenum, const struct page_t* src);`
- In this design, writing to a page using buffer should always be occured only if the resouce is taken, which means that the pin should be set before write to page using buffer. 


## free a page using buffer
- `void buf_free_page(int64_t table_id, pagenum_t pagenum);`
- In this design, freeing a page should always be occured only if the resource is taken. In this procedure, allocated memory should be freed.

## Applying buffer manager api to on-disk b+tree
My on-disk b+tree design uses `Node` class to represent all types of node. 

### `pagenum_t Node::write_to_disk()`
```cpp
pagenum_t Node::write_to_disk(){
    if(is_on_disk){
        buf_write_page(tid, pn, &default_page);
    }
    else {
        pagenum_t pagenum = buf_alloc_page(tid);
        buf_write_page(tid, pagenum, &default_page);
        //printf("allocated: %d\n",pagenum); 
        pn = pagenum;
        is_on_disk = true;
    }
    return pn;
}
```

### `Node(int64_t table_id, pagenum_t pagenum);`
- This is the constructor for `Node` class. When the instance of `Node` is created using this constructor, `buf_read_page` is immediately called to load page. By calling `buf_read_page`, a pin is set to the corresponding buffer frame.
- **Lifetime of this instance immediately starts after creation, and ends until the `buf_unpin` is called. So, `write_to_disk` should be called during this lifetime. (`is_on_disk` is true in this case)**

### `Node(bool is_leaf, int64_t table_id);`
- This is another constructor for `Node` class. This constructor does nothing with buffer. It just creates the naive instance of `Node` class. 
- If `write_to_disk` is called on this instance, because `is_on_disk` is false, it first allocates page using `buf_alloc_page` and then write to page using `buf_write_page`. By calling `buf_alloc_page`, a pin is set to the buffer frame corresponding to the newly allocated page.
- **Lifetime of this instance does not immediately start after creation, but after calling `write_to_disk`. It ends until the `buf_unpin` is called. So, additional `write_to_disk` should be called during this lifetime.**

### freeing nodes
- Pages can be freed in the procedure of deletion. In this case, `buf_free_page` is directly called in the procedure, and must be called in the **lifetime** mentioned above.

## Tests
- The tests are conducted in my laptop which performance is not good, so the measured time may be slower than usual. Also the time it takes to perform a test may vary from device to device.

### 1. Sequential insert and delete 500000 keys (1~500000)
**without buffer**

| operation | time (ms) |
|-----------|-----------|
| insertion | 536760    |
| deletion  | 536621    |

**with buffer**

buffer size: 3000

| operation | time (ms) |
|-----------|-----------|
| insertion | 35078     |
| deletion  | 15291     |

- With buffer size of 3000, insertion is 15 times faster and deletion is 35 times faster than not using buffers.

### 2. Random insert and delete 500000 keys (1~500000)
- To insert 500000 keys, total 14723 pages are used. In this test, the total pages are assumed to be approximately 14720

**without buffer**

| operation | time (ms) |
|-----------|-----------|
| insertion | 669493    |
| deletion  | 656046    |

**with buffer**

| percentage | buffer size | operation | time (ms)|
|------------|-------------|-----------|--------|
| 30%        | 4416        | insertion | 224077 |
| 30%        | 4416        | deletion  | 198203 |
| 50%        | 7360        | insertion | 134524 |
| 50%        | 7360        | deletion  | 133570 |
| 70%        | 10304       | insertion | 78741  |
| 70%        | 10304       | deletion  | 66641  |
- percentage is (buffer size / total page usage)

- In this test, with buffer size of 4416 which is 30% of total page usage,both insertion and deletion are almost 3 times faster than not using buffers.

- When using buffers of 70% of total page usage, insertion and deletion are almost 3 times faster than using buffers of 30% of total page usage. 

Furthermore, it is clear that the degree of time reduction of using buffer in sequential insertion and deletion is much greater than using buffer in random insertion and deletion.



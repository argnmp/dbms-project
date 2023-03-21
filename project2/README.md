# Project2 : milestone1
## Introduction
**Project2 : milestone** is to implement **disk space manager** that is the lowest part of DBMS. So, it aims to implement serveral apis that handles database file in a paginated arcitecture. These apis will be provided to upper layer for further implementations.

The most important property of this layer is to read and write **pages** to  database file, which means moving data between memory and disk.

## Getting Started
Running `cbnt.sh` shell script file at the root directory automatically remove existing build files, build the project with test, and run tests.

## Design
### Data Unit
- Initial database file size is 10MB, and it grows double of its previous size if no more free pages are left
- Page size is 4MB.

### Pagination
- Physical address space of database file(.db) is divided into 4MB pages.
- Three types of pages
1. Header page
2. Free page
3. Allocated page 
- All free pages are connected in unidirectional way from headeer page.
- All type of pages are packed, so getting physical page offset is done by calculating (page number * PAGE_SIZE)

- First page(first 4096 bytes) is for **header page**, and all other pages are for **free page** or **allocated page**.

#### Header page
| byte      | description      |
|-----------|------------------|
| 0 ~ 7     | magic_number     |
| 8 ~ 15    | free_page_number |
| 16 ~ 23   | number_of_pages  |
| 24 ~ 4095 | reserved         |


- magic_number is to verify database file. If the database file is valid, magic_number is set to 2022.
- free_page_number is to link all free pages. It stores next free page number.
- number_of_pages indicates the number of pages in a database file. If the size of database file is doubled, the number_of_pages value is also doubled.
- Except above three entries, left space is reserved for upper layer.

#### Free page
| byte     | description           |
|----------|-----------------------|
| 0 ~ 7    | next_free_page_number |
| 8 ~ 4095 | reserved              |

- next_free_page_number is to link free pages. It stores next free page number. 
- Except above one entry, left space is reserved for upper layer.


#### Allocated page
| byte     | description |
|----------|-------------|
| 0 ~ 4095 | data        |
- All space in this page is used in upper layer. So data should not be managed at this layer.

## Implementation
### variables
```cpp
#define INITIAL_DB_FILE_SIZE (10 * 1024 * 1024)  
#define PAGE_SIZE (4 * 1024)                    
```
- Define initial database file size to 10MB and page size to 4MB.

```cpp
typedef uint64_t pagenum_t;
extern int global_fd; 
```
- Page number is stored in 8 bytes of address space. **uint64_t** indicates 8bytes.
- global_fd for storing opend database file. Once database file is opened, that status is retained for quite a long time. This is for closing the file after all operations ends.

### page struct

```cpp
// header page structure
struct h_page_t {
    uint64_t magic_number;
    pagenum_t free_page_number;
    uint64_t number_of_pages;
    uint8_t reserved[PAGE_SIZE-24];
};
```
```cpp
// free page structure
struct f_page_t {
  pagenum_t next_free_page_number;
  uint8_t reserved[PAGE_SIZE-8];
};
```
```cpp
// allocated page structure
struct page_t {
  uint8_t data[PAGE_SIZE];
};
```

## Apis
6 apis are implemented in this layer as **disk space manager**

### 1. file_open_database_file
```cpp
int file_open_database_file(const char* pathname);
```
This api opens database file using `pathname` and return file descriptor. If the file in path specified in `pathname` does not exist, create new database file using `pathname`.

- Open database file which path is specified in `pathname` and check the `magic_number` in header page whether it is equal to **2022**. 
- If database file is not in that path, this api creates new one and initialize header page and link free pages using `next_free_page_number`.
- Store file descriptor in the global variable, `global_fd`.
- Return file descriptor of opened database file.


### 2. file_alloc_page
```cpp
pagenum_t file_alloc_page(int fd);
```
This api allocates one free page and return pagenumber which is allocated.

- First check if free pages are left. If it's not, double the size of the database file and link all free pages using `next_free_page_number`.
- And then, allocate one page modifying the free page list.
- Return the page number of allocated page.

### 3. file_free_page
```cpp
void file_free_page(int fd, pagenum_t pagenum);
```
This api frees the allocated page.

- Connect the going-to-be freed page specified by `pagenum` to free page list. It is initialized using `f_page_t` struct. 

### 4. file_read_page
```cpp
void file_read_page(int fd, pagenum_t pagenum, struct page_t* dest);
```
This api reads the page in database file and store it to the memory.

- Read page at offset, (`pagenum` * `PAGE_SIZE`) in database file to the `dest` (memory).

### 5. file_write_page
```cpp
void file_write_page(int fd, pagenum_t pagenum, const struct page_t* src);
```
This api writes the page in memory to database file.

- Write `src` (memory) to database file at offset (`pagenum` * `PAGE_SIZE`).

### 6. file_close_database_file
```cpp
void file_close_database_file();
```
This api closes file which was open in **file_open_database_file** api.

- Close the database file using file descriptor that was stored in `global_fd` variable.

## Unit tests
There are 9 unit tests that are to examine all the functionalities of this layer.

**3** tests are required from specification (project 2: milestone 1), and they are marked with "required in specification"
- **FileInitTest.HandlesInitialization**, **FileTest.HandlesPageAllocation**, **FileTest.CheckReadWriteOperation** are those 3 tests.

**6** test are written arbitrary for testing other functionalities whether it works perfectly.

```cpp
/*
 * 0: Turn off the test
 * 1: Turn on the test
 */

// Test for existing files
#define ExistFileInitSeries 1 
// Extend File to 80MB in size, this test requires quite long time
#define ExtendFileSeries 0
```
- **ExistFileInitSeries** includes test **ExistFileValidInit** and **ExistFileInvalidInit**. By setting this preprocessor to 0 or 1, it is able to decide whether to run those two tests. These test are set to run by default.
- **ExtendFileSeries** includes test **ExtendFileSeriesA** and **ExtendFileSeriesB**. By setting this preprocessor to 0 or 1, it is able to decide whether to run those two tests. **ExtendFileSeriesA** extend file size from 20MB to 40MB and **ExtendFileSeriesB** extends file from 40MB to 80MB. These two tests need quite long time to finish. So They are not set to run by default.

### FileInitTest

#### HandlesInitialization : required in specification
- This test opens database file named "init_test.db" and checks that the returned file descriptor is positive.
- It checks `number_of_pages` value from the header page whether it is same as `INITIAL_DB_FILE_SIZE / PAGE_SIZE` for testing if it is correctly initialized.

#### ExistFileValidInit
- This test opens the existing file named "validdb.db" from the project root and closes the database file.
- It checks returned file descriptor from api if it is not negative.

#### ExistFileInvalidInit
- This test opens the existing file named "invaliddb.db" that is invalid database file which magic_number is not 2022 from the project root, and closes the database file.
- It checks returned file descriptor from api if it is **negative**, because the api returns negative value if the file is invalid.

### FileTest
Unit test using class instance. This test class opens database file named "init_test.db" when test object is constructed, and closes the file when it is desctructed.
```cpp
class FileTest : public ::testing::Test {
 protected:
  FileTest() { fd = file_open_database_file(pathname.c_str()); }

  ~FileTest() {
    if (fd >= 0) {
      file_close_database_file();
    }
  }

  int fd;                
  std::string pathname = "init_test.db";
};
```

#### HandlesPageAllocation : required in specification
- This test allocates two pages from free pages using `file_alloc_page` and frees one of them using `file_free_page`.
- It traverses the free page list, and checks that **the page in allocated state is not in the free page list** and **the page that was freed is in the free page list**.

#### ExtendFile
- This is test for extending file size when there are no free page left. 
- First, it allocates all free pages from database file. Then, it allocates one free page by doubling the file size and it checks the allocated page number if it is equal to the last page of file that was doubled in size (which means the page at the end of the file.)
- Last, it allocates all the left free pages for next test. So, there would be no free pages left, and the number of pages that were allocated will be **number_of_pages/2 -1** because the doulbled file size means the doubled free pages and one of them was already allocated.

> In this test, the size of the database file will be 20MB.

#### ExtendFileSeriesA
- Same as ExtendFile.
- But this test is temporarily **off**.
> In this test, the size of the database file will be 40MB.

#### ExtendFileSeriesB
- Same as ExtendFile.
- But this test is temporarily **off**.
> In this test, the size of the database file will be 80MB.

#### FreePageTest
- First, this test allocates 120 pages and stores the page numbers returned from **file_alloc_page** in `allocated pages`, and frees the pages numbers in `allocated pages` from in index 30 to index 109.
- Second, it counts the number of free pages by traversing the free page list. 
- In the previous test, all free pages are allocated. So in this test, the test file gets doubled, so the extended database file would have the same number of free pages as the previous number of pages (total number of pages in the database file before extended). So, it checks the count the number of free pages if it is equal to **(the number of pages from previous one) - 40**

#### CheckReadWriteOperation : required in specification
- First, it initizlizes 4096 bytes(src) with char 'A' to 'Z'.
- Seoncd, it allocates one free page and write the initialized 4096 bytes(src) to the page using `file_write_page`.
- Then, It retrieves 4096 bytes of chars from the allocated page, and checks if retrieved data is equal to written data.




# Project2 : milestone2
## Introduction
This **Project2 : milestone2** wiki page analyzes the given b+ tree code (bpt.h, bpt.cc) and introduce the naive design of disk-based b+tree.

## Variable and Structures
### variables
```cpp
extern int order;
```
**order** determines the maximum number and minimum number of entries in a node. This **order** value works as the criteria of splitting and merging case when inserting or deleting entries in b+ tree.

### structures
Only two structures are used to represent b+ tree. One is for the node including internal node and leaf node and the other is for the record that contains a value of a key.

#### node struct
```cpp
typedef struct node {
    void ** pointers;
    int * keys;
    struct node * parent;
    bool is_leaf;
    int num_keys;
    struct node * next; // Used for queue.
} node;
```
- `void ** pointers` points lower level node if it's internal node or points `record` if it's leaf node. 
- `int * keys` represents the keys stored in the node. For internal node, it is used to route among nodes. For leaf node, it represents records stored on the b+ tree structure.

The number of entries of keys and pointers is different according to the type of node. If there are n keys in the node, there always exists n+1 pointers in the internal node. However, the leaf node has the same n pointers with the last pointer pointing to the next leaf node.

A node is created with **order - 1** keys array and **order** pointers array.

- `struct node * parent` points parent node.
- `bool is_leaft` represents if the node is a leaf node.
- `int num_keys` represents the number of keys stored in the node.
- `struct node * next` is used for traversing the b+ tree.

#### record struct
```cpp
typedef struct record {
    int value;
} record;
```
This stores the data represented by key.

## Operation
### Insertion
Insertion procedure starts from `insert` function
#### `node * insert( node * root, int key, int value )`
1. If duplicate key exists, return root.
2. Create record using `make_record`.
3. If tree not exists, initialize new tree using `start_new_tree` and return root.
4. Search leaf node that the new key would be placed.
5. If leaf node has room for key and pointer, call `insert_into_leaf`.
6. If leaf node has no room for key and pointer, call `insert_into_leaf_after_splitting`.

#### `node * insert_into_leaf( node * leaf, int key, record * pointer )`
Insert key and pointer in the right place and return leaf.

#### `node * insert_into_leaf_after_splitting(node * root, node * leaf, int key, record * pointer)`
1. Create new array, temp_keys and temp_pointers and store all existing entries with new key and pointer that needs to be inserted.
2. Divide the temp array to existing leaf node and new_leaft node based on the result of calling `cut` function.
3. Set the properties of existing node and new_node.
4. Call `insert_into_parent` to insert new_node to tree.

#### `node * insert_into_parent(node * root, node * left, int key, node * right)` 
1. If no same root exists between exsting node and new node, create new root and insert to it by calling `insert_into_new_root`.
2. New node should be inserted in to parent's entry. If the parent has room for new node, call `insert_into_node`.
3. If the parent node has no room for new node the parent node should be splitted. So it calls `insert_into_node_after_splitting`.

#### `node * insert_into_node(node * root, node * n, int left_index, int key, node * right)`
Insert key and pointer in the right place and return root.

#### `node * insert_into_node_after_splitting(node * root, node * old_node, int left_index, int key, node * right)`
1. Create temp_keys with **order** size and temp_pointers with **order+1** size. This is different from splitting leaf node because the number of pointers of internal node is always one more than the number of keys.
2. Store all keys and pointers into temp array. 
3. Divid the temp array to existing node and new node based on the result of calling `cut` function.
4. Set the properties of existing node and new node.
5. Call `insert_into_parent` to insert new node that was splitted.

As a result, inserting key and value in to the b+ tree occurs recursively by splitting node if there's no room for new node or key/value. 

If the split occurs in the root consequently by recursive calls, new root is created.

### Deletion
Deletion procedure starts from `db_delete` function

#### `node * db_delete(node * root, int key)`
1. Find record of the key, and leaf node of the key.
2. If the record and the leaf node exists, call `delete_entry` to start deletion procedure.
3. Free the found record and return root.

#### `node * delete_entry( node * root, node * n, int key, void * pointer )`
1. By calling `remove_entry_from_node`, remove key and pointer from the node.
2. If the node that has contained deleted key is root, call `adjust_root`.
3. If the number of keys after deletion is more or equal than the minimum number of keys in a node, return root. Nothing happens.
4. If the number of keys falls below minimum, coalescence or redistributuion occurs.
5. Find index of the neighbor by calling `get_neighbor_index`. Neighbor is typically the left node in the same level. If no neighbor exists (the left most node), set the neighbor node to the right node.
6. For internal node, coalescence occurs only when the sum of left and right node is `< order-1`. For leaf node, coalescence occurs only when the sum of left and right node is `< order`. This is done by calling `coalesce_nodes` function.
7. If not that case, redistribute the entries by calling `redistribute_nodes` function.

#### `node *coalesce_nodes(node * root, node * n, node * neighbor, int neighbor_index, int k_prime)`
In this description, **target node** means the node that is being coalesced to neighbor node.
1. If neighbor index is -1 which means the node is the leftmost, swap the target node and the **will be coalesced** node.
2. If the target node is internal node, copy all keys and pointers from target node to neighbor node. In this procedure, k_prime should be inserted first and all the other keys and pointers should be inserted. 
3. If the target node is leaf node, copy all keys and pointers from target node to neighbor node and reconnect the next leaf node.
4. Connect all the child nodes to the new parent(neighbor node).
5. Recursively call `delete_entry` to remove the target node.

#### `node * redistribute_nodes(node * root, node * n, node * neighbor, int neighbor_index, int k_prime_index, int k_prime)`
In this description, **target node** means the node that is being redistributed to neighbor node.
1. If the neighbor is in the left, move the rightmost key and pointer from neighbor node to the target node. 
- If the target node is internal node, only pull the rightmost pointer and set the keys[0] of the target node as k_prime. Also, set keys[k_prime_index] in the parent of the target node as the rightmost key in the neighbor node.
- If the target node is leaf node, pull both rightmost key and pointer, and set keys[k_prime_index] in the parent of the target node as the rightmost key in the neighbor node.
2. If the neighbor is in the right(target node is the leftmost node among the nodes in the same parent), move the leftmost key and pointer from neighbor node to the target node.
- If the target node is leaf node, pull the leftmost key and pointer and set keys[k_prime_index] in the parent of the target node as keys[1] in the neighbor node.
- If the target node is internal node, only pull the rightmost pointer and set the rightmost key in the target node as k_prime. Also, set keys[k_prime_index] in the parent of the target node as the leftmost key in the neighbor node. 
3. update number of keys in target node and neighbor node and return root.

## Structure modification
### Split
- Node splitting occurs during the insert operation.
- Two functions does this operation. One is `node * insert_into_leaf_after_splitting(node * root, node * leaf, int key, record * pointer)` and the other is `node * insert_into_leaf_after_splitting(node * root, node * leaf, int key, record * pointer)`.
- As described, key insertion always starts from the bottom, from the leaf node and it go towards the root of the tree recursively. During this procedure, a key and a pointer to the child node is inserted into the parent node. If there are no space left for a key and a pointer, the node is split into two dividing the keys and the pointers in half.
- The structure of leaf node and internal node is different. So the splitting keys and pointers into two is slightly different between leaf node and internal node.
#### Splitting leaf node
- If there's no space left in leaf node for new key and pointer, this modification occurs.
- Half of key-pointer pair goes to original node and the other half of key-pointer pair goes to new node. The pointer to next-leaf-node of original node is given to the new node, and the pointer to next-leaf-node of original node is set as the new node.
- Then, with the first key of the new node, new node is inserted into the parent of the original node.
- Parent of the original node is internal node.
#### Splitting internal node
- If there's no space left in internal node for new key and pointer, this modification occurs.
- Internal node has one more pointer than key. 
- Assume that there are 4 keys(key[8]) with 5 pointers(pointer[9]) and this includes new key and pointer. key[0~3] and pointer[0~4] is given to original node. key[4] is the k_prime that is used when inserting into the **parent** node. key[5~7] and pointer[5~8] is given to new node.
- Then with the k_prime key, new node is inserted into the parent of the original node.

### Merge
- Node merging occurs during the deletion operation. After deleting the entry, select the neighbor node. **If sum of the num of keys of neighbor node and the number of keys of target node(which entry is deleted) is able to merged into one node, merge operation occurs.**
- If merging is impossible, redistribution between target node and neighbor node is occured by function, `node * redistribute_nodes(node * root, node * n, node * neighbor, int neighbor_index, int k_prime_index, int k_prime)`

- One function does this merging operation. `node *coalesce_nodes(node * root, node * n, node * neighbor, int neighbor_index, int k_prime)`

- In the merging procedure, neighbor is always set to the left of the target node. (If the target node is leftmost, neighbor is set to right of the target node and **swap the target node and neighbor node**)

1. Target key and pointer are deleted from the node(**target node**) and all other keys and pointers are packed to the front. If the target node is root, and if the keys are left in the root, finish this procedure. But if the target node is root and if there are no keys left in the root, check the target node is leaf node or internal node. If it is leaf node, and because there are no keys left, make the root NULL. If it is internal node, promote the first child as the new root.
2. If the number of keys of target node is above minimum, end the procedure. 
3. Set the neighbor. Set k_prime (key at the neighbor node's index)
4. If the target node is leaf node, push all the keys and pointers from the target node to the neighbor node.
5. If the target node is internal node, first push the k_prime to the neighbor node and then push all other keys and pointers to the neighbor node. 
6. Set the target key as k_prime and go to **1** to delete entry recursively.

## Disk-based B+ tree design
To represent node, I will introduce this `Node` class

```cpp
class Node {

public:
    page_t default_page;
    internal_page_t* internal_ptr;
    leaf_page_t* leaf_ptr;
    // table id of the node
    int64_t tid;
    // page number of the node
    pagenum_t pn;
    bool is_on_disk;

    Node(const Node& n);
    Node(bool is_leaf, int64_t table_id);
    Node(int64_t table_id, pagenum_t pagenum);
    
    ~Node();
    pagenum_t write_to_disk();
}
```
This `Node` class internally uses `page_t` / `internal_page_t` / `leaf_page_t` structures.

```cpp
struct page_t {
    uint64_t parent_page_number;
    uint32_t is_leaf;
    uint32_t number_of_keys;
    uint8_t reserved[112];
    uint8_t data[PAGE_SIZE - 128];
};

struct internal_page_t {
    uint64_t parent_page_number;
    uint32_t is_leaf;
    uint32_t number_of_keys;
    uint8_t reserved[104];
    uint64_t one_more_page_number;
    
    // space for key and pagenum
    uint8_t data[PAGE_SIZE - 128];
};

struct leaf_page_t {
    uint64_t parent_page_number;
    uint32_t is_leaf;
    uint32_t number_of_keys;
    uint8_t reserved[96];
    uint64_t amount_of_free_space;
    uint64_t right_sibling_page_number;

    // space for slot and value
    uint8_t data[PAGE_SIZE - 128]; 
};
```
Because both internal page and leaf page are allocated page, they have same page header structure. I represented this common structure as `page_t`. Subdivided structures are represented by `internal_page_t` and `leaf_page_t`.

To represent both internal page and leaf page as one same `node` structure, `internal_page_t* internal_ptr` and `leaf_page_t* leaf_ptr` exists. These pointers points to `page_t default_page`. If the node is leaf node, accessing the leaf page structure can be done by using `leaf_ptr`. For internal node, it is also possible by using internal_ptr.

```cpp
Node(const Node& n);
Node(bool is_leaf, int64_t table_id);
Node(int64_t table_id, pagenum_t pagenum);
```
These 3 contructors are involved in creating node. 
- First one is **copy constructor** which copies the entire node to other. 
- Second is used when creating **new node** which is not yet written to disk. Because this node only exists on memory, not on disk, `bool is_on_disk` is set to false. `int64_t tid` is set with the parameter because this node will be eventually written to disk(table file), but `pagenum_t pn` is not set because this node is not written so no page is allocated to this node.
- Third is used when creating **existing node** from **existing page** in the disk(table file). With `table_id` and `pagenum`, read the page from the disk to `default_page`(class member variable). `bool is_on_disk` is set to true and `int64_t tid`, `pagenum_t pn` are initialized because this node exists in disk.

All the tasks that are done in the node can be added using class member functions.

After all the modification task of node is finished, a node can be written to disk using `pagenum_t write_to_disk()`. If `bool is_on_disk` is false, it first allocates one page from disk and write to disk using **api** from `file.h`. Otherwise, just write to disk using **api** from `file.h`.

Key and page number pair is stored in `uint8_t data[PAGE_SIZE - 128]` of `internal_page_t`.
```cpp
class kpn_t{
public:
    uint8_t data[16];
    int64_t get_key();
    uint64_t get_pagenum();
    void set_key(int64_t key); 
    void set_pagenum(uint64_t pagenum);
};
```
This structure is used to store key and page number pair in `data`.

Slots and values are stored in `uint8_t data[PAGE_SIZE - 128]` of `leaf_page_t`.
```cpp
class slot_t{
public:
    uint8_t data[12];
    int64_t get_key();
    uint16_t get_size();
    uint16_t get_offset();
    void set_key(int64_t key);
    void set_size(uint16_t size);
    void set_offset(uint16_t offset);
};
```
This structure is used to store slots in `data`.

# Project2 : milestone3
## Introduction
This **Project2: milestone3** wiki page describes the implementation design of on-disk b+tree.

## Design
The design of on-disk b+ tree was extended from the naive design described in **Project2 : milestone2** wiki.

### Structures
There are three types of structures which are stored on file, *header page*, *free page* and *allocated page*. The *allocated page* is again classified in two types, *leaf (node) page* and *internal (node) page*. These two pages are needed to represent b+ tree.

The structures here are defined in `page.h` header file.

#### `h_page_t`
- header page structure
```cpp
struct h_page_t {
    uint64_t magic_number;
    pagenum_t free_page_number;
    uint64_t number_of_pages;
    uint64_t root_page_number;
    uint8_t reserved[PAGE_SIZE-32];
};
```
#### `f_page_t`
- free page structure
```cpp
struct f_page_t {
    pagenum_t next_free_page_number;
    uint8_t reserved[PAGE_SIZE-8];
};
```

#### `page_t`
- allocated page structure
```cpp
struct page_t {
    uint64_t parent_page_number;
    uint32_t is_leaf;
    uint32_t number_of_keys;
    uint8_t reserved[112];
    uint8_t data[PAGE_SIZE - 128];
};
```
##### `leaf_page_t`
- leaf (node) page structure. One of the types of allocated page.
```cpp
struct leaf_page_t {
    uint64_t parent_page_number;
    uint32_t is_leaf;
    uint32_t number_of_keys;
    uint8_t reserved[96];
    uint64_t amount_of_free_space;
    uint64_t right_sibling_page_number;

    // space for slot and value
    uint8_t data[PAGE_SIZE - 128]; 
};
```

##### `internal_page_t`
- internal (node) page structure. One of the types of allocated page.
```cpp
struct internal_page_t {
    uint64_t parent_page_number;
    uint32_t is_leaf;
    uint32_t number_of_keys;
    uint8_t reserved[104];
    uint64_t one_more_page_number;
    
    // space for key and pagenum
    uint8_t data[PAGE_SIZE - 128];
};
```

- Because leaf page and internal page has the common structure that occupies first 16 bytes of 4KB page (which is represented by `page_t`, allocated page), the structures of first 16 bytes of `page_t`, `leaf_page_t`, `internal_page_t`are all the same.
- The difference between `leaf_page_t` and `internal_page_t` is the leftover space after page header which is represented by `uint8_t data[PAGE_SIZE - 128]` and the reserved space which is represented by `uint8_t reserved[96]` in `page_t` structure. 
- So, it is possible to use common structure with `page_t`. If leaf or internal page structure is needed from `page_t`, it is possible by casting `page_t*` pointer type to one of `leaf_page_t*` and `internal_page_t*`.


#### `slot_t` class
- slot structure. 
```cpp
//slot structure for leaf_page_t
class slot_t{
public:
    uint8_t data[12];
    int64_t get_key();
    uint16_t get_size();
    uint16_t get_offset();
    void set_key(int64_t key);
    void set_size(uint16_t size);
    void set_offset(uint16_t offset);
};
```
- Slot consists of three values, 8 bytes of *key*, 2 bytes of *size*, and 2 bytes of *offset*. Because slot is stored in `uint8_t data[PAGE_SIZE - 128]` of `leaf_page_t` which is an array of 1 bytes each, I made the slot structure to store the values in an array of 1 bytes each also. So, It is possible to just `memcpy` the `uint8_t data[12]` of `slot_t` into `uint8_t data[PAGE_SIZE - 128]` of `leaf_page_t`. 

#### `kpn_t` class
- {key and pagenum} pair structure. Which is the entry of internal page.
```cpp 
class kpn_t{
public:
    uint8_t data[16];
    int64_t get_key();
    uint64_t get_pagenum();
    void set_key(int64_t key); 
    void set_pagenum(uint64_t pagenum);
};
```
- Entry of internal page consists of two values, 8 bytes of *key*, 8 bytes of *page number*. And I will call this entry as **kpn**. Because kpn is stored in `uint8_t data[PAGE_SIZE - 128]` of `internal_page_t` which is an array of 1 bytes each, I also made the kpn structure to store the values in an array of 1 bytes each. So, It is possible to just `memcpy` the `uint8_t data[12]` of `kpn_t` into `uint8_t data[PAGE_SIZE - 128]` of `internal_page_t`. 

### Representation
#### `Node` class
Because all nodes of b+ tree structure should be stored on disk using file, I made **Node** class to represent all types of nodes in one type of *object*.

By using this class structure, all nodes in b+ tree is managed as the identical way regardless of it is leaf node or internal node. 

```cpp
class Node {

public:
    page_t default_page;
    internal_page_t* internal_ptr;
    leaf_page_t* leaf_ptr;
    // table id of the node
    int64_t tid;
    // page number of the node
    pagenum_t pn;
    bool is_on_disk;

    Node(const Node& n);
    Node(bool is_leaf, int64_t table_id);
    Node(int64_t table_id, pagenum_t pagenum);

    Node& operator=(const Node& n);
    
    ~Node();
    pagenum_t write_to_disk();

    int leaf_insert(int64_t key, const char* value, uint16_t val_size);
    int leaf_append_unsafe(int64_t key, const char* value, uint16_t val_size);
    int leaf_find(int64_t key, char* ret_val, uint16_t* val_size);
    int leaf_find_slot(int64_t key);
    void leaf_move_slot(slot_t* dest, uint16_t slotnum);
    void leaf_move_value(char* dest, uint16_t size, uint16_t offset);
    void leaf_print_all();
    void leaf_remove_unsafe(int64_t key);
    void leaf_pack_values(int from, int to);

    int internal_append_unsafe(int64_t key, pagenum_t pagenum);    
    void internal_set_leftmost_pagenum(pagenum_t pagenum);    
    kpn_t internal_get_kpn_index(int index);
    void internal_set_kpn_index(int64_t key, pagenum_t pagenum, int index);
    void internal_print_all();
    int internal_find_kpn(int64_t key);
    int internal_insert(int64_t key, pagenum_t pagenum);
    void internal_remove_unsafe(int64_t key);

    bool isLeaf();

};
```

##### member variables
```cpp
    ...
    page_t default_page;
    internal_page_t* internal_ptr;
    leaf_page_t* leaf_ptr;
    // table id of the node
    int64_t tid;
    // page number of the node
    pagenum_t pn;
    bool is_on_disk;
    ...
```

- **Node** class internally uses structures that are already defined in `page.h` header file. The object of **Node** class carries one `page_t` struct. According to the type of node(leaf or internal), `internal_ptr` or `leaf_ptr` are made to point to the `default_page`, So it is possible to use extended structure of `internal_page_t` or `leaf_page_t` using `internal_ptr` or `leaf_ptr`.

- `tid` represents the table id(file) that the node is belong to and `pn` represents the page number which means offset inside of file that the node is sotred. `is_on_disk` represents the status of the node whether it is stored on disk(file). If the node is newly created, `is_on_disk` is set to false. And the node doesn't have `tid` and `pn` value. `is_on_disk` is used in `write_to_disk` function.

##### member functions
```cpp
    ...
    Node(bool is_leaf, int64_t table_id);
    Node(int64_t table_id, pagenum_t pagenum);
    Node(const Node& n);
    Node& operator=(const Node& n);
    ...
```
There are 2 constructors to initialize node. 
1. `Node(bool is_leaf, int64_t table_id);` This constructor creates new node belong in to certain table. Because this node is not from disk(file), `is_on_disk` is set to false.
2. `Node(int64_t table_id, pagenum_t pagenum);` This constructor creates node by reading page from disk using `table_id` and `pagenum`. All data in the page from disk is stored in this object. Because this node is from the disk, `is_on_disk` is set to true.

`Node(const Node& n);` defines copy constructor that is used when copying existing node to new one.

`Node& operator=(const Node& n);` is *assign operator overloading*. This is used when it is necessary to overwrite an existing node.

```cpp
    ...
    pagenum_t write_to_disk();
    ...
```
`write_to_disk` function writes `default_page` member variable to page in file specified by `tid` and `pn`. If `is_on_disk`is set to false, this function first allocate one page to this node, and write to disk. In contrast, if `is_on_disk`is set to true, just write to disk.

```cpp
    ...
    int leaf_insert(int64_t key, const char* value, uint16_t val_size);
    int leaf_append_unsafe(int64_t key, const char* value, uint16_t val_size);
    int leaf_find(int64_t key, char* ret_val, uint16_t* val_size);
    int leaf_find_slot(int64_t key);
    void leaf_move_slot(slot_t* dest, uint16_t slotnum);
    void leaf_move_value(char* dest, uint16_t size, uint16_t offset);
    void leaf_print_all();
    void leaf_remove_unsafe(int64_t key);
    void leaf_pack_values(int from, int to);
    ...
```
These are the functions to manipulate the leaf node by using `leaf_ptr` that points `default_page`.

```cpp
    ...
    int internal_append_unsafe(int64_t key, pagenum_t pagenum);    
    void internal_set_leftmost_pagenum(pagenum_t pagenum);    
    kpn_t internal_get_kpn_index(int index);
    void internal_set_kpn_index(int64_t key, pagenum_t pagenum, int index);
    void internal_print_all();
    int internal_find_kpn(int64_t key);
    int internal_insert(int64_t key, pagenum_t pagenum);
    void internal_remove_unsafe(int64_t key);
    ...
```
These are the functions to manipulate the internal node by using `internal_ptr` that points `default_page`.

```cpp
    ...
    bool isLeaf();
    ...
```
This function checks if this node is Leaf node or not.


#### B+ tree operations
These operations are written to implement on-disk b+ tree structure. These operations use the `Node` class mentioned above to represent node which is mapped to **page** on disk(file).

```cpp
int insert_into_new_root(int64_t table_id, Node left, Node right, int64_t key);
int insert_into_parent(int64_t table_id, Node left, Node right, int64_t key);
int dbpt_insert(int64_t table_id, int64_t key, const char* value, uint16_t val_size);
```
These operations are used to insert key and value to b+ tree. In this procedure, new key and value is inserted by recursively calling `insert_into_parent` function. **Split** occurs inside of `dbpt_insert`(splits leaf node if needed) and `insert_into_parent` (splits internal node if needed). 

```cpp
void redistribute_nodes(int64_t table_id, Node parent, Node target, Node neighbor, int neighbor_index, int64_t cur_key_prime, int key_prime_index);
void coalesce_nodes(int64_t table_id, Node target, Node neighbor, int neighbor_index, int64_t cur_key_prime, int key_prime_index);
void delete_entry(int64_t table_id, Node target, int64_t key);
int dbpt_delete(int64_t table_id, int64_t key);
```
These operations are used to delete key and value from b+ tree. This procedure is also recursive. In `delete_entry` function, it checks the free space of a node. If the free space is above the threshold, tree modification occurs. If merge is possible after deletion, it calls `coalesce_nodes`. If merge is impossible after deletion, it calls `redistribute_nodes`. In turn, `coalesce_nodes` calls `delete_entry` to delete entry in the parent node. However `redistribute_nodes` does not call `delete_entry`. It just move keys from one node to other.

## Apis
In the `db.h` file, 7 apis are specified.
```cpp
extern std::vector<char*> allocated_memory_ptr;

int64_t open_table(const char* pathname);
int db_insert(int64_t table_id, int64_t key, const char* value, uint16_t val_size);
int db_find(int64_t table_id, int64_t key, char* ret_val, uint16_t* val_size);
int db_delete(int64_t table_id, int64_t key);
int db_scan(int64_t table_id, int64_t begin_key, int64_t end_key, std::vector<int64_t>* keys, std::vector<char*>* values, std::vector<uint16_t>* val_sizes);
int init_db();
int shutdown_db();
```
### **`allocated_memory_ptr`**
- This vector is used to store all pointers of the heap-allocated memory that are used during the entire lifetime of program. Later, the heap-allocated memory spaces are all freed by the `shutdown_db` api.
### `open_table` 
- This api internally uses `file_open_table_file` api from `file.h`
### `db_insert`
- This api internally uses `dbpt_insert` that is specified as part of the on-disk b+ tree implementation.
### `db_find`
- This api first searches the leaf node, and if the key exists in the leaf node, copy value and value size to given memory.
### `db_delete`
- This api internally uses `dbpt_delete` that is specified as part of the on-disk b+ tree implementation.
### `db_scan` 
- This api first checks if the begin_key is smaller or equal than end_key. And then it internally calls `dbpt_scan`. This operation pushes keys and values to the given vector. But value stored in b+ tree has variable length. So `dbpt_scan` api should allocate memory on heap, save the value to the allocated memory on heap, and push the pointer of memory to the given vector. To prevent the leak of memory, the memory on heap should be freed. So, to free all the allocated memory for value, this api not only push the pointer to the given array, but also push to **`allocated_memory_ptr`**. The allocated memory pointers in this vector are freed before termination.
### `init_db`
- This api prepares required resources.
### `shutdown_db`
- This api is responsible to free all the allocated memory. The pointers to the allocated memory are stored in `allocated_memory_ptr` vector. By using this vector, this api clean up the resources.

## Additional implementations
- To manage opened tables uniquely, `map<string, int64_t> opened_tables;` is added to lower layer, which is **file manager** that I design in **Project2 : milestone1**. Using this structure, reopening of the same table can be prevented. If the user tries to reopen the already opened table, `file_open_table_file` api checks the `opened_tables` and returns the table_id of it, not reopening the same table file.
## Tests
### Interactive command line program
It is possible to test the db api interactively by executing the build file,`disk_based_db`.

To execute the program after clean build, Run **`start_client.sh`** shell script file.

```cpp
    "o <k> -- Open table <k>. If the table does not exist, create new table."
    "i <k> <v> -- Insert key <k> (an integer) and value <v>."
    "f <k>  -- Find the value under key <k>."
    "r <k1> <k2> -- Print the keys and values found in the range [<k1>, <k2>]"
    "d <k>  -- Delete key <k> and its associated value."
    "t -- Print the B+ tree."
    "l -- Print the keys and values of the leaves (bottom row of the tree)."
    "v -- Toggle output of pagenum in the tree node"
    "q -- Quit. (Or use Ctl-D.)"
    "? -- Print this help message."
```
These commands are available in the program.

### Google Tests
To start google test after clean build, Run **`start_test.sh`** shell script file.
#### RandomInsertionDeletionTest
1. Insertion phase: Insert 10000 random keys in the range of 0 to 10000000 with random value length. Save the random keys in `rands` vector. Also save the inserted values inside the `inserted_values` map to check in the deletion phase.
2. Shuffle phase: Shuffle the inserted values from `rands` to `shuffled_inserted_keys` to delete the keys in random order.
3. Deletion phase: Deletion procedure runs in the order of shuffled `inserted_values`. First, retrieve the value and value size with the key. And then, compare with the value stored in `inserted_values` map. If the values are not same, test fails. Second, deletes the key, but does not delete it with a 1 in 100 chance. If deletion fails, which means that the inserted key is not in the database, the test fails.
4. Leftover deletion phase: Perform the same task as *#3* for the remaining keys.

#### ScanTest
1. Insertion phase: Insert a key number from 0 to 10000 sequentially. Because all keys are unique, all insertion should be successful. If an insertion fails, the test fails. Also insert inserted values into the `inserted_values` to check the returned values from `db_scan`.
2. Scan phase: Scan a key number from 1000 to 6000. The returned number of keys should be equal to the number of scaned keys. If they are different, test fails.
3. Validation phase: Compare the returned values with the inserted values. If they are different, test fails.


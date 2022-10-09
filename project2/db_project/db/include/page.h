#ifndef DB_PAGE_H_
#define DB_PAGE_H_
#include <stdint.h>
#include <map>
#include <string>
#include <cstring>
using namespace std;

/*
// These definitions are not requirements.
// You may build your own way to handle the constants.
#define INITIAL_DB_FILE_SIZE (10 * 1024 * 1024)  // 10 MiB
#define PAGE_SIZE (4 * 1024)                     // 4 KiB
#define PAGE_HEADER_SIZE 128
#define SLOT_SIZE 12
#define KPN_SIZE 16
#define SPLIT_THRESHOLD 1984

#define MAX_KPN_NUMBER 248
#define DELETE_MODIFICATION_THRESHOLD 2500
*/


#define INITIAL_DB_FILE_SIZE 573440 
#define PAGE_SIZE (224)                     // 224bytes
#define PAGE_HEADER_SIZE 128
#define SLOT_SIZE 12
#define KPN_SIZE 16
#define SPLIT_THRESHOLD 48 //1984

#define MAX_KPN_NUMBER 6
#define DELETE_MODIFICATION_THRESHOLD 60



typedef uint64_t pagenum_t;

// keeping this data structure not to open same file multiple times
extern map<string, int64_t> opened_tables;

// header page structure
struct h_page_t {
    uint64_t magic_number;
    pagenum_t free_page_number;
    uint64_t number_of_pages;
    uint64_t root_page_number;
    uint8_t reserved[PAGE_SIZE-32];
};

// free page structure
struct f_page_t {
    pagenum_t next_free_page_number;
    uint8_t reserved[PAGE_SIZE-8];
};

// allocated page structure
struct page_t {
    uint64_t parent_page_number;
    uint32_t is_leaf;
    uint32_t number_of_keys;
    uint8_t reserved[112];
    uint8_t data[PAGE_SIZE - 128];
};

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

// allocated - leaf page structure
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

// key and page number pair for internal_page_t
class kpn_t{
public:
    uint8_t data[16];
    int64_t get_key();
    uint64_t get_pagenum();
    void set_key(int64_t key); 
    void set_pagenum(uint64_t pagenum);
};

// allocated - internal page structure
struct internal_page_t {
    uint64_t parent_page_number;
    uint32_t is_leaf;
    uint32_t number_of_keys;
    uint8_t reserved[104];
    uint64_t one_more_page_number;
    
    // space for key and pagenum
    uint8_t data[PAGE_SIZE - 128];
};

#endif

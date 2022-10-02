#ifndef DB_PAGE_H_
#define DB_PAGE_H_
#include <stdint.h>
#include <map>
#include <string>
using namespace std;

// These definitions are not requirements.
// You may build your own way to handle the constants.
#define INITIAL_DB_FILE_SIZE (10 * 1024 * 1024)  // 10 MiB
#define PAGE_SIZE (4 * 1024)                     // 4 KiB

typedef uint64_t pagenum_t;

// keeping this data structure not to open same file multiple times
extern map<string, int64_t> opened_tables;

// header page structure
struct h_page_t {
    uint64_t magic_number;
    pagenum_t free_page_number;
    uint64_t number_of_pages;
    uint8_t reserved[PAGE_SIZE-24];
};

// free page structure
struct f_page_t {
  pagenum_t next_free_page_number;
  uint8_t reserved[PAGE_SIZE-8];
};

// allocated page structure
struct page_t {
  uint8_t data[PAGE_SIZE];
};

#endif

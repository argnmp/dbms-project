#ifndef DB_H_
#define DB_H_

#include "dbpt.h"
#include "file.h"
#include "buffer.h"
#include <cstdint>
#include <vector>

extern std::vector<char*> allocated_memory_ptr;

// Open an existing database file or create one if not exist.
int64_t open_table(const char* pathname);

// Insert a record to the given table. / 0 for success, -1 for fail
int db_insert(int64_t table_id, int64_t key, const char* value, uint16_t val_size);

// Find a record with the matching key from the given table. 
int db_find(int64_t table_id, int64_t key, char* ret_val, uint16_t* val_size);

// Delete a record with the matching key from the given table. / 0 for success, -1 for fail
int db_delete(int64_t table_id, int64_t key);

// Find records with a key between the range: begin_key <= key <= end_key
int db_scan(int64_t table_id, int64_t begin_key, int64_t end_key, std::vector<int64_t>* keys, std::vector<char*>* values, std::vector<uint16_t>* val_sizes);

// Initialize the database system.
int init_db(int num_buf);

// Shutdown the database system.
int shutdown_db();


#endif


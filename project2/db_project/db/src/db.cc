#include "db.h"
#include "dbpt.h"
#include "file.h"

int64_t open_table(const char* pathname){
    int64_t table_id = file_open_table_file(pathname);  
    return table_id;
}

int db_insert(int64_t table_id, int64_t key, const char* value, uint16_t val_size){
    return insert(table_id, key, value, val_size);
}

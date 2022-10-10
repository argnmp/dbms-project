#include "db.h"
#include "dbpt.h"
#include "file.h"

int64_t open_table(const char* pathname){
    int64_t table_id = file_open_table_file(pathname);  
    return table_id;
}

int db_insert(int64_t table_id, int64_t key, const char* value, uint16_t val_size){
    return dbpt_insert(table_id, key, value, val_size);
}

int db_find(int64_t table_id, int64_t key, char* ret_val, uint16_t* val_size){
    h_page_t header_node;
    file_read_page(table_id, 0, (page_t*) &header_node);      
    Node leaf = find_leaf(table_id, header_node.root_page_number, key);
    return leaf.leaf_find(key, ret_val, val_size);
}

int db_delete(int64_t table_id, int64_t key){
    return dbpt_delete(table_id, key);
}

int db_scan(int64_t table_id, int64_t begin_key, int64_t end_key, std::vector<int64_t>* keys, std::vector<char*>* values, std::vector<uint16_t>* val_sizes){

}

int init_db(){
    return 0;
}

int shutdown_db(){
    file_close_database_file();
    return 0;
}

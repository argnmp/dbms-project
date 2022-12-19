#include "db.h"

std::vector<char*> allocated_memory_ptr;

int64_t open_table(const char* pathname){
    int64_t table_id = buf_open_table_file(pathname);  
    return table_id;
}

int db_insert(int64_t table_id, int64_t key, const char* value, uint16_t val_size){
    return dbpt_insert(table_id, key, value, val_size);
}

int db_find_depreciated(int64_t table_id, int64_t key, char* ret_val, uint16_t* val_size){
    h_page_t header_node;
    buf_read_page(table_id, 0, (page_t*) &header_node);      
    if(header_node.root_page_number==0){
        return -1;
    }
    Node leaf = find_leaf(table_id, header_node.root_page_number, key);
    //?
    buf_unpin(table_id, 0);
    int result = leaf.leaf_find(key, ret_val, val_size);

    //?
    buf_unpin(table_id, leaf.pn);
    return result;
}

int db_delete(int64_t table_id, int64_t key){
    return dbpt_delete(table_id, key);
}

int db_scan(int64_t table_id, int64_t begin_key, int64_t end_key, std::vector<int64_t>* keys, std::vector<char*>* values, std::vector<uint16_t>* val_sizes){
    if(begin_key > end_key) return -1;
    return dbpt_scan(table_id, begin_key, end_key, keys,values, val_sizes, &allocated_memory_ptr); 
}

int init_db(int buf_num, int flag, int log_num, char* log_path, char* logmsg_path){
    int g_result;
    g_result = init_buffer(buf_num);
    g_result = init_trx();
    g_result = log_manager.init_lm(log_path, logmsg_path);
    log_manager.show_lb_buffer(); 

    return 0;
}

int shutdown_db(){
    for (auto i: allocated_memory_ptr){
        delete[] i;
    }
    fini_buffer();
    file_close_database_file();
    return 0;
}

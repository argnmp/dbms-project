#ifndef DB_DBPT_H_
#define DB_DBPT_H_
#include "file.h"
#include "page.h"
#include "buffer.h"
#include <vector>
#include <cstring>
#include <queue>

#define DPT printf("------DEBUG POINT------\n");

//utility function

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

    // 0 for success / -1 for no space / -2 for key exists
    int leaf_insert(int64_t key, const char* value, uint16_t val_size);

    // 0 for success / -1 for no space 
    // not checking for key existance
    int leaf_append_unsafe(int64_t key, const char* value, uint16_t val_size);

    // 0 for success / -1 for fail
    int leaf_find(int64_t key, char* ret_val, uint16_t* val_size);

    // 0 for success / -1 for fail
    int leaf_find_slot(int64_t key);

    void leaf_move_slot(slot_t* dest, uint16_t slotnum);
    void leaf_move_value(char* dest, uint16_t size, uint16_t offset);
    void leaf_print_all();

    // do not check if the key exists for optimization because the existance of key is already checked in the deleting procedure.
    void leaf_remove_unsafe(int64_t key);

    // pack values that n number of slots 
    void leaf_pack_values(int from, int to);

    // 0 for success / -1 for no space 
    // not checking for key existance
    int internal_append_unsafe(int64_t key, pagenum_t pagenum);    

    void internal_set_leftmost_pagenum(pagenum_t pagenum);    
    kpn_t internal_get_kpn_index(int index);
    void internal_set_kpn_index(int64_t key, pagenum_t pagenum, int index);
    void internal_print_all();
    int internal_find_kpn(int64_t key);
    int internal_insert(int64_t key, pagenum_t pagenum);

    // do not check if the key exists for optimization.
    void internal_remove_unsafe(int64_t key);

    bool isLeaf();

};

pagenum_t get_root_pagenum(int64_t table_id);
void set_root_pagenum(int64_t table_id, pagenum_t pagenum);

void print_tree(int64_t table_id, bool pagenum_p);
void print_leaves(int64_t table_id);

Node find_leaf(int64_t table_id, pagenum_t root, int64_t key);

int insert_into_new_root(int64_t table_id, Node left, Node right, int64_t key);
int insert_into_parent(int64_t table_id, Node left, Node right, int64_t key);
int dbpt_insert(int64_t table_id, int64_t key, const char* value, uint16_t val_size);

void redistribute_nodes(int64_t table_id, Node parent, Node target, Node neighbor, int neighbor_index, int64_t cur_key_prime, int key_prime_index);
void coalesce_nodes(int64_t table_id, Node target, Node neighbor, int neighbor_index, int64_t cur_key_prime, int key_prime_index);
void delete_entry(int64_t table_id, Node target, int64_t key);
int dbpt_delete(int64_t table_id, int64_t key);

int dbpt_scan(int64_t table_id, int64_t begin_key, int64_t end_key, std::vector<int64_t>* keys, std::vector<char*>* values, std::vector<uint16_t>* val_sizes, std::vector<char*>* allocated_memory_ptr);

void test(int64_t table_id);
#endif

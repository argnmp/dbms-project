#ifndef DB_DBPT_H_
#define DB_DBPT_H_
#include "file.h"
#include "page.h"
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

public:

    Node(const Node& n);
    Node(bool is_leaf, int64_t table_id);
    Node(int64_t table_id, pagenum_t pagenum);
    
    ~Node();
    pagenum_t write_to_disk();

    // 0 for success / -1 for no space / -2 for key exists
    int leaf_insert(int64_t key, const char* value, uint16_t val_size);

    // 0 for success / -1 for fail
    int leaf_find(int64_t key, char* ret_val, uint16_t* val_size);

    // 0 for success / -1 for fail
    int leaf_find_slot(int64_t key);
    void leaf_move_slot(slot_t* dest, uint16_t slotnum);
    void leaf_move_value(char* dest, uint16_t size, uint16_t offset);
    void leaf_print_all();

    void internal_set_leftmost_pagenum(pagenum_t pagenum);    
    kpn_t internal_get_kpn_index(int index);
    void internal_set_kpn_index(int64_t key, pagenum_t pagenum, int index);
    void internal_print_all();
    int internal_find_kpn(int64_t key);
    int internal_insert(int64_t key, pagenum_t pagenum);

    bool isLeaf();

};

h_page_t* read_header_node(int64_t table_id);

void print_tree(int64_t table_id, bool pagenum_p);
Node find_leaf(int64_t table_id, pagenum_t root, int64_t key);
int insert_into_new_root(int64_t table_id, Node left, Node right, int64_t key);
int insert_into_parent(int64_t table_id, Node left, Node right, int64_t key);
int insert(int64_t table_id, int64_t key, const char* value, uint16_t val_size);

void test(int64_t table_id);
#endif

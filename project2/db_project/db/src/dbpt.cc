#include "dbpt.h"

Node::Node(const Node& n){
    tid = n.tid;
    pn = n.pn;
    is_on_disk = n.is_on_disk;
    default_page.parent_page_number = n.default_page.parent_page_number;
    default_page.is_leaf = n.default_page.is_leaf;
    default_page.number_of_keys = n.default_page.number_of_keys;
    memcpy(default_page.reserved, n.default_page.reserved, sizeof(default_page.reserved));
    memcpy(default_page.data, n.default_page.data, sizeof(default_page.data));
    if(isLeaf()){
        leaf_ptr = (leaf_page_t*) &default_page; 
    }
    else {
        internal_ptr = (internal_page_t*) &default_page; 
    }
}
Node::Node(bool is_leaf, int64_t table_id){
    default_page.parent_page_number = 0;
    pn = -1;
    if(is_leaf){
        leaf_ptr = (leaf_page_t*) &default_page; 
        leaf_ptr -> is_leaf = 1;
        leaf_ptr -> number_of_keys = 0;
        leaf_ptr -> amount_of_free_space = PAGE_SIZE - PAGE_HEADER_SIZE;
        leaf_ptr -> right_sibling_page_number = 0;
    }
    else {
        internal_ptr = (internal_page_t*) &default_page; 
        internal_ptr -> is_leaf = 0;
        internal_ptr -> number_of_keys = 0;
    }
    tid = table_id;
    is_on_disk = false;
    //printf("create amount_of_free_space: %d\n",leaf_ptr->amount_of_free_space);
}
Node::Node(int64_t table_id, pagenum_t pagenum){
    file_read_page(table_id, pagenum, &default_page);
    if(isLeaf()){
        leaf_ptr = (leaf_page_t*) &default_page;
    }
    else {
        internal_ptr = (internal_page_t*) &default_page;
    }
    tid = table_id; 
    pn = pagenum;
    is_on_disk = true;
}
Node::~Node(){
    //printf("freed %d %d page on memory\n", tid, pn);
}
pagenum_t Node::write_to_disk(){
    if(is_on_disk){
        file_write_page(tid, pn, &default_page);
    }
    else {
        pagenum_t pagenum = file_alloc_page(tid);
        file_write_page(tid, pagenum, &default_page);
        pn = pagenum;
        is_on_disk = true;
    }
    return pn;
}

int Node::leaf_insert(int64_t key, const char* value, uint16_t val_size){
    //prevent duplicate keys
    if(leaf_find_slot(key)==0){
        return -2;
    }
    if(leaf_ptr->amount_of_free_space - TEST_INSERTION_THRESHOLD < SLOT_SIZE + val_size){
        return -1;
    }
    
    int i;
    for(i = 0; i<leaf_ptr->number_of_keys; i++){
        slot_t tmp;
        memcpy(&tmp.data, leaf_ptr->data + i*SLOT_SIZE, sizeof(tmp.data)); 
        int64_t slot_key = tmp.get_key();
        if(slot_key > key){
            break;
        }
    }
    int insertion_point = i;

    for(int i = leaf_ptr->number_of_keys; i>insertion_point; i--){
        slot_t tmp;
        memcpy(&tmp.data, leaf_ptr->data + (i-1)*SLOT_SIZE, sizeof(tmp.data));
        memcpy(leaf_ptr->data + i*SLOT_SIZE, &tmp.data, sizeof(tmp.data));
    }

    slot_t slot;
    slot.set_key(key);
    slot.set_offset(leaf_ptr->amount_of_free_space + SLOT_SIZE * leaf_ptr -> number_of_keys - val_size);   
    slot.set_size(val_size);
    memcpy(leaf_ptr->data + insertion_point *SLOT_SIZE, &slot, sizeof(slot));
    memcpy(leaf_ptr->data + slot.get_offset(), value, sizeof(uint8_t)*val_size);
    leaf_ptr->amount_of_free_space -= SLOT_SIZE + val_size;
    leaf_ptr->number_of_keys += 1;
    //printf("key: %lld, size: %lld, offset: %lld\n",slot.get_key(), slot.get_size(), slot.get_offset());
    //printf("amount of free space after : %d",leaf_ptr->amount_of_free_space);
    return 0;
}
int Node::leaf_find(int64_t key, char* ret_val, uint16_t* val_size){
    for(int i = 0; i<leaf_ptr->number_of_keys; i++){
        slot_t tmp;
        memcpy(&tmp, leaf_ptr->data + i*SLOT_SIZE, sizeof(tmp)); 
        if(tmp.get_key()==key){
            memcpy(ret_val, leaf_ptr->data + tmp.get_offset(), sizeof(uint8_t)* tmp.get_size());
            *val_size = tmp.get_size();
            /*
            printf("found: ");
            for(int i = 0; i<*val_size; i++){
                printf("%c",ret_val[i]);
            }
            printf("\n");
            */
            return 0;
        }
    }
    /*
    printf("not found\n");
    */
    return -1;

}
void Node::leaf_print_all(){
    for(int i = 0; i<leaf_ptr->number_of_keys; i++){
        slot_t tmp;
        memcpy(&tmp, leaf_ptr->data + i*SLOT_SIZE, sizeof(tmp)); 
        char ret_val[112];
        uint16_t val_size;
        leaf_find(tmp.get_key(), ret_val, &val_size);
        printf("key: %lld, size: %lld, offset: %lld, value: ",tmp.get_key(), tmp.get_size(), tmp.get_offset());
        for(int i = 0; i<val_size; i++){
            printf("%c",ret_val[i]);
        }
        printf("\n");
    }
    printf("\n");

}
int Node::leaf_find_slot(int64_t key){
    for(int i = 0; i<leaf_ptr->number_of_keys; i++){
        slot_t tmp;
        memcpy(&tmp, leaf_ptr->data + i*SLOT_SIZE, sizeof(tmp)); 
        if(tmp.get_key()==key){
            return 0;
        }
    }
    return -1;

}
void Node::leaf_move_slot(slot_t* dest, uint16_t slotnum){
    memcpy(dest, leaf_ptr->data + slotnum*SLOT_SIZE, sizeof(slot_t));
}
void Node::leaf_move_value(char* dest, uint16_t size, uint16_t offset){
    memcpy(dest, leaf_ptr->data + offset, sizeof(uint8_t)*size);
}

void Node::internal_set_leftmost_pagenum(pagenum_t pagenum){
    internal_ptr->one_more_page_number = pagenum;
}   
kpn_t Node::internal_get_kpn_index(int index){
    kpn_t ret;
    memcpy(&ret, internal_ptr->data + index*KPN_SIZE, sizeof(ret.data));
    return ret;
}
void Node::internal_set_kpn_index(int64_t key, pagenum_t pagenum, int index){
    kpn_t tmp;
    tmp.set_key(key);
    tmp.set_pagenum(pagenum);
    memcpy(internal_ptr->data + index*KPN_SIZE, tmp.data, sizeof(tmp.data)); 
}
void Node::internal_print_all(){
    printf("pagenum: %lld\n", internal_ptr->one_more_page_number);
    for(int i = 0; i<internal_ptr->number_of_keys; i++){
        kpn_t tmp;
        memcpy(&tmp, internal_ptr->data + i*KPN_SIZE, sizeof(tmp)); 
        printf("key: %lld, pagenum: %lld\n", tmp.get_key(), tmp.get_pagenum());
    }
    printf("\n");
}
int Node::internal_find_kpn(int64_t key){
    for(int i = 0; i<internal_ptr->number_of_keys; i++){
        kpn_t tmp;
        memcpy(&tmp, internal_ptr->data + i*KPN_SIZE, sizeof(tmp)); 
        if(tmp.get_key()==key){
            return 0;
        }
    }
    return -1;
}
int Node::internal_insert(int64_t key, pagenum_t pagenum){
    printf("input key : %d\n", key);
    printf("internal num key :%d\n", internal_ptr->number_of_keys);
    if(internal_find_kpn(key)==0){
        return -2;
    } 
    if(internal_ptr->number_of_keys >=248 - TEST_INTERNAL_INSERTION_THRESHOLD){
        return -1;
    }
    int i;
    for(i = 0; i<internal_ptr->number_of_keys; i++){
        kpn_t tmp;
        tmp = internal_get_kpn_index(i);
        if(tmp.get_key() > key) break;
    }
    int insertion_point = i;

    for(int i = internal_ptr->number_of_keys; i>insertion_point; i--){
        kpn_t tmp;
        tmp = internal_get_kpn_index(i-1);
        internal_set_kpn_index(tmp.get_key(), tmp.get_pagenum(), i); 
    }

    internal_set_kpn_index(key, pagenum, insertion_point);
    internal_ptr -> number_of_keys += 1;
    return 0;
}


bool Node::isLeaf(){
    return default_page.is_leaf;
}

void print_tree(int64_t table_id, bool pagenum_p){
    h_page_t header_node;
    file_read_page(table_id, 0, (page_t*) &header_node);      
    if(header_node.root_page_number == 0){
        printf("Empty tree\n");
    } 
    queue<pair<pagenum_t, int>> q; 
    q.push({header_node.root_page_number, 0});
    if(pagenum_p) printf("<%d>\n", header_node.root_page_number);
    int cur_rank = 0;
    while(!q.empty()){
        auto t = q.front();
        q.pop();
        Node target(table_id, t.first);
        if(cur_rank != t.second){
            printf("\n");
            cur_rank ++;
        }
        if(!target.isLeaf()){
            q.push({target.internal_ptr->one_more_page_number, t.second +1});
            if(pagenum_p) printf("<%d>", target.internal_ptr->one_more_page_number);
            for(int i = 0; i<target.internal_ptr->number_of_keys; i++){
                kpn_t tmp = target.internal_get_kpn_index(i);
                printf("|%d|",tmp.get_key());
                if(pagenum_p) printf("<%d>",tmp.get_pagenum());
                q.push({tmp.get_pagenum(), t.second + 1});

            }
            printf(" ");
        }
        else{
            for(int i = 0; i<target.leaf_ptr->number_of_keys; i++){
                slot_t tmp;
                target.leaf_move_slot(&tmp, i);
                printf("{%d}",tmp.get_key());
            }
            printf(" ");
        }
    }
    printf("\n\n");

}

Node find_leaf(int64_t table_id, pagenum_t root, int64_t key){
    //traverse, using node poitner, important!!!!
    Node* cursor = new Node(table_id, root);
    while(!cursor->isLeaf()){
        /* not implemented */
        int i = 0;
        kpn_t tmp;
        pagenum_t next_pagenum = cursor->internal_ptr->one_more_page_number;
        while (i < cursor->internal_ptr->number_of_keys){
            tmp = cursor->internal_get_kpn_index(i);
            if (key > tmp.get_key()){
                i++;
                next_pagenum = tmp.get_pagenum();
            }
            else {
                break; 
            }
        }
        cursor = new Node(table_id, next_pagenum);
        
    }
    return *cursor;
}

int insert_into_new_root(int64_t table_id, Node left, Node right, int64_t key){
    Node root(false, table_id); 
    root.internal_set_kpn_index(key, right.pn, 0);    
    root.internal_set_leftmost_pagenum(left.pn);
    root.internal_ptr->number_of_keys += 1;
    root.internal_ptr->parent_page_number = 0;
    pagenum_t root_pn = root.write_to_disk();
    
    //set header page and write
    h_page_t header_node;
    file_read_page(table_id, 0, (page_t*) &header_node);      
    header_node.root_page_number = root_pn;
    file_write_page(table_id, 0, (page_t*) &header_node);

    left.default_page.parent_page_number = root_pn;
    right.default_page.parent_page_number = root_pn;
    left.write_to_disk();
    right.write_to_disk();

    return 0;
}
int insert_into_parent(int64_t table_id, Node left, Node right, int64_t key){

    // new root
    if(left.default_page.parent_page_number == 0){
        insert_into_new_root(table_id, left, right, key); 
        return 0;
    }

    // find parent and insert
    Node parent(table_id, left.default_page.parent_page_number);
    int result = parent.internal_insert(key, right.pn);
    if(result==0){
        parent.write_to_disk();
        return 0;
    }
    else if(result==-2){
        // this never will be the case
        cout << "key collision" << endl;
        return -1;
    }
    else {
        cout << "parent split!" << endl;
        //split parent and insert 
        Node cinternal = parent;
        Node new_internal(false, table_id);
        parent.default_page.number_of_keys = 0;
        new_internal.default_page.parent_page_number = cinternal.default_page.parent_page_number;
        DPT

        int split_point = cinternal.default_page.number_of_keys / 2;
        printf("splitpoint! : %d\n",split_point);
        int i;
        bool new_key_flag = true; 
        int64_t k_prime;
        for(i = 0; i<split_point; i++){
            kpn_t tmp = cinternal.internal_get_kpn_index(i);
            if(new_key_flag && tmp.get_key()>key){
                new_key_flag = false;
                parent.internal_insert(key, right.pn);
            }
            parent.internal_insert(tmp.get_key(),tmp.get_pagenum());
        }
        parent.internal_ptr->one_more_page_number = cinternal.internal_ptr->one_more_page_number;
        kpn_t s = cinternal.internal_get_kpn_index(i);
        i+=1;
        k_prime = s.get_key();
        new_internal.internal_ptr->one_more_page_number = s.get_pagenum();
        for(;i<cinternal.default_page.number_of_keys; i++){
            kpn_t tmp = cinternal.internal_get_kpn_index(i);
            if(new_key_flag && tmp.get_key()>key){
                new_key_flag = false;
                new_internal.internal_insert(key, right.pn);
            }
            new_internal.internal_insert(tmp.get_key(),tmp.get_pagenum());
        }
        if(new_key_flag){
            new_internal.internal_insert(key, right.pn);
        }
        //new node should have pagenum!!!
        new_internal.write_to_disk();
        Node child(table_id, new_internal.internal_ptr->one_more_page_number);
        child.default_page.parent_page_number = new_internal.pn;
        child.write_to_disk();
        for(int i = 0; i<new_internal.internal_ptr->number_of_keys; i++){
            kpn_t tmp = new_internal.internal_get_kpn_index(i);
            Node child(table_id, tmp.get_pagenum());
            child.default_page.parent_page_number = new_internal.pn;
            child.write_to_disk();
        }
        parent.write_to_disk();
        return insert_into_parent(table_id, parent, new_internal, k_prime);
    }
    
}

int insert(int64_t table_id, int64_t key, const char* value, uint16_t val_size){
    //tree does not exist
    h_page_t header_node;
    file_read_page(table_id, 0, (page_t*) &header_node);      

    if(header_node.root_page_number == 0) {
        //create new leaf node
        Node leaf(true, table_id);
        leaf.leaf_insert(key, value, val_size);
        pagenum_t pn = leaf.write_to_disk();
        file_read_page(table_id, 0, (page_t*) &header_node);      
        header_node.root_page_number = pn;
        file_write_page(table_id, 0, (page_t*) &header_node);
        return 0;
    }

    Node target_leaf = find_leaf(table_id, header_node.root_page_number, key);
    target_leaf.leaf_print_all(); 

    int result = target_leaf.leaf_insert(key, value, val_size);
    if(result==0){
        //insert into leaf if the space exists.
        target_leaf.write_to_disk(); 
        return 0;
    }
    else if(result == -2){
        return 0;
    }
    else {
        printf("split!\n");
        //split and insert into leaf.
        Node cleaf = target_leaf; 
        Node new_leaf(true, table_id);
        new_leaf.write_to_disk(); 
        target_leaf.leaf_ptr->amount_of_free_space = PAGE_SIZE - PAGE_HEADER_SIZE;
        target_leaf.leaf_ptr->number_of_keys = 0;
        //right_sibling_page_number not set yet

        new_leaf.leaf_ptr->parent_page_number = cleaf.leaf_ptr->parent_page_number;
        new_leaf.leaf_ptr->right_sibling_page_number = cleaf.leaf_ptr->right_sibling_page_number;

        bool new_key_flag = true;
        uint16_t next;
        uint16_t acc_size = 0;
        for(next = 0; next<cleaf.leaf_ptr->number_of_keys; next++){
            if(acc_size > SPLIT_THRESHOLD){
                break;
            }
            slot_t tmp;
            cleaf.leaf_move_slot(&tmp, next); 
            char value[120];
            cleaf.leaf_move_value(value, tmp.get_size(), tmp.get_offset());
            if(new_key_flag && key < tmp.get_key()){
                new_key_flag = false;
                target_leaf.leaf_insert(key, value, val_size);
            }
            target_leaf.leaf_insert(tmp.get_key(), value, tmp.get_size());
            acc_size += SLOT_SIZE + tmp.get_size(); 
        }
        for(; next<cleaf.leaf_ptr->number_of_keys; next++){
            slot_t tmp;
            cleaf.leaf_move_slot(&tmp, next); 
            char value[120];
            cleaf.leaf_move_value(value, tmp.get_size(), tmp.get_offset());
            if(new_key_flag && key < tmp.get_key()){
                new_key_flag = false;
                new_leaf.leaf_insert(key, value, val_size);
            }
            new_leaf.leaf_insert(tmp.get_key(), value, tmp.get_size());
        }
        if(new_key_flag){
            new_leaf.leaf_insert(key, value, val_size); 
        }

        pagenum_t pagenum = new_leaf.write_to_disk();
        target_leaf.leaf_ptr->right_sibling_page_number = pagenum;
        target_leaf.write_to_disk();
        slot_t tmp;
        new_leaf.leaf_move_slot(&tmp, 0);

        //new node should have pagenum!!
        insert_into_parent(table_id, target_leaf, new_leaf, tmp.get_key());
         
        return 0;
    }
}

void test(int64_t table_id){
    Node a(true, table_id);
    a.write_to_disk();
}

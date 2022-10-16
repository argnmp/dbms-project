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
Node& Node::operator=(const Node& n){
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
    return *this;
}
Node::~Node(){
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
    if(leaf_ptr->amount_of_free_space < SLOT_SIZE + val_size){
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
int Node::leaf_append_unsafe(int64_t key, const char* value, uint16_t val_size){
    if(leaf_ptr->amount_of_free_space < SLOT_SIZE + val_size){
        return -1;
    }
    slot_t slot;
    slot.set_key(key);
    slot.set_offset(leaf_ptr->amount_of_free_space + SLOT_SIZE * leaf_ptr -> number_of_keys - val_size);   
    slot.set_size(val_size);
    memcpy(leaf_ptr->data + (leaf_ptr->number_of_keys)*SLOT_SIZE, &slot, sizeof(slot));
    memcpy(leaf_ptr->data + slot.get_offset(), value, sizeof(uint8_t)*val_size);
    leaf_ptr->amount_of_free_space -= SLOT_SIZE + val_size;
    leaf_ptr->number_of_keys += 1;
    return 0;
}
int Node::leaf_find(int64_t key, char* ret_val, uint16_t* val_size){
    //binary search
    int low = 0; 
    int high = leaf_ptr->number_of_keys -1;
    int target;
    while(low<=high){
        target = (low+high)/2;
        slot_t tmp;
        memcpy(&tmp, leaf_ptr->data + target*SLOT_SIZE, sizeof(tmp)); 
        if(tmp.get_key()==key){
            memcpy(ret_val, leaf_ptr->data + tmp.get_offset(), sizeof(uint8_t)* tmp.get_size());
            *val_size = tmp.get_size();
            return 0;
        }
        else if (tmp.get_key() > key){
            high = target-1;
        }
        else {
            low = target+1;
        }
    }
    return -1;

}
void Node::leaf_print_all(){
    for(int i = 0; i<leaf_ptr->number_of_keys; i++){
        slot_t tmp;
        memcpy(&tmp, leaf_ptr->data + i*SLOT_SIZE, sizeof(tmp)); 
        char ret_val[112];
        uint16_t val_size;
        leaf_find(tmp.get_key(), ret_val, &val_size);
        printf("key: %ld, size: %u, offset: %u, value: ",tmp.get_key(), tmp.get_size(), tmp.get_offset());
        for(int i = 0; i<val_size; i++){
            printf("%c",ret_val[i]);
        }
        printf("\n");
    }
    printf("\n");

}
int Node::leaf_find_slot(int64_t key){
    //binary search
    int low = 0; 
    int high = leaf_ptr->number_of_keys -1;
    int target;
    while(low<=high){
        target = (low+high)/2;
        slot_t tmp;
        memcpy(&tmp, leaf_ptr->data + target*SLOT_SIZE, sizeof(tmp)); 
        if(tmp.get_key()==key){
            return 0;
        }
        else if (tmp.get_key() > key){
            high = target-1;
        }
        else {
            low = target+1;
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

void Node::leaf_remove_unsafe(int64_t key){
      
    // remove one slot and pack all the values to the end of the page.
    Node copy = *this;

    // reset the leaf node and re-insert except for the key
    leaf_ptr->number_of_keys = 0;
    leaf_ptr->amount_of_free_space = PAGE_SIZE - PAGE_HEADER_SIZE;
    for(int i = 0; i<copy.leaf_ptr->number_of_keys; i++){
        slot_t tmp;
        //change for leaf_move_slot
        memcpy(&tmp, copy.leaf_ptr->data + i*SLOT_SIZE, sizeof(tmp)); 
        if(tmp.get_key() == key) continue;
        char ret_val[MAX_VALUE_SIZE];
        copy.leaf_move_value(ret_val, tmp.get_size(), tmp.get_offset());
        leaf_append_unsafe(tmp.get_key(), ret_val, tmp.get_size());
    }
    
}
void Node::leaf_pack_values(int from, int to){

    Node leaf_copy = *this;
    // reset the leaf node and re-insert to pack values;
    leaf_ptr->number_of_keys = 0;
    leaf_ptr->amount_of_free_space = PAGE_SIZE - PAGE_HEADER_SIZE;

    for(int i = from; i<=to; i++){
        slot_t tmp;
        //change for leaf_move_slot
        memcpy(&tmp, leaf_copy.leaf_ptr->data + i*SLOT_SIZE, sizeof(tmp)); 
        char ret_val[MAX_VALUE_SIZE];
        leaf_copy.leaf_move_value(ret_val, tmp.get_size(), tmp.get_offset());
        leaf_append_unsafe(tmp.get_key(), ret_val, tmp.get_size());
    }
}
int Node::internal_append_unsafe(int64_t key, pagenum_t pagenum){
    if(internal_ptr->number_of_keys >=MAX_KPN_NUMBER){
        return -1;
    }
    internal_set_kpn_index(key, pagenum, internal_ptr->number_of_keys);
    internal_ptr->number_of_keys += 1;
    return 0;
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
    printf("pagenum: %lu\n", internal_ptr->one_more_page_number);
    for(int i = 0; i<internal_ptr->number_of_keys; i++){
        kpn_t tmp;
        memcpy(&tmp, internal_ptr->data + i*KPN_SIZE, sizeof(tmp)); 
        printf("key: %ld, pagenum: %lu\n", tmp.get_key(), tmp.get_pagenum());
    }
    printf("\n");
}
int Node::internal_find_kpn(int64_t key){
    //binary search
    int low = 0; 
    int high = internal_ptr->number_of_keys -1;
    int target;
    while(low<=high){
        target = (low+high)/2;
        kpn_t tmp;
        memcpy(&tmp, internal_ptr->data + target*KPN_SIZE, sizeof(tmp)); 
        if(tmp.get_key()==key){
            return 0;
        }
        else if (tmp.get_key() > key){
            high = target-1;
        }
        else {
            low = target+1;
        }
    }

    return -1;
}
int Node::internal_insert(int64_t key, pagenum_t pagenum){
    //printf("input key : %d\n", key);
    //printf("internal num key :%d\n", internal_ptr->number_of_keys);
    if(internal_find_kpn(key)==0){
        return -2;
    } 
    if(internal_ptr->number_of_keys >=MAX_KPN_NUMBER){
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

void Node::internal_remove_unsafe(int64_t key){
    bool find_flag = false;
    int i;
    for(i = 0; i<internal_ptr->number_of_keys; i++){
        kpn_t tmp;
        tmp = internal_get_kpn_index(i);
        if(tmp.get_key() == key) break;
    }
    int deletion_point = i;
    for(int k = deletion_point + 1; k<internal_ptr->number_of_keys; k++){
        kpn_t tmp;
        tmp = internal_get_kpn_index(k);
        internal_set_kpn_index(tmp.get_key(), tmp.get_pagenum(), k-1); 
    }
    internal_ptr->number_of_keys -= 1;
}


bool Node::isLeaf(){
    return default_page.is_leaf;
}

pagenum_t get_root_pagenum(int64_t table_id){
    h_page_t header_node;
    file_read_page(table_id, 0, (page_t*) &header_node);      
    return header_node.root_page_number;
}
void set_root_pagenum(int64_t table_id, pagenum_t pagenum){
    h_page_t header_node;
    file_read_page(table_id, 0, (page_t*) &header_node);      
    header_node.root_page_number = pagenum;
    file_write_page(table_id, 0, (page_t*) &header_node);      
}

void print_tree(int64_t table_id, bool pagenum_p){
    printf("================PRINT_TREE===============\n");
    h_page_t header_node;
    file_read_page(table_id, 0, (page_t*) &header_node);      
    if(header_node.root_page_number == 0){
        printf("Empty tree\n");
    } 
    queue<pair<pagenum_t, int>> q; 
    q.push({header_node.root_page_number, 0});
    if(pagenum_p) printf("<%lu>\n", header_node.root_page_number);
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
            if(pagenum_p) printf("<%lu>", target.internal_ptr->one_more_page_number);
            for(int i = 0; i<target.internal_ptr->number_of_keys; i++){
                kpn_t tmp = target.internal_get_kpn_index(i);
                printf("|%ld|",tmp.get_key());
                if(pagenum_p) printf("<%lu>",tmp.get_pagenum());
                q.push({tmp.get_pagenum(), t.second + 1});

            }
            printf(" ");
        }
        else{
            for(int i = 0; i<target.leaf_ptr->number_of_keys; i++){
                slot_t tmp;
                target.leaf_move_slot(&tmp, i);
                printf("{%ld}",tmp.get_key());
            }
            printf(" ");
        }
    }
    printf("\n");
    printf("=========================================\n");
    printf("\n");
}
void print_leaves(int64_t table_id){
    printf("================PRINT_LEAVES===============\n");
    h_page_t header_node;
    file_read_page(table_id, 0, (page_t*) &header_node);      
    if(header_node.root_page_number == 0){
        printf("Empty tree\n");
    } 
    Node cur(table_id, header_node.root_page_number);
    while(!cur.isLeaf()){
        cur = Node(table_id, cur.internal_ptr->one_more_page_number);
    }
    while(cur.leaf_ptr->right_sibling_page_number!=0){
        for(int i = 0; i<cur.leaf_ptr->number_of_keys; i++){
            slot_t tmp;
            cur.leaf_move_slot(&tmp, i);
            printf("{%ld}",tmp.get_key());
        }
        printf(" ");
        cur = Node(table_id, cur.leaf_ptr->right_sibling_page_number);
    }
    for(int i = 0; i<cur.leaf_ptr->number_of_keys; i++){
        slot_t tmp;
        cur.leaf_move_slot(&tmp, i);
        printf("{%ld}",tmp.get_key());
    }
    printf("\n");
    printf("==========================================\n");
    printf("\n");
}


Node find_leaf(int64_t table_id, pagenum_t root, int64_t key){
    //traverse, using node poitner, important!!!!
    /*
    Node* cursor = new Node(table_id, root);
    while(!cursor->isLeaf()){
        int i = 0;
        kpn_t tmp;
        pagenum_t next_pagenum = cursor->internal_ptr->one_more_page_number;
        while (i < cursor->internal_ptr->number_of_keys){
            tmp = cursor->internal_get_kpn_index(i);
            if (key >= tmp.get_key()){
                i++;
                next_pagenum = tmp.get_pagenum();
            }
            else {
                break; 
            }
        }
        delete cursor;
        cursor = new Node(table_id, next_pagenum);
    }
    return *cursor;
    */
    Node cursor(table_id, root);
    while(!cursor.isLeaf()){
        int i = 0;
        kpn_t tmp;
        pagenum_t next_pagenum = cursor.internal_ptr->one_more_page_number;
        while (i < cursor.internal_ptr->number_of_keys){
            tmp = cursor.internal_get_kpn_index(i);
            if (key >= tmp.get_key()){
                i++;
                next_pagenum = tmp.get_pagenum();
            }
            else {
                break; 
            }
        }
        cursor = Node(table_id, next_pagenum);
    }
    return cursor;
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
    //printf("insert_into_parent key: %d\n",key);

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
        cout<<"key collision"<< endl;
        // this must never be the case, key collision
        return -1;
    }
    else {
        //cout << "parent split!" << endl;
        //split parent and insert 
        Node cinternal = parent;
        Node new_internal(false, table_id);
        parent.default_page.number_of_keys = 0;
        new_internal.default_page.parent_page_number = cinternal.default_page.parent_page_number;
        //DPT

        int split_point = cinternal.default_page.number_of_keys / 2;
        //printf("splitpoint! : %d\n",split_point);
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
        if(new_key_flag && key < s.get_key()){
            k_prime = key;
            new_internal.internal_ptr->one_more_page_number = right.pn;
            new_key_flag = false;
        }
        else {
            k_prime = s.get_key();    
            new_internal.internal_ptr->one_more_page_number = s.get_pagenum();
            i+=1;
        }
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

int dbpt_insert(int64_t table_id, int64_t key, const char* value, uint16_t val_size){
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

    int result = target_leaf.leaf_insert(key, value, val_size);
    if(result==0){
        //insert into leaf if the space exists.
        target_leaf.write_to_disk(); 
        return 0;
    }
    else if(result == -2){
        //duplicate key exists.
        return -1;
    }
    else {
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
        int result;
        for(next = 0; next<cleaf.leaf_ptr->number_of_keys; next++){
            slot_t tmp;
            cleaf.leaf_move_slot(&tmp, next); 
            char value_copy[MAX_VALUE_SIZE];
            cleaf.leaf_move_value(value_copy, tmp.get_size(), tmp.get_offset());
            if(new_key_flag && key < tmp.get_key()){
                new_key_flag = false;
                target_leaf.leaf_insert(key, value, val_size);
                acc_size += SLOT_SIZE + tmp.get_size(); 
                if(acc_size > SPLIT_THRESHOLD){
                    break;
                }
            }
            target_leaf.leaf_insert(tmp.get_key(), value_copy, tmp.get_size());
            acc_size += SLOT_SIZE + tmp.get_size(); 
            if(acc_size > SPLIT_THRESHOLD){
                next += 1;
                break;
            }
        }
        for(; next<cleaf.leaf_ptr->number_of_keys; next++){
            slot_t tmp;
            cleaf.leaf_move_slot(&tmp, next); 
            char value_copy[MAX_VALUE_SIZE];
            cleaf.leaf_move_value(value_copy, tmp.get_size(), tmp.get_offset());
            if(new_key_flag && key < tmp.get_key()){
                new_key_flag = false;
                new_leaf.leaf_insert(key, value, val_size);
            }
            new_leaf.leaf_insert(tmp.get_key(), value_copy, tmp.get_size());
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
        return insert_into_parent(table_id, target_leaf, new_leaf, tmp.get_key());
    }
}
void coalesce_nodes(int64_t table_id, Node target, Node neighbor, int neighbor_index, int64_t cur_key_prime, int key_prime_index){
    /* 
     * if the target node is leftmost child in parent, swap target node with neighbor node.
     * Because all the entries of target node can be pushed to neighbor node, all the entries of neighbor node can be pushed to target node.
     */ 
    //printf("colaesce! cur_key_prime: %d, key_prime_index: %d\n",cur_key_prime, key_prime_index);
    
    if(neighbor_index==-1){
        Node tmp = target;
        target = neighbor;
        neighbor = tmp;
    }

    /*
     * case : internal node
     * append all kpn from target node to neighbor node
     */
    if(!target.isLeaf()){
        //neighbor.internal_print_all();
        //target.internal_print_all();
        // first append k_prime
        neighbor.internal_append_unsafe(cur_key_prime, target.internal_ptr->one_more_page_number);
        for(int i = 0; i<target.internal_ptr->number_of_keys; i++){
            kpn_t tmp = target.internal_get_kpn_index(i);
            neighbor.internal_append_unsafe(tmp.get_key(), tmp.get_pagenum());
        }
        // refresh child's parent page number
        for(int i = 0; i<neighbor.internal_ptr->number_of_keys; i++){
            kpn_t tmp = neighbor.internal_get_kpn_index(i);
            Node child(table_id, tmp.get_pagenum());
            child.default_page.parent_page_number = neighbor.pn;
            child.write_to_disk();
        }
    }
    /*
     * case :leaf node
     * append all slots and values from target node to neighbor node
     */
    else {
        //neighbor.leaf_print_all();
        //target.leaf_print_all();
        for(int i = 0; i<target.leaf_ptr->number_of_keys; i++){
            slot_t tmp;
            target.leaf_move_slot(&tmp, i);
            char ret_val[MAX_VALUE_SIZE];
            target.leaf_move_value(ret_val, tmp.get_size(), tmp.get_offset());
            neighbor.leaf_append_unsafe(tmp.get_key(), ret_val, tmp.get_size());
        }
        neighbor.leaf_ptr->right_sibling_page_number = target.leaf_ptr->right_sibling_page_number;
    }

    // modification of nodes finish, and the neighbor node will not be modified again.
    // so write the neighbor node to disk
    neighbor.write_to_disk();
    Node parent(table_id, target.default_page.parent_page_number);
    delete_entry(table_id, parent, cur_key_prime);
    //after deleteing entry from the parent node, free the merged page
    file_free_page(table_id, target.pn);
}

void redistribute_nodes(int64_t table_id, Node parent, Node target, Node neighbor, int neighbor_index, int64_t cur_key_prime, int key_prime_index){
    /*
     * case: neighbor node is the left of the target node
     */ 
    // k prime index is the index of kpn which contains k_prime
    if (neighbor_index != -1){
        //pull entries until it reaches the minimum free space threshold
        if(target.isLeaf()){
            // 아마 일어나진 않겠지만 혹시모르니 1개 키는 남겨두도록 하자
            for(int i = neighbor.leaf_ptr->number_of_keys - 1; i>0; i--){
                slot_t tmp;
                neighbor.leaf_move_slot(&tmp, i);
                char ret_val[MAX_VALUE_SIZE];
                neighbor.leaf_move_value(ret_val, tmp.get_size(), tmp.get_offset());
                neighbor.leaf_ptr->number_of_keys -= 1;
                
                target.leaf_insert(tmp.get_key(), ret_val, tmp.get_size());
                if(target.leaf_ptr->amount_of_free_space < DELETE_MODIFICATION_THRESHOLD){
                    break;
                }
            }
            neighbor.leaf_pack_values(0, neighbor.leaf_ptr->number_of_keys -1);
            slot_t tmp;
            target.leaf_move_slot(&tmp, 0);
            kpn_t kpn = parent.internal_get_kpn_index(key_prime_index);
            parent.internal_set_kpn_index(tmp.get_key(), kpn.get_pagenum(), key_prime_index);
        }
        else {
            for(int i = neighbor.internal_ptr->number_of_keys -1; i>0; i--){
                kpn_t rightmost_kpn = neighbor.internal_get_kpn_index(i);
                kpn_t new_kpn;
                new_kpn.set_key(cur_key_prime);
                new_kpn.set_pagenum(target.internal_ptr->one_more_page_number);
                cur_key_prime = rightmost_kpn.get_key();
                target.internal_ptr->one_more_page_number = rightmost_kpn.get_pagenum();
                target.internal_insert(new_kpn.get_key(), new_kpn.get_pagenum());

                Node new_child(table_id, rightmost_kpn.get_pagenum());
                new_child.default_page.parent_page_number = target.pn;
                new_child.write_to_disk();

                neighbor.internal_ptr->number_of_keys -= 1;
                if(PAGE_SIZE - PAGE_HEADER_SIZE - (int)(target.internal_ptr->number_of_keys) * KPN_SIZE < DELETE_MODIFICATION_THRESHOLD){
                    break;
                }
            }
            kpn_t kpn = parent.internal_get_kpn_index(key_prime_index);
            kpn.set_key(cur_key_prime);
            parent.internal_set_kpn_index(kpn.get_key(), kpn.get_pagenum(), key_prime_index);
            
        }
    } 
    else {
        if(target.isLeaf()){
            int i;
            for(i = 0; i<neighbor.leaf_ptr->number_of_keys; i++){
                slot_t tmp;
                neighbor.leaf_move_slot(&tmp, i);
                char ret_val[MAX_VALUE_SIZE];
                neighbor.leaf_move_value(ret_val, tmp.get_size(), tmp.get_offset());

                target.leaf_append_unsafe(tmp.get_key(),ret_val,tmp.get_size());
                if(target.leaf_ptr->amount_of_free_space < DELETE_MODIFICATION_THRESHOLD){
                    break;
                } 
            }
            neighbor.leaf_pack_values(i+1, neighbor.leaf_ptr->number_of_keys-1);
            slot_t tmp;
            neighbor.leaf_move_slot(&tmp, 0);
            kpn_t kpn = parent.internal_get_kpn_index(key_prime_index);
            parent.internal_set_kpn_index(tmp.get_key(), kpn.get_pagenum(), key_prime_index);
        }
        else {
            int i;
            for(i = 0; i<neighbor.leaf_ptr->number_of_keys -1; i++){
                kpn_t leftmost_kpn = neighbor.internal_get_kpn_index(i);
                kpn_t new_kpn;
                new_kpn.set_key(cur_key_prime);
                new_kpn.set_pagenum(neighbor.internal_ptr->one_more_page_number);
                cur_key_prime = leftmost_kpn.get_key();
                neighbor.internal_ptr->one_more_page_number = leftmost_kpn.get_pagenum(); 
                target.internal_append_unsafe(new_kpn.get_key(), new_kpn.get_pagenum());

                Node child(table_id, new_kpn.get_pagenum());
                child.default_page.parent_page_number = target.pn;
                child.write_to_disk();

                if(PAGE_SIZE - PAGE_HEADER_SIZE - (int)(target.internal_ptr->number_of_keys) * KPN_SIZE < DELETE_MODIFICATION_THRESHOLD){
                    break;
                } 
            }    
            Node copy = neighbor;
            neighbor.internal_ptr->number_of_keys = 0;
            for(int k = i+1; k<copy.internal_ptr->number_of_keys; k++){
                kpn_t tmp = copy.internal_get_kpn_index(k);
                neighbor.internal_append_unsafe(tmp.get_key(), tmp.get_pagenum());    
            }
            kpn_t kpn = parent.internal_get_kpn_index(key_prime_index);
            kpn.set_key(cur_key_prime);
            parent.internal_set_kpn_index(kpn.get_key(), kpn.get_pagenum(), key_prime_index);
            
        }
    }

    parent.write_to_disk();
    neighbor.write_to_disk();
    target.write_to_disk();
}

void delete_entry(int64_t table_id, Node target, int64_t key){
    //printf("delete entry! key: %d\n",key);
    // remove slot and value 
    if(target.isLeaf()==1){
        //printf("target node\n");
        //target.leaf_print_all();
        target.leaf_remove_unsafe(key);
    }
    else {
        //printf("target node\n");
        //target.internal_print_all();
        target.internal_remove_unsafe(key); 
    }
    //can this be optimized?
    target.write_to_disk();

    // case : target node is the root 
    if(target.pn == get_root_pagenum(table_id)){
        // case : nonempty root
        if (target.default_page.number_of_keys > 0){
            return;
        }
        
        // case : empty root
        // if the node is internal node which means that there are only one child in the root
        if (!target.isLeaf()){
            Node child(table_id, target.internal_ptr->one_more_page_number);
            child.default_page.parent_page_number = 0;
            set_root_pagenum(table_id, child.pn); 
            child.write_to_disk();
            file_free_page(table_id, target.pn);
        }
        //if the node is leaf node which means only one node exists in the tree. 
        else {
            file_free_page(table_id, target.pn);
            set_root_pagenum(table_id, 0);
        }
        return;
    }

    uint64_t free_space;
    if(target.isLeaf()){
        free_space = target.leaf_ptr->amount_of_free_space;
    }
    else {
        free_space = ( MAX_KPN_NUMBER - target.internal_ptr->number_of_keys ) * KPN_SIZE;
    }

    if(free_space < DELETE_MODIFICATION_THRESHOLD){
        return;
    }

    /*
     * node should be merged or if impossible, redistribute.
     */

    //find neighbor 이부분에 오류가 있을수도 있다.
    Node parent(table_id, target.default_page.parent_page_number);
    //parent is always internal node not leaf.
    int i;
    if(parent.internal_ptr->one_more_page_number == target.pn){
        i = -1;
    }
    else {
        for(i = 0; i<parent.internal_ptr->number_of_keys; i++){
            kpn_t tmp = parent.internal_get_kpn_index(i);
            if(tmp.get_pagenum() == target.pn){
                break;
            }
        }
    }
    int neighbor_index = i;
    kpn_t tmp = parent.internal_get_kpn_index(neighbor_index);
    pagenum_t neighbor_pagenum;
    if(neighbor_index == -1){
        neighbor_pagenum = parent.internal_get_kpn_index(0).get_pagenum();
    }
    else {
        //neighbor pagenum is stored in i-1
        if(neighbor_index == 0){
            neighbor_pagenum = parent.internal_ptr->one_more_page_number;
        }
        else {
            kpn_t tmp = parent.internal_get_kpn_index(neighbor_index-1);
            neighbor_pagenum = tmp.get_pagenum();
        }
    }
    // key_prime_index is the index of the kpn which should store the key_prime.
    int key_prime_index = neighbor_index == -1 ? 0 : neighbor_index;
    int cur_key_prime = parent.internal_get_kpn_index(key_prime_index).get_key();

    // coalescence or redistribution
    Node neighbor(table_id, neighbor_pagenum);
    // re-think about the possibility of merge;
    bool can_be_merged = false;
    if(target.isLeaf()){
        if((int)(neighbor.leaf_ptr->amount_of_free_space) >= (PAGE_SIZE - PAGE_HEADER_SIZE - (int)(target.leaf_ptr->amount_of_free_space)))
            can_be_merged = true;
    }
    else {
        //there should be one more kpn space because new kpn that has k_prime should be inserted.
        if((MAX_KPN_NUMBER - (int)(neighbor.internal_ptr->number_of_keys) - 1) >= (int)(target.internal_ptr->number_of_keys))
            can_be_merged = true;
    }
    if(can_be_merged){
        coalesce_nodes(table_id, target, neighbor, neighbor_index, cur_key_prime, key_prime_index);
    }
    else {
        redistribute_nodes(table_id, parent, target, neighbor, neighbor_index, cur_key_prime, key_prime_index);
    }


}

int dbpt_delete(int64_t table_id, int64_t key){
    h_page_t header_node;
    file_read_page(table_id, 0, (page_t*) &header_node);      

    if(header_node.root_page_number==0){
        return -1;
    }
    Node leaf = find_leaf(table_id, header_node.root_page_number, key);
    if(leaf.leaf_find_slot(key)==0){
        delete_entry(table_id, leaf, key);
        return 0;
    }
    return -1;
}

int dbpt_scan(int64_t table_id, int64_t begin_key, int64_t end_key, std::vector<int64_t>* keys, std::vector<char*>* values, std::vector<uint16_t>* val_sizes, std::vector<char*>* allocated_memory_ptr){
    h_page_t header_node;
    file_read_page(table_id, 0, (page_t*) &header_node);      
    
    if(header_node.root_page_number == 0){
        return -1;
    } 
    if(begin_key > end_key) return -1;
    Node target = find_leaf(table_id, header_node.root_page_number, begin_key); 
    int idx;
    for(idx = 0; idx<target.leaf_ptr->number_of_keys; idx++){
        slot_t tmp;
        memcpy(&tmp, target.leaf_ptr->data + idx*SLOT_SIZE, sizeof(tmp));
        if(tmp.get_key()>=begin_key) break;
    }
    if(idx == target.leaf_ptr->number_of_keys){
        if(target.leaf_ptr->right_sibling_page_number == 0){
            return -1;
        }
        else {
            idx = 0;
            target = Node(table_id, target.leaf_ptr->right_sibling_page_number);
        }
    }
    bool exit_flag = false;
    while(!exit_flag){
        for(; idx<target.leaf_ptr->number_of_keys; idx++){
            slot_t tmp;
            memcpy(&tmp, target.leaf_ptr->data + idx*SLOT_SIZE, sizeof(tmp));
            if(tmp.get_key() > end_key){
                exit_flag = true;
                break;
            }
            keys->push_back(tmp.get_key());
            char* ptr = new char[tmp.get_size()]; 
            target.leaf_move_value(ptr, tmp.get_size(), tmp.get_offset());
            values->push_back(ptr); 
            val_sizes->push_back(tmp.get_size()); 
            allocated_memory_ptr->push_back(ptr);
        }
        if(target.leaf_ptr->right_sibling_page_number == 0){
            exit_flag = true;
        }
        target = Node(table_id, target.leaf_ptr->right_sibling_page_number);
        idx = 0;
    }
    return 0;
}

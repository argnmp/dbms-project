#include "log.h"
#include "trx.h"
#include <cstdlib>

LOG_MANAGER log_manager;

int LOG_MANAGER::init_lm(char* log_path, char* logmsg_path){
    int g_result = 0;

    //initialize lob_buffer struct
    pthread_mutexattr_t mtx_attribute;
    g_result = pthread_mutexattr_init(&mtx_attribute);
    g_result = pthread_mutexattr_settype(&mtx_attribute, PTHREAD_MUTEX_NORMAL);
    g_result = pthread_mutex_init(&lb.log_buffer_latch, &mtx_attribute);
    lb.next_offset = 0;
    lb.g_lsn = 1;
    memset(lb.data, 0, sizeof(lb.data));

    
    log_record current_record;
    log_fd = open(log_path, O_RDWR | O_SYNC );
    if(log_fd == -1){
        log_fd = open(log_path, O_RDWR | O_CREAT | O_SYNC, 0644);
        
        if(log_fd==-1) g_result = -1;
    }
    else {
        while(1){
            int read_bytes = pread(log_fd, &current_record, sizeof(log_record), lb.next_offset);
            if(read_bytes == 0){
                lb.g_lsn = current_record.lsn + 1; 
                break;
            }

            trx_table.g_trx_id = std::max(current_record.transaction_id, trx_table.g_trx_id);

            memcpy(lb.data + lb.next_offset, &current_record, sizeof(log_record));
            lb.next_offset += sizeof(log_record);
             
        }
        trx_table.g_trx_id += 1;
        
    }

    log_msg_fd = open(logmsg_path, O_RDWR | O_SYNC );
    if(log_msg_fd == -1){
        log_msg_fd = open(logmsg_path, O_RDWR | O_CREAT | O_SYNC, 0644);
        if(log_msg_fd==-1) g_result = -1;
    }
    else {
    }
    log_fp = fdopen(log_fd, "w");
    log_msg_fp = fdopen(log_msg_fd, "w");

    if (g_result!=0) return -1;
    return 0;
}


void LOG_MANAGER::acquire_lb_latch(){
    int result = pthread_mutex_lock(&lb.log_buffer_latch); 
    if(result != 0) {
        perror("lb latch lock failed: ");
        exit(EXIT_FAILURE);
    }
}


void LOG_MANAGER::release_lb_latch(){
    int result = pthread_mutex_unlock(&lb.log_buffer_latch); 
    if(result != 0) {
        perror("lb latch lock failed: ");
        exit(EXIT_FAILURE);
    }
}
log_record LOG_MANAGER::init_log_record(){
    log_record lr;
    lr.log_size = sizeof(log_record);
    lr.lsn = 0;
    lr.prev_lsn = 0;
    lr.transaction_id = 0;
    lr.type = 9999;
    lr.table_id = 0;
    lr.page_number = 0; 
    lr.offset = 0;
    lr.data_length = 0;
    memset(&lr.old_image, 0, sizeof(lr.old_image));
    memset(&lr.new_image, 0, sizeof(lr.new_image));
    lr.next_undo_lsn = 0;
    return lr;
}
int LOG_MANAGER::write_lb_023(uint32_t xid, uint32_t type){
    //printf("write_lb_023 begin\n");
    acquire_lb_latch(); 
    
    log_record new_record = init_log_record();
    new_record.lsn = lb.g_lsn;

    new_record.prev_lsn = trx_table.trx_map[xid].last_lsn;
    trx_table.trx_map[xid].last_lsn = lb.g_lsn;

    lb.g_lsn+=1;
    new_record.transaction_id = xid;
    new_record.type = type;

    memcpy(lb.data + lb.next_offset, &new_record, sizeof(new_record));
    lb.next_offset += sizeof(new_record);

    release_lb_latch();
    //printf("write_lb_023 end\n");
    return 0;
}
uint64_t LOG_MANAGER::write_lb_14(uint32_t xid, uint32_t type, uint64_t tid, uint64_t page_number, uint16_t offset, uint16_t data_length, uint8_t *old_image, uint8_t *new_image, uint64_t next_undo_lsn){
    uint64_t page_lsn;
    acquire_lb_latch();
    {
        log_record new_record = init_log_record();
        new_record.lsn = lb.g_lsn;
        page_lsn = lb.g_lsn;
        new_record.prev_lsn = trx_table.trx_map[xid].last_lsn;
        trx_table.trx_map[xid].last_lsn = lb.g_lsn;
        lb.g_lsn+=1;

        new_record.transaction_id = xid;
        new_record.type = type;
        new_record.table_id = tid;
        new_record.page_number = page_number;
        new_record.offset = offset;
        new_record.data_length = data_length;
        memcpy(new_record.old_image, old_image, sizeof(uint8_t)* data_length);
        memcpy(new_record.new_image, new_image, sizeof(uint8_t)* data_length);
        new_record.next_undo_lsn = next_undo_lsn;

        memcpy(lb.data + lb.next_offset, &new_record, sizeof(new_record));
        lb.next_offset += sizeof(new_record);
    }
    release_lb_latch();

    return page_lsn;
}
int LOG_MANAGER::flush_lb(){
    acquire_lb_latch();
   
    int result = pwrite(log_fd, lb.data, lb.next_offset, 0);
    sync();

    release_lb_latch();
    return 0;
}

void LOG_MANAGER::show_lb_buffer(){
    acquire_lb_latch();

    printf("%10s %10s %10s %10s %10s %10s %10s %10s %10s %10s %10s %10s\n", "type", "log_size", "lsn", "prev_lsn", "xid","tid", "pn", "offset", "d_length", "n_u_lsn", "old_img", "new_img");
    int offset = 0;
    while(1){
        log_record current_record;
        memcpy(&current_record, lb.data+offset, sizeof(log_record));
        if(current_record.log_size!=320) break;
        
        string tp;
        switch(current_record.type){
            case 0:
                tp = "begin";
                break;
            case 1:
                tp = "update";
                break;
            case 2:
                tp = "commit";
                break;
            case 3:
                tp = "rollback";
                break;
            case 4:
                tp = "compensate";
                break;
        }
        string old_value(current_record.old_image, current_record.old_image + current_record.data_length);
        string new_value(current_record.new_image, current_record.new_image + current_record.data_length);

        printf("%10s %10d %10d %10d %10d %10d %10d %10d %10d %10d %s %s\n",tp.c_str(), current_record.log_size, current_record.lsn, current_record.prev_lsn, current_record.transaction_id, current_record.table_id, current_record.page_number, current_record.offset, current_record.data_length, current_record.next_undo_lsn, old_value.c_str(), new_value.c_str() );
        offset += sizeof(log_record);
    }

    release_lb_latch();
}

void LOG_MANAGER::ANALYZE(){
    fprintf(log_msg_fp, "[ANALYSIS] Analysis pass start\n");
    acquire_lb_latch(); 
    {
        int offset = 0;
        log_record current_record;
        while(offset != lb.next_offset){
            memcpy(&current_record, lb.data+offset, sizeof(log_record));
            if(current_record.type == 0){
                loser_trx_id.insert(current_record.transaction_id);
            }
            else if(current_record.type == 2 || current_record.type == 3){
                if(loser_trx_id.find(current_record.transaction_id) != loser_trx_id.end()){
                    loser_trx_id.erase(current_record.transaction_id);
                    winner_trx_id.insert(current_record.transaction_id);
                } 
            }
            offset += sizeof(log_record);
        }
        
    }
    release_lb_latch();
    fprintf(log_msg_fp, "[ANALYSIS] Analysis success. Winner:");
    for(int trx_id: winner_trx_id){
        fprintf(log_msg_fp, " %d", trx_id);
    }
    fprintf(log_msg_fp, ", Loser:");
    for(int trx_id: loser_trx_id){
        fprintf(log_msg_fp, " %d", trx_id);
    }
    fprintf(log_msg_fp, "\n");
}
void LOG_MANAGER::REDO(bool crash_flag, int* acc_log_num, int limit_log_num){
    fprintf(log_msg_fp, "[REDO] Redo pass start\n");
    *acc_log_num += 1; if(*acc_log_num==limit_log_num && crash_flag == true) return;
     
    acquire_lb_latch(); 
    {
        int offset = 0;
        log_record current_record;
        while(offset != lb.next_offset){
            memcpy(&current_record, lb.data+offset, sizeof(log_record));
            switch(current_record.type){
                case 0:
                    fprintf(log_msg_fp, "LSN %lu [BEGIN] Transaction id %d\n", current_record.lsn, current_record.transaction_id);
                    *acc_log_num += 1; if(*acc_log_num==limit_log_num && crash_flag == true){
                        release_lb_latch();
                        return;
                    }
                    break;
                case 1:
                case 4: {
                            Node target_node(current_record.table_id, current_record.page_number);
                            if(current_record.lsn <= target_node.default_page.page_lsn) {

                                fprintf(log_msg_fp, "LSN %lu [CONSIDER-REDO] Transaction id %d\n", current_record.lsn, current_record.transaction_id);
                                *acc_log_num += 1; if(*acc_log_num==limit_log_num && crash_flag == true){
                                    buf_unpin(current_record.table_id, current_record.page_number);
                                    release_lb_latch();
                                    return;
                                }

                            }
                            else{

                                uint16_t old_val_size;

                                target_node.leaf_update_using_offset(current_record.offset, (char*)current_record.new_image, current_record.data_length, &old_val_size);
                                target_node.write_to_disk();

                                fprintf(log_msg_fp, "LSN %lu [UPDATE] Transaction id %d redo apply\n", current_record.lsn, current_record.transaction_id);
                                *acc_log_num += 1; if(*acc_log_num==limit_log_num && crash_flag == true) {
                                    buf_unpin(current_record.table_id, current_record.page_number);
                                    release_lb_latch();
                                    return;
                                }

                            }
                            buf_unpin(current_record.table_id, current_record.page_number);
                            break;
                        }
                case 2:
                    fprintf(log_msg_fp, "LSN %lu [COMMIT] Transaction id %d\n", current_record.lsn, current_record.transaction_id);
                    *acc_log_num += 1; if(*acc_log_num==limit_log_num && crash_flag == true){
                        release_lb_latch();
                        return;
                    }
                    break;
                case 3:
                    fprintf(log_msg_fp, "LSN %lu [ROLLBACK] Transaction id %d\n", current_record.lsn, current_record.transaction_id);
                    *acc_log_num += 1; if(*acc_log_num==limit_log_num && crash_flag == true){
                        release_lb_latch();
                        return;
                    }
                    break;
            }
            offset += sizeof(log_record);
        }
        
    }
    release_lb_latch();

    fprintf(log_msg_fp, "[REDO] Redo pass end\n");
    *acc_log_num += 1; if(*acc_log_num==limit_log_num && crash_flag == true) return;
}
void LOG_MANAGER::UNDO(bool crash_flag, int* acc_log_num, int limit_log_num){
    fprintf(log_msg_fp, "[UNDO] Undo pass start\n");
    *acc_log_num += 1; if(*acc_log_num==limit_log_num && crash_flag == true) return;
    std::unordered_map<int32_t, uint64_t> next_undo_lsn_map;
        
    acquire_lb_latch();
    {
        int offset = lb.next_offset - sizeof(log_record);
        log_record current_record;
        while(offset >= 0){
            if(!(current_record.type==0 or current_record.type==1 or current_record.type==4)){
                offset -= sizeof(log_record);
                continue;
            }
            memcpy(&current_record, lb.data+offset, sizeof(log_record));

            if(loser_trx_id.find(current_record.transaction_id)!=loser_trx_id.end()){

                if(next_undo_lsn_map.find(current_record.transaction_id)!=next_undo_lsn_map.end()){
                    if(next_undo_lsn_map[current_record.transaction_id]<current_record.lsn){
                        offset -= sizeof(log_record);
                        continue;
                    }
                    else{
                        if(current_record.next_undo_lsn!=0){
                            next_undo_lsn_map[current_record.transaction_id] = current_record.next_undo_lsn;
                            offset -= sizeof(log_record);
                            continue;
                        }
                    }
                }
                else{
                    if(current_record.next_undo_lsn!=0){
                        next_undo_lsn_map.insert({current_record.transaction_id, current_record.next_undo_lsn});
                        offset -= sizeof(log_record);
                        continue;
                    }
                }

                //when undo met begin log
                if(current_record.type == 0){
                    write_lb_023(current_record.transaction_id, 3);
                    loser_trx_id.erase(current_record.type);
                    fprintf(log_msg_fp, "LSN %lu [BEGIN] Transaction id %d\n", current_record.lsn, current_record.transaction_id);
                    *acc_log_num += 1; if(*acc_log_num==limit_log_num && crash_flag == true){
                        release_lb_latch();
                        return;
                    }
                }
                else if(current_record.type == 1){
                    if(next_undo_lsn_map.find(current_record.transaction_id)!=next_undo_lsn_map.end()){
                        if(next_undo_lsn_map[current_record.transaction_id]<current_record.lsn){
                            offset -= sizeof(log_record);
                            continue;
                        }
                        else{
                            if(current_record.next_undo_lsn!=0){
                                next_undo_lsn_map[current_record.transaction_id] = current_record.next_undo_lsn;
                                offset -= sizeof(log_record);
                                continue;
                            }
                        }
                    }
                    else{
                        if(current_record.next_undo_lsn!=0){
                            next_undo_lsn_map.insert({current_record.transaction_id, current_record.next_undo_lsn});
                            offset -= sizeof(log_record);
                            continue;
                        }
                    }
                    uint16_t old_val_size;
                    Node target_node(current_record.table_id, current_record.page_number);
                    target_node.leaf_update_using_offset(current_record.offset, (char*)current_record.old_image, current_record.data_length, &old_val_size);
                    buf_unpin(current_record.table_id, current_record.page_number);

                    write_lb_14(current_record.transaction_id, 4, current_record.table_id, current_record.page_number, current_record.offset, current_record.data_length, current_record.new_image, current_record.old_image, current_record.prev_lsn);

                    fprintf(log_msg_fp, "LSN %lu [UPDATE] Transaction id %d undo apply\n", current_record.lsn, current_record.transaction_id);
                    *acc_log_num += 1; if(*acc_log_num==limit_log_num && crash_flag == true){
                        release_lb_latch();
                        return;
                    }
                }
                else if(current_record.type==4){
                    if(next_undo_lsn_map.find(current_record.transaction_id)!=next_undo_lsn_map.end()){
                        if(next_undo_lsn_map[current_record.transaction_id]<current_record.lsn){
                            offset -= sizeof(log_record);
                            continue;
                        }
                        else{
                            if(current_record.next_undo_lsn!=0){
                                next_undo_lsn_map[current_record.transaction_id] = current_record.next_undo_lsn;
                                offset -= sizeof(log_record);
                                continue;
                            }
                        }
                    }
                    else{
                        if(current_record.next_undo_lsn!=0){
                            next_undo_lsn_map.insert({current_record.transaction_id, current_record.next_undo_lsn});
                            offset -= sizeof(log_record);
                            continue;
                        }
                    }
                    uint16_t old_val_size;
                    Node target_node(current_record.table_id, current_record.page_number);
                    target_node.leaf_update_using_offset(current_record.offset, (char*)current_record.old_image, current_record.data_length, &old_val_size);
                    buf_unpin(current_record.table_id, current_record.page_number);

                    write_lb_14(current_record.transaction_id, 4, current_record.table_id, current_record.page_number, current_record.offset, current_record.data_length, current_record.new_image, current_record.old_image, current_record.prev_lsn);

                    fprintf(log_msg_fp, "LSN %lu [CLR] next undo lsn %lu\n", current_record.lsn, current_record.next_undo_lsn);
                    *acc_log_num += 1; if(*acc_log_num==limit_log_num && crash_flag == true){
                        release_lb_latch();
                        return;
                    }
                }
            }

            
            offset -= sizeof(log_record);

        }
    }
    release_lb_latch();
    fprintf(log_msg_fp, "[UNDO] Undo pass end\n");
    *acc_log_num += 1; if(*acc_log_num==limit_log_num && crash_flag == true) return;
}

void LOG_MANAGER::ABORT(int trx_id){
    acquire_lb_latch();
    {
        int offset = lb.next_offset - sizeof(log_record);      
        log_record current_record;
        while(offset >= 0){
            if(current_record.transaction_id != trx_id){
                offset -= sizeof(log_record);
                continue;
            }
            memcpy(&current_record, lb.data+offset, sizeof(log_record));
            if(current_record.type == 0){
                write_lb_023(current_record.transaction_id, 3);
                break;
            }

            if(current_record.type == 1){
            printf("found\n");
                uint16_t old_val_size;
                Node target_node(current_record.table_id, current_record.page_number);

                target_node.leaf_update_using_offset(current_record.offset, (char*)current_record.old_image, current_record.data_length, &old_val_size);

                buf_unpin(current_record.table_id, current_record.page_number);

                write_lb_14(current_record.transaction_id, 4, current_record.table_id, current_record.page_number, current_record.offset, current_record.data_length, current_record.new_image, current_record.old_image, current_record.prev_lsn);

            } 
            
            offset -= sizeof(log_record);
        }
    }
    release_lb_latch();
}

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
    
    int log_fd = open(log_path, O_RDWR | O_SYNC );
    if(log_fd == -1){
        log_fd = open(log_path, O_RDWR | O_CREAT | O_SYNC, 0644);
        if(log_fd==-1) g_result = -1;
    }
    else {
    }

    int log_msg_fd = open(logmsg_path, O_RDWR | O_SYNC );
    if(log_msg_fd == -1){
        log_msg_fd = open(logmsg_path, O_RDWR | O_CREAT | O_SYNC, 0644);
        if(log_msg_fd==-1) g_result = -1;
    }
    else {
    }

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
int LOG_MANAGER::write_lb_14(uint32_t xid, uint32_t type, uint64_t tid, uint64_t page_number, uint16_t offset, uint16_t data_length, uint8_t *old_image, uint8_t *new_image, uint64_t next_undo_lsn){
    acquire_lb_latch();

    log_record new_record = init_log_record();
    new_record.lsn = lb.g_lsn;
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
    
    release_lb_latch();

    return 0;
}

void LOG_MANAGER::show_lb_buffer(){
    acquire_lb_latch();

    printf("%10s %10s %10s %10s %10s %10s %10s %10s %10s %10s %10s\n", "type", "log_size", "lsn", "prev_lsn", "xid","tid", "pn", "d_length", "n_u_lsn", "old_img", "new_img");
    int offset = 0;
    while(1){
        log_record current_record;
        memcpy(&current_record, lb.data+offset, sizeof(log_record));
        if(current_record.log_size==0) break;
        
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

        printf("%10s %10d %10d %10d %10d %10d %10d %10d %10d %s %s\n",tp.c_str(), current_record.log_size, current_record.lsn, current_record.prev_lsn, current_record.transaction_id, current_record.table_id, current_record.page_number, current_record.data_length, current_record.next_undo_lsn, old_value.c_str(), new_value.c_str() );
        offset += sizeof(log_record);
    }

    release_lb_latch();
}

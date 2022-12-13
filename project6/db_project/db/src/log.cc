#include "log.h"
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
int LOG_MANAGER::write_lb_023(uint32_t type, uint32_t xid, uint64_t tid){
    acquire_lb_latch(); 


    release_lb_latch();
    return 0;
}

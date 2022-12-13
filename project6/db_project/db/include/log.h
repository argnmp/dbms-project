#ifndef LOG_H_
#define LOG_H_

#include <stdio.h>
#include <unistd.h>
#include <fcntl.h>
#include <stdint.h>
#include <pthread.h>
#include <string.h>
#include <cerrno>

#define LOG_BUFFER_SIZE 4096

typedef struct log_buffer log_buffer;
typedef struct log_record log_record;

struct log_buffer{
    pthread_mutex_t log_buffer_latch;
    int next_offset;
    uint8_t data[LOG_BUFFER_SIZE];
};

class LOG_MANAGER {
    private:
        log_buffer lb;
        int log_fd;
        int log_msg_fd;
        
    public:
        int init_lm(char* log_path, char* logmsg_path);
        void acquire_lb_latch();
        void release_lb_latch();
        int write_lb_023(uint32_t type, uint32_t xid, uint64_t tid);

};

extern LOG_MANAGER log_manager;

struct log_record {
    uint32_t log_size;
    uint64_t lsn;
    uint64_t prev_lsn;
    uint32_t transaction_id;
    /*
     * begin: 0
     * update: 1
     * commit: 2
     * rollback: 3
     * compensate: 4,
     */
    uint32_t type;
    uint64_t table_id;
    uint64_t page_number;
    uint16_t offset; 
    uint16_t data_length;
    uint8_t old_image[130];
    uint8_t new_image[130];
    uint64_t next_undo_lsn;
};

#endif

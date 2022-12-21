#ifndef LOG_H_
#define LOG_H_

#include <stdio.h>
#include <unistd.h>
#include <fcntl.h>
#include <stdint.h>
#include <pthread.h>
#include <string.h>
#include <cerrno>
#include <algorithm>
#include <set>


#define LOG_BUFFER_SIZE 1000000000

typedef struct log_buffer log_buffer;
typedef struct log_record log_record;

struct log_buffer{
    pthread_mutex_t log_buffer_latch;
    uint64_t g_lsn;
    int next_offset;
    uint8_t data[LOG_BUFFER_SIZE];
};

class LOG_MANAGER {
    private:
        log_buffer lb;
        int log_fd;
        int log_msg_fd;
        FILE* log_fp;
        FILE* log_msg_fp;
        std::set<int32_t> winner_trx_id;
        std::set<int32_t> loser_trx_id;
        
    public:
        int init_lm(char* log_path, char* logmsg_path);

        // need for function that already acquired trx_table_latch
        void acquire_lb_latch();
        void release_lb_latch();

        log_record init_log_record();

        int write_lb_023(uint32_t xid, uint32_t type);
        int write_lb_023_without_lock(uint32_t xid, uint32_t type);
        uint64_t write_lb_14(uint32_t xid, uint32_t type, uint64_t tid, uint64_t page_number, uint16_t offset, uint16_t data_length, uint8_t* old_image, uint8_t* new_image, uint64_t next_undo_lsn);
        uint64_t write_lb_14_without_lock(uint32_t xid, uint32_t type, uint64_t tid, uint64_t page_number, uint16_t offset, uint16_t data_length, uint8_t* old_image, uint8_t* new_image, uint64_t next_undo_lsn);
        int flush_lb();

        void show_lb_buffer();

        void ANALYZE();
        void REDO(bool crash_flag, int* acc_log_num, int limit_log_num);
        void UNDO(bool crash_flag, int* acc_log_num, int limit_log_num);
        void ABORT(int trx_id);
};

extern LOG_MANAGER log_manager;

struct log_record {
    uint32_t log_size;
    uint64_t lsn;
    // if prev_lsn == 0, it is first log record of the transaction
    uint64_t prev_lsn;
    int32_t transaction_id;
    /*
     * begin: 0
     * update: 1
     * commit: 2
     * rollback: 3
     * compensate: 4
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

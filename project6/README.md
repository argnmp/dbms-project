# Project6

## Introduction
This Project6 wiki page describes the design and implementation of logging and recovery module.

## Design
In my design, this module is managed by one global instance(log_manager) of `LOG_MANAGER` class.
```cpp
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
```

### Common for Logging and Recovery
When the database is initialized, `init_lm` function is called. Basically, this function initialized the variables declared in the class. More important part is that it creates log data file and log message file if not exists. 
If log data file exists, it loads all log data into `log_buffer lb` and calculates the next log sequence number and transaction id. Also, it updates `next_offset` variable in log buffer.

### Logging
For logging, this design uses `log_buffer` struct that is a member of `LOG_MANAGER` class. 
```cpp
struct log_buffer{
    pthread_mutex_t log_buffer_latch;
    uint64_t g_lsn;
    int next_offset;
    uint8_t data[LOG_BUFFER_SIZE];
};
```
There are 4 variables in this structure.
* log_buffer_latch: this mutex is used to lock entire structure (log buffer) that can be accessed by many other threads concurrently.
* g_lsn: Because all log sequence number should be unique and used sequentially regardless of transaction, so this variable stores the next sequential number which would be used for next log.
* data: this is the space where real log records are stored sequentially from front to back.
* next_offset: because all log records should be also stored sequentially in `data`, so this variable points to the offset in `data` where next log record should resides in.

Using this structure, these 4 functions writes each log records int to log buffer.
```cpp
        int write_lb_023(uint32_t xid, uint32_t type);
        int write_lb_023_without_lock(uint32_t xid, uint32_t type);
        uint64_t write_lb_14(uint32_t xid, uint32_t type, uint64_t tid, uint64_t page_number, uint16_t offset, uint16_t data_length, uint8_t* old_image, uint8_t* new_image, uint64_t next_undo_lsn);
        uint64_t write_lb_14_without_lock(uint32_t xid, uint32_t type, uint64_t tid, uint64_t page_number, uint16_t offset, uint16_t data_length, uint8_t* old_image, uint8_t* new_image, uint64_t next_undo_lsn);
```

There are 5 types of log record:
```cpp
    /*
     * begin: 0
     * update: 1
     * commit: 2
     * rollback: 3
     * compensate: 4
     */
```
`write_lb_023` is used to write log record of *begin*, *rollback*, *compensate* and `write_lb_14` is used to write log record of *update*, *compensate*. The reason why they are grouped by 2 is that the necessary field of each log record is different.

`write_lb_023_without_lock` and `write_lb_14_without_lock` is used for the situation when the lock of log buffer must be not acquired. So, basically `write_lb_023` and `write_lb_14` acquire lock of log buffer first.

```cpp
        int flush_lb();
```
According to WAL rule, log buffer should be flushed into the disk. `flush_lb` is used for that situation.

### Recovery
There are 3 phases for recovery protocol.

```cpp
        void ANALYZE();
        void REDO(bool crash_flag, int* acc_log_num, int limit_log_num);
        void UNDO(bool crash_flag, int* acc_log_num, int limit_log_num);
```

First phase is analyze and `ANALYZE` function does this work. This function pairs up the same transaction log records *begin* - *commit*, or *begin* - *rollback*, and classify them to *winner* and saves to `winner_trx_id`. Otherwise, it saves to `loser_trx_id`

Second phase is redo and `REDO` function does this work. Because ARIES redo all updates regardless to winner or loser, this function processes all log records.
By comparing log sequence number of current processing record, if page lsn is bigger than current lsn, it does *consider-redo* which does not do redo work.

Third phase is undo and `UNDO` function does this work. Undo work is done over *loser* transactions. So, this function checks all the log records from recent to old, and if the log record is owned by *loser* transaction, skip or execute *undo* work.
Every compensate log record has **next undo log sequence number**, which means that it is possible to skip undo work on the log record because restoring to old value has done while executing REDO phase.(update - compensate)
For example, if there is a compensate log record which lsn is 12 and **next unod lsn** is 6, the log records of the same transaction do not have to do undo work.
To store this information to check if it is possible to skip undo work, I used `std::unordered_map<int32_t, uint64_t> next_undo_lsn_map` in `UNDO` function.
For each undo work on log records, this function creates new compensate log record and stores **next undo lsn** as **prev lsn**. In this way, it can skip undo works efficiently.
While processing log records, it will meet *begin* log record. This means that all undo works has done on the transaction. By creating *rollback* log record, loser transaction becomes winner transaction in next ANALYZE phase.

### Abort
If deadlock is detected while acquiring lock object, the transaction should rollback all the modifications to original vaule.
In project5, I used different structure to roll the modifications bun in this project, log record can be used to rollback.
`ABORT` function does this work. Undo all the update log records, leave compensate log records, and leave *rollback* log record at the end of undo works.
In this way, this aborted transaction becomes winner transaction in next ANALYZE phase.

## Test
There are transactions A and B, and each is given a unique number. Transaction B does not do anything. Transaction A increases the value of record by 1 for 20 times. However, after A performs the 10th execution, transaction B commits and the log buffer is flushed. And then a database crashes.

### First run
![1](https://user-images.githubusercontent.com/52111798/226519720-32ab6e12-5364-44b3-bb5a-d93ff814c067.png)

This is the state of log buffer when the crash occured. transaction id(xid) 1 is A, and transaction id(xid) 2 is B. transaction B(2) successfully commits, but transaction A(1) failed to commit according to log buffer.


### Second run (restart)
This is the log message created during recovery when database starts up in second run.

![2](https://user-images.githubusercontent.com/52111798/226519749-282a3ec6-83a9-4275-b252-0a2e07ba68d4.png)

This is the log message of recovery work. transaction 2 is winner and transaction 1 is loser. Because none of the pages were flushed to disk, *consider-redo* work was not executed. All undo work is done in reverse order, and it terminates when it meets *begin* log record of transaction 1.

During the UNDO phase, compensate log record is created.

This is the state of log buffer at the end of second run.

![3](https://user-images.githubusercontent.com/52111798/226519761-ed2fd751-4743-46e5-8c53-0d1a9e891354.png)

lsn 13 ~ 21 is the compensate log record created during UNDO phase. After UNDO works all finished, lsn 22 *rollback* log record is created to indicate the undo work is over, and loser transaction becomes winner in the next ANALYZE phase.

After executing recovery, as in the first run, crash occurs after 10th execution



### Third run (restart)
This is the log message created during recovery when database starts up in third run

![4](https://user-images.githubusercontent.com/52111798/226519770-af7e467a-ee26-40bc-af4a-4894be75e2f2.png)

Because undo work of transaction 1 is successfully done, transaction 1 became winner transaction because of *rollback* record.

This is the state of log buffer at the end of third run.

![5](https://user-images.githubusercontent.com/52111798/226519786-d43e7a4e-7cba-45e1-9d20-19980bcdc812.png)

As shown in the buffer, no compensate log record was created for compensate log record that was created in second run (13~21). Because transaction 1 is now winner transaction. Also, next undo lsn (n_u_lsn) value is set for all compensate log records. This value is used to skip undo work of the loser transaction.



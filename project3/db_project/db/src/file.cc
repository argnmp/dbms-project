#include "file.h"

map<string, int64_t> opened_tables;

int64_t fd_map[30] = {0,};
int64_t fd_count = 0;

int64_t file_open_table_file(const char* pathname) { 
    //if table already opened
    auto target = opened_tables.find((string)pathname);
    if(target!=opened_tables.end()){
        return target->second; 
    }
    
    if(opened_tables.size()>=20){
        return -2; 
    }

    h_page_t header_page_buf;
    f_page_t free_page_buf;
    page_t page_buf;

    
    int fd = open(pathname, O_RDWR | O_SYNC );


    //if the file not existing, create new one
    if(fd == -1){
        //total size of index
        int total_index = INITIAL_DB_FILE_SIZE / PAGE_SIZE;

        fd = open(pathname, O_RDWR | O_CREAT | O_SYNC, 0644);

        //case for file not created
        if(fd == -1){
            return -1;
        }

        //initialize header page;
        header_page_buf.magic_number = 2022;
        header_page_buf.free_page_number = total_index-1;
        header_page_buf.number_of_pages = total_index;
        header_page_buf.root_page_number = 0;
        pwrite(fd, &header_page_buf, PAGE_SIZE, 0);
        sync();


        //initialize pages;
        for(int i = 1; i<total_index; i++){
            free_page_buf.next_free_page_number = i-1;
            pwrite(fd, &free_page_buf, PAGE_SIZE, i*PAGE_SIZE);
            sync();
        }
    }
    else {
        pread(fd, &header_page_buf, PAGE_SIZE, 0);
        if(header_page_buf.magic_number != 2022){
            return -1;
        }
    }
    
    opened_tables.insert({pathname, fd_count});
    fd_map[fd_count] = fd;
    int tid = fd_count;
    fd_count += 1;
    return tid; 
}

// Allocate an on-disk page from the free page list
pagenum_t file_alloc_page(int64_t table_id) { 
    int64_t table_fd = fd_map[table_id];
    h_page_t header_page_buf;
    f_page_t free_page_buf;
    page_t page_buf;
    pagenum_t allocated_page_number;

    pread(table_fd, &header_page_buf, PAGE_SIZE, 0);
    
    if(header_page_buf.free_page_number == 0) {
        pagenum_t prev_free_page_number;
        pagenum_t prev_number_of_pages;

        // if no more free page left, double the size of database file
        pread(table_fd, &header_page_buf, PAGE_SIZE, 0);

        prev_free_page_number = header_page_buf.free_page_number;
        prev_number_of_pages = header_page_buf.number_of_pages;
        
        header_page_buf.number_of_pages = header_page_buf.number_of_pages + prev_number_of_pages;
        header_page_buf.free_page_number = header_page_buf.number_of_pages - 1;

        pwrite(table_fd, &header_page_buf, PAGE_SIZE, 0);
        sync();
       
        //initialize page numbers and link free page list
        
        free_page_buf.next_free_page_number = prev_free_page_number; 
        pwrite(table_fd, &free_page_buf, PAGE_SIZE, prev_number_of_pages * PAGE_SIZE);
        sync();

        for(int i = 1; i<prev_number_of_pages; i++){
            free_page_buf.next_free_page_number = prev_number_of_pages + i - 1;
            pwrite(table_fd, &free_page_buf, PAGE_SIZE, (prev_number_of_pages + i) * PAGE_SIZE);
            sync();
        }
             
    }
    allocated_page_number = header_page_buf.free_page_number;

    pread(table_fd, &free_page_buf, PAGE_SIZE, allocated_page_number * PAGE_SIZE);
    
    header_page_buf.free_page_number = free_page_buf.next_free_page_number; 

    pwrite(table_fd, &header_page_buf, PAGE_SIZE, 0);
    sync();

    return allocated_page_number; 
}

void file_free_page(int64_t table_id, pagenum_t pagenum) {

    int64_t table_fd = fd_map[table_id];

    h_page_t header_page_buf;
    f_page_t free_page_buf;

    pread(table_fd, &header_page_buf, PAGE_SIZE, 0);
    
    pagenum_t temp = header_page_buf.free_page_number;

    header_page_buf.free_page_number = pagenum;

    pwrite(table_fd, &header_page_buf, PAGE_SIZE, 0);
    sync();
    
    // resets the page and connect to free page list
    free_page_buf.next_free_page_number = temp;
    pwrite(table_fd, &free_page_buf, PAGE_SIZE, pagenum * PAGE_SIZE);
    sync();
}
void file_read_page(int64_t table_id, pagenum_t pagenum, struct page_t* dest) {
    int64_t table_fd = fd_map[table_id];
    pread(table_fd, dest, PAGE_SIZE, pagenum * PAGE_SIZE);
}

void file_write_page(int64_t table_id, pagenum_t pagenum, const struct page_t* src) {
    int64_t table_fd = fd_map[table_id];
    pwrite(table_fd, src, PAGE_SIZE, pagenum * PAGE_SIZE);
    sync();
}

// Close the database file
void file_close_database_file() {
    for(int i = 0; i<30; i++){
        printf("%d ", fd_map[i]);
    }
    for(pair<string, int> file: opened_tables){
        printf("%s %d : %lu free pages\n",file.first.c_str(), fd_map[file.second], free_page_count(file.second));
        close(fd_map[file.second]); 
    }
    printf("\n");
    opened_tables.clear();
}

uint64_t free_page_count(int64_t table_id){
    int64_t table_fd = fd_map[table_id];
    
    h_page_t header_page_buf;
    f_page_t free_page_buf;
    uint64_t count = 0;
    pread(table_fd, &header_page_buf, PAGE_SIZE, 0);
    pagenum_t npm = header_page_buf.free_page_number;
    while(1){
        if(npm == 0){
            break;
        }
        pread(table_fd, &free_page_buf, PAGE_SIZE, npm * PAGE_SIZE);
        count += 1;
        npm = free_page_buf.next_free_page_number;
    } 
    return count;
}

int64_t fd_mapper(int64_t table_id){
    return fd_map[table_id];
}

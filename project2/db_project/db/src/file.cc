#include "file.h"

map<string, int64_t> opened_tables;

// Open existing database file or create one if it doesn't exist
int64_t file_open_table_file(const char* pathname) { 
    //if table already opened
    auto target = opened_tables.find((string)pathname);
    if(target!=opened_tables.end()){
        return target->second; 
    }

    h_page_t header_page_buf;
    f_page_t free_page_buf;
    page_t page_buf;

    FILE* fp;
    fp = fopen(pathname, "rb+");  
    

    //if the file not existing, create new one
    if(fp == NULL){
        //total size of index
        int total_index = INITIAL_DB_FILE_SIZE / PAGE_SIZE;

        fp = fopen(pathname, "wb+");  
        //case for file not created
        if(fp == NULL){
            cout<<"failed"<<endl;
            return -1;
        }

        //initialize header page;
        header_page_buf.magic_number = 2022;
        header_page_buf.free_page_number = total_index-1;
        header_page_buf.number_of_pages = total_index;
        fwrite(&header_page_buf, PAGE_SIZE, 1, fp);
        fflush(fp);
        fsync(fileno(fp));

        //initialize pages;
        for(int i = 1; i<total_index; i++){
            free_page_buf.next_free_page_number = i-1;
            fwrite(&free_page_buf, PAGE_SIZE, 1, fp);
            fflush(fp);
            fsync(fileno(fp));
        }

    }
    else {
        fread(&header_page_buf, PAGE_SIZE, 1, fp);
        if(header_page_buf.magic_number != 2022){
            return -1;
        }
    }
    
    opened_tables.insert({pathname, fileno(fp)});
    return fileno(fp); 
}

// Allocate an on-disk page from the free page list
pagenum_t file_alloc_page(int64_t table_id) { 
    FILE* fp = fdopen(table_id, "rb+");  
    h_page_t header_page_buf;
    f_page_t free_page_buf;
    page_t page_buf;
    pagenum_t allocated_page_number;

    fseek(fp, 0, SEEK_SET);
    fread(&header_page_buf, PAGE_SIZE, 1, fp);
    
    if(header_page_buf.free_page_number == 0) {
        pagenum_t prev_free_page_number;
        pagenum_t prev_number_of_pages;

        // if no more free page left, double the size of database file
        fseek(fp, 0, SEEK_SET);
        fread(&header_page_buf, PAGE_SIZE, 1, fp);

        prev_free_page_number = header_page_buf.free_page_number;
        prev_number_of_pages = header_page_buf.number_of_pages;
        
        header_page_buf.number_of_pages = header_page_buf.number_of_pages + prev_number_of_pages;
        header_page_buf.free_page_number = header_page_buf.number_of_pages - 1;

        fseek(fp, 0, SEEK_SET);
        fwrite(&header_page_buf, PAGE_SIZE, 1, fp);
        fflush(fp);
        fsync(table_id);
       
        //initialize page numbers and link free page list
        
        free_page_buf.next_free_page_number = prev_free_page_number; 
        fseek(fp, prev_number_of_pages * PAGE_SIZE, SEEK_SET);
        fwrite(&free_page_buf, PAGE_SIZE, 1, fp);
        fflush(fp);
        fsync(table_id);
        for(int i = 1; i<prev_number_of_pages; i++){
            free_page_buf.next_free_page_number = prev_number_of_pages + i - 1;
            fwrite(&free_page_buf, PAGE_SIZE, 1, fp);
            fflush(fp);
            fsync(table_id);
        }
             
    }
    allocated_page_number = header_page_buf.free_page_number;

    fseek(fp, allocated_page_number * PAGE_SIZE, SEEK_SET);
    fread(&free_page_buf, PAGE_SIZE, 1, fp);
    
    header_page_buf.free_page_number = free_page_buf.next_free_page_number; 

    fseek(fp, 0, SEEK_SET);
    fwrite(&header_page_buf, PAGE_SIZE, 1, fp);
    fflush(fp);
    fsync(table_id);

    return allocated_page_number; 
}

// Free an on-disk page to the free page list
void file_free_page(int64_t table_id, pagenum_t pagenum) {
    h_page_t header_page_buf;
    f_page_t free_page_buf;
    FILE* fp = fdopen(table_id, "rb+");

    fseek(fp, 0, SEEK_SET);
    fread(&header_page_buf, PAGE_SIZE, 1, fp);
    
    pagenum_t temp = header_page_buf.free_page_number;

    header_page_buf.free_page_number = pagenum;

    fseek(fp, 0, SEEK_SET);
    fwrite(&header_page_buf, PAGE_SIZE, 1, fp);
    fflush(fp);
    fsync(table_id);
    
    // resets the page and connect to free page list
    free_page_buf.next_free_page_number = temp;
    fseek(fp, pagenum * PAGE_SIZE, SEEK_SET);
    fwrite(&free_page_buf, PAGE_SIZE, 1, fp);
    fflush(fp);
    fsync(table_id);
}

// Read an on-disk page into the in-memory page structure(dest)
void file_read_page(int64_t table_id, pagenum_t pagenum, struct page_t* dest) {
    FILE* fp = fdopen(table_id, "rb+");
    fseek(fp, pagenum * PAGE_SIZE, SEEK_SET);
    fread(dest, PAGE_SIZE, 1, fp);
}

// Write an in-memory page(src) to the on-disk page
void file_write_page(int64_t table_id, pagenum_t pagenum, const struct page_t* src) {
    FILE* fp = fdopen(table_id, "rb+");
    fseek(fp, pagenum * PAGE_SIZE, SEEK_SET);
    fwrite(src, PAGE_SIZE, 1, fp);
    fflush(fp);
    fsync(table_id);
}

// Close the database file
void file_close_database_file() {
    for(pair<string, int> file: opened_tables){
        FILE* fp = fdopen(file.second, "rb+");
        fclose(fp); 
    }
    opened_tables.clear();
}

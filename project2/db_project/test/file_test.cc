#include "file.h"
#include <gtest/gtest.h>
#include <string>

#define GTEST_COUT(args) std::cerr << "[          ] [ DEBUG ] " args << std::endl;

/*
 * 0: Turn off the test
 * 1: Turn on the test
 */

// Test for existing files
#define ExistFileInitSeries 1 
// Extend File to 80MB in size, this test requires quite long time
#define ExtendFileSeries 0

class FileInitTest : public ::testing::Test {
    protected:
        FileInitTest() {

        }

        ~FileInitTest() {
        }
};

TEST(FileInitTest, FileDescriptorInit){
    int fd;                                 // file descriptor
    std::string pathname = "init_test.db";  // customize it to your test file
    fd = file_open_database_file(pathname.c_str()); 
    file_close_database_file();
    //GTEST_COUT(<<fd);
    ASSERT_TRUE(fd >= 0) << "Failed to create new database file";
}

#if ExistFileInitSeries
TEST(FileInitTest, ExistFileValidInit){
    int fd;                                 // file descriptor
    std::string pathname = "../../validdb.db";
    fd = file_open_database_file(pathname.c_str()); 
    file_close_database_file();
    ASSERT_TRUE(fd >= 0);
}
TEST(FileInitTest, ExistFileInvalidInit){
    int fd;                                 // file descriptor
    std::string pathname = "../../invaliddb.db";
    fd = file_open_database_file(pathname.c_str()); 
    file_close_database_file();
    ASSERT_TRUE(fd == -1);
}
#endif

/*
 * "File Initialization" test required from specification
 */
TEST(FileInitTest, HandlesInitialization){

    //Open database file
    int fd;                                 
    std::string pathname = "init_test.db";  
    fd = file_open_database_file(pathname.c_str());
    
    
    //check if the database file is opened properly
    ASSERT_TRUE(fd >= 0);  

    // check if the allocated page is equal to initial number of pages stored in the head page;
    h_page_t header_page_buf; 
    FILE* fp = fdopen(fd, "rb+");
    fseek(fp, 0, SEEK_SET);
    fread(&header_page_buf, PAGE_SIZE, 1, fp);
    ASSERT_EQ(header_page_buf.number_of_pages, INITIAL_DB_FILE_SIZE / PAGE_SIZE);

    // Close all database files
    file_close_database_file();
}


class FileTest : public ::testing::Test {
 protected:
  FileTest() { fd = file_open_database_file(pathname.c_str()); }

  ~FileTest() {
    if (fd >= 0) {
      file_close_database_file();
    }
  }

  int fd;                
  //std::string pathname = "../../validdb.db";
  std::string pathname = "init_test.db";
};

/*
 * "File Management" test required from specification
 */
TEST_F(FileTest, HandlesPageAllocation) {
    h_page_t prev_header_page_buf;
    h_page_t header_page_buf;
    f_page_t free_page_buf;

    FILE* fp = fdopen(fd, "rb+");

    // Allocate the pages 
    pagenum_t allocated_page, freed_page;

    allocated_page = file_alloc_page(fd);
    freed_page = file_alloc_page(fd);
    // Free one page
    file_free_page(fd, freed_page);

    // Traverse the free page list and check the existence of the freed/allocated
    // pages. You might need to open a few APIs soley for testing.
    fseek(fp, 0, SEEK_SET);
    fread(&header_page_buf, PAGE_SIZE, 1, fp);
    pagenum_t target = header_page_buf.free_page_number; 
    
    bool f1 = false;
    bool f2 = true;
    while(target!=0){
        if(target==freed_page && f1 == false){
            f1 = true;
        }
        if(target==allocated_page){
            f2 = false; 
            break;
        }
        fseek(fp, target*PAGE_SIZE, SEEK_SET);
        fread(&free_page_buf, PAGE_SIZE, 1, fp);
        target = free_page_buf.next_free_page_number; 
    }
    ASSERT_TRUE(f1 && f2);

    // Free allocated page for other tests
    file_free_page(fd, allocated_page);
}
TEST_F(FileTest, ExtendFile) {
    FILE* fp = fdopen(fd, "rb+");
    h_page_t header_page_buf;
    f_page_t free_page_buf;
    pagenum_t allocated;

    //allocate all free pages
    while(1){
        fseek(fp, 0, SEEK_SET);
        fread(&header_page_buf, PAGE_SIZE, 1, fp);
        if(header_page_buf.free_page_number == 0){
            break;
        }
        allocated = file_alloc_page(fd);
    } 

    //test file size increases and free pages are linked
    allocated = file_alloc_page(fd);
    //GTEST_COUT(<<"allocated page: "<<allocated);
    fseek(fp, 0, SEEK_SET);
    fread(&header_page_buf, PAGE_SIZE, 1, fp);
    ASSERT_EQ(allocated, header_page_buf.number_of_pages - 1);

    //count all free pages and allocate
    //expect the number of free pages is (number_of_pages(from header) / 2 - 1)
    int count = 0;
    while(1){
        fseek(fp, 0, SEEK_SET);
        fread(&header_page_buf, PAGE_SIZE, 1, fp);
        if(header_page_buf.free_page_number == 0){
            break;
        }
        allocated = file_alloc_page(fd);
        count++;
    } 
    ASSERT_EQ(count, header_page_buf.number_of_pages / 2 - 1);
}

#if ExtendFileSeries
TEST_F(FileTest, ExtendFileSeriesA) {
    FILE* fp = fdopen(fd, "rb+");
    h_page_t header_page_buf;
    f_page_t free_page_buf;
    pagenum_t allocated;

    //allocate all free pages
    while(1){
        fseek(fp, 0, SEEK_SET);
        fread(&header_page_buf, PAGE_SIZE, 1, fp);
        if(header_page_buf.free_page_number == 0){
            break;
        }
        allocated = file_alloc_page(fd);
    } 

    //test file size increases and free pages are linked
    allocated = file_alloc_page(fd);
    //GTEST_COUT(<<"allocated page: "<<allocated);
    fseek(fp, 0, SEEK_SET);
    fread(&header_page_buf, PAGE_SIZE, 1, fp);
    ASSERT_EQ(allocated, header_page_buf.number_of_pages - 1);

    //count all free pages and allocate
    //expect the number of free pages is (number_of_pages(from header) / 2 - 1)
    int count = 0;
    while(1){
        fseek(fp, 0, SEEK_SET);
        fread(&header_page_buf, PAGE_SIZE, 1, fp);
        if(header_page_buf.free_page_number == 0){
            break;
        }
        allocated = file_alloc_page(fd);
        count++;
    } 
    ASSERT_EQ(count, header_page_buf.number_of_pages / 2 - 1);
}
TEST_F(FileTest, ExtendFileSeriesB) {
    FILE* fp = fdopen(fd, "rb+");
    h_page_t header_page_buf;
    f_page_t free_page_buf;
    pagenum_t allocated;

    //allocate all free pages
    while(1){
        fseek(fp, 0, SEEK_SET);
        fread(&header_page_buf, PAGE_SIZE, 1, fp);
        if(header_page_buf.free_page_number == 0){
            break;
        }
        allocated = file_alloc_page(fd);
    } 

    //test file size increases and free pages are linked
    allocated = file_alloc_page(fd);
    //GTEST_COUT(<<"allocated page: "<<allocated);
    fseek(fp, 0, SEEK_SET);
    fread(&header_page_buf, PAGE_SIZE, 1, fp);
    ASSERT_EQ(allocated, header_page_buf.number_of_pages - 1);

    //count all free pages and allocate
    //expect the number of free pages is (number_of_pages(from header) / 2 - 1)
    int count = 0;
    while(1){
        fseek(fp, 0, SEEK_SET);
        fread(&header_page_buf, PAGE_SIZE, 1, fp);
        if(header_page_buf.free_page_number == 0){
            break;
        }
        allocated = file_alloc_page(fd);
        count++;
    } 
    ASSERT_EQ(count, header_page_buf.number_of_pages / 2 - 1);
}
    

#endif
    
TEST_F(FileTest, FreePageTest){
    FILE* fp = fdopen(fd, "rb+");
    h_page_t header_page_buf;
    h_page_t prev_header_page_buf;
    f_page_t free_page_buf;

    fseek(fp, 0, SEEK_SET);
    fread(&prev_header_page_buf, PAGE_SIZE, 1, fp);
    
    // random test case
    pagenum_t allocated_pages[120];
    for(int i = 0; i<120; i++){
        allocated_pages[i] = file_alloc_page(fd);  
    }
    for(int i = 30; i<110; i++){
        file_free_page(fd, allocated_pages[i]);
    }
    
    /*
     * 120 pages are allocated and 80 pages are freed
     * check if the number of free pages equals to "previous NUMBER_OF_PAGES(from header pages) - 40(allocated page)"
     */
    int count = 0;
    fseek(fp, 0, SEEK_SET);
    fread(&header_page_buf, PAGE_SIZE, 1, fp);
    pagenum_t npm = header_page_buf.free_page_number;
    while(1){
        fseek(fp, npm * PAGE_SIZE, SEEK_SET);
        fread(&free_page_buf, PAGE_SIZE, 1, fp);
        count += 1;
        npm = free_page_buf.next_free_page_number;
        if(npm == 0){
            break;
        }
    } 
    ASSERT_EQ(count, prev_header_page_buf.number_of_pages - 40);
}

/*
 * "File I/O" test required from specification
 */

TEST_F(FileTest, CheckReadWriteOperation){
    page_t src;    
    for(int i = 0; i<PAGE_SIZE; i++){
        src.data[i] = 65 + (i%26);
    } 
    pagenum_t pagenum = file_alloc_page(fd);
    file_write_page(fd, pagenum, &src);
    
    page_t dest;
    file_read_page(fd, pagenum, &dest);
    ASSERT_EQ(memcmp(src.data, dest.data, PAGE_SIZE), 0);
}


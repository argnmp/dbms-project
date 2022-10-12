#include "db.h"
#include "dbpt.h"
#include <gtest/gtest.h>
#include <string>

using namespace std;

#define GTEST_COUT(args) std::cerr << "[          ] [ DEBUG ] " args << std::endl;

class ExpTest : public ::testing::Test {
    protected:
        ExpTest() {

        }

        ~ExpTest() {
        }
};

TEST(ExpTest, MockTest){
    string pathname = "a";
    int64_t table_id = file_open_table_file(pathname.c_str());

    FILE* fp = fdopen(table_id, "rb+");
    h_page_t header_page_buf;
    f_page_t free_page_buf;
    pagenum_t allocated;

    fseek(fp, 0, SEEK_SET);
    fread(&header_page_buf, PAGE_SIZE, 1, fp);
    printf("%lu\n",header_page_buf.free_page_number);

    allocated = file_alloc_page(table_id);
    printf("%lu\n",allocated);
    allocated = file_alloc_page(table_id);
    printf("%lu\n",allocated);
    allocated = file_alloc_page(table_id);
    printf("%lu\n",allocated);
    allocated = file_alloc_page(table_id);
    printf("%lu\n",allocated);

    FILE* new_fp = fdopen(table_id, "rb+");
    fseek(fp, 0, SEEK_SET);
    fread(&header_page_buf, PAGE_SIZE, 1, fp);
    printf("existing: %lu\n",header_page_buf.free_page_number);

    fseek(new_fp, 0, SEEK_SET);
    fread(&header_page_buf, PAGE_SIZE, 1, new_fp);
    printf("new: %lu\n",header_page_buf.free_page_number);

}

#include "file.h"
#include <gtest/gtest.h>
#include <string>

#define GTEST_COUT(args) std::cerr << "[          ] [ DEBUG ] " args << std::endl;

class ExpTest : public ::testing::Test {
    protected:
        ExpTest() {

        }

        ~ExpTest() {
        }
};

TEST(ExpTest, MockTest){
    std::string pathname1 = "a.db";  
    std::string pathname2 = "b.db";  
    std::string pathname3 = "c.db";  
    int64_t id = file_open_table_file(pathname1.c_str());   
    GTEST_COUT(<<id);
    id = file_open_table_file(pathname2.c_str());   
    GTEST_COUT(<<id);
    id = file_open_table_file(pathname3.c_str());   
    GTEST_COUT(<<id);
    id = file_open_table_file(pathname2.c_str());   
    GTEST_COUT(<<id);
    file_close_database_file();
}

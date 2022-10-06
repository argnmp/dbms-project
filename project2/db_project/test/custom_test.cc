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
    int64_t t1 = open_table(pathname.c_str());
    get_header_page(t1);
}

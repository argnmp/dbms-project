#include "dbpt.h"
#include "db.h"
#include <random>
#include <algorithm>
#include <sstream>
#include <set>
#include <fcntl.h>
using namespace std;

void usage_2( void ) {
    printf("Enter any of the following commands after the prompt > :\n"
    "\to <k> -- Open table <k>. If the table does not exist, create new table.\n"
    "\ti <k> <v> -- Insert key <k> (an integer) and value <v>.\n"
    "\tf <k>  -- Find the value under key <k>.\n"
    "\tr <k1> <k2> -- Print the keys and values found in the range "
            "[<k1>, <k2>]\n"
    "\td <k>  -- Delete key <k> and its associated value.\n"
    "\tt -- Print the B+ tree.\n"
    "\tl -- Print the keys and values of the leaves (bottom row of the tree).\n"
    "\tv -- Toggle output of pagenum in the tree node\n"
    "\tq -- Quit. (Or use Ctl-D.)\n"
    "\t? -- Print this help message.\n");
}

//maximum two inputs
void read_inputs(vector<string>& ret){
    string input_value;
    std::getline(std::cin, input_value);
    istringstream ss(input_value);
    string s;
    string concat = "";
    char delim = ' ';
    int count = 0;
    while(getline(ss, s, delim)){
        if(s == "") continue;
        if(count==1) {
            concat += " " + s;
        } else {
            ret.push_back(s);
            count ++;
        }
    }
    if(concat!="") {
        concat.erase(0,1);
        ret.push_back(concat);
    }
    return; 
}

int db_client(){
    int input, range2;
    char instruction;

    string table_name;
    int64_t table_id = -1;
    int64_t input_key;
    vector<string> input_values;
    string input_value;
    bool pagenum_toggle = false;
    char value[120];
    uint16_t val_size;


    int64_t begin_key;
    int64_t end_key;
    vector<int64_t> keys;
    vector<char*> values;
    vector<uint16_t> val_sizes;

    int result;


    printf("> ");
    while (scanf("%c", &instruction) != EOF) {
        switch (instruction) {
        case 'o':
            input_values.clear();
            read_inputs(input_values);
            if(input_values.size() != 1){
                printf("Wrong input. \n");
                break;
            }
            table_id = open_table(input_values[0].c_str()); 
            if(table_id==-2){
                printf("Exceeded maximum number of tables which is 20");
            }
            if(table_id==-1){
                printf("Error opening table");
            }
            table_name = input_values[0];
            printf("Switched to table %s\n", table_name.c_str());
            break;
        case 'd':
            if(table_id == -1) {
                printf("Specify table name\n");
                break;
            }
            input_values.clear();
            read_inputs(input_values);
            if(input_values.size() != 1){
                printf("Wrong input. \n");
                break;
            }
            try {
                input_key = stoi(input_values[0]);
            } catch (...){
                printf("Wrong input.\n");
                break;
            }
            result = db_delete(table_id, input_key); 
            if(result==-1){
                printf("Specified key is not in the table \n");
            }
            print_tree(table_id, pagenum_toggle);
            break;
        case 'i':
            if(table_id == -1) {
                printf("Specify table name\n");
                break;
            }
            input_values.clear();
            read_inputs(input_values);
            if(input_values.size() != 2){
                printf("Wrong input. \n");
                break;
            }
            try {
                input_key = stoi(input_values[0]);
            } catch (...){
                printf("Wrong input.\n");
                break;
            }
            printf("Insert key: %ld, length: %lu\n", input_key, input_values[1].length()); 
            db_insert(table_id, input_key, input_values[1].c_str(), input_values[1].length());
            print_tree(table_id, pagenum_toggle);
            break;
        case 'f':
            if(table_id == -1) {
                printf("Specify table name\n");
                break;
            }
            input_values.clear();
            read_inputs(input_values);
            if(input_values.size() != 1){
                printf("Wrong input. \n");
                break;
            }
            try {
                input_key = stoi(input_values[0]);
            } catch (...){
                printf("Wrong input.\n");
                break;
            }
            result = db_find(table_id, input_key, value, &val_size);
            if(result == -1){
                printf("Specified key is not in the table \n");
                break;
            }
            for(int i = 0; i<val_size; i++){
                printf("%c",value[i]);
            } 
            printf("\n");
            break;
        case 'r':
            if(table_id == -1) {
                printf("Specify table name\n");
                break;
            }
            input_values.clear();
            read_inputs(input_values);
            if(input_values.size() != 2){
                printf("Wrong input. \n");
                break;
            }
            try {
                begin_key = stoi(input_values[0]);
                end_key = stoi(input_values[1]);
            } catch (...){
                printf("Wrong input.\n");
                break;
            }
            keys.clear();
            values.clear();
            val_sizes.clear();
            result = db_scan(table_id, begin_key, end_key, &keys, &values, &val_sizes);
            if(result == -1){
                printf("Specified range is not in the table \n");
                break;
            }
            printf("total-keys-size: %lu, total-values-size: %lu, total-val_sizes-size: %lu\n", keys.size(), values.size(), val_sizes.size());
            for(int i = 0; i<keys.size(); i++){
                printf("key: %ld | value: ",keys[i]);
                for(int k = 0; k<val_sizes[i]; k++){
                    printf("%c",values[i][k]); 
                }
                printf(" | val_size: %hu\n",val_sizes[i]);
            }
            printf("\n");
            break;
        case 'l':
            if(table_id == -1) {
                printf("Specify table name\n");
                break;
            }
            print_leaves(table_id);
            while (getchar() != (int)'\n');
            break;
        case 'q':
            while (getchar() != (int)'\n');
            shutdown_db(); 
            return EXIT_SUCCESS;
            break;
        case 't':
            if(table_id == -1) {
                printf("Specify table name\n");
                break;
            }
            print_tree(table_id, pagenum_toggle);
            while (getchar() != (int)'\n');
            break;
        case 'v':
            pagenum_toggle = !pagenum_toggle;
            if(pagenum_toggle){
                printf("Changed to print page numbers of tree");
            }
            else {
                printf("Changed not to print page numbers of tree");
            }
            break;
        default:
            usage_2();
            break;
        }
        //while (getchar() != (int)'\n');
        cout<<table_name;
        printf("> ");
    }
    printf("\n");
    return EXIT_SUCCESS;    
}

int main( int argc, char ** argv ) {
    db_client();
}

#include "dbpt.h"
#include "db.h"
#include <random>
#include <algorithm>
#include <sstream>
#include <set>
using namespace std;

#define TEST 0

void inserting(int64_t table_id, int k){
    string value = "thisisvalue" + to_string(k);
    printf("inserting %d--------------\n",k);
    db_insert(table_id, k, value.c_str(), value.size());
    print_tree(table_id,false);

}
void random_test(int64_t table_id){
    printf("------RANDOM_TEST------\n");
    bool global_procedure_success = true;

    random_device rd;
    mt19937 mt(rd());
    uniform_int_distribution<int> insert_range(0, 10000);
    uniform_int_distribution<int> delete_range(0, 1000);
    uniform_int_distribution<int> value_multiplied_range(1, 20);
    
    set<int> rands;
    map<int, string> inserted_values;
    for(int i = 0; i<=10000; i++){
        int64_t val = insert_range(mt);
        rands.insert(val);
        string value = "thisisvalue";
        for(int k = 0; k<value_multiplied_range(mt); k++){
            value += to_string(val);
        }
        if(inserted_values.find(val)==inserted_values.end()){
            inserted_values.insert({val, value}); 
        }
        int result = db_insert(table_id, val, value.c_str(), value.length());
    }
    cout<< "free pages: " << free_page_count(table_id) << endl;
    print_tree(table_id, false);
    print_leaves(table_id);
    std::vector<int> shuffled_inserted_keys;
    for(auto i: rands){
        shuffled_inserted_keys.push_back(i); 
    }
    shuffle(shuffled_inserted_keys.begin(), shuffled_inserted_keys.end(), mt);
    
    vector<int> not_deleted_keys;
    for(auto i: shuffled_inserted_keys){
        char ret_val[120]; 
        uint16_t val_size;
        db_find(table_id, i, ret_val, &val_size); 
        if(memcmp(ret_val, inserted_values[i].c_str(), val_size) != 0){
            global_procedure_success = false;
        }
        
        if(delete_range(mt)==0){
            //printf(" do not delete\n");
            not_deleted_keys.push_back(i);
            continue;
        }
        if(db_delete(table_id, i)!=0){
            global_procedure_success = false; 
        }
    }
    cout<< "free pages: " << free_page_count(table_id) << endl;
    print_tree(table_id, false);
    print_leaves(table_id);
    
    print_tree(table_id, true);
    cout<< "<not_deleted_keys>" << endl;
    for(auto i: not_deleted_keys){
        char ret_val[120]; 
        uint16_t val_size;
        db_find(table_id, i, ret_val, &val_size); 
        printf("key: %d, value: ",i);
        for(int i = 0; i<val_size; i++){
            printf("%c",ret_val[i]);
        }
        if(memcmp(ret_val, inserted_values[i].c_str(), val_size) != 0){
            global_procedure_success = false;
        }
        if(db_delete(table_id, i)!=0){
            global_procedure_success = false;
        }
        cout << endl;
    }

    printf("GLOBAL_PROCEDURE_SUCCESS: %d\n",global_procedure_success);
    printf("--------------END------\n");
}
void scan_test(int64_t table_id){
    random_device rd;
    mt19937 mt(rd());
    uniform_int_distribution<int> insert_range(0, 10000);
    uniform_int_distribution<int> value_multiplied_range(1, 5);
    
    for(int i = 0; i<=100; i++){
        int64_t val = insert_range(mt);
        string value = "thisisvalue";
        for(int k = 0; k<value_multiplied_range(mt); k++){
            value += to_string(val);
        }
        int result = db_insert(table_id, val, value.c_str(), value.length());
    }
    print_tree(table_id, false);

    vector<int64_t> keys;
    vector<char*> values;
    vector<uint16_t> val_sizes;

    db_scan(table_id, 8000, 100000, &keys, &values, &val_sizes);
    printf("total-keys-size: %lu, total-values-size: %lu, total-val_sizes-size: %lu\n", keys.size(), values.size(), val_sizes.size());
    for(int i = 0; i<keys.size(); i++){
        printf("key: %ld | value: ",keys[i]);
        for(int k = 0; k<val_sizes[i]; k++){
            printf("%c",values[i][k]); 
        }
        printf(" | val_size: %hu\n",val_sizes[i]);
    }
    printf("\n");
    shutdown_db();

}
void db_test(){
    string pathname = "test.db"; 
    int64_t table_id = open_table(pathname.c_str()); 
    
    FILE* fp = fopen(pathname.c_str(), "wb+");
    
    FILE* new_fp = fdopen(fileno(fp), "rb+");
    printf("%p | %p", fp, new_fp);
    
    /*
    random_test(table_id); 
    random_test(table_id); 
    
    scan_test(table_id); 
    */

}

void usage_2( void ) {
    printf("Enter any of the following commands after the prompt > :\n"
    "\ti <k> <v> -- Insert key <k> (an integer) and value <v> ).\n"
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
            cout << "concat!" << endl;
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
        case 'u':
            input_values.clear();
            read_inputs(input_values);
            if(input_values.size() != 1){
                printf("Wrong input. \n");
                break;
            }
            table_id = open_table(input_values[0].c_str()); 
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
#if TEST
    db_test();
    return 0;
#else
    return db_client();
#endif
}

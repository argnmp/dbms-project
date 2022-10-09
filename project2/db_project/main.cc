#include "dbpt.h"
#include "db.h"
#include <random>
#include <algorithm>
using namespace std;

// MAIN
void usage_2( void ) {
    printf("Enter any of the following commands after the prompt > :\n"
    "\ti <k>  -- Insert <k> (an integer) as both key and value).\n"
    "\tf <k>  -- Find the value under key <k>.\n"
    "\tp <k> -- Print the path from the root to key k and its associated "
           "value.\n"
    "\tr <k1> <k2> -- Print the keys and values found in the range "
            "[<k1>, <k2>\n"
    "\td <k>  -- Delete key <k> and its associated value.\n"
    "\tx -- Destroy the whole tree.  Start again with an empty tree of the "
           "same order.\n"
    "\tt -- Print the B+ tree.\n"
    "\tl -- Print the keys of the leaves (bottom row of the tree).\n"
    "\tv -- Toggle output of pointer addresses (\"verbose\") in tree and "
           "leaves.\n"
    "\tq -- Quit. (Or use Ctl-D.)\n"
    "\t? -- Print this help message.\n");
}

int main( int argc, char ** argv ) {

    /*
    string pathname = "test.db"; 
    int64_t table_id = open_table(pathname.c_str()); 
    
    cout << "after open_table" << endl;
    cout<< "free pages: " << free_page_count(table_id) << endl;
    
    print_tree(table_id, false);
    for(int i = 0; i<=10000; i++){
        string value = "thisisvalue" + to_string(i);
        db_insert(table_id, i, value.c_str(), value.length());
    }
    print_tree(table_id, false);
    cout<< "free pages: " << free_page_count(table_id) << endl;

    for(int i = 5; i<=9995; i++){
        db_delete(table_id, i);
    }
    print_tree(table_id, true);

    cout<< "free pages: " << free_page_count(table_id) << endl;
    */

    
    /*
    string value = "thisisneverthat";
    db_insert(table_id, -234, value.c_str(), value.length());
    db_insert(table_id, 234, value.c_str(), value.length());
    print_tree(table_id, false);
    */

    /*//bulk test
    for(int i = 0; i<=2000; i++){
        string value = "thisisvalue" + to_string(i);
        db_insert(table_id, i, value.c_str(), value.length());
    }
    print_tree(table_id, false); 
    int acc = 0;
    for(int i = 0; i<=2000; i++){
        char ret_val[200];
        uint16_t val_size;
        int result = db_find(table_id, i, ret_val, &val_size); 
        if(result == 0){
            acc++;
            printf("key: %d, value: ",i);
            for(int i = 0; i<val_size; i++){
                printf("%c", ret_val[i]);
            }
            printf("\n");
        }
    }
    printf("total keys: %d", acc);
    */
    

    /* //random test
    random_device rd;
    mt19937 mt(rd());
    uniform_int_distribution<int> range(0, 99);
    
    vector<int> rands;
    for(int i = 0; i<=50; i++){
        int val = range(mt);
        rands.push_back(val);
        string value = "thisisvalue" + to_string(val);
        db_insert(table_id, val, value.c_str(), value.length());
    }
    
    print_tree(table_id, false);
    
    for(int i: rands){
        printf("%d,",i);
    }
    cout<<endl;
    for(int i = 0; i<=100; i++){
        char ret_val[200];
        uint16_t val_size;
        int result = db_find(table_id, i, ret_val, &val_size); 
        if(result == 0){
            printf("key: %d, value: ",i);
            for(int i = 0; i<val_size; i++){
                printf("%c", ret_val[i]);
            }
            printf("\n");
        }
    }
    */
    

    int input, range2;
    char instruction;

    string table_name;
    int64_t table_id = -1;
    int64_t input_key;
    string input_value;
    bool pagenum_toggle = false;
    char value[120];
    uint16_t val_size;

    int result;


    printf("> ");
    while (scanf("%c", &instruction) != EOF) {
        switch (instruction) {
        case 'u':
            cin >> table_name;
            table_id = open_table(table_name.c_str()); 
            printf("Switched to table %s\n", table_name.c_str());
            break;
        case 'd':
            if(table_id == -1) {
                printf("Specify table name\n");
                break;
            }
            scanf("%ld", &input_key);
            db_delete(table_id, input_key); 
            print_tree(table_id, pagenum_toggle);
            break;
        case 'i':
            if(table_id == -1) {
                printf("Specify table name\n");
                break;
            }
            scanf("%ld", &input_key);
            cin >> input_value;
            db_insert(table_id, input_key, input_value.c_str(), input_value.length());
            print_tree(table_id, pagenum_toggle);
            break;
        case 'f':
            if(table_id == -1) {
                printf("Specify table name\n");
                break;
            }
            scanf("%ld", &input_key);
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
            scanf("%d %d", &input, &range2);
            printf("not implemented");
            break;
        case 'l':
            printf("not implemented");
            break;
        case 'q':
            while (getchar() != (int)'\n');
            shutdown_db(); 
            return EXIT_SUCCESS;
            break;
        case 't':
            if(result == -1){
                printf("Specified key is not in the table \n");
                break;
            }
            print_tree(table_id, pagenum_toggle);
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
        case 'x':
            printf("not implemented");
            break;
        default:
            usage_2();
            break;
        }
        while (getchar() != (int)'\n');
        cout<<table_name;
        printf("> ");
    }
    printf("\n");

    return EXIT_SUCCESS;
}

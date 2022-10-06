#include "bpt.h"
#include "dbpt.h"
#include "db.h"
#include <random>
#include <algorithm>
using namespace std;

// MAIN

int main( int argc, char ** argv ) {
    string pathname = "test.db"; 
    int64_t table_id = open_table(pathname.c_str()); 

    print_tree(table_id, false);
    
    /*
    string value = "thisisneverthat";
    db_insert(table_id, -234, value.c_str(), value.length());
    db_insert(table_id, 234, value.c_str(), value.length());
    print_tree(table_id, false);
    */

     //bulk test
    for(int i = 0; i<=20000; i++){
        string value = "thisisvalue" + to_string(i);
        db_insert(table_id, i, value.c_str(), value.length());
    }
    print_tree(table_id, false); 
    int acc = 0;
    for(int i = 0; i<=20000; i++){
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
    

    /*
    char * input_file;
    FILE * fp;
    node * root;
    int input, range2;
    char instruction;
    char license_part;

    root = NULL;
    verbose_output = false;

    if (argc > 1) {
        order = atoi(argv[1]);
        if (order < MIN_ORDER || order > MAX_ORDER) {
            fprintf(stderr, "Invalid order: %d .\n\n", order);
            usage_3();
            exit(EXIT_FAILURE);
        }
    }

    license_notice();
    usage_1();  
    usage_2();

    if (argc > 2) {
        input_file = argv[2];
        fp = fopen(input_file, "r");
        if (fp == NULL) {
            perror("Failure  open input file.");
            exit(EXIT_FAILURE);
        }
        while (!feof(fp)) {
            fscanf(fp, "%d\n", &input);
            root = insert(root, input, input);
        }
        fclose(fp);
        print_tree(root);
    }

    printf("> ");
    while (scanf("%c", &instruction) != EOF) {
        switch (instruction) {
        case 'd':
            scanf("%d", &input);
            root = db_delete(root, input);
            print_tree(root);
            break;
        case 'i':
            scanf("%d", &input);
            root = insert(root, input, input);
            print_tree(root);
            break;
        case 'f':
        case 'p':
            scanf("%d", &input);
            find_and_print(root, input, instruction == 'p');
            break;
        case 'r':
            scanf("%d %d", &input, &range2);
            if (input > range2) {
                int tmp = range2;
                range2 = input;
                input = tmp;
            }
            find_and_print_range(root, input, range2, instruction == 'p');
            break;
        case 'l':
            print_leaves(root);
            break;
        case 'q':
            while (getchar() != (int)'\n');
            return EXIT_SUCCESS;
            break;
        case 't':
            print_tree(root);
            break;
        case 'v':
            verbose_output = !verbose_output;
            break;
        case 'x':
            if (root)
                root = destroy_tree(root);
            print_tree(root);
            break;
        default:
            usage_2();
            break;
        }
        while (getchar() != (int)'\n');
        printf("> ");
    }
    printf("\n");
    */

    return EXIT_SUCCESS;
}

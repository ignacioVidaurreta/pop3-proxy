#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include "CuTest.h"
#include "tmp/include/config.h"

// void replace_string(char *previous, char *new){
//     char *tmp = realloc(previous, strlen(new)+1);
//     if( tmp == NULL){
//         fprintf(stdout, "Memory Error: Couldn't reallocate");
//         exit(1);
//     }

//     strncpy(previous, new, strlen(new) +1 );
// }


void test_replace_string(CuTest *tc){
    
    char *prev = malloc(strlen("Hello") +1);
    char *new  = malloc(strlen("Hi")+1);

    strncpy(prev, "Hello", strlen("Hello"));
    strncpy(new, "Hi", strlen("Hi"));
    replace_string(prev, new);
    
    char *actual = prev;
    char *expected = "Hi";
    CuAssertStrEquals(tc, expected, actual);

}


CuSuite* ConfigUtilGetSuite() {
    CuSuite* suite = CuSuiteNew();
    SUITE_ADD_TEST(suite, test_replace_string);
    return suite;
}
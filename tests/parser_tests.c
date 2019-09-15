#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include "CuTest.h"

#define TRUE 1
#define FALSE 0
#define BUFFER_MAX_SIZE 512

#define TEST_STR_1 "Hello World\r\n How are you \r\n \r\n.\r\n"

int read_multiline_commands(char buffer[]){
    int found_CR = FALSE;
    int found_LF = FALSE;
    int found_dot = FALSE;
    int ended = FALSE;

    if(buffer[0] == '-'){
        return -1; //Do not parse, it's actually single line
    }

    for(int i = 0; i < BUFFER_MAX_SIZE && ended == FALSE; i++){
        char c = buffer[i];
        switch(c){
            case '\r':
                found_CR = TRUE;
                break;
            case '\n':
                if(found_CR){
                    if(found_dot){
                        ended = TRUE;
                    }
                    found_LF = TRUE;
                }
                break;
            case '.':
                if(found_CR && found_LF){
                    found_dot = TRUE;
                }else{
                    found_dot = FALSE; //Take into account the \r\n..\r\n case
                }
                found_CR  = FALSE;
                found_LF  = FALSE;
                break;
            default: //It isn't any interesting character, so restore everything to default
                found_CR  = FALSE;
                found_LF  = FALSE;
                found_dot = FALSE;
        }

    }

    return ended;

}
void test_parse_test(CuTest *tc){
    
    char *str = malloc(BUFFER_MAX_SIZE);
    memset(str, 'a', BUFFER_MAX_SIZE);

    strncpy(str, TEST_STR_1, strlen(TEST_STR_1));

    int ret_value = read_multiline_commands(str);

    CuAssertIntEquals(tc, TRUE, ret_value);

}

void test_server_returns_error(CuTest *tc){
    char *str = malloc(BUFFER_MAX_SIZE);
    memset(str, 'a', BUFFER_MAX_SIZE);

    strncpy(str, "-asdas sadasdsad", strlen("-asdas sadasdsad"));

    int ret_value = read_multiline_commands(str);

    CuAssertIntEquals(tc, -1, ret_value);
}



CuSuite* ParserUtilGetSuite() {
    CuSuite* suite = CuSuiteNew();
    SUITE_ADD_TEST(suite, test_parse_test);
    SUITE_ADD_TEST(suite, test_server_returns_error);
    return suite;
}
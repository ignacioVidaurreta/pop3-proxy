#include <stdio.h>
#include <stdlib.h>
#include "CuTest.h"

#include "../include/cmd_queue.h"
#define N 10000

void test_create_list(CuTest *tc){
    queue* list = create_queue();

    CuAssertPtrNotNull(tc, list);
    CuAssertPtrEquals(tc, NULL, list->first);
}

void test_add_element_list(CuTest *tc){
    queue* list = create_queue(); // Already tested
    int expected_value = 3;
    add_element(list, (void*)&expected_value);

    CuAssertPtrNotNull(tc, list->first);
    CuAssertIntEquals(tc, expected_value, *(int*)(list->first->elem));
    CuAssertPtrEquals(tc, NULL, list->first->next);
    free_queue(list);
}

void test_add_two_elements_list(CuTest *tc){
    queue* list = create_queue();
    int first_expected = 2;
    int second_expected = 3;
    add_element(list, (void*)&first_expected);
    add_element(list, (void*)&second_expected);

    CuAssertPtrNotNull(tc, list->first);
    CuAssertIntEquals(tc, first_expected, *(int*)(list->first->elem)); //Already tested
    CuAssertIntEquals(tc, second_expected, *(int*)(list->first->next->elem));
    CuAssertPtrEquals(tc, NULL, list->first->next->next);

    free_queue(list);
}

void test_add_n_elements_list(CuTest *tc){
    queue* list = create_queue();
    int expected = 0;
    
    add_element(list, (void*)&expected);
    CuAssertPtrNotNull(tc, list->first);
    node* curr_node = list->first;
    CuAssertIntEquals(tc, expected, *(int*)(curr_node->elem)); //Already tested

    for(int i = 1; i <= N; i++){
        add_element(list, (void*)&expected);
        curr_node = curr_node->next;
        expected = i;
        CuAssertIntEquals(tc, expected, *(int*)(curr_node->elem)); //Already tested
    }    

    free_queue(list);
}

CuSuite* LinkedListUtilGetSuite() {
    CuSuite* suite = CuSuiteNew();
    SUITE_ADD_TEST(suite, test_create_list);
    SUITE_ADD_TEST(suite, test_add_element_list);
    SUITE_ADD_TEST(suite, test_add_two_elements_list);
    SUITE_ADD_TEST(suite, test_add_n_elements_list);
    return suite;
}

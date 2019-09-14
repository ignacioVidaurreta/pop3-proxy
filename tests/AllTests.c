#include <stdio.h>
#include "CuTest.h"

CuSuite* ConfigUtilGetSuite();
CuSuite* ParserUtilGetSuite();

void RunAllTests(void){
    CuString *output = CuStringNew();
    CuSuite* suite = CuSuiteNew();

    CuSuiteAddSuite(suite, ConfigUtilGetSuite());
    CuSuiteAddSuite(suite, ParserUtilGetSuite());

    CuSuiteRun(suite);
    CuSuiteSummary(suite, output);
    CuSuiteDetails(suite, output);
    printf("%s\n", output->buffer);

}

int main(void) {
    RunAllTests();
}
#include "helper.h"
#include <stdio.h>

void print_array(char** array){
    while(*array){
        printf("%s", *array);
        fflush(stdout);
        array++;
    }
    printf("\n");
}

int main(int argc, char* argv[]){
    char test1[27] = "/this/is/an/absolute/path/";
    char test2[26] = "/this/is/an/absolute/path";
    char test3[25] = "this/is/an/absolute/path";

    char** result1 = split(test1);
    char** result2 = split(test2);
    char** result3 = split(test3);

    if(result1)
        print_array(result1);
    else
        printf("not absolute path\n");
    if(result2)
        print_array(result2);
    else
        printf("not absolute path\n");
    if(result3)
        print_array(result3);
    else
        printf("not absolute path\n");

    return 0;
}

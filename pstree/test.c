#include<stdio.h>

char * get_string(){
    printf("Enter a string:\n");
    char p[20] = "ABC";
    scanf("%s", p);

    printf("The string is %s\n", p);
    return p;
}

int main(){
    char *s = get_string();
    printf("%s\n", s);
    return 0;
}
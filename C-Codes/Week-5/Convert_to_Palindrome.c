/*
Given a string A consisting only of lowercase characters, we need to
check whether  it is possible to make this string a palindrome after
removing exactly one character from this.

If it is possible then return 1 else return 0.

Input Format:

There are 1 lines in the input
Line 1 ( Corresponds to arg 1 ) : A single string
*/
#include <stdio.h>
#include<stdlib.h>
#define MAX_LEN 100

void scan_str(char *str1) // scaning string
{
    int len=0;
    while((*str1++=getchar())!='\n')
        len++;
    str1--;
    *str1='\0';
    str1=str1-len;
}

int length(char* str)   //Find length of string
{
    int len=0;
    while(*str!='\0')
    {
        len++;
        str++;
    }
    return len;
}

int check(char *A,int i,int j)
{
    while(i<j)
    {
        if(A[i]!=A[j])
            return 0;
        i++;
        j--;
    }
    return 1;
}

int solve(char *A) 
{
    int n=length(A);
    int i=0,j=n-1;
    while(i<j)
    {
        if(A[i]!=A[j])
        {
            return check(A,i+1,j)||check(A,i,j-1);
        }
        i++;
        j--;
    }
    return 1;
}

int main()
{
    char *str;
    str=(char *)malloc(sizeof(char)* MAX_LEN);
    scan_str(str);
    printf("String : %s \n",str);
    if(solve(str))
        printf("It is possible\n");
    else
        printf("It is not possible\n");
    return 0;
}

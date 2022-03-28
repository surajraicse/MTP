/*Problem Description

Given a string A, find length of the longest repeating sub-sequence
such that the two subsequence don’t  have same string  character at 
same  position, i.e. , any  i’th character in the  two subsequences 
shouldn’t have the same index in the original string.

NOTE: Sub-sequence length should be greater than or equal to 2.. */

#include <stdio.h>
#include<stdlib.h>
#define MAX_LEN 100

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

void scan_str(char *str1) // scaning string
{
    int len=0;
    while((*str1++=getchar())!='\n')
        len++;
    str1--;
    *str1='\0';
    str1=str1-len;
}

void copy_str(char* str1,char* str2)
{
    while(*str2!='\0')
    {
        *str1=*str2;
        str1++;
        str2++;
    }
    *str1='\0';
}

int solve(char* A) 
{
    int n=length(A);
    char *B;
    B=(char *)malloc(sizeof(char)*(n+1));
    copy_str(B,A);
    int t[n+1][n+1];
    for(int i=1;i<=n;i++)
    {
        t[i][0]=0;
        t[0][i]=0;
    }
    for(int i=1;i<=n;i++)
    {
        for(int j=1;j<=n;j++)
        {
            if(A[i-1]==B[j-1] && i!=j)
                t[i][j]=1+t[i-1][j-1];
            else if(t[i-1][j]>t[i][j-1])
                t[i][j]=t[i-1][j];
            else
                t[i][j]=t[i][j-1];
        }
    }
    if(t[n][n]>1)
        return 1;
    return 0;
}
int main()
{
    char *str1,*str2;
    str1=(char *)malloc(sizeof(char)* MAX_LEN);
    scan_str(str1);
    if(solve(str1))
        printf("There is a repeating subsequence \n");
    else
        printf("There is no repeating subsequence \n");
    return 0;
}
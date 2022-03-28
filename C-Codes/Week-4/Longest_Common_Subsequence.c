/*Problem Description

Given two strings A and B. Find the longest common sequence 
(A sequence which does not need to be contiguous), which is
common in both the strings.You need to return the length of
such longest common subsequence.*/

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

int solve(char* A, char* B) 
{
    int n=length(A)+1;
    int m=length(B)+1;
    int i,j;
    int t[n][m];
    for(i=0;i<n;i++)
    {
        for(j=0;j<m;j++)
        {
            if(i==0||j==0)
              t[i][j]=0;  
        }
    }
    for(i=1;i<n;i++)
    {
        for(j=1;j<m;j++)
        {
            if(A[i-1]==B[j-1])
                t[i][j]=t[i-1][j-1]+1;
            else
            { 
                if(t[i-1][j]>t[i][j-1])
                    t[i][j]=t[i-1][j];
                else
                    t[i][j]=t[i][j-1];
            }
        }
    }
    return t[n-1][m-1];
}
int main()
{
    char *str1,*str2;
    str1=(char *)malloc(sizeof(char)* MAX_LEN);
    str2=(char *)malloc(sizeof(char)* MAX_LEN);
    scan_str(str1);
    scan_str(str2);
    printf("Longest Common Subsequence : %d \n",solve(str1,str2));
    return 0;
}
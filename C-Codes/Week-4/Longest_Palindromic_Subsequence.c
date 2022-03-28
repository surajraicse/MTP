/* Problem Description-

Given a string A, find the common palindromic sequence
(  A sequence which does not need to be contiguous and 
is a pallindrome ), which is common in itself.You need
to return the length of longest palindromic subsequence in A. */

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

void rev_str(char *str1,char *str2,int n) //reversing string
{
    str1[n--]='\0';
    while(n--)
    {
        str1[n]=*str2++;
    }
}

int solve(char* A) 
{
    char *B;
    int n=length(A)+1;
    B=(char *)malloc(sizeof(char)*n);
    int s=0,e=n-2;
    int t[n][n];
    rev_str(B,A,n);
    for(int i=0;i<n;i++)
    {
        t[i][0]=0;
        t[0][i]=0;
    }
    for(int i=1;i<n;i++)
    {
        for(int j=1;j<n;j++)
        {
            if(A[i-1]==B[j-1])
            {
                t[i][j]=1+t[i-1][j-1];
            }
            else if(t[i-1][j]>t[i][j-1])
            {
                t[i][j]=t[i-1][j];
            }
            else
            {
                t[i][j]=t[i][j-1];
            }
        }
    }
    return t[n-1][n-1];
}
int main()
{
    char *str1,*str2;
    str1=(char *)malloc(sizeof(char)* MAX_LEN);
    scan_str(str1);
    printf("Longest Palindromic Subsequence : %d\n",solve(str1));
    return 0;
}
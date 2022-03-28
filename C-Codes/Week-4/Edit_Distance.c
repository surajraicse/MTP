/*Problem Description

Given two strings A and B, find the minimum number of steps required to convert A to B. (each operation is counted as 1 step.)

You have the following 3 operations permitted on a word:

    Insert a character
    Delete a character
    Replace a character
Example :
    Input:
        A = "Anshuman"
        B = "Antihuman"

    Output:
        2

    Explanation:
        => Operation 1: Replace s with t.
        => Operation 2: Insert i.
*/

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
    int m=length(A)+1;
    int n=length(B)+1;
    int t[m][n];
    int ins=0,del=0,rep=0,cmn=0;
    for(int i=0;i<m;i++)
    {
        t[i][0]=i;
    }
    for(int j=0;j<n;j++)
    {
        t[0][j]=j;
    }
    for(int i=1;i<m;i++)
    {
        for(int j=1;j<n;j++)
        {
            if(A[i-1]!=B[j-1])
            {
                int a=t[i-1][j-1]+1;
                int b=t[i-1][j]+1;
                int c=t[i][j-1]+1;
                if(a<=b && a<=c)
                    t[i][j]=a;
                else if(b<=a && b<=c)
                    t[i][j]=b;
                else
                    t[i][j]=c;
            }
            else
            {
                t[i][j]=t[i-1][j-1];
            }
        }
    }
    return t[m-1][n-1];
}

int main()
{
    char *str1,*str2;
    str1=(char *)malloc(sizeof(char)* MAX_LEN);
    str2=(char *)malloc(sizeof(char)* MAX_LEN);
    scan_str(str1);
    scan_str(str2);
    printf("Minimum number of steps required : %d \n",solve(str1,str2));
    return 0;
}
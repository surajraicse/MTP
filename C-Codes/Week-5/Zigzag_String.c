/*
The  string  "PAYPALISHIRING"  is written in a zigzag pattern
on a given number of rows like this: (you may want to display 
this pattern in a fixed font for better legibility)

P.......A........H.......N
..A..P....L....S....I...I....G
....Y.........I........R

And then read line by line: PAHNAPLSIIGYIR
Write the code that will take a string and make this conversion
given a number of rows:


Input Format

There are 2 lines in the input

Line 1 ( Corresponds to arg 1 ) : A single string
    For example, String: "HelloWorld" will be written as "HelloWorld"(without quotes).

Line 2 ( Corresponds to arg 2 ) : A single integer
    For example, Integer: 5 will be written as "5"(without quotes).*/

#include <stdio.h>
#include<stdlib.h>
#define MAX_LEN 1000

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

char * convert(char *A, int B) 
{
    if(B==1)
        return A;
    int len=length(A);
    char *ans=(char *)malloc((len+1)*sizeof(char));
    int p=0;
    for(int i=1;i<=B;i++)
    {
        int p1=(B-i)*2;
        int p2=(i-1)*2;
        int c=0;
        for(int j=i-1;j<len;)
        {
            ans[p++]=A[j];
            if(c==0 || p2==0)
                j+=p1;
            if(c==1 || p1==0)
                j+=p2;
            c=!c;
        }
    }
    ans[len]='\0';
    return ans;
}
int main()
{
    char *str;
    int row;
    str=(char *)malloc(sizeof(char)* MAX_LEN);
    scan_str(str);
    scanf("%d",&row);
    printf("String : %s \n",str);
    printf("ZigZag String : %s \n",convert(str,row));
    return 0;
}

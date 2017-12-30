//
// Created by miron on 28.12.17.
//
#include<iostream>
#include <stdio.h>
using namespace std;


int main(){
    int n,m;
    scanf("%d%d" ,&n,&m);
    for(int i=0;i<n;i++)
        for(int j=i+1;j<n;j++){
            if(i+j%13)
                cout <<i<<" "<<j<<" "<< (i+j)*(i+j)*(i+j)%m+1<<endl;
        }
}
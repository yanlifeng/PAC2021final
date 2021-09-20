#ifndef MYHASH
#define MYHASH
#include<iostream>
#include "mytime.h"
#include "common.h"
using namespace std;

struct node{
    uint64 key;
    Position1 val;
    node* next;
    node(uint64& k,Position1& v):key(k),val(v),next(nullptr){    }
};
class hash_map{
private:
    node** arr; 
    // const uint64 LEN=1e9+7;
	const uint32 LEN=536870912;
    long _size=0;
public:
	hash_map(){
		arr=new node*[LEN]();
	}
    long size(){return _size;}
	void insert(uint64 k,Position1& v);
    Position1* find(uint64& k);
    void stat();
    // uint32 hash(uint64 k);
    ~hash_map(){
        cout<<"**********destroy hash_map*********"<<endl;
        for(uint64 i=0;i<LEN;i++){
            if(arr[i]){
                node* p=arr[i];
                while(p){
                    node* old=p;
                    p=p->next;
                    delete old;
                }
                 
            }
        }
        delete []arr;
    }
};
#endif
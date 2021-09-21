#include "myhash.h"
// inline uint32 hash_(uint64 k){
//   k = (~k) + (k << 18); // key = (key << 18) - key - 1;
//   k = k ^ (k >> 31);
//   k = k * 21; // key = (key + (key << 2)) + (key << 4);
//   k = k ^ (k >> 11);
//   k = k + (k << 6);
//   k = k ^ (k >> 22);
//   return ((uint32) (k&0xfffffffc))>>2;
//     // return (uint32)(k%LEN);
// }
void hash_map::insert(uint64 k,Position1& v){
    node* new_node=new node(k,v);
    node* old_head=arr[k%LEN];
    while(!arr[k%LEN].compare_exchange_weak(old_head,new_node,memory_order_relaxed))
        new_node->next=old_head;
}
Position1* hash_map::find(uint64& k){
    // uint64 kk=k;
    // kk ^= kk >> 30;
    // kk *= 0xbf58476d1ce4e5b9U;
    // kk ^= kk >> 27;
    // kk *= 0x94d049bb133111ebU;
    // kk ^= kk >> 31;
    // uint32 idx=(uint32)();

    node* p=arr[k%LEN];
    while(p){
        if(p->key==k)
            return &(p->val);
        p=p->next;
    }
    return NULL;
}
void hash_map::stat(){
    timeb start,end;
    ftime(&start);
    uint64 max_=0,count=0,total=0;
    for(uint64 i=0;i<LEN;i++){
        if(arr[i]){
            uint64 len_local=0;
            node* p=arr[i];
                while(p){
                    len_local++;
                    p=p->next;
                }
            max_=max(len_local,max_);
            count++;
            total+=len_local;
        }
    }
    ftime(&end);
    printTime(start,end,"HashStat cost");
    cout<<"max len:"<<max_<<endl;
    cout<<"count:"<<count<<endl;
    cout<<"total:"<<total<<" avg:"<<(double)total/count<<endl;
}
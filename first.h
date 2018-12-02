
typedef struct _node{
    unsigned long long int tag;
    struct _node *next;
}node;


int isPow2(int x){
    if(x == 0){
        return 0;
    }if(x == 1){
        return 1;
    }
    while(x != 1){
        if(x % 2 != 0){
            return 0;
        }
        x = x / 2;
    }
    return 1;
}

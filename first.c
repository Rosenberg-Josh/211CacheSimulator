#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <math.h>
#include "first.h"


int main(int argc, char** argv){
    int cacheSize;
    char* associativity;
    char* cachePolicy;
    int nCachePolicy;
    int blockSize;
    char* traceFile;
    int assocWay = -1;
    int blocks;
    int sets;


    //output variables
    //np = no prefect
    //wp = with prefetch

    int npmr = 0;
    int npmw = 0;
    int npch = 0;
    int npcm = 0;
    int wpmr = 0;
    int wpmw = 0;
    int wpch = 0;
    int wpcm = 0;


    if(argc != 6){
        printf("Bad input: not enough parameters\n");
        return 0;
    }
    cacheSize = atoi(argv[1]);
    if(isPow2(cacheSize) == 0){
        printf("Bad input: cache size not power of 2\n");
    }
    associativity = argv[2];
    if((strcmp(associativity, "direct") == 0) || (strcmp(associativity, "assoc") == 0)){
        //do nothing
    }else if(strchr(associativity, ':')){
        char* ptr = strchr(associativity, ':');
        ptr++;
        int num = atoi(ptr);
        if(isPow2(num)){
            assocWay = num;
        }else{
            printf("Error: assoc dimension not power of 2\n");
        }

    }
    cachePolicy = argv[3];
    if((strcmp(cachePolicy, "fifo") != 0) || strcmp(cachePolicy, "lru") != 0){
        if(strcmp(cachePolicy, "fifo") == 0){
            nCachePolicy = 1;
        }else{
            nCachePolicy = 0;
        }
    }
    blockSize = atoi(argv[4]);
    if(isPow2(blockSize) == 0){
        printf("Bad input: cache size not power of 2\n");
    }
    traceFile = argv[5];
    FILE *fp;
    fp = fopen(traceFile, "r");
    if(fp == NULL){
        printf("ERROR: file %s cannot be opened\n", traceFile);
    }

    blocks = cacheSize / blockSize;

    if(assocWay == -1){
        if(strcmp(associativity, "direct") == 0){
            sets = blocks;
        }else if (strcmp(associativity, "assoc") == 0){
            sets = 1;
        }
    }else{
        sets = assocWay;
    }
    int blocksInSet = blocks / sets;
    printf("sets: %d\n", sets);
    char pc[20];
    unsigned long long int address;
    unsigned long long int paddress;
    int check = fscanf(fp, "%s", &pc[0]);
    char* eofCheck = "#eof";
    if(check == EOF){
        return 0;
    }


    int offset = log2(blockSize);
    int bIndex = log2(sets);


    //init caches
    unsigned long long int cache[sets][blocksInSet];
    unsigned long long int cacheWithPre[sets][blocksInSet];

    //init lls
    //node* npArr[sets];
    //node* wpArr[sets];
    int npCounts[sets];
    int wpCounts[sets];
    int y;
    for(y = 0; y < sets; y++){
        //npArr[y] = NULL;
        //wpArr[y] = NULL;
        npCounts[y] = 0;
        wpCounts[y] = 0;
    }


    int k;
    int o;
    for(k = 0; k < sets; k++){
        for(o = 0; o < blocksInSet; o++){
            cache[k][o] = -1;
            cacheWithPre[k][o] = -1;
        }
    }


    while(strcmp(pc, eofCheck) != 0){
        char tempAddress[25];
        char mode[2];
        fscanf(fp, "%s", &mode[0]);
        fscanf(fp, "%s", &tempAddress[0]);
        fscanf(fp, "%s", &pc[0]);

        address = strtol(tempAddress, NULL, 16);
        paddress = address + blockSize;
        //strip offset
        address >>= offset;
        paddress >>= offset;
        int setNum = -1;
        int pSetNum = -1;
        if(bIndex > 0){
            setNum = address % (int)pow(2, bIndex);
            pSetNum = paddress % (int)pow(2, bIndex);
            address >>= bIndex;
            paddress >>= bIndex;
        }else{
            setNum = 0;
            pSetNum = 0;
        }

        int nphit = 0;
        int phit = 0;
        unsigned long long int tag = address;
        int i;

        for(i = 0; i < blocksInSet; i++){
            if(tag == cache[setNum][i] && nphit == 0){
                npch++;
                nphit = 1;
                if(mode[0] == 'W'){
                    npmw++;
                }

            }

            if(tag == cacheWithPre[setNum][i] && phit == 0){
                wpch++;
                phit = 1;

                if(mode[0] == 'W'){
                    wpmw++;
                }

            }

        }

        if(nphit == 1 && phit == 1){
            continue;
        }

        //no-prefecth miss
        if(nphit == 0){
            npcm++;
            if(mode[0] == 'W'){
                npmw++;
                npmr++;
            }else{
                npmr++;
            }
            int cold = 0;
            for(i = 0; i < blocksInSet; i++){ //check for cold miss
                if(cache[setNum][i] == -1){

                    cache[setNum][i] = tag;
                    cold = 1;

                    break;
                }
            }

            if(cold == 0){ //no cold miss, must replace
                if(nCachePolicy){
                    int i = npCounts[setNum];
                    cache[setNum][i] = tag;
                    npCounts[setNum]++;
                    if(npCounts[setNum] == blocksInSet){
                        npCounts[setNum] = 0;
                    }
                }
            }

        }
        //with-prefecth miss

        if(phit == 0){
            wpcm++;
            if(mode[0] == 'W'){
                wpmw++;
                wpmr++;
            }else{
                wpmr++;
            }

            int cold = 0;
            unsigned long long int ptag = paddress;


            for(i = 0; i < blocksInSet; i++){ //check for cold miss
                if(cacheWithPre[setNum][i] == -1){
                    cacheWithPre[setNum][i] = tag;
                    cold = 1;
                    break;
                }
            }
            if(cold == 0){ //no cold miss, must replace
                if(nCachePolicy){
                    int i = wpCounts[setNum];
                    cacheWithPre[setNum][i] = tag;
                    wpCounts[setNum]++;
                    if(wpCounts[setNum] == blocksInSet){
                        wpCounts[setNum] = 0;
                    }
                }


            }
            int jhit = 0;


            //insert paddress

            for(i = 0; i < blocksInSet; i++){ //check for cold miss
                if(cacheWithPre[pSetNum][i] == ptag){   //hit
                    jhit = 1;
                    break;
                }
            }

            if(jhit != 1){
                wpmr++;
                for(i = 0; i < blocksInSet; i++){
                    if(cacheWithPre[pSetNum][i] == -1){

                        cacheWithPre[pSetNum][i] = ptag;
                        cold = 1;

                        break;
                    }
                }
            }
            if(cold == 0 && jhit == 0){ //no cold miss, must replace
                if(nCachePolicy){
                    int i = wpCounts[pSetNum];
                    cacheWithPre[pSetNum][i] = ptag;
                    wpCounts[pSetNum]++;
                    if(wpCounts[pSetNum] == blocksInSet){
                        wpCounts[pSetNum] = 0;
                    }
                }


            }

        }


    }


    printf("no-prefetch\n");
    printf("Memory reads: %d\nMemory writes: %d\nCache hits: %d\nCache misses: %d\n", npmr, npmw, npch, npcm);
    printf("with-prefetch\n");
    printf("Memory reads: %d\nMemory writes: %d\nCache hits: %d\nCache misses: %d\n", wpmr, wpmw, wpch, wpcm);

    return 0;
}


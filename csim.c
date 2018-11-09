#include "cachelab.h"
#include <stdio.h>
#include <string.h>
#include <strings.h>
#include <unistd.h>
#include <getopt.h>
#include <stdlib.h>
#define MAGIC_LRU_NUM 999
typedef struct{
    int valid;       
    int tag;          
    int LruNumber;   
} Line;

typedef struct{
    Line* lines;    
} Set;

typedef struct {
    int set_num;   
    int line_num;   
    Set* sets;     
} Sim_Cache;


int misses;
int hits;
int evictions;


int getSet(int addr,int s,int b){
    addr = addr >> b;
    int mask =  (1<<s)-1;
    return addr &mask;
}


int getTag(int addr,int s,int b){
    int mask = s+b;
    return addr >> mask;
}


int findMinLruNumber(Sim_Cache *sim_cache,int setBits){
    int i;
    int minIndex=0;
    int minLru = MAGIC_LRU_NUM;
    for(i=0;i<sim_cache->line_num;i++){
        if(sim_cache->sets[setBits].lines[i].LruNumber < minLru){
            minIndex = i;
            minLru = sim_cache->sets[setBits].lines[i].LruNumber;
        }
    }
    return minIndex;
}


void updateLruNumber(Sim_Cache *sim_cache,int setBits,int hitIndex){
        sim_cache->sets[setBits].lines[hitIndex].LruNumber = MAGIC_LRU_NUM;
        int j;
        for(j=0;j<sim_cache->line_num;j++){
            if(j!=hitIndex) sim_cache->sets[setBits].lines[j].LruNumber--;
        }
}


int isMiss(Sim_Cache *sim_cache,int setBits,int tagBits){
    int i;
    int isMiss = 1;
    for(i=0;i<sim_cache->line_num;i++){
        if(sim_cache->sets[setBits].lines[i].valid == 1 && sim_cache->sets[setBits].lines[i].tag == tagBits){
            isMiss = 0;
            updateLruNumber(sim_cache,setBits,i);
        }
    }
    return isMiss;
}


int updateCache(Sim_Cache *sim_cache,int setBits,int tagBits){
    int i;
    int isfull = 1;
    for(i=0;i<sim_cache->line_num;i++){
        if(sim_cache->sets[setBits].lines[i].valid == 0){
            isfull = 0;
            break;
        }
    }
    if(isfull == 0){
        sim_cache->sets[setBits].lines[i].valid = 1;
        sim_cache->sets[setBits].lines[i].tag = tagBits;
        updateLruNumber(sim_cache,setBits,i);
    }else{
        int evictionIndex = findMinLruNumber(sim_cache,setBits);
        sim_cache->sets[setBits].lines[evictionIndex].valid = 1;
        sim_cache->sets[setBits].lines[evictionIndex].tag = tagBits;
        updateLruNumber(sim_cache,setBits,evictionIndex);
    }
    return isfull;
}

void loadData(Sim_Cache *sim_cache,int addr,int size,int setBits,int tagBits ,int isVerbose){

    if(isMiss(sim_cache,setBits,tagBits)==1){ 
        misses++;
        if(isVerbose == 1) printf("miss ");
        if(updateCache(sim_cache,setBits,tagBits) == 1){
            evictions++;
            if(isVerbose==1) printf("eviction ");
        }
    }else{ 
       hits++;
       if(isVerbose == 1) printf("hit ");
    }
}

void storeData(Sim_Cache *sim_cache,int addr,int size,int setBits,int tagBits ,int isVerbose){
    loadData(sim_cache,addr,size,setBits,tagBits,isVerbose);
}

void modifyData(Sim_Cache *sim_cache,int addr,int size,int setBits,int tagBits ,int isVerbose){
    loadData(sim_cache,addr,size,setBits,tagBits,isVerbose);
    storeData(sim_cache,addr,size,setBits,tagBits,isVerbose);
}

void init_SimCache(int s,int E,int b,Sim_Cache *cache){
    if(s < 0){
        printf("invaild cache sets number\n!");
        exit(0);
    }
    cache->set_num = 2 << s; //2^s ื้
    cache->line_num = E;
    cache->sets = (Set *)malloc(cache->set_num * sizeof(Set));
    if(!cache->sets){
        printf("Set Memory error\n");
        exit(0);
    }
    int i ,j;
    for(i=0; i< cache->set_num; i++)
    {
        cache->sets[i].lines = (Line *)malloc(E*sizeof(Line));
        if(!cache->sets){
            printf("Line Memory error\n");
            exit(0);
        }
        for(j=0; j < E; j++){
            cache->sets[i].lines[j].valid = 0;
            cache->sets[i].lines[j].LruNumber = 0;
        }
    }
    return ;
}

void checkOptarg(char *curOptarg){
    if(curOptarg[0]=='-'){
        printf("./csim :Missing required command line argument\n");
        exit(0);
    }
}

int get_Opt(int argc,char **argv,int *s,int *E,int *b,char *tracefileName,int *isVerbose){
    int c;
    while((c = getopt(argc,argv,"hvs:E:b:t:"))!=-1)
    {
        switch(c)
        {
        case 'v':
            *isVerbose = 1;
            break;
        case 's':
            checkOptarg(optarg);
            *s = atoi(optarg);
            break;
        case 'E':
            checkOptarg(optarg);
            *E = atoi(optarg);
            break;
        case 'b':
            checkOptarg(optarg);
            *b = atoi(optarg);
            break;
        case 't':
            checkOptarg(optarg);
            strcpy(tracefileName,optarg);
            break;
        case 'h':
        default:
            exit(0);
        }
    }
    return 1;
}


int main(int argc,char **argv){
    int s,E,b,isVerbose=0;
    char tracefileName[100],opt[10];

    int addr,size;
    misses = hits = evictions =0;

    Sim_Cache cache;

    get_Opt(argc,argv,&s,&E,&b,tracefileName,&isVerbose);
    init_SimCache(s,E,b,&cache);
    FILE *tracefile = fopen(tracefileName,"r");

    while(fscanf(tracefile,"%s %x,%d",opt,&addr,&size) != EOF){
        if(strcmp(opt,"I")==0)continue;
        int setBits = getSet(addr,s,b);
        int tagBits = getTag(addr,s,b);
        if(isVerbose == 1) printf("%s %x,%d ",opt,addr,size);
        if(strcmp(opt,"S")==0) {
            storeData(&cache,addr,size,setBits,tagBits,isVerbose);
        }
        if(strcmp(opt,"M")==0) {
            modifyData(&cache,addr,size,setBits,tagBits,isVerbose);
        }
        if(strcmp(opt,"L")==0) {
            loadData(&cache,addr,size,setBits,tagBits,isVerbose);
        }
        if(isVerbose == 1) printf("\n");
    }
    printSummary(hits,misses,evictions);
    return 0;
}

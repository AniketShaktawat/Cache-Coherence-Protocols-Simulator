/*******************************************************
                          main.cc
********************************************************/

#include <stdlib.h>
#include <assert.h>
#include <fstream>
using namespace std;

#include "cache.h"

int main(int argc, char *argv[])
{
    
    ifstream fin;
    FILE * pFile;

    if(argv[1] == NULL){
         printf("input format: ");
         printf("./smp_cache <cache_size> <assoc> <block_size> <num_processors> <protocol> <trace_file> \n");
         exit(0);
        }

    ulong cache_size     = atoi(argv[1]);
    ulong cache_assoc    = atoi(argv[2]);
    ulong blk_size       = atoi(argv[3]);
    ulong num_processors = atoi(argv[4]);
    ulong protocol       = atoi(argv[5]); /* 0:MODIFIED_MSI 1:DRAGON*/
    char *fname        = (char *) malloc(20);
    fname              = argv[6];

    printf("===== 506 Personal information =====\n");
    //printing personal information here
    cout<<"Name: Aniket Singh Shaktawat"<<endl;
    cout<<"UnityID: ashakta"<<endl;
    cout<<"ECE492 Students? NO"<<endl;

    printf("===== 506 SMP Simulator configuration =====\n");
    // print out simulator configuration here
    cout<<"L1_SIZE: "<<cache_size<<endl;
    cout<<"L1_ASSOC: "<<cache_assoc<<endl;
    cout<<"L1_BLOCKSIZE: "<<blk_size<<endl;
    cout<<"NUMBER OF PROCESSORS: "<<num_processors<<endl;

    if(protocol==0)
        cout<<"COHERENCE PROTOCOL: MSI"<< endl;
    else if(protocol==1)
        cout<<"COHERENCE PROTOCOL: Dragon"<< endl;
    else    
        cout<<"COHERENCE PROTOCOL IS WRONG"<< endl;

    cout<<"TRACE FILE: "<< fname << endl;


    // Using pointers so that we can use inheritance */
    Cache** cacheArray = (Cache **) malloc(num_processors * sizeof(Cache));
    for(ulong i = 0; i < num_processors; i++) {
                
            cacheArray[i] = new Cache(cache_size, cache_assoc, blk_size);
    }

    pFile = fopen (fname,"r");
    if(pFile == 0)
    {   
        printf("Trace file problem\n");
        exit(0);
    }
    
    ulong proc;
    char op;
    ulong addr;

    // int line = 1;
    while(fscanf(pFile, "%lu %c %lx", &proc, &op, &addr) != EOF)
    {
#ifdef _DEBUG
        // printf("%d\n", line);
#endif

    // cout<<"test"<<endl;

    cacheArray[proc] -> isUniqueCopy = true;
    
    ulong i=0;
    while(i<num_processors)
    {
        if(cacheArray[i]->findLine(addr) && i!=proc)
        {
            cacheArray[proc] -> isUniqueCopy = false;
        }

        i++;
    }

    if(protocol==0)
        cacheArray[proc] -> MSI_Modified_Access(proc, addr, op, num_processors, cacheArray);
    else if(protocol==1)
        cacheArray[proc] -> Dragon_Access(proc, addr, op, num_processors, cacheArray);    
    else
        cout<<"Unknown Protocol"<<endl;
        // propagate request down through memory hierarchy
        // by calling cachesArray[processor#]->Access(...)

        // line++;
    
    }

    fclose(pFile);

    //********************************//
    //print out all caches' statistics //
    //********************************//

    for(ulong i=0; i<num_processors; i++)
    {
        cacheArray[i] -> printStats(protocol,i);
    }
    
    
}

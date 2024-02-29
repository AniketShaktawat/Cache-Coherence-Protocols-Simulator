/*******************************************************
                          cache.h
********************************************************/

#ifndef CACHE_H
#define CACHE_H

#include <cmath>
#include <iostream>

typedef unsigned long ulong;
typedef unsigned char uchar;
typedef unsigned int uint;

/****add new states, based on the protocol****/
enum {
   INVALID = 0,
   CLEAN,
   VALID,
   DIRTY,
   EXCLUSIVE,
   SM,
   SC
};

class cacheLine 
{
protected:
   ulong tag;
   ulong Flags;   // 0:invalid, 1:valid, 2:dirty 
   ulong seq; 
 
public:
   cacheLine()                { tag = 0; Flags = 0; }
   ulong getTag()             { return tag; }
   ulong getFlags()           { return Flags;}
   ulong getSeq()             { return seq; }
   void setSeq(ulong Seq)     { seq = Seq;}
   void setFlags(ulong flags) {  Flags = flags;}
   void setTag(ulong a)       { tag = a; }
   void invalidate()          { tag = 0; Flags = INVALID; } //useful function
   bool isValid()             { return ((Flags) != INVALID); }
};

class Cache
{
protected:
   ulong size, lineSize, assoc, sets, log2Sets, log2Blk, tagMask, numLines;
   ulong reads,readMisses,writes,writeMisses,writeBacks;

   //******///
   //add coherence counters here///
   //******///
   ulong invalidations, flushes, busrdx_transactions, memory_transactions,busupdate_transactions, interventions;
   

   cacheLine **cache;
   ulong calcTag(ulong addr)     { return (addr >> (log2Blk) );}
   ulong calcIndex(ulong addr)   { return ((addr >> log2Blk) & tagMask);}
   ulong calcAddr4Tag(ulong tag) { return (tag << (log2Blk));}
   
public:
    ulong currentCycle;  
    bool busrd, busrdx, busupdate, isUniqueCopy;
     
    Cache(int,int,int);
   ~Cache() { delete cache;}
   
   cacheLine *findLineToReplace(ulong addr);
   cacheLine *fillLine(ulong addr);
   cacheLine * findLine(ulong addr);
   cacheLine * getLRU(ulong);
   
   ulong getRM()     {return readMisses;} 
   ulong getWM()     {return writeMisses;} 
   ulong getReads()  {return reads;}       
   ulong getWrites() {return writes;}
   ulong getWB()     {return writeBacks;}
   ulong getInvalidations()   {return invalidations;}
   ulong getInterventions()   {return interventions;}
   ulong getbusrdx_transactions()   {return busrdx_transactions;}
   ulong getbusupdate_transactions()   {return busupdate_transactions;}
   ulong getFlushes()   {return flushes;}
   ulong getMemory_Transactions() {return memory_transactions;}
   
   void writeBack(ulong) {writeBacks++;}
   // void Access(ulong,uchar);
   void printStats(ulong,ulong);
   void updateLRU(cacheLine *);
   void MSI_Modified_Access(ulong, ulong, uchar, ulong, Cache**);
   void Dragon_Access(ulong, ulong, uchar, ulong, Cache**);
   void MSI_Modified_Busrd(ulong);
   void MSI_Modified_Busrdx(ulong);
   void Dragon_Busrd(ulong);
   void Dragon_Busupdate(ulong);

   //******///
   //add other functions to handle bus transactions///
   //******///

};

#endif

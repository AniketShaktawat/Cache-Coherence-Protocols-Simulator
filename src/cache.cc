/*******************************************************
                          cache.cc
********************************************************/

#include <stdlib.h>
#include <assert.h>
#include "cache.h"
#include<iomanip>
using namespace std;

Cache::Cache(int s,int a,int b )
{
   ulong i, j;
   reads = readMisses = writes = 0; 
   writeMisses = writeBacks = currentCycle = 0;

   size       = (ulong)(s);
   lineSize   = (ulong)(b);
   assoc      = (ulong)(a);   
   sets       = (ulong)((s/b)/a);
   numLines   = (ulong)(s/b);
   log2Sets   = (ulong)(log2(sets));   
   log2Blk    = (ulong)(log2(b));   
  
   //*******************//
   //initialize your counters here//
   //*******************//
   memory_transactions = 0;
   busrdx_transactions = 0;
   busupdate_transactions = 0;
   busrd = false;
   busrdx = false;
   flushes = 0;
   invalidations = 0;
   interventions = 0;
   currentCycle = 0;
   writeBacks = 0;
   writeMisses = 0;
   writes = 0;
   readMisses = 0;
   reads = 0;
 
   tagMask =0;
   for(i=0;i<log2Sets;i++)
   {
      tagMask <<= 1;
      tagMask |= 1;
   }
   
   /**create a two dimentional cache, sized as cache[sets][assoc]**/ 
   cache = new cacheLine*[sets];
   for(i=0; i<sets; i++)
   {
      cache[i] = new cacheLine[assoc];
      for(j=0; j<assoc; j++) 
      {
         cache[i][j].invalidate();
      }
   }      
   
}

void Cache::MSI_Modified_Access(ulong proc, ulong addr, uchar op, ulong num_processors, Cache** cacheArray)
{
   
   
   currentCycle++;/*per cache global counter to maintain LRU order 
                    among cache ways, updated on every cache access*/

   busrd = busrdx = false;
   
         
   if(op == 'w') writes++;
   else          reads++;
   
   cacheLine * line = findLine(addr);
   if(line == NULL)/*miss*/
   {
      memory_transactions++;
      if(op == 'w') writeMisses++;
      else readMisses++;
      

      cacheLine *newline = fillLine(addr);
      
      if(op == 'w')
      {
         busrdx = true;
         busrdx_transactions++;
         newline->setFlags(DIRTY);    
      }
      else
      {
         busrd = true;
         newline->setFlags(CLEAN);
      }
      
   }
   else
   {
      /**since it's a hit, update LRU and update dirty flag**/
      updateLRU(line);
      if(op == 'w')
         line->setFlags(DIRTY);
   }



   ulong temp = 0;
   while(temp<num_processors)
   {
      if(temp!=proc)
      {
         if(busrd)        
            cacheArray[temp]->MSI_Modified_Busrd(addr);     
         if(busrdx)
            cacheArray[temp]->MSI_Modified_Busrdx(addr);                
      }
      temp++;
   }




}


void Cache::MSI_Modified_Busrd(ulong addr)
{
   cacheLine * line = findLine(addr);
   if(line!=NULL)
   {
      if(line->getFlags()==DIRTY)
      {
         
         flushes++;
         invalidations++;
         writeBacks++;
         line->setFlags(INVALID);
         memory_transactions++;
      }

      if(line->getFlags()==CLEAN)
      {
         invalidations++;
         line->setFlags(INVALID);
      }

   }



}


void Cache::MSI_Modified_Busrdx(ulong addr)
{

   cacheLine * line = findLine(addr);
   if(line!=NULL)
   {
      if(line->getFlags()==DIRTY)
      {
         
         flushes++;
         invalidations++;
         writeBacks++;
         line->setFlags(INVALID);
         memory_transactions++;
      }

      if(line->getFlags()==CLEAN)
      {
         invalidations++;
         line->setFlags(INVALID);
      }

   }



}

void Cache::Dragon_Access(ulong proc, ulong addr, uchar op, ulong num_processors, Cache** cacheArray)
{

   currentCycle++;/*per cache global counter to maintain LRU order 
                    among cache ways, updated on every cache access*/

   busrd = busupdate = false;
         
   if(op == 'w') writes++;
   else          reads++;
   
   cacheLine * line = findLine(addr);
   if(line == NULL)/*miss*/
   {
      memory_transactions++;
      if(op == 'w') writeMisses++;
      else readMisses++;

      cacheLine *newline = fillLine(addr);
      
      if(op == 'w')
      {
         newline->setFlags(DIRTY); 
         busrd = true;
         if(isUniqueCopy)
         {
            newline->setFlags(DIRTY);
         }   
         else
         {
            busupdate = true;
            busupdate_transactions++;
            newline->setFlags(SM);
         }
      }
      else
      {
         if(isUniqueCopy)
         {
            busrd = true;
            newline->setFlags(EXCLUSIVE);
         }
         else
         {
            busrd=true;
            newline->setFlags(SC);
         }

      }
      
   }
   else
   {
      /**since it's a hit, update LRU and update dirty flag**/
      updateLRU(line);
      if(op == 'w')
      {
         if(line->getFlags()==SC)
         {
             if(isUniqueCopy)
             {
               busupdate = 1;
               busupdate_transactions++;
               line->setFlags(DIRTY);
             }
             else
             {
               // busupdate = 1;
               // busupdate_transactions++;
               line->setFlags(SM);
             }
         }
         if(line->getFlags() ==SM)
         {
            if(isUniqueCopy)
            {
               busupdate = true;
               busupdate_transactions++;
               line->setFlags(DIRTY);
            }
            else
            {
               busupdate = true;
               busupdate_transactions++;

            }
         }
         if(line->getFlags()==EXCLUSIVE)
         {
            line->setFlags(DIRTY);
         }
         
      }
   }


   ulong temp = 0;
   while(temp<num_processors)
   {
      if(temp!=proc)
      {
         if(busrd)
         {         
            cacheArray[temp]->Dragon_Busrd(addr);     
         }
         if(busupdate)
            cacheArray[temp]->Dragon_Busupdate(addr);                
      }

      temp++;

   }







}

void Cache::Dragon_Busrd(ulong addr)
{
   cacheLine * line = findLine(addr);
   if(line!=NULL)
   {
      if(line->getFlags()==DIRTY)
      {
         
         interventions++;
         line->setFlags(SM);
      }

      if(line->getFlags()==SM)
      {
         flushes++;
           //new
         writeBacks++;       
         memory_transactions++;
      }

      if(line->getFlags()==EXCLUSIVE)
      {
         interventions++;
         line->setFlags(SC);
      }

   }



}

void Cache::Dragon_Busupdate(ulong addr)
{
   cacheLine * line = findLine(addr);
   if(line)
   {
      if(line->getFlags()==SM)
         line->setFlags(SC);
   }



}



/**you might add other parameters to Access()
since this function is an entry point 
to the memory hierarchy (i.e. caches)**/


/*look up line*/
cacheLine * Cache::findLine(ulong addr)
{
   ulong i, j, tag, pos;
   
   pos = assoc;
   tag = calcTag(addr);
   i   = calcIndex(addr);
  
   for(j=0; j<assoc; j++)
   if(cache[i][j].isValid()) {
      if(cache[i][j].getTag() == tag)
      {
         pos = j; 
         break; 
      }
   }
   if(pos == assoc) {
      return NULL;
   }
   else {
      return &(cache[i][pos]); 
   }
}

/*upgrade LRU line to be MRU line*/
void Cache::updateLRU(cacheLine *line)
{
   line->setSeq(currentCycle);  
}

/*return an invalid line as LRU, if any, otherwise return LRU line*/
cacheLine * Cache::getLRU(ulong addr)
{
   ulong i, j, victim, min;

   victim = assoc;
   min    = currentCycle;
   i      = calcIndex(addr);
   
   for(j=0;j<assoc;j++)
   {
      if(cache[i][j].isValid() == 0) { 
         return &(cache[i][j]); 
      }   
   }

   for(j=0;j<assoc;j++)
   {
      if(cache[i][j].getSeq() <= min) { 
         victim = j; 
         min = cache[i][j].getSeq();}
   } 

   assert(victim != assoc);
   
   return &(cache[i][victim]);
}

/*find a victim, move it to MRU position*/
cacheLine *Cache::findLineToReplace(ulong addr)
{
   cacheLine * victim = getLRU(addr);
   updateLRU(victim);
  
   return (victim);
}

/*allocate a new line*/
cacheLine *Cache::fillLine(ulong addr)
{ 
   ulong tag;
  
   cacheLine *victim = findLineToReplace(addr);
   assert(victim != 0);
   
   if(victim->getFlags() == DIRTY || victim->getFlags()==SM) {
      writeBack(addr);
      memory_transactions++;
   }
      
   tag = calcTag(addr);   
   victim->setTag(tag);
   victim->setFlags(VALID);    
   /**note that this cache line has been already 
      upgraded to MRU in the previous function (findLineToReplace)**/

   return victim;
}

void Cache::printStats(ulong protocol, ulong proc)
{ 
   cout<<"===== Simulation results Cache("<<proc<<") =====\n";
   cout<<"01. number of reads:                            "<<getReads()<<endl;
   cout<<"02. number of read misses:                      "<<getRM()<<endl;
   cout<<"03. number of writes:                           "<<getWrites()<<endl;
   cout<<"04. number of write misses:                     "<<getWM()<<endl;
   cout<<"05. total miss rate:                            "<<fixed<<setprecision(2)<<(static_cast<double>(getRM()+getWM())*100 / (getReads()+getWrites()))<<"%"<<endl;
   cout<<"06. number of writebacks:                       "<<getWB()<<endl;
   cout<<"07. number of memory transactions:              "<<getMemory_Transactions()<<endl;
   if(protocol==0)
      cout<<"08. number of invalidations:                    "<<getInvalidations()<<endl;
   else  
      cout<<"08. number of interventions:                    "<<getInterventions()<<endl;
   cout<<"09. number of flushes:                          "<<getFlushes()<<endl;
   if(protocol==0)
      cout<<"10. number of BusRdX:                           "<<getbusrdx_transactions()<<endl;
   else
      cout<<"10. number of Bus Transactions(BusUpd):         "<<getbusupdate_transactions()<<endl;

   /****print out the rest of statistics here.****/
   /****follow the ouput file format**************/
}



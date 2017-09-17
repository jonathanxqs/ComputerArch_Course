/*
Cache Simulator
Level one L1 and level two L2 cache parameters are read from file (block size, line per set and set per cache).
The 32 bit address is divided into tag bits (t), set index bits (s) and block offset bits (b)
s = log2(#sets)   b = log2(block size)  t=32-s-b
*/
#include <iostream>
#include <fstream>
#include <string>
#include <sstream>
#include <vector>
#include <iomanip>
#include <stdlib.h>
#include <cmath>
#include <bitset>

using namespace std;
//access state:
#define NA 0 // no action
#define RH 1 // read hit
#define RM 2 // read miss
#define WH 3 // Write hit
#define WM 4 // write miss



struct config{
       int L1blocksize;
       int L1setsize;
       int L1size;
       int L1IndexSize;
       int L2blocksize;
       int L2setsize;
       int L2size;
       int L2IndexSize;
       };


class Block
{
public:
	vector<char>  blockInData;
	bitset<32> accessaddr;
	int dirtyFlag ;  // 0 not  ,1 dirty
	int validFlag ;  // 0 not  ,1 valid

	Block() {
		dirtyFlag = validFlag = 0;	
	}
};

/* you can define the cache class here, or design your own data structure for L1 and L2 cache
*/
class Cache {
	const int blockSize, setSize, indexSize;
	int tagLength, indexLength, offsetLength, cacheLevel;

	// each SET way 
	class SetWay {
	private:
		int blockSize, setSize, indexSize;
		int tagLength, indexLength, offsetLength, cacheLevel;
		Cache *nextLevelCache;

	public:
		
		vector<Block>  blocks;

		SetWay(int blockSize, int indexSize, Cache *nextLevelCache = NULL) :
			nextLevelCache(nextLevelCache)
		{
			this->blockSize = blockSize;
			this->indexSize = indexSize;
			this->indexLength = log2(indexSize);
			this->offsetLength = log2(blockSize);
			this->tagLength = 32 - indexLength - offsetLength;

			blocks.resize(indexSize + 1);
			/*				
			for (int i = 0; i < indexSize; i++) {
				Block tmp_block1;
				blocks.push_back(tmp_block1);
			}
			*/

		};

		int readMem(bitset<32> accessaddr) {
			unsigned int indexNum = ((bitset<32>)(accessaddr.to_string().substr(tagLength, indexLength))).to_ulong();
			bitset<32> offsetNum = (bitset<32>)(accessaddr.to_string().substr(tagLength + indexLength, offsetLength));
			bitset<32> tagNum = (bitset<32>)(accessaddr.to_string().substr(0, tagLength));

			if (blocks[indexNum].accessaddr.to_string().substr(0, tagLength) == accessaddr.to_string().substr(0, tagLength)
				&& blocks[indexNum].validFlag == 1) {
				// valid and match 
				return 1;
			}

			if (blocks[indexNum].validFlag == 1) return 2; //Read Miss but valid

			return 3; // Read Miss and not valid

		}

		// putMem might occur evictions , writeMem will not
		int putMem(bitset<32> accessaddr, int dirtyFlag = 0) {
			unsigned int indexNum = ((bitset<32>)(accessaddr.to_string().substr(tagLength, indexLength))).to_ulong();
			bitset<32> offsetNum = (bitset<32>)(accessaddr.to_string().substr(tagLength + indexLength, offsetLength));
			bitset<32> tagNum = (bitset<32>)(accessaddr.to_string().substr(0, tagLength));

			blocks[indexNum].accessaddr = accessaddr;
			blocks[indexNum].validFlag = 1;
			blocks[indexNum].dirtyFlag = dirtyFlag;

			if (blocks[indexNum].validFlag == 0) // not valid , new and no eviction
			{
				return 1;
			}
			else if (blocks[indexNum].dirtyFlag == 0) //not dirty , only evict without put into low level
			{
				return 2;
			}
			else if (blocks[indexNum].dirtyFlag == 1) { // dirty , evict and put into low level
				if (this->cacheLevel == 1)// L1
				{
					nextLevelCache->putMem(blocks[indexNum].accessaddr, blockSize, blocks[indexNum].dirtyFlag); // dirty flag/blockSize should be transfered together

					return 4;

				}
				else return 5 ;				// MAIN MEM ,  no transfer to lower level

			}


		}

		int writeMem() {
			return 1;
		}

	};


	vector<int> roundRobinCounters ;  // counter and cacheLevel flag
	std::vector<SetWay *> sets;
	const Cache *nextLevelCache;


public:
	//init
	Cache(int blockSize, int setSize, int indexSize, int cacheLevel = 1, Cache *nextLevelCache = NULL) :cacheLevel(cacheLevel), blockSize(blockSize), setSize(setSize), indexSize(indexSize), indexLength(log2(indexSize)), offsetLength(log2(blockSize))
	{
		// cacheLevel 0 RG , 1 L1 , 2 L2 , 3 L3 ,5 MAIN MEM
		this->tagLength = 32 - indexLength - offsetLength;
		for (int i = 0; i < indexSize; i++)
			roundRobinCounters.push_back(0);

		

		for (int i = 0; i<setSize; i++) {
			SetWay* tmp = new SetWay(blockSize, indexSize, nextLevelCache);
			sets.push_back(tmp);
		}

	}

	// return 0 NA 1 RH , 2 RM
	int readMem(bitset<32> accessaddr) {
		unsigned int indexNum = ((bitset<32>)(accessaddr.to_string().substr(tagLength, indexLength))).to_ulong();
		bitset<32> offsetNum = (bitset<32>)(accessaddr.to_string().substr(tagLength + indexLength, offsetLength));
		bitset<32> tagNum = (bitset<32>)(accessaddr.to_string().substr(0, tagLength));

		int maxValidSet = -1;
		for (int i = 0; i<setSize; i++) {
			if ((sets[i])->readMem(accessaddr) == 1) // R hit in one set
			{
				return RH;  // 1  RH
			}

			if ((sets[i])->readMem(accessaddr) == 2) // RM but valid
				maxValidSet = i;

			//else RM ,not valid 

		}

		// All miss , wait for evicting and put new data in cache.

		if (maxValidSet < setSize - 1) {
			// put on empty without eviction
			sets[maxValidSet + 1]->putMem(accessaddr);
			
		}
		else { // might occur eviction , dirty to lower level
			sets[roundRobinCounters[indexNum]]->putMem(accessaddr);
			roundRobinCounters[indexNum] = (roundRobinCounters[indexNum] + 1) % setSize;
			
		}

		return RM;  // 2  Read Miss return
	}



	int writeMem(bitset<32> accessaddr) {
		unsigned int indexNum = ((bitset<32>)(accessaddr.to_string().substr(tagLength, indexLength))).to_ulong();
		bitset<32> offsetNum = (bitset<32>)(accessaddr.to_string().substr(tagLength + indexLength, offsetLength));
		bitset<32> tagNum = (bitset<32>)(accessaddr.to_string().substr(0, tagLength));


		for (int i = 0; i<setSize; i++) {
			if ((sets[i])->readMem(accessaddr) == 1) // WR hit in one set
			{
				(sets[i])->blocks[indexNum].accessaddr = accessaddr;
				(sets[i])->blocks[indexNum].dirtyFlag = 1;
				 return WH; // end write hit and update , return 3
			}			
		}

		return WM;  // write miss return

	}

	// higher level eviction may occur that ,extend the support for Block copy
	int putMem(bitset<32> accessaddr, int blockSizeHighLevel, int dirtyFlag = 0) {

		if (blockSizeHighLevel > blockSize) {
			// special range
			// Will not appear in the test case 
		};


		unsigned int indexNum = ((bitset<32>)(accessaddr.to_string().substr(tagLength, indexLength))).to_ulong();
		bitset<32> offsetNum = (bitset<32>)(accessaddr.to_string().substr(tagLength + indexLength, offsetLength));
		bitset<32> tagNum = (bitset<32>)(accessaddr.to_string().substr(0, tagLength));

		int maxValidSet = -1;
		for (int i = 0; i<setSize; i++) {
			if ((sets[i])->readMem(accessaddr) == 1) // R hit in one set
			{
				(sets[i])->blocks[indexNum].accessaddr = accessaddr;
				(sets[i])->blocks[indexNum].dirtyFlag = dirtyFlag;
				return 1;
			}else if ((sets[i])->readMem(accessaddr) == 2) // RM but valid
					maxValidSet = i;
					//else RM ,not valid 
		}

		// All miss , wait for evicting and put new data in cache.

		if (maxValidSet < setSize - 1) {
			// put on empty without eviction
			sets[maxValidSet + 1]->putMem(accessaddr);
			return 2;
		}
		else {
			sets[roundRobinCounters[indexNum]]->putMem(accessaddr);  // might occur eviction , dirty to lower level
			roundRobinCounters[indexNum] = (roundRobinCounters[indexNum] + 1) % setSize;
			return 3;
		}

	}


};











int main(int argc, char* argv[]){


    
    config cacheconfig;
    ifstream cache_params;
    string dummyLine;
    //cache_params.open(argv[1]);
	cache_params.open("cacheconfig.txt");   // in WIN

    while(!cache_params.eof())  // read config file
    {
      cache_params>>dummyLine;
      cache_params>>cacheconfig.L1blocksize;
      cache_params>>cacheconfig.L1setsize;              
      cache_params>>cacheconfig.L1size;
      cache_params>>dummyLine;              
      cache_params>>cacheconfig.L2blocksize;           
      cache_params>>cacheconfig.L2setsize;        
      cache_params>>cacheconfig.L2size;
      }
    
  
  
   // Implement by you: 
   // initialize the hirearch cache system with those configs
   // probably you may define a Cache class for L1 and L2, or any data structure you like
   

   // Transfer the setsize and get the index size 
   if (cacheconfig.L1setsize == 0 )  cacheconfig.L1setsize = cacheconfig.L1size * 1024 / cacheconfig.L1blocksize;
   if (cacheconfig.L2setsize == 0 )  cacheconfig.L2setsize = cacheconfig.L2size * 1024 / cacheconfig.L2blocksize;

   cacheconfig.L1IndexSize = cacheconfig.L1size * 1024 / cacheconfig.L1setsize / cacheconfig.L1blocksize;
   cacheconfig.L2IndexSize = cacheconfig.L2size * 1024 / cacheconfig.L2setsize / cacheconfig.L2blocksize;
   
   Cache cacheL2(cacheconfig.L2blocksize, cacheconfig.L2setsize, cacheconfig.L2IndexSize , 2   );
   Cache cacheL1(cacheconfig.L1blocksize, cacheconfig.L1setsize, cacheconfig.L1IndexSize, 1, &cacheL2);
   
  int L1AcceState =0; // L1 access state variable, can be one of NA, RH, RM, WH, WM;
  int L2AcceState =0; // L2 access state variable, can be one of NA, RH, RM, WH, WM;
   
   
    ifstream traces;
    ofstream tracesout;
    string outname;
    
	//outname = string(argv[2]) + ".out";
	//traces.open(argv[2]);
	outname =  "traceAns.out";    
	traces.open("trace.txt");

    tracesout.open(outname.c_str());
    
    string line;
    string accesstype;  // the Read/Write access type from the memory trace;
    string xaddr;       // the address from the memory trace store in hex;
    unsigned int addr;  // the address from the memory trace store in unsigned int;        
    bitset<32> accessaddr; // the address from the memory trace store in the bitset;
    
    if (traces.is_open() && tracesout.is_open()){    
        while (getline (traces,line)){   // read mem access file and access Cache
            
            istringstream iss(line); 
            if (!(iss >> accesstype >> xaddr)) {break;}

            stringstream saddr(xaddr);
            saddr >> std::hex >> addr;            
            accessaddr = bitset<32> (addr);  // even if addr only 28 bits , still filled by 0 
           
           
           // access the L1 and L2 Cache according to the trace;
              if (accesstype.compare("R")==0)
              
             {    
                 //Implement by you:
                 // read access to the L1 Cache, 
                 //  and then L2 (if required), 
                 //  update the L1 and L2 access state variable;
                 
              
                 L1AcceState = cacheL1.readMem(accessaddr);
                 if (L1AcceState == 1) 
                    L2AcceState = 0;  // L1 Read Hit , then L2 NA
                 else if (L1AcceState == 2){
                    L2AcceState = cacheL2.readMem(accessaddr);  // L1 Read Miss , then L2 Read
                 }             
                 
                 
                 }
             else 
             {    
                   //Implement by you:
                  // write access to the L1 Cache, 
                  //and then L2 (if required), 
                  //update the L1 and L2 access state variable;
                  
                  
                  L1AcceState = cacheL1.writeMem(accessaddr);
                  if (L1AcceState == 3) // write Hit in L1 , write back
                  {
                        L2AcceState = NA; // NA
                  }
                  else if (L1AcceState == 4) // write Miss in L1 , write no-allocate
                      {
                            L2AcceState = cacheL2.writeMem(accessaddr); // write to next level
                      }                 
                  
                  
              }
              
              
             
            tracesout<< L1AcceState << " " << L2AcceState << endl;  // Output hit/miss results for L1 and L2 to the output file;
             
             
        }
        traces.close();
        tracesout.close(); 
    }
    else cout<< "Unable to open trace or traceout file ";



    return 0;
}

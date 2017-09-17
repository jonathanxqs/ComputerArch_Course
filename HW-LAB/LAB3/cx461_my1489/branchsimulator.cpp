/*
Branch Predict

last M bit = index
if M = 0 , no index , predict all T

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
#define ST 3 // strong belief in Taken  ,11
#define WT 2 // 10
#define WNT 1 // 01
#define SNT 0 // 00



struct config{
   unsigned int M;  // 0 - 32
};


class Block
{
public:
	char blockInData;
	
	Block() {
		blockInData = ST;
	}

	bool Predict() {
		if (blockInData >= WT) // T
			return 1;
		else return false;
	}

	char UpdateStatus(int tOrN ) {  // TorN == 1 T  , 0 N

		if (blockInData == ST) {
			if (tOrN == 1) { blockInData = ST;  }
			else { blockInData = WT ;  }
			 
		}
		else if (blockInData == WT) {
			if (tOrN == 1) { blockInData = ST; }
			else { blockInData = SNT; }
		}
		else if (blockInData == SNT) {
			if (tOrN == 1) { blockInData = WNT; }
			else { blockInData = SNT; }
		}
		else if (blockInData == WNT) {
			if (tOrN == 1) { blockInData = ST; }
			else { blockInData = SNT; }
		}

		return blockInData;
	}

};

//Buffer for Predict , vector
class BranchPredictionBuffer {
	const unsigned int M , indexSize;
	std::vector<Block *> blocks;
	unsigned int indexLength;
	int missTimes ,predTimes ;
	double missRate ;

public:


	//init
	BranchPredictionBuffer(int m = 1) : M(m) , indexSize( 1 << m )
	{
		
		blocks.reserve(indexSize);
        missTimes = predTimes = 0;

		for (int i = 0; i< indexSize ; i++) {
			Block* tmp = new Block();
			blocks.push_back(tmp);
		}

	}

	// 0 NT , 1 T
	int branchPredict(bitset<32> accessaddr , int RA = 1) {  //RA =1 T , 0 NT

		unsigned int indexNum = ((bitset<32>)(accessaddr.to_string().substr(32-M, M))).to_ulong();
		bool ans = blocks[indexNum]->Predict();
		predTimes++; 
		if (RA != ans) missTimes++;
		
		blocks[indexNum]->UpdateStatus(RA);
		return ans;
	}

	double showMissRate() {
		missRate = (double)missTimes / predTimes;
		return missRate;
	}

};





int main(int argc, char* argv[]){

    config saturate_config;
    ifstream saturate_params;

    
    saturate_params.open(argv[1]);
	//saturate_params.open("config.txt");   // in WIN

    while(!saturate_params.eof())  // read config file
    {
      saturate_params >> saturate_config.M ;
     }
    
   // Implement by you: 
   // initialize the buffer with those configs

	BranchPredictionBuffer BP_Buf1(saturate_config.M );
   
   
    ifstream traces;
    ofstream tracesout;
    string outname;
    
	outname = string(argv[2]) + ".out";
	traces.open(argv[2]);
	// outname =  "traceAns.txt";    
	// traces.open("trace.txt");

    tracesout.open(outname.c_str());
    
    string line;
    int RealTN;  // real Taken or Not : 1 == T
	string str_rTN;
    string xaddr;       // the address from the memory trace store in hex;
    unsigned int addr;  // the address from the memory trace store in unsigned int;        
    bitset<32> accessaddr; // the address from the memory trace store in the bitset;
    
    if (traces.is_open() && tracesout.is_open()){    
        while (getline (traces,line)){   // read mem access file and access Cache
            
            istringstream iss(line); 
            if (!(iss >> xaddr >> RealTN )) {break;}

            stringstream saddr(xaddr);
            saddr >> std::hex >> addr;            
            accessaddr = bitset<32> (addr);  // even if addr only 28 bits , still filled by 0 
           
           
           // access the L1 and L2 Cache according to the trace;
			if (saturate_config.M == 0)  //ALL T
			{
				tracesout << 1 << endl;
			  }
			else {  // usually
				tracesout << BP_Buf1.branchPredict(accessaddr , RealTN) << endl;
			}      
              
        }
        traces.close();
        tracesout.close(); 

		// cout << " Write done! When M= "<< saturate_config.M << "  -> Miss Rate = " << BP_Buf1.showMissRate() << endl;

		
    }
    else cout<< "Unable to open trace or traceout file ";

	// system("pause");

    return 0;
}

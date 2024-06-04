/* 046267 Computer Architecture - Winter 20/21 - HW #2 */

#include <cstdlib>
#include <iostream>
#include <fstream>
#include <sstream>
#include <queue>
#include <cmath>

using std::FILE;
using std::string;
using std::cout;
using std::endl;
using std::cerr;
using std::ifstream;
using std::stringstream;
using namespace std;

class Block {
public:
    unsigned tag;
    bool valid;
    bool dirty;
    Block():tag(0),valid(0),dirty(0){}
    ~Block(){}
    bool GetValid() {return valid;}
    bool GetDirty() {return dirty;}
    unsigned GetTag() {
		return tag;}
    void SetValid(bool _valid){valid=_valid;}
    void SetDirty(bool _dirty){dirty=_dirty;}
    void SetTag(unsigned _tag){tag=_tag;}
};
class Cache {
public:
    Block** blocks;
    queue<unsigned>* LRU;
    unsigned NumOfSets;
    unsigned Assoc;
    unsigned AccessTime;
    int TotalAccessCount;
    int MissCount;
    Cache(unsigned Assoc,unsigned NumOfSets, unsigned AccessTime):
    NumOfSets(NumOfSets),Assoc(Assoc),AccessTime(AccessTime),TotalAccessCount(0),MissCount(0){
        blocks = new Block *[NumOfSets]();
        for(int i=0;i<NumOfSets;i++)
            blocks[i] = new Block[Assoc]();
        LRU = new queue<unsigned> [NumOfSets]();
    }
    ~Cache(){
        delete[] LRU;
        for(int i=0;i<NumOfSets;i++)
            delete[] blocks[i];
        delete[] blocks;
    }
   bool BlockLookup(unsigned set,unsigned tag,unsigned *time){
        TotalAccessCount++;
        *time += AccessTime;
        for(int i=0;i<Assoc;i++){
            if((blocks[set][i].GetTag() == tag) && blocks[set][i].GetValid()) {
                //tag found in cache and is valid
                return true; // HIT
            }
        }
        MissCount++;
        return false; // MISS
   }

   bool UpdateBlock(unsigned set,unsigned tag, unsigned *removedTag, bool *tagValid, bool *is_dirty){
        for (int i = 0; i < Assoc; i++) {
            if (!blocks[set][i].GetValid()) {
                //found free way for the block
                blocks[set][i].SetTag(tag);
                blocks[set][i].SetValid(1);
                PushToLRU(set,tag); //add to LRU Queue
                return 0;
            }
        }
        unsigned LRUBlockTag = LRU[set].front();
        LRU[set].pop();
        for (int i = 0; i < Assoc; i++) {
            if((blocks[set][i].GetTag() == LRUBlockTag)){
                *removedTag = blocks[set][i].GetTag();
                *tagValid = true;
                if(blocks[set][i].GetDirty()){
                    *is_dirty = true;
                }
                blocks[set][i].SetTag(tag);
                blocks[set][i].SetDirty(0);
                PushToLRU(set,tag); //add to LRU Queue
            }
        }
        return 1;
    }
   void SetDirty(unsigned set,unsigned tag,bool value){
        for(int i=0;i<Assoc;i++) {
            if ((blocks[set][i].GetTag() == tag) && (blocks[set][i].GetValid())) {
				blocks[set][i].SetDirty(value);
            }
        }
   }
   bool GetDirty(unsigned set,unsigned tag){
        for(int i=0;i<Assoc;i++) {
            if ((blocks[set][i].GetTag() == tag) && blocks[set][i].GetValid()) {
                return blocks[set][i].GetDirty();
            }
        }
    }
   void SetInvalid(unsigned set,unsigned tag){
        for(int i=0;i<Assoc;i++) {
            if (blocks[set][i].GetTag() == tag) {
                blocks[set][i].SetValid(0);
            }
        }
    }
   double MissRate() const{
        return (double)MissCount/TotalAccessCount;
    }
   void PushToLRU(unsigned set,unsigned tag){
        queue<unsigned> tempQueue = queue<unsigned>();
        while(!(LRU[set].empty())){
            unsigned poppedTag = LRU[set].front();
            LRU[set].pop();
            if(!(poppedTag == tag))
                tempQueue.push(poppedTag);
        }
        while(!tempQueue.empty()){
            LRU[set].push(tempQueue.front());
            tempQueue.pop();
        }
       LRU[set].push(tag);
    }
    void RemoveFromLRU(unsigned set,unsigned tag){
        queue<unsigned> tempQueue = queue<unsigned>();
        while(!(LRU[set].empty())){
            unsigned poppedTag = LRU[set].front();
            LRU[set].pop();
            if(!(poppedTag == tag))
                tempQueue.push(poppedTag);
        }
        while(!tempQueue.empty()){
            LRU[set].push(tempQueue.front());
            tempQueue.pop();
        }
    }
};

void parse_set_tag (unsigned old_tag, unsigned old_set, unsigned b_size, unsigned new_set_size ,unsigned new_tag_size, unsigned *new_set, unsigned *new_tag) { // Parse set and tag
	unsigned new_addrs = old_tag << new_set_size;
	new_addrs += old_set << b_size;
	*new_tag = new_addrs >> (b_size + new_set_size);
	*new_set = new_addrs << new_tag_size;
	*new_set = (*new_set) >> (b_size + new_tag_size);
}

int main(int argc, char **argv) {

	if (argc < 19) {
		cerr << "Not enough arguments" << endl;
		return 0;
	}

	// Get input arguments

	// File
	// Assuming it is the first argument
	char* fileString = argv[1];
	ifstream file(fileString); //input file stream
	string line;
	if (!file || !file.good()) {
		// File doesn't exist or some other error
		cerr << "File not found" << endl;
		return 0;
	}

	unsigned MemCyc = 0, BSize = 0, L1Size = 0, L2Size = 0, L1Assoc = 0,
			L2Assoc = 0, L1Cyc = 0, L2Cyc = 0, WrAlloc = 0;

	for (int i = 2; i < 19; i += 2) {
		string s(argv[i]);
		if (s == "--mem-cyc") {
			MemCyc = atoi(argv[i + 1]);
		} else if (s == "--bsize") {
			BSize = atoi(argv[i + 1]);
		} else if (s == "--l1-size") {
			L1Size = atoi(argv[i + 1]);
		} else if (s == "--l2-size") {
			L2Size = atoi(argv[i + 1]);
		} else if (s == "--l1-cyc") {
			L1Cyc = atoi(argv[i + 1]);
		} else if (s == "--l2-cyc") {
			L2Cyc = atoi(argv[i + 1]);
		} else if (s == "--l1-assoc") {
			L1Assoc = atoi(argv[i + 1]);
		} else if (s == "--l2-assoc") {
			L2Assoc = atoi(argv[i + 1]);
		} else if (s == "--wr-alloc") {
			WrAlloc = atoi(argv[i + 1]);
		} else {
			cerr << "Error in arguments" << endl;
			return 0;
		}
	}

	unsigned NumOfSetsL1,NumOfSetsL2,TagSizeL1,TagSizeL2,SetSizeL1,SetSizeL2;
	//Calculating Set and Tag Sizes
	SetSizeL1 = L1Size - BSize - L1Assoc;
    SetSizeL2 = L2Size - BSize - L2Assoc;
    NumOfSetsL1 = pow(2,SetSizeL1);
    NumOfSetsL2 = pow(2,SetSizeL2);
    TagSizeL1 = 64 - SetSizeL1 - BSize;
    TagSizeL2 = 64 - SetSizeL2 - BSize;
    //Creating 2 Caches
    Cache L1(pow(2,L1Assoc),NumOfSetsL1,L1Cyc);
    Cache L2(pow(2,L2Assoc),NumOfSetsL2,L2Cyc);
    int count = 0;
    unsigned totalTime = 0;
	while (getline(file, line)) {
        count++;
		stringstream ss(line);
		string address;
		char operation = 0; // read (R) or write (W)
		if (!(ss >> operation >> address)) {
			// Operation appears in an Invalid format
			cout << "Command Format error" << endl;
			return 0;
		}
		

		string cutAddress = address.substr(2); // Removing the "0x" part of the address


		unsigned long int num = 0;
		num = strtoul(cutAddress.c_str(), NULL, 16);


		//extracting Tag and Set from address
        unsigned L1Tag, L2Tag, L1Set, L2Set;
        L1Tag = num >> (BSize + SetSizeL1);
        L2Tag = num >> (BSize + SetSizeL2);
        L1Set = num << TagSizeL1 >> (TagSizeL1 + BSize);
        L2Set = num << TagSizeL2 >> (TagSizeL2 + BSize);
        unsigned time = 0, deletedTag = 0, tmp_set = 0, tmp_tag = 0;
        bool tagValid = false;
		bool is_dirty = false;

		 
		// Search for the block in L1:
		if (L1.BlockLookup(L1Set, L1Tag, &time) == false) { // In case tag wasn't found in L1
			if (L2.BlockLookup(L2Set, L2Tag, &time) == false) { // In case tag wasn't found in L2
				// In case the block wasn't found in L1 or L2 --> Access memory:
				time += MemCyc; // Add memory access time to total access time
				if ((operation == 'r') || (WrAlloc == 1)) { // In case of read operation, or Write Allocation policy - import block from memory
					if (L2.UpdateBlock(L2Set, L2Tag, &deletedTag, &tagValid, &is_dirty) == 1) { // In case block is removed from L2 - remove from L1 too
						// Remove from L1:
						parse_set_tag(deletedTag, L2Set, BSize, SetSizeL1, TagSizeL1, &tmp_set, &tmp_tag); // Create new tag and set inorder to find and remove block in L1
						L1.RemoveFromLRU(tmp_set, tmp_tag);
                        L1.SetInvalid(tmp_set, tmp_tag);
					}
					if (L1.UpdateBlock(L1Set, L1Tag, &deletedTag, &tagValid, &is_dirty) == 1) { // In case block is removed from L1 and dirty ==1 - update L2 accordingly
						if (is_dirty == true) {
							parse_set_tag(deletedTag, L1Set, BSize, SetSizeL2, TagSizeL2, &tmp_set, &tmp_tag); // Create new tag and set inorder to find block in L2
							L2.SetDirty(tmp_set, tmp_tag, true);
							L2.PushToLRU(tmp_set, tmp_tag); //ADI 1
						}						
					}
					if ((operation == 'w') && (WrAlloc == 1)) { 	// In case of write operation with Write Allocation policy - Update dirty bit for L1 and L2 blcoks
						L1.SetDirty(L1Set, L1Tag, true);
					}
				}
			}
			else {// In case tag was found in L2 - Push block to LRU
				L2.PushToLRU(L2Set, L2Tag);
				// Write block to L1 as well & push to L1 LRU: check ?? @@@@@@@@@@@@@@@@@@@@@ CHECK ASAP
				if ((operation == 'r') || (WrAlloc == 1)) {
					if (L1.UpdateBlock(L1Set, L1Tag, &deletedTag, &tagValid, &is_dirty) == 1) { // In case block is removed from L1 - consider dirty bit and update L2 accordingly
						if (is_dirty == true) {
							parse_set_tag(deletedTag, L1Set, BSize, SetSizeL2, TagSizeL2, &tmp_set, &tmp_tag); // Create new tag and set inorder to find block in L2
							L2.SetDirty(tmp_set, tmp_tag, true);
							L2.PushToLRU(tmp_set, tmp_tag); //ADI 1  // In case a block was removed 
						}
					}
				}
				if ((operation == 'w') && (WrAlloc == 1)) {  // In case of Write Allocation policy - L1 marked as dirty, L2 as not dirty
					L1.SetDirty(L1Set, L1Tag, true);  
					L2.SetDirty(L2Set, L2Tag, false); 
				}
				if ((operation == 'w') && (WrAlloc == 0)) { // In case of no Write Allocation policy - L2 marked as dirty
					L2.SetDirty(L2Set, L2Tag, true);
				}
			}
		}
		else {    // In case tag was found in L1 - Push block to L1 LRU
			L1.PushToLRU(L1Set, L1Tag);
			if (operation == 'w') { // In case of write command - update dirty
				L1.SetDirty(L1Set, L1Tag, true);
			}
		}
		totalTime += time;
	}



	double L1MissRate = L1.MissRate();
	double L2MissRate = L2.MissRate();
	double avgAccTime = (double)totalTime / count;


	printf("L1miss=%.03f ", L1MissRate);
	printf("L2miss=%.03f ", L2MissRate);
	printf("AccTimeAvg=%.03f\n", avgAccTime);

	return 0;
}

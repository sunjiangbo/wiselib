#ifndef EXTERNAL_STACK4_HPP
#define EXTERNAL_STACK4_HPP
#include "virt_sd.hpp"
template<class T, int BUFFERSIZE=1, bool PERSISTENT=true>
class ExternalStack{
    private:
	VirtualSD* sd_;
	const int MAX_ITEMS_PER_BLOCK = 512/sizeof(T);
	const int MAX_ITEMS_IN_BUFFER = MAX_ITEMS_PER_BLOCK*BUFFERSIZE;

	block_data_t buffer_[BUFFERSIZE*512];

	int itemsInBuffer_;

	int blocksOnSd_;

	const int minBlock_;
	const int maxBlock_;

	const int BLOCK_SIZE = 512;

    public:
	ExternalStack(VirtualSD* sd,int beginMem, int endMem, bool forceNew=false): sd_(sd), minBlock_(beginMem+1), maxBlock_(endMem){
	    if(!PERSISTENT || forceNew){
		initNewStack();
	    } else {
		sd_->read(buffer_, beginMem, 1);

		blockRead<int>(buffer_,0,&itemsInBuffer_);
		blockRead<int>(buffer_,1,&blocksOnSd_);

		int tmpMinBlock=-1;
		blockRead<int>(buffer_,2,&tmpMinBlock);
		int tmpMaxBlock=-1;
		blockRead<int>(buffer_,3,&tmpMaxBlock);
		int tmpSizeof=-1;
		blockRead<int>(buffer_,4,&tmpSizeof);
		int tmpValcode=-1;
		blockRead<int>(buffer_,5,&tmpValcode);

		int valCode = itemsInBuffer_+blocksOnSd_;
		if(tmpMinBlock!= minBlock_ || tmpMaxBlock != maxBlock_ || tmpSizeof != sizeof(T) || tmpValcode != valCode){
		    initNewStack();
		    std::cout << tmpMinBlock <<" "<<tmpMaxBlock<<" "<<tmpSizeof<<std::endl;
		} else {
		    if(itemsInBuffer_>0){
			sd_->read(buffer_,minBlock_+blocksOnSd_, 1);
		    }
		}

	    }
	}
	void  initNewStack(){
	    itemsInBuffer_=0;
	    blocksOnSd_=0;
	}


	~ExternalStack(){
		printf("\nstack destructor...\n"); //TODO
	    if(PERSISTENT){
		int fullBlocksToWrite=itemsInBuffer_/MAX_ITEMS_PER_BLOCK;
		int blocksToWrite=fullBlocksToWrite+(itemsInBuffer_%MAX_ITEMS_PER_BLOCK>0?1:0);

		if(blocksToWrite>0)sd_->write(buffer_,minBlock_+blocksOnSd_, blocksToWrite);
		itemsInBuffer_-=fullBlocksToWrite*512;
		blocksOnSd_+=fullBlocksToWrite;

		blockWrite<int>(buffer_, 0, itemsInBuffer_);
		blockWrite<int>(buffer_, 1, blocksOnSd_);

		blockWrite<int>(buffer_, 2, minBlock_);
		blockWrite<int>(buffer_, 3, maxBlock_);
		blockWrite<int>(buffer_, 4, sizeof(T));

		int valCode = itemsInBuffer_+blocksOnSd_;
		blockWrite<int>(buffer_, 5, valCode);

		sd_->write(buffer_,minBlock_-1,1);
	    }
	}

	bool push(T x){
	    if(minBlock_-1+blocksOnSd_+BUFFERSIZE>maxBlock_){
		return false;
	    }
	    if(itemsInBuffer_>=MAX_ITEMS_IN_BUFFER){
		flushFullBlocks();
	    }
	    blockWrite<T>(buffer_,itemsInBuffer_,x);
	    itemsInBuffer_+=1;
	    return true;
	}

	bool top(T* x){
	    if(itemsInBuffer_<=0){
		bool succ = loadOneBlockIntoBuffer();
		if(!succ) return false;
	    }
	    blockRead<T>(buffer_,itemsInBuffer_-1,x);
	    return true;
	}

	bool pop(T* x){
	    bool succ = top(x);
	    if(succ) itemsInBuffer_-=1;
	    return succ;
	}

    private:
	void flushFullBlocks(){
	    uint32_t fullBlocksToWrite=itemsInBuffer_/MAX_ITEMS_PER_BLOCK;
	    sd_->write(buffer_,minBlock_+blocksOnSd_, fullBlocksToWrite);
	    blocksOnSd_+=fullBlocksToWrite;
	    itemsInBuffer_-=fullBlocksToWrite*MAX_ITEMS_PER_BLOCK;

	    if(itemsInBuffer_>0){
		for(uint32_t i=0; i<BLOCK_SIZE; i++){
		    buffer_[i]=buffer_[i+BLOCK_SIZE*fullBlocksToWrite];
		}
	    }

	}
	bool loadOneBlockIntoBuffer(){
	    if(blocksOnSd_<=0) return false;
	    sd_->read(buffer_, minBlock_+blocksOnSd_-1, 1);
	    itemsInBuffer_+=MAX_ITEMS_PER_BLOCK;
	    blocksOnSd_-=1;
	    return true;
	}
	template<class S>
	    void blockRead(block_data_t* block, int idx, S* x){ //idx = index
		int maxPerBlock = 512/sizeof(S);			//How many S fit in 1 block
		S* castedBlock = (S*) &block[512*(idx/maxPerBlock)];	//S points to an array (block) which consists of enough blocks to fit all elements of Type S (note, itenger math is used, so you don't get any problems, everything fits).
		*x=castedBlock[idx%maxPerBlock];
	    }

	template<class S>
	    void blockWrite(block_data_t* block, int idx, S x){
		int maxPerBlock = 512/sizeof(S);
		S* castedBlock = (S*) &block[512*(idx/maxPerBlock)];
		castedBlock[idx%maxPerBlock]=x;
	    }
};

#endif

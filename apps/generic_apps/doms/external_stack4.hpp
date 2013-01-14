#ifndef EXTERNAL_STACK_HPP
#define EXTERNAL_STACK_HPP
#include "virt_sd.hpp"
typedef unsigned int uint16_t;
typedef unsigned int uint32_t;
template<class T, uint16_t BUFFERSIZE=1, bool PERSISTENT=true>
class ExternalStack{
    private:
	VirtualSD* sd_;
	const uint16_t MAX_ITEMS_PER_BLOCK = 512/sizeof(T);
	const uint16_t MAX_ITEMS_IN_BUFFER = MAX_ITEMS_PER_BLOCK*BUFFERSIZE;

	block_data_t buffer_[BUFFERSIZE*512];

	uint32_t itemsInBuffer_;

	uint32_t blocksOnSd_;

	const uint32_t minBlock_;
	const uint32_t maxBlock_;

    public:
	ExternalStack(VirtualSD* sd,uint32_t beginMem, uint32_t endMem, bool forceNew=false): sd_(sd), minBlock_(beginMem+1), maxBlock_(endMem){
	    if(BUFFERSIZE<2){ //da buffersize konstant=>kein Rechenaufwand
		if(BUFFERSIZE==1){
		    //TODO Warning Moegliches WorstCaseSzenario
		} else {
		    //TODO ERROR buffer muss mindestens 1 sein
		    return;
		}
	    }
	    if(!PERSISTENT || forceNew){
		initNewStack();
	    } else {
		sd_->read(buffer_, beginMem, 1);

		blockRead<uint32_t>(buffer_,0,&itemsInBuffer_);
		blockRead<uint32_t>(buffer_,1,&blocksOnSd_);

		uint32_t tmpMinBlock;
		blockRead<uint32_t>(buffer_,2,&tmpMinBlock);
		uint32_t tmpMaxBlock;
		blockRead<uint32_t>(buffer_,3,&tmpMaxBlock);
		uint32_t tmpSizeof;
		blockRead<uint32_t>(buffer_,4,&tmpSizeof);
		uint32_t tmpValcode;
		blockRead<uint32_t>(buffer_,5,&tmpValcode);

		uint32_t valCode = itemsInBuffer_+blocksOnSd_;
		if(tmpMinBlock!= minBlock_ || tmpMaxBlock != maxBlock_ || tmpSizeof != sizeof(T) || tmpValcode != valCode){
		    initNewStack();
		} else {
		    if(itemsInBuffer_>0){
			sd_->read(buffer_,minBlock_+blocksOnSd_, 1);
		    }
		}

	    }
	}

	~ExternalStack(){
	    if(PERSISTENT){
		uint32_t fullBlocksToWrite=itemsInBuffer_/MAX_ITEMS_PER_BLOCK;
		uint32_t blocksToWrite=fullBlocksToWrite+(itemsInBuffer_%MAX_ITEMS_PER_BLOCK>0?1:0);

		if(blocksToWrite>0)sd_->write(buffer_,minBlock_+blocksOnSd_, blocksToWrite);
		itemsInBuffer_-=fullBlocksToWrite*512;
		blocksOnSd_+=fullBlocksToWrite;

		blockWrite<uint32_t>(buffer_, 0, itemsInBuffer_);
		blockWrite<uint32_t>(buffer_, 1, blocksOnSd_);

		blockWrite<uint32_t>(buffer_, 2, minBlock_);
		blockWrite<uint32_t>(buffer_, 3, maxBlock_);
		blockWrite<uint32_t>(buffer_, 4, sizeof(T));

		uint32_t valCode = itemsInBuffer_+blocksOnSd_;
		blockWrite<uint32_t>(buffer_, 5, valCode);

		sd_->write(buffer_,minBlock_-1,1);
	    }
	}

	bool push(T x){
	    if(minBlock_-1+blocksOnSd_+BUFFERSIZE>maxBlock_) return false;
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
	void  initNewStack(){
	    itemsInBuffer_=0;
	    blocksOnSd_=0;
	}

	void flushFullBlocks(){
	    uint32_t fullBlocksToWrite=itemsInBuffer_/MAX_ITEMS_PER_BLOCK;
	    sd_->write(buffer_,minBlock_+blocksOnSd_, fullBlocksToWrite);
	    blocksOnSd_+=fullBlocksToWrite;
	    itemsInBuffer_-=fullBlocksToWrite*MAX_ITEMS_PER_BLOCK;

	    if(itemsInBuffer_>0){
		for(uint32_t i=0; i<512; i++){
		    buffer_[i]=buffer_[i+512*fullBlocksToWrite];
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
	    void blockRead(block_data_t* block, uint32_t idx, S* x){
		uint32_t maxPerBlock = 512/sizeof(S);
		S* castedBlock = (S*) &block[512*(idx/maxPerBlock)];
		*x=castedBlock[idx%maxPerBlock];
	    }

	template<class S>
	    void blockWrite(block_data_t* block, uint32_t idx, S x){
		uint32_t maxPerBlock = 512/sizeof(S);
		S* castedBlock = (S*) &block[512*(idx/maxPerBlock)];
		castedBlock[idx%maxPerBlock]=x;
	    }
};

#endif

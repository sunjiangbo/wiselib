#ifndef EXTERNAL_STACK_HPP
#define EXTERNAL_STACK_HPP
#include <external_interface/external_interface.h>
//#include <external_interface/arduino/arduino_sdcard.h>
//#include <external_interface/arduino/arduino_debug.h>
//#include <external_interface/arduino/arduino_clock.h>
#define BLOCK_SIZE 512
//#define DEBUG
#define INFO
//#define WARNING
using namespace wiselib;
//14011050



template<class T, uint32_t BUFFERSIZE=2, bool PERSISTENT=true>
class ExternalStack{

    public:
	typedef wiselib::OSMODEL Os;
	typedef typename Os::block_data_t block_data_t;

    private:
	wiselib::ArduinoSdCard<Os>* sd_;
	const uint32_t MAX_ITEMS_PER_BLOCK = BLOCK_SIZE/sizeof(T);
	const uint32_t MAX_ITEMS_IN_BUFFER = MAX_ITEMS_PER_BLOCK*BUFFERSIZE;

	block_data_t buffer_[BUFFERSIZE*BLOCK_SIZE];

	uint32_t itemsInBuffer_;

	uint32_t blocksOnSd_;

	const uint32_t minBlock_;
	const uint32_t maxBlock_;

	uint16_t pushCounter_;

	Os::Debug::self_pointer_t debug_;
    public:
	ExternalStack(wiselib::ArduinoSdCard<Os>* sd, uint32_t beginMem, uint32_t endMem, bool forceNew=false): sd_(sd), minBlock_(beginMem+1), maxBlock_(endMem){
#ifdef INFO
	    debug_->debug("EXTERNALSTACK INFO: begin init stack");
#endif
	    if(BUFFERSIZE<2){ //da buffersize konstant=>kein Rechenaufwand
		if(BUFFERSIZE==1){
#ifdef WARNING
		    debug_->debug("EXTERNAL STACK WARNING: May be inefficient with buffersize 1!");
#endif
		    // Warning Moegliches WorstCaseSzenario
		} else {
		    debug_->debug("EXTERNAL STACK ERROR: buffersize has to be at least 1!");
		    exit(1);
		    // ERROR buffer muss mindestens 1 sein
		}
	    }
	    if(!PERSISTENT || forceNew){
		initNewStack();
	    } else {
		sd_->read(buffer_, minBlock_-1, 1);

		blockRead<uint32_t>(buffer_,0,&itemsInBuffer_);
		blockRead<uint32_t>(buffer_,1,&blocksOnSd_);

		uint32_t tmpMinBlock=0;   blockRead<uint32_t>(buffer_,2,&tmpMinBlock);
		uint32_t tmpMaxBlock=0;   blockRead<uint32_t>(buffer_,3,&tmpMaxBlock);
		uint32_t tmpSizeof=0;     blockRead<uint32_t>(buffer_,4,&tmpSizeof);
		uint32_t tmpValcode=0;    blockRead<uint32_t>(buffer_,5,&tmpValcode);

		uint32_t valCode = itemsInBuffer_+blocksOnSd_;
		if(tmpMinBlock!= minBlock_ || tmpMaxBlock != maxBlock_ || tmpSizeof != sizeof(T) || tmpValcode != valCode){
		    initNewStack();
		} else {
		    if(itemsInBuffer_>0){
			sd_->read(buffer_,minBlock_+blocksOnSd_, 1);
		    }
#ifdef INFO
		    debug_->debug("EXTERNALSTACK INFO: reloaded old stack");
#endif
		}

	    }
	    pushCounter_=0;
	}


	~ExternalStack(){
	    if(PERSISTENT){
		flush(buffer_);
	    }
	}

	bool push(T x){
	    float ratio=0.7;
	    ratio+=0.3*pushCounter_/MAX_ITEMS_IN_BUFFER;
	    if(ratio>1.0) ratio=1.0;
	    return push(x,ratio);
	}

	bool push(T x, float ratio){
#ifdef DEBUG
	    debug_->debug("EXTERNALSTACK DEBUG: push(%d)",x);
#endif
	    if(minBlock_-1+blocksOnSd_+BUFFERSIZE>maxBlock_){
#ifdef DEBUG
		debug_->debug("EXTERNALSTACK DEBUG: push failed");
#endif
		return false;
	    }
	    if(itemsInBuffer_>=MAX_ITEMS_IN_BUFFER){
		flushFullBlocks(ratio);
	    }
	    blockWrite<T>(buffer_,itemsInBuffer_,x);
	    ++itemsInBuffer_;
	    if(pushCounter_<MAX_ITEMS_IN_BUFFER) ++pushCounter_;
	    return true;
	}

	bool top(T* x){
	    bool succ;
	    if(itemsInBuffer_<=0){
		succ = loadOneBlockIntoBuffer();
		if(!succ) return false;
	    }
	    blockRead<T>(buffer_,itemsInBuffer_-1,x);
#ifdef DEBUG
	    if(succ){
		debug_->debug("EXTERNALSTACK DEBUG: top(%d) ",*x);
	    } else {
		debug_->debug("EXTERNALSTACK DEBUG: top unsuccessful");
	    }
#endif
	    pushCounter_=0;
	    return true;
	}

	bool pop(T* x){
	    bool succ = top(x);
	    if(succ) itemsInBuffer_-=1;
#ifdef DEBUG
	    if(succ){
		debug_->debug("EXTERNALSTACK DEBUG: pop(%d) ",*x);
	    } else {
		debug_->debug("EXTERNALSTACK DEBUG: pop unsuccessful");
	    }
#endif
	    return succ;
	}

	uint64_t size(){
#ifdef DEBUG
	    debug_->debug("EXTERNALSTACK DEBUG: size() -> %d",blocksOnSd_*MAX_ITEMS_PER_BLOCK+itemsInBuffer_);
#endif
	    return blocksOnSd_*MAX_ITEMS_PER_BLOCK+itemsInBuffer_;
	}

	bool isEmpty(){
#ifdef DEBUG
	    debug_->debug("EXTERNALSTACK DEBUG: isEmpty() -> %d", !(itemsInBuffer_>0 || blocksOnSd_>0));
#endif
	    return !(itemsInBuffer_>0 || blocksOnSd_>0);
	}
	void flush(){
#ifdef DEBUG 
	    debug_->debug("EXTERNALSTACK DEBUG: flush()");
#endif
	    block_data_t tmpBlock[512];
	    flush(tmpBlock);
	}

    private:
	void flush(block_data_t *tmpBlock){
	    uint32_t fullBlocksToWrite=itemsInBuffer_/MAX_ITEMS_PER_BLOCK;
	    uint32_t blocksToWrite=fullBlocksToWrite+(itemsInBuffer_%MAX_ITEMS_PER_BLOCK>0?1:0);

	    if(blocksToWrite>0) sd_->write(buffer_,minBlock_+blocksOnSd_, blocksToWrite);
	    itemsInBuffer_-=fullBlocksToWrite*MAX_ITEMS_PER_BLOCK;
	    blocksOnSd_+=fullBlocksToWrite;

	    blockWrite<uint32_t>(tmpBlock, 0, itemsInBuffer_);
	    blockWrite<uint32_t>(tmpBlock, 1, blocksOnSd_);

	    blockWrite<uint32_t>(tmpBlock, 2, minBlock_);
	    blockWrite<uint32_t>(tmpBlock, 3, maxBlock_);
	    blockWrite<uint32_t>(tmpBlock, 4, sizeof(T));

	    uint32_t valCode = itemsInBuffer_+blocksOnSd_;
	    blockWrite<uint32_t>(tmpBlock, 5, valCode);

	    sd_->write(tmpBlock,minBlock_-1,1);
	}

	void  initNewStack(){
	    itemsInBuffer_=0;
	    blocksOnSd_=0;
#ifdef INFO
	    debug_->debug("EXTERNALSTACK INFO: inited new stack");
#endif
	}

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

	void flushFullBlocks(float ratio){//Buffer has to be full
	    //Calc the blocks to write
	    uint32_t blocksToWrite=BUFFERSIZE*ratio;
	    if(blocksToWrite<=0) blocksToWrite=1;
	    //Write the blocks
	    sd_->write(buffer_,minBlock_+blocksOnSd_, blocksToWrite);

	    //move the remaining blocks to the begining
	    uint16_t remainingBlocks = BUFFERSIZE-blocksToWrite;
	    for(uint16_t i=0; i<remainingBlocks; i++){
		for(uint16_t j=0; j<512; j++){
		    buffer_[i*512+j]=buffer_[(remainingBlocks+i)*512+j];
		}
	    }
	    itemsInBuffer_ -= blocksToWrite*MAX_ITEMS_PER_BLOCK;
	    blocksOnSd_+=blocksToWrite;

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
		uint32_t maxPerBlock = BLOCK_SIZE/sizeof(S);
		S* castedBlock = (S*) &block[BLOCK_SIZE*(idx/maxPerBlock)];
		*x=castedBlock[idx%maxPerBlock];
	    }

	template<class S>
	    void blockWrite(block_data_t* block, uint32_t idx, S x){
		uint32_t maxPerBlock = BLOCK_SIZE/sizeof(S);
		S* castedBlock = (S*) &block[BLOCK_SIZE*(idx/maxPerBlock)];
		castedBlock[idx%maxPerBlock]=x;
	    }
};

#endif

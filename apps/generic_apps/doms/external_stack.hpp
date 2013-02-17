#ifndef EXTERNAL_STACK_HPP
#define EXTERNAL_STACK_HPP

#include <external_interface/external_interface.h>
#define BLOCK_SIZE_DEFINE 512
#define CLEANBLOCKS_OPTIMIZATION_ENABLED
//#define DEBUG
//#define INFO
//#define WARNING

using namespace wiselib;
//14011050

template<typename Type_P, uint8_t BUFFERSIZE=2, bool PERSISTENT=true>
class ExternalStack{
    public:
	typedef wiselib::OSMODEL Os;
	typedef typename Os::block_data_t block_data_t;
	typedef Os::size_t address_t;

    private:
	typedef Type_P T;

	const uint16_t MAX_ITEMS_PER_BLOCK = BLOCK_SIZE_DEFINE/sizeof(T);
	const uint16_t MAX_ITEMS_IN_BUFFER = MAX_ITEMS_PER_BLOCK*BUFFERSIZE;

	block_data_t buffer_[BUFFERSIZE*BLOCK_SIZE_DEFINE];

	uint16_t itemsInBuffer_;

	address_t blocksOnSd_;

	const address_t minBlock_;
	const address_t maxBlock_;

#ifdef CLEANBLOCKS_OPTIMIZATION_ENABLED
	uint8_t cleanBlocks_;
	uint8_t unmodBlocks_;
#endif 

	Os::Debug::self_pointer_t debug_;
	Os::BlockMemory::self_pointer_t sd_;
    public:
	ExternalStack(Os::BlockMemory::self_pointer_t sd, address_t beginMem, address_t endMem, bool forceNew=false): sd_(sd), minBlock_(beginMem+1), maxBlock_(endMem){
	    if(BUFFERSIZE<2){ //da buffersize konstant=>kein Rechenaufwand
		if(BUFFERSIZE==1){
#ifdef WARNING
		    debug_->debug("EXTERNAL STACK WARNING: May be inefficient with buffersize 1!");
#endif
		    // Warning: Moegliches Seitenflattern
		} else {
		    debug_->debug("EXTERNAL STACK ERROR: buffersize has to be at least 1!");
		    exit(1);
		    // ERROR buffer muss mindestens 1 sein
		}
	    }
	    if(!PERSISTENT || forceNew){
		initNewStack();
	    } else {
		/**
		 * Wiederherstellen des Stacks aus persistenten Daten der SD
		 */
		sd_->read(buffer_, minBlock_-1, 1);

		blockRead<uint16_t>(buffer_,0,&itemsInBuffer_);
		blockRead<uint32_t>(buffer_,4,&blocksOnSd_);

		uint32_t tmpMinBlock=0;   blockRead<uint32_t>(buffer_,8,&tmpMinBlock);
		uint32_t tmpMaxBlock=0;   blockRead<uint32_t>(buffer_,12,&tmpMaxBlock);
		uint32_t tmpSizeof=0;     blockRead<uint32_t>(buffer_,16,&tmpSizeof);
		uint32_t tmpValcode=0;    blockRead<uint32_t>(buffer_,20,&tmpValcode);

		uint32_t valCode = itemsInBuffer_+blocksOnSd_;
		if(tmpMinBlock!= minBlock_ || tmpMaxBlock != maxBlock_ || tmpSizeof != sizeof(T) || tmpValcode != valCode){
		    //INKONSISTENT => Neuen Stack erstellen
		    initNewStack();
		} else {
		    //KONSISTENT => Buffer wiederherstellen
		    if(itemsInBuffer_>0){
			sd_->read(buffer_,minBlock_+blocksOnSd_, 1);
			debug_->debug("LOAD BUFFER: %u0000",(minBlock_+blocksOnSd_)/10000);
			debug_->debug("             +%u",(minBlock_+blocksOnSd_)%10000);
		    }
#ifdef INFO
		    debug_->debug("EXTERNAL_STACK INFO: reloaded old stack");
		    debug_->debug("itemsInBuffer_=%u",itemsInBuffer_);
		    debug_->debug("blocksOnSd_=%u0000",blocksOnSd_/10000);
		    debug_->debug("            +%u",blocksOnSd_%10000);
#endif
		}

	    }
#ifdef CLEANBLOCKS_OPTIMIZATION_ENABLED
	    cleanBlocks_=0;
	    unmodBlocks_=0;
#endif
	}


	~ExternalStack(){
	    if(PERSISTENT){
		flush(buffer_);
	    }
#ifdef INFO
	    debug_->debug("EXTERNAL_STACK INFO: destroyed stack");
#endif
	}

	/**
	 * Fuegt ein Element ans Ende des Stacks ein
	 */
	int push(T x){
	    if(minBlock_-1+blocksOnSd_+BUFFERSIZE>maxBlock_){
		return Os::ERR_NOMEM;
	    }
	    if(itemsInBuffer_>=MAX_ITEMS_IN_BUFFER){
		int err = flushBuffer();
		if(err!=Os::SUCCESS) return err;
	    }
	    blockWrite<T>(buffer_,itemsInBuffer_,x);
	    ++itemsInBuffer_;
#ifdef CLEANBLOCKS_OPTIMIZATION_ENABLED
	    //CleanBlock
	    if(cleanBlocks_>0){
		if(MAX_ITEMS_IN_BUFFER-cleanBlocks_*MAX_ITEMS_PER_BLOCK<itemsInBuffer_) cleanBlocks_-=1;
	    }
	    //
#endif

	    return Os::SUCCESS;
	}

	/**
	 * Holt das letzte Element des Stacks ohne es zu entfernen
	 */
	int top(T* x){
	    if(itemsInBuffer_<=0){
		int err = loadOneBlockIntoBuffer();
		if(err!=Os::SUCCESS) return err;
	    }
	    blockRead<T>(buffer_,itemsInBuffer_-1,x);
	    return Os::SUCCESS;
	}

	/**
	 * Holt das letzte Element des Stacks und entfernt es.
	 */
	int pop(T* x){
	    int succ = top(x);
	    if(succ == Os::SUCCESS) itemsInBuffer_-=1;
#ifdef DEBUG
	    if(succ == Os::SUCCESS){
		debug_->debug("EXTERNAL_STACK DEBUG: pop(%d) ",*x);
	    } else {
		debug_->debug("EXTERNAL_STACK DEBUG: pop unsuccessful");
	    }
#endif

#ifdef CLEANBLOCKS_OPTIMIZATION_ENABLED
	    if(unmodBlocks_*MAX_ITEMS_PER_BLOCK>itemsInBuffer_){
		unmodBlocks_-=1;
	    }
#endif
	    return succ;
	}

	/**
	 * Gibt die Anzahl der im Stack befindlichen Elemente zurueck. Der Rueckgabetyp ist mit 64Bit relativ gross, bei 32Bit gab es jedoch Probleme.
	 */
	uint64_t size(){
#ifdef DEBUG
	    debug_->debug("EXTERNAL_STACK DEBUG: size() -> %d",blocksOnSd_*MAX_ITEMS_PER_BLOCK+itemsInBuffer_);
#endif
	    return blocksOnSd_*MAX_ITEMS_PER_BLOCK+itemsInBuffer_;
	}

	/**
	 * Testet ob der Stack leer ist
	 */ 
	bool isEmpty(){
#ifdef DEBUG
	    //   debug_->debug("EXTERNAL_STACK DEBUG: isEmpty() -> %d", !(itemsInBuffer_>0 || blocksOnSd_>0));
#endif
	    return !(itemsInBuffer_>0 || blocksOnSd_>0);
	}

	/**
	 * Fuehrt ein Flush. Sollte nach diesem Flush keine weitere Operation auf dem Stack ausgefuehrt werden, so laesst er sich garantiert wiederherstellen.
	 * Fuer diese Operation wird ein temporaer Block erstellt.
	 */
	void flush(){
	    block_data_t tmpBlock[BLOCK_SIZE_DEFINE];
	    flush(tmpBlock);
	}

    private:
	/**
	 * Fuehrt den Flush aus. Der tmpBlock wird zum erstellen der zu Schreibenden Bloecke genutzt. Dies kann der Buffer sein oder ein neu erstellter Block.
	 */
	void flush(block_data_t *tmpBlock){
	    uint16_t fullBlocksToWrite=itemsInBuffer_/MAX_ITEMS_PER_BLOCK;
	    uint16_t blocksToWrite=fullBlocksToWrite+(itemsInBuffer_%MAX_ITEMS_PER_BLOCK>0?1:0);

	    if(blocksToWrite>0){ 
		sd_->write(buffer_,minBlock_+blocksOnSd_, blocksToWrite);
		itemsInBuffer_-=fullBlocksToWrite*MAX_ITEMS_PER_BLOCK;
		blocksOnSd_+=fullBlocksToWrite;
		if(tmpBlock!=buffer_ && fullBlocksToWrite>0 && fullBlocksToWrite<blocksToWrite){
		    moveBlocks(&buffer_[fullBlocksToWrite*BLOCK_SIZE_DEFINE],buffer_,1);
		}
	    }

	    blockWrite<uint16_t>(tmpBlock, 0, itemsInBuffer_);
	    blockWrite<uint32_t>(tmpBlock, 4, blocksOnSd_);

	    blockWrite<uint32_t>(tmpBlock, 8, minBlock_);
	    blockWrite<uint32_t>(tmpBlock, 12, maxBlock_);
	    blockWrite<uint32_t>(tmpBlock, 16, sizeof(T));

	    uint32_t valCode = itemsInBuffer_+blocksOnSd_;
	    blockWrite<uint32_t>(tmpBlock, 20, valCode);

	    sd_->write(tmpBlock,minBlock_-1,1);
	}

	/**
	 * Setzt die Standardparameter fuer die Erstellung des Stacks
	 */
	void  initNewStack(){
	    itemsInBuffer_=0;
	    blocksOnSd_=0;
#ifdef INFO
	    debug_->debug("EXTERNAL_STACK INFO: inited new stack");
#endif
	}

	/**
	 * Schreibt den Buffer auf SD und leer ihn vollstaendig.
	 * Vorbedingung: Buffer ist voll
	 * Nachbedingung: Buffer ist leer
	 */
	int flushBuffer(){//Buffer has to be full
#ifdef CLEANBLOCKS_OPTIMIZATION_ENABLED
	    if(unmodBlocks_>0){
		moveBlocks(&buffer_[unmodBlocks_*BLOCK_SIZE_DEFINE],buffer_,BUFFERSIZE-unmodBlocks_);
		itemsInBuffer_-=unmodBlocks_*MAX_ITEMS_PER_BLOCK;
		blocksOnSd_+=unmodBlocks_;
		unmodBlocks_=0;
		return Os::SUCCESS;
	    }
#endif
	    int err = sd_->write(buffer_,minBlock_+blocksOnSd_, BUFFERSIZE);
	    if(err!=Os::SUCCESS) return err;

	    itemsInBuffer_ = 0;
	    blocksOnSd_+=BUFFERSIZE;
#ifdef CLEANBLOCKS_OPTIMIZATION_ENABLED
	    cleanBlocks_=BUFFERSIZE;
	    unmodBlocks_=0;
#endif
	}

	/**
	 * Laedt einen Block in den Buffer
	 * Vorbedingung: Buffer ist leer
	 * Nachbedingung: Buffer enthaelt mindestens einen Block
	 */
	int loadOneBlockIntoBuffer(){

#ifdef CLEANBLOCKS_OPTIMIZATION_ENABLED
	    if(cleanBlocks_>0){
		moveBlocks(&buffer_[BLOCK_SIZE_DEFINE*(BUFFERSIZE-cleanBlocks_)],buffer_,cleanBlocks_);
		//debug_->debug(">>>>>>>>>>>>>>>>>>> cleanblocks %u",cleanBlocks_);
		itemsInBuffer_+=cleanBlocks_*MAX_ITEMS_PER_BLOCK;
		blocksOnSd_-=cleanBlocks_;
		unmodBlocks_=cleanBlocks_;
		cleanBlocks_=0;
		return Os::SUCCESS;
	    }
	    cleanBlocks_=0;
#endif

	    if(blocksOnSd_<=0) return Os::ERR_UNSPEC;//empty
	    int err = sd_->read(buffer_, minBlock_+blocksOnSd_-1, 1);
	    if(err!=Os::SUCCESS) return err;
	    itemsInBuffer_=MAX_ITEMS_PER_BLOCK;
	    blocksOnSd_-=1;
	    return Os::SUCCESS;
	}


	void moveBlocks(block_data_t* from, block_data_t* to, uint8_t count){
	    for(uint8_t i=0;i<count; i++){
		for(uint16_t j=0; j<BLOCK_SIZE_DEFINE; j++){
		    to[i*BLOCK_SIZE_DEFINE+j]=from[i*BLOCK_SIZE_DEFINE +j];
		}
	    }
	}



	template<class S>
	    void blockRead(block_data_t* block, uint16_t idx, S* x){
		uint16_t maxPerBlock = BLOCK_SIZE_DEFINE/sizeof(S);
		S* castedBlock = (S*) &block[BLOCK_SIZE_DEFINE*(idx/maxPerBlock)];
		*x=castedBlock[idx%maxPerBlock];
	    }

	template<class S>
	    void blockWrite(block_data_t* block, uint16_t idx, S x){
		uint16_t maxPerBlock = BLOCK_SIZE_DEFINE/sizeof(S);
		S* castedBlock = (S*) &block[BLOCK_SIZE_DEFINE*(idx/maxPerBlock)];
		castedBlock[idx%maxPerBlock]=x;
	    }
};

#endif

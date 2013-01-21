#ifndef EXTERNAL_STACK_HPP
#define EXTERNAL_STACK_HPP
#include <external_interface/external_interface.h>
#include <external_interface/arduino/arduino_sdcard.h>
#include <external_interface/arduino/arduino_debug.h>
#include <external_interface/arduino/arduino_clock.h>
#define BLOCK_SIZE 512
//#define DEBUG
#define INFO
//#define WARNING
using namespace wiselib;
//14011050
typedef wiselib::OSMODEL Os;
typedef typename Os::block_data_t block_data_t;


template<typename Type_P, uint32_t BUFFERSIZE=2, bool PERSISTENT=true>
class ExternalStack{
    private:
	typedef Type_P T;

	wiselib::ArduinoSdCard<Os>* sd_;
	const uint32_t MAX_ITEMS_PER_BLOCK = BLOCK_SIZE/sizeof(T);
	const uint32_t MAX_ITEMS_IN_BUFFER = MAX_ITEMS_PER_BLOCK*BUFFERSIZE;

	block_data_t buffer_[BUFFERSIZE*BLOCK_SIZE];

	uint32_t itemsInBuffer_;

	uint32_t blocksOnSd_;

	const uint32_t minBlock_;
	const uint32_t maxBlock_;

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
		/**
		  * Wiederherstellen des Stacks aus persistenten Daten der SD
		  */
		sd_->read(buffer_, minBlock_-1, 1);

		blockRead<uint32_t>(buffer_,0,&itemsInBuffer_);
		blockRead<uint32_t>(buffer_,1,&blocksOnSd_);

		uint32_t tmpMinBlock=0;   blockRead<uint32_t>(buffer_,2,&tmpMinBlock);
		uint32_t tmpMaxBlock=0;   blockRead<uint32_t>(buffer_,3,&tmpMaxBlock);
		uint32_t tmpSizeof=0;     blockRead<uint32_t>(buffer_,4,&tmpSizeof);
		uint32_t tmpValcode=0;    blockRead<uint32_t>(buffer_,5,&tmpValcode);

		uint32_t valCode = itemsInBuffer_+blocksOnSd_;
		if(tmpMinBlock!= minBlock_ || tmpMaxBlock != maxBlock_ || tmpSizeof != sizeof(T) || tmpValcode != valCode){
		    //INKONSISTENT => Neuen Stack erstellen
		    initNewStack();
		} else {
		    //KONSISTENT => Buffer wiederherstellen
		    if(itemsInBuffer_>0){
			sd_->read(buffer_,minBlock_+blocksOnSd_, 1);
		    }
#ifdef INFO
		    debug_->debug("EXTERNALSTACK INFO: reloaded old stack");
#endif
		}

	    }
	}


	~ExternalStack(){
	    if(PERSISTENT){
		flush(buffer_);
	    }
	}

	/**
	  * Fuegt ein Element ans Ende des Stacks ein
	  */
	bool push(T x){
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
		flushBuffer();
	    }
	    blockWrite<T>(buffer_,itemsInBuffer_,x);
	    ++itemsInBuffer_;
	    return true;
	}

	/**
	  * Holt das letzte Element des Stacks ohne es zu entfernen
	  */
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
	    return true;
	}

	/**
	  * Holt das letzte Element des Stacks und entfernt es.
	  */
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

	/**
	  * Gibt die Anzahl der im Stack befindlichen Elemente zurueck. Der Rueckgabetyp ist mit 64Bit relativ gross, bei 32Bit gab es jedoch Probleme.
	  */
	uint64_t size(){
#ifdef DEBUG
	    debug_->debug("EXTERNALSTACK DEBUG: size() -> %d",blocksOnSd_*MAX_ITEMS_PER_BLOCK+itemsInBuffer_);
#endif
	    return blocksOnSd_*MAX_ITEMS_PER_BLOCK+itemsInBuffer_;
	}

	/**
	  * Testet ob der Stack leer ist
	  */ 
	bool isEmpty(){
#ifdef DEBUG
	    debug_->debug("EXTERNALSTACK DEBUG: isEmpty() -> %d", !(itemsInBuffer_>0 || blocksOnSd_>0));
#endif
	    return !(itemsInBuffer_>0 || blocksOnSd_>0);
	}

	/**
	  * Fuehrt ein Flush. Sollte nach diesem Flush keine weitere Operation auf dem Stack ausgefuehrt werden, so laesst er sich garantiert wiederherstellen.
	  * Fuer diese Operation wird ein temporaer Block erstellt.
	  */
	void flush(){
#ifdef DEBUG 
	    debug_->debug("EXTERNALSTACK DEBUG: flush()");
#endif
	    block_data_t tmpBlock[BLOCK_SIZE];
	    flush(tmpBlock);
	}

    private:
	/**
	  * Fuehrt den Flush aus. Der tmpBlock wird zum erstellen der zu Schreibenden Bloecke genutzt. Dies kann der Buffer sein oder ein neu erstellter Block.
	  */
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

	/**
	  * Setzt die Standardparameter fuer die Erstellung des Stacks
	  */
	void  initNewStack(){
	    itemsInBuffer_=0;
	    blocksOnSd_=0;
#ifdef INFO
	    debug_->debug("EXTERNALSTACK INFO: inited new stack");
#endif
	}

	/**
	  * Schreibt den Buffer auf SD und leer ihn vollstaendig.
	  * Vorbedingung: Buffer ist voll
	  * Nachbedingung: Buffer ist leer
	  */
	void flushBuffer(){//Buffer has to be full
	    sd_->write(buffer_,minBlock_+blocksOnSd_, BUFFERSIZE);

	    itemsInBuffer_ = 0;
	    blocksOnSd_+=BUFFERSIZE;

	}

	/**
	  * Laedt einen Block in den Buffer
	  * Vorbedingung: Buffer ist leer
	  * Nachbedingung: Buffer enthaelt einen Block
	  */
	bool loadOneBlockIntoBuffer(){
	    if(blocksOnSd_<=0) return false;
	    sd_->read(buffer_, minBlock_+blocksOnSd_-1, 1);
	    itemsInBuffer_=MAX_ITEMS_PER_BLOCK;
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

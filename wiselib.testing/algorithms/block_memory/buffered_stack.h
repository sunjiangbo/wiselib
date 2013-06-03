/**
  * Sehr effiziente Implementierung eines Stacks fuer SD-Karten.
  * Ist Persistent und fuer (sehr)grosse Datenmengen geeignet.
  * Braucht mindestens einen Block im Arbeitsspeicher (durch Persistenz muss der Stack aber nicht dauerhaft im Speicher sein)
  * Author: Dominik Krupke
  * Das Wissen ueber die Implementierung von externen Stacks und Queues wurde auf 10 Seiten PDF festgehalten.
  */
#ifndef BUFFERED_STACK_H
#define BUFFERED_STACK_H

#include <external_interface/external_interface.h>

//#define DEBUG

using namespace wiselib;

/**
  * Template:
  * 	Type_P: Type der zu speichernden Elemente 
  *			(darf durch Bit-Kompatible Typen getauscht werden sizeof(alt)==sizeof(neu))
  * 	BUFFERSIZE: Groesse des Buffers in Bloeckgroessen. Mindestens 1.
  * 			(darf beliebig geaendert werden)
  * 	PERSISTENT: Entscheidet ob der Stack persistent sein soll. Falls false wird ein neuer Stack erstellt und bei Zerstoerung nicht gespeichert.
  * 
  * Konstruktor:
  * 	sd: Pointer auf das BlockMemory
  * 	beginMem: Erster zugeteilter Block (wird fuer MetaDaten verwendet)
  * 			(Nicht aenderbar)
  * 	endMem: Letzter zugeteilter Block
  * 			(Nicht aenderbar)
  * 	forceNew: Verhindert das Wiederherstellen bzw. erzwingt das Erstellen eines neuen leeren Stacks.
  *
  * ACHTUNG: Der zugeteilte Speicherbereich muss innerhalb des zulaessigen Bereichs des Blockmemorys liegen. Andernfalls kann es zu undefinierten Verhalten (Auch Datenverlust auf dem kompletten Speicher) kommen!
  */
  
//TODO: The type of BlockMemory should be a template parameter...
template<typename Type_P, uint8_t BUFFERSIZE=2, bool PERSISTENT=true> 
class BufferedStack{
    public:
	typedef wiselib::OSMODEL Os;
	typedef typename Os::BlockMemory::block_data_t block_data_t;
	typedef typename Os::BlockMemory::address_t address_t;

    private:
	typedef Type_P T;

	const uint16_t MAX_ITEMS_PER_BLOCK = Os::BlockMemory::BLOCK_SIZE/sizeof(T);
	const uint16_t MAX_ITEMS_IN_BUFFER = MAX_ITEMS_PER_BLOCK*BUFFERSIZE;

	block_data_t buffer_[BUFFERSIZE*Os::BlockMemory::BLOCK_SIZE];

	uint16_t itemsInBuffer_;

	address_t blocksOnSd_;

	/*const*/ address_t minBlock_; //TODO : Would be nice to make these back into const
	/*const*/ address_t maxBlock_;

	uint8_t cleanBlocks_;
	uint8_t unmodBlocks_;

	Os::Debug::self_pointer_t debug_;
	Os::BlockMemory::self_pointer_t sd_;
    public:
	int init(Os::BlockMemory::self_pointer_t sd, Os::Debug::self_pointer_t debug, address_t beginMem, address_t endMem, bool forceNew=false) {
		this->sd_ = sd;
		this->debug_ = debug;
		this->minBlock_ = beginMem+1;
		this->maxBlock_ = endMem;
		
		if(BUFFERSIZE<1){ //da buffersize konstant=>kein Rechenaufwand
			debug_->debug("EXTERNAL STACK ERROR: buffersize has to be at least 1!");
			exit(1);
			// ERROR buffer muss mindestens 1 sein
	    	}
	    	
		if(!PERSISTENT || forceNew){
			initNewStack();
		} else {
			/**
			 * Wiederherstellen des Stacks aus persistenten Daten der SD
			 */
			sd_->read(buffer_, minBlock_-1, 1);

			blockRead<uint16_t>(buffer_,0,&itemsInBuffer_);
			uint64_t tmpBlocksOnSd=0; blockRead<uint64_t>(buffer_,1,&tmpBlocksOnSd); blocksOnSd_=(address_t)tmpBlocksOnSd;

			uint64_t tmpMinBlock=0;   blockRead<uint64_t>(buffer_,2,&tmpMinBlock);
			uint64_t tmpMaxBlock=0;   blockRead<uint64_t>(buffer_,3,&tmpMaxBlock);
			uint64_t tmpSizeof=0;     blockRead<uint64_t>(buffer_,4,&tmpSizeof);
			uint64_t tmpValcode=0;    blockRead<uint64_t>(buffer_,5,&tmpValcode);

			address_t valCode = itemsInBuffer_+blocksOnSd_;
			if(tmpMinBlock!= minBlock_ || tmpMaxBlock != maxBlock_ || tmpSizeof != sizeof(T) || tmpValcode != valCode){
				//INKONSISTENT => Neuen Stack erstellen
				initNewStack();
			} else {
				//KONSISTENT => Buffer wiederherstellen
				if(itemsInBuffer_>0){
					sd_->read(buffer_,minBlock_+blocksOnSd_, 1);
				}
#ifdef DEBUG
				debug_->debug("EXTERNAL_QUEUE INFO: reloaded old stack");
#endif
			}

		}
		cleanBlocks_=0;
		unmodBlocks_=0;
	
	}
	
	BufferedStack() {}
	
	/*BufferedStack(Os::BlockMemory::self_pointer_t sd, address_t beginMem, address_t endMem, bool forceNew=false): sd_(sd), minBlock_(beginMem+1), maxBlock_(endMem){
	    if(BUFFERSIZE<1){ //da buffersize konstant=>kein Rechenaufwand
		debug_->debug("EXTERNAL STACK ERROR: buffersize has to be at least 1!");
		exit(1);
		// ERROR buffer muss mindestens 1 sein
	    }
	    if(!PERSISTENT || forceNew){
		initNewStack();
	    } else {

		//Wiederherstellen des Stacks aus persistenten Daten der SD

		sd_->read(buffer_, minBlock_-1, 1);

		blockRead<uint16_t>(buffer_,0,&itemsInBuffer_);
		uint64_t tmpBlocksOnSd=0; blockRead<uint64_t>(buffer_,1,&tmpBlocksOnSd); blocksOnSd_=(address_t)tmpBlocksOnSd;

		uint64_t tmpMinBlock=0;   blockRead<uint64_t>(buffer_,2,&tmpMinBlock);
		uint64_t tmpMaxBlock=0;   blockRead<uint64_t>(buffer_,3,&tmpMaxBlock);
		uint64_t tmpSizeof=0;     blockRead<uint64_t>(buffer_,4,&tmpSizeof);
		uint64_t tmpValcode=0;    blockRead<uint64_t>(buffer_,5,&tmpValcode);

		address_t valCode = itemsInBuffer_+blocksOnSd_;
		if(tmpMinBlock!= minBlock_ || tmpMaxBlock != maxBlock_ || tmpSizeof != sizeof(T) || tmpValcode != valCode){
		    //INKONSISTENT => Neuen Stack erstellen
		    initNewStack();
		} else {
		    //KONSISTENT => Buffer wiederherstellen
		    if(itemsInBuffer_>0){
			sd_->read(buffer_,minBlock_+blocksOnSd_, 1);
		    }
#ifdef DEBUG
		    debug_->debug("EXTERNAL_QUEUE INFO: reloaded old stack");
#endif
		}

	    }
	    cleanBlocks_=0;
	    unmodBlocks_=0;
	}*/


	~BufferedStack(){/*
	    if(PERSISTENT){
		flush(buffer_);
	    }
#ifdef DEBUG
	    debug_->debug("EXTERNAL_STACK INFO: destroyed stack");
#endif*/
	}

	/**
	 * Fuegt ein Element ans Ende des Stacks ein
	 */
	int push(T x){
	    int err = Os::SUCCESS;
	    if(minBlock_-1+blocksOnSd_+BUFFERSIZE>=maxBlock_){
		return Os::ERR_NOMEM;
	    }
	    if(itemsInBuffer_>=MAX_ITEMS_IN_BUFFER){
		err = flushBuffer();
		if(err!=Os::SUCCESS) return err;
	    }
	    blockWrite<T>(buffer_,itemsInBuffer_,x);
	    ++itemsInBuffer_;
	    if(cleanBlocks_>0){
		if(MAX_ITEMS_IN_BUFFER-cleanBlocks_*MAX_ITEMS_PER_BLOCK<itemsInBuffer_) cleanBlocks_-=1;
	    }

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
	    int err = top(x);
	    if(err!=Os::SUCCESS) return err;
	    itemsInBuffer_-=1;

	    if(unmodBlocks_*MAX_ITEMS_PER_BLOCK>itemsInBuffer_){
		unmodBlocks_-=1;
	    }
	    return Os::SUCCESS;
	}

	/**
	 * Gibt die Anzahl der im Stack befindlichen Elemente zurueck. 
	 */
	uint64_t size(){
	    return blocksOnSd_*MAX_ITEMS_PER_BLOCK+itemsInBuffer_;
	}

	/**
	 * Testet ob der Stack leer ist
	 */ 
	bool isEmpty(){
	    return !(itemsInBuffer_>0 || blocksOnSd_>0);
	}

	/**
	 * Fuehrt ein Flush. Sollte nach diesem Flush keine weitere Operation auf dem Stack ausgefuehrt werden, so laesst er sich garantiert wiederherstellen.
	 * Fuer diese Operation wird ein temporaer Block erstellt.
	 */
	int flush(){
	    block_data_t tmpBlock[Os::BlockMemory::BLOCK_SIZE];
	    return flush(tmpBlock);
	}

    private:
	/**
	 * Fuehrt den Flush aus. Der tmpBlock wird zum erstellen der zu Schreibenden Bloecke genutzt. Dies kann der Buffer sein oder ein neu erstellter Block.
	 */
	int flush(block_data_t *tmpBlock){
	    int err=Os::SUCCESS;
	    uint16_t fullBlocksToWrite=itemsInBuffer_/MAX_ITEMS_PER_BLOCK;
	    uint16_t blocksToWrite=fullBlocksToWrite+(itemsInBuffer_%MAX_ITEMS_PER_BLOCK>0?1:0);

	    if(blocksToWrite>0){ 
		err = sd_->write(buffer_,minBlock_+blocksOnSd_, blocksToWrite);
		if(err!=Os::SUCCESS) return err;
		itemsInBuffer_-=fullBlocksToWrite*MAX_ITEMS_PER_BLOCK;
		blocksOnSd_+=fullBlocksToWrite;
		if(tmpBlock!=buffer_ && fullBlocksToWrite>0 && fullBlocksToWrite<blocksToWrite){
		    moveBlocks(&buffer_[fullBlocksToWrite*Os::BlockMemory::BLOCK_SIZE],buffer_,1);
		}
	    }

	    blockWrite<uint16_t>(tmpBlock, 0, (uint16_t)itemsInBuffer_);
	    blockWrite<uint64_t>(tmpBlock, 1, (uint64_t)blocksOnSd_);

	    blockWrite<uint64_t>(tmpBlock, 2, (uint64_t)minBlock_);
	    blockWrite<uint64_t>(tmpBlock, 3, (uint64_t)maxBlock_);
	    blockWrite<uint64_t>(tmpBlock, 4, (uint64_t)sizeof(T));

	    uint32_t valCode = itemsInBuffer_+blocksOnSd_;
	    blockWrite<uint64_t>(tmpBlock, 5, (uint64_t)valCode);

	    err= sd_->write(tmpBlock,minBlock_-1,1);
	    return err;
	}

	/**
	 * Setzt die Standardparameter fuer die Erstellung des Stacks
	 */
	void  initNewStack(){
	    itemsInBuffer_=0;
	    blocksOnSd_=0;
#ifdef DEBUG
	    debug_->debug("EXTERNAL_STACK INFO: inited new stack");
#endif
	}

	/**
	 * Schreibt den Buffer auf SD und leer ihn vollstaendig.
	 * Vorbedingung: Buffer ist voll
	 * Nachbedingung: Buffer ist leer
	 */
	int flushBuffer(){//Buffer has to be full
	    int err = Os::SUCCESS;
	    if(unmodBlocks_>0){
		moveBlocks(&buffer_[unmodBlocks_*Os::BlockMemory::BLOCK_SIZE],buffer_,BUFFERSIZE-unmodBlocks_);
		itemsInBuffer_-=unmodBlocks_*MAX_ITEMS_PER_BLOCK;
		blocksOnSd_+=unmodBlocks_;
		unmodBlocks_=0;
		return err;
	    }
	    err = sd_->write(buffer_,minBlock_+blocksOnSd_, BUFFERSIZE);
	    if(err!=Os::SUCCESS) return err;
	    itemsInBuffer_ = 0;
	    blocksOnSd_+=BUFFERSIZE;
	    cleanBlocks_=BUFFERSIZE;
	    unmodBlocks_=0;
	    return Os::SUCCESS;
	}

	/**
	 * Laedt einen Block in den Buffer
	 * Vorbedingung: Buffer ist leer
	 * Nachbedingung: Buffer enthaelt mindestens einen Block
	 */
	int loadOneBlockIntoBuffer(){
	    int err = Os::SUCCESS;

	    if(cleanBlocks_>0){
		moveBlocks(&buffer_[Os::BlockMemory::BLOCK_SIZE*(BUFFERSIZE-cleanBlocks_)],buffer_,cleanBlocks_);
		itemsInBuffer_+=cleanBlocks_*MAX_ITEMS_PER_BLOCK;
		blocksOnSd_-=cleanBlocks_;
		unmodBlocks_=cleanBlocks_;
		cleanBlocks_=0;
		return Os::SUCCESS;
	    }
	    cleanBlocks_=0;

	    if(blocksOnSd_<=0) return Os::ERR_NOMEM;
	    err = sd_->read(buffer_, minBlock_+blocksOnSd_-1, 1);
	    if(err!=Os::SUCCESS) return err;
	    itemsInBuffer_=MAX_ITEMS_PER_BLOCK;
	    blocksOnSd_-=1;
	    return Os::SUCCESS; 
	}


	/**
	  * Verschiebt n Bloecke im Hauptspeicher
	  */
	void moveBlocks(block_data_t* from, block_data_t* to, uint8_t count){
	    for(uint8_t i=0;i<count; i++){
		for(uint16_t j=0; j<Os::BlockMemory::BLOCK_SIZE; j++){
		    to[i*Os::BlockMemory::BLOCK_SIZE+j]=from[i*Os::BlockMemory::BLOCK_SIZE +j];
		}
	    }
	}


/**
  * Hilfsfunktion zum Auslesen von Elementen aus Bloecken
  */
	template<class S>
	    void blockRead(block_data_t* block, uint16_t idx, S* x){
		uint16_t maxPerBlock = Os::BlockMemory::BLOCK_SIZE/sizeof(S);
		S* castedBlock = (S*) &block[Os::BlockMemory::BLOCK_SIZE*(idx/maxPerBlock)];
		*x=castedBlock[idx%maxPerBlock];
	    }
/**
  * Hilfsfunktion zum Schreiben von Elementen in Bloecke
  */
	template<class S>
	    void blockWrite(block_data_t* block, uint16_t idx, S x){
		uint16_t maxPerBlock = Os::BlockMemory::BLOCK_SIZE/sizeof(S);
		S* castedBlock = (S*) &block[Os::BlockMemory::BLOCK_SIZE*(idx/maxPerBlock)];
		castedBlock[idx%maxPerBlock]=x;
	    }
};

#endif

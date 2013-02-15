#ifndef EXTERNAL_QUEUE_HPP
#define EXTERNAL_QUEUE_HPP
#include <external_interface/external_interface.h>
//#include <external_interface/arduino/arduino_sdcard.h>
//#include <external_interface/arduino/arduino_debug.h>
//#include <external_interface/arduino/arduino_clock.h>
#define BLOCK_SIZE 512

//#define DEBUG
//#define INFO

using namespace wiselib;
//14011050


/**
 * Eine extrem IO-effiziente Implementierung einer Queue fuer die Wiselib. Ihr Nachteil ist leider ein relativ hoher RAM-Verbrauch 
 * von mindestens 2 Bloecken, dafuer erreicht sie fast den minimal moeglichen IO-Durchschnitt pro Operation.
 * Die Queue ist standardmaessig persistent und kann nicht ueberlaufen. Als Folge dieser Schutzmassnahme ist 
 * es nicht immer Moeglich den Speicherplatz vollstaendig zu nutzen. 
 * Sollte man mehrere Queue im begrenzten RAM haben, so sollte man die Persistenz ausnutzen und 
 * die Queue zerstoeren und wiederherstellen bei Bedarf
 */
template<class T, uint8_t BUFFERSIZE=2, bool PERSISTENT=true>
class ExternalQueue{

    public:
	typedef wiselib::OSMODEL Os;
	typedef typename Os::block_data_t block_data_t;

    private:
	wiselib::ArduinoSdCard<Os>* sd_;
	const uint16_t MAX_ITEMS_PER_BLOCK = BLOCK_SIZE /sizeof(T);

	block_data_t buffer_[BUFFERSIZE*BLOCK_SIZE ]; //Der Buffer

	uint32_t itemsInRead_; //Anzahl der Elemente im Readbuffer
	uint32_t itemsInWrite_; //Anzahl der Elemente im Writebuffer
	uint32_t idxRead_; //Der Readbuffer ist eine Queue und hat einen rotierenden Anfang. Der Index zeigt auf das aelteste Element

	uint32_t blocksOnSd_; //Auf der SD gespeicherte VOLLSTAENDIGE Bloecke
	uint32_t idxBeginSd_; //Die Bloecke auf SD bilden eine Queue mit rotierendem Anfang. Der Index zeigt auf den aeltesten Block

	const uint32_t minBlock_; //Blockgrenze links
	const uint32_t maxBlock_; //Blockgrenze rechts

	Os::Debug::self_pointer_t debug_;
    public:

	/**
	 * Konstruiert die Queue. 
	 *
	 * sd - Pointer auf das zu verwendende Blockinterface
	 * beginMem - Erster zu verwendende Block des Blockinterfaces
	 * endMem - Letzter zu verwendender Block des Blockinterfaces. Dieser Index wird noch verwendet.
	 * forceNew - Erzwingt das erstellen einer neuen Queue. Die alte Queue dieses Speicherplatzes wird, falls sie vorhanden war, ueberschrieben
	 *
	 */
	ExternalQueue(wiselib::ArduinoSdCard<Os>* sd,uint32_t beginMem, uint32_t endMem, bool forceNew=false):sd_(sd),minBlock_(beginMem+1),maxBlock_(endMem){
#ifdef INFO
	    debug_->debug("EXTERNALQUEUE INFO: begin init queue");
#endif
	    if(BUFFERSIZE<2){
		debug_->debug("EXTERNAL_QUEUE ERROR: Tried to init with Buffersize %d, but Buffersize has to be at least 2");
		exit(1);
	    }
	    if(!PERSISTENT||forceNew){ //Neue Queue anlegen
		initNewQueue(beginMem);
	    } else { //Alte Queue versuchen zu reloaden
		sd_->read(buffer_, beginMem, 1);

		blockRead<uint32_t>(buffer_, 0, &itemsInRead_);
		blockRead<uint32_t>(buffer_, 1, &itemsInWrite_);
		blockRead<uint32_t>(buffer_, 2, &idxRead_);
		blockRead<uint32_t>(buffer_, 3, &blocksOnSd_);
		blockRead<uint32_t>(buffer_, 4, &idxBeginSd_);

		uint32_t tmpMinBlock; blockRead<uint32_t>(buffer_, 5, &tmpMinBlock);
		uint32_t tmpMaxBlock; blockRead<uint32_t>(buffer_, 6, &tmpMaxBlock);
		uint32_t tmpSizeof;   blockRead<uint32_t>(buffer_, 7, &tmpSizeof);
		uint32_t tmpValCode;  blockRead<uint32_t>(buffer_, 8, &tmpValCode);

		uint32_t valCode=itemsInRead_+itemsInWrite_+idxRead_+blocksOnSd_+idxBeginSd_;

		if(minBlock_!=tmpMinBlock || maxBlock_!=tmpMaxBlock || sizeof(T)!= tmpSizeof || valCode != tmpValCode){//Inkonsistent=>Neuer Stack
		    initNewQueue(beginMem);
		} else {
		    if(itemsInWrite_>0){
			sd_->read(&buffer_[512], calcIdxOfNextFreeBlock(),1);
		    }
		    if(itemsInRead_>0){
			sd_->read(buffer_,calcIdxOfFirstFreeBlock(),1);
		    }
#ifdef INFO
		    debug_->debug("EXTERNALQUEUE INFO: reloaded old queue");
#endif
		}
	    }
	}


	~ExternalQueue(){
	    if(PERSISTENT){
		flush(buffer_);
	    }
	}



	bool addLast(T x){//TODO groessere Sicherheit vor Datenverlust
#ifdef DEBUG
	    debug_->debug("EXTERNALQUEUE DEBUG: push(%d)->%d",x,minBlock_+blocksOnSd_+BUFFERSIZE>maxBlock_);
#endif
	    if(blocksOnSdLeft()<BUFFERSIZE) return false;
	    if(blocksOnSd_<=0){
		if(itemsInWrite_>0 || isReadBufferFull()){
		    if(isWriteBufferFull()) flushWrite();
		    addLast_write(x);
		} else {
		    addLast_read(x);
		}
	    } else {
		if(isWriteBufferFull()) {//Sollte eigentlich niemals eintretten.
		    if(!flushWrite()) return false;
		}
		addLast_write(x);
	    }
	    return true;
	}

	bool addFirst(T x){
	    if(blocksOnSdLeft()<BUFFERSIZE) return false;
	    if(isReadBufferFull()){
		flushRead();
	    }
	    addFirst_read(x);
	    return true;
	}

	bool removeFirst(T* x){//Ergaenzung um einen float-param
	    if(itemsInRead_<=0){
		bool succ=fillReadBuffer();
#ifdef DEBUG
		if(!succ) debug_->debug("EXTERNALQUEUE DEBUG: pop unsuccessfull");
#endif 
		if(!succ) return false;
	    } 
	    removeFirst_read(x);
#ifdef DEBUG
	    debug_->debug("EXTERNALQUEUE DEBUG: pop(%d)",*x);
#endif
	    return true;
	}

	bool removeLast(T* x){
	    if(itemsInWrite_==0){
		if(blocksOnSd_==0){
		    if(itemsInRead_==0) return false;
		    removeLast_read(x);
		    return true;
		} else {
		    sd_->read(&buffer_[BLOCK_SIZE],calcIdxOfLastBlock(),1);
		}
	    }
	    removeLast_write(x);
	    return true;
	}


	bool top(T* x){//same as above
	    if(itemsInRead_<=0){
		bool succ=fillReadBuffer();
#ifdef DEBUG
		if(!succ) debug_->debug("EXTERNALQUEUE DEBUG: pop unsuccessfull");
#endif 
		if(!succ) return false;
	    } 
	    blockRead<T>(buffer_,idxRead_, x);
#ifdef DEBUG
	    debug_->debug("EXTERNALQUEUE DEBUG: pop(%d)",*x);
#endif
	    return true;
	}

	bool isEmpty(){
#ifdef DEBUG
	    debug_->debug("EXTERNALQUEUE DEBUG: isEmpty()->%d",!(blocksOnSd_>0 || itemsInRead_>0 || itemsInWrite_>0));
#endif
	    return !(blocksOnSd_>0 || itemsInRead_>0 || itemsInWrite_>0);
	}

	uint64_t size(){
#ifdef DEBUG
	    debug_->debug("EXTERNALQUEUE DEBUG: size()->%d",blocksOnSd_*MAX_ITEMS_PER_BLOCK+itemsInWrite_+itemsInRead_);
#endif
	    return blocksOnSd_*MAX_ITEMS_PER_BLOCK+itemsInWrite_+itemsInRead_;
	}

	void flush(){
#ifdef DEBUG
	    debug_->debug("EXTERNALQUEUE DEBUG: flush()");
#endif
	    block_data_t tmpBlock[512];
	    flush(tmpBlock);
	}



    private:
	void flush(block_data_t *tmpBlock){
	    //Writebuffer schreiben
	    uint8_t fullBlocksToWrite=itemsInWrite_/MAX_ITEMS_PER_BLOCK;
	    uint8_t blocksToWrite=fullBlocksToWrite+(itemsInWrite_%MAX_ITEMS_PER_BLOCK>0?1:0);
	    if(blocksToWrite>0){
		uint8_t max=maxBlock_-(calcIdxOfNextFreeBlock()-1);
		if(max>=blocksToWrite){
		    sd_->write(&buffer_[BLOCK_SIZE ], calcIdxOfNextFreeBlock(), blocksToWrite);
		    blocksOnSd_+=fullBlocksToWrite;
		} else {
		    sd_->write(&buffer_[BLOCK_SIZE ], calcIdxOfNextFreeBlock(), max);
		    blocksOnSd_+=max;
		    sd_->write(&buffer_[BLOCK_SIZE +BLOCK_SIZE *max], calcIdxOfNextFreeBlock(), blocksToWrite-max);
		    blocksOnSd_+=fullBlocksToWrite-max;
		}
		itemsInWrite_%=MAX_ITEMS_PER_BLOCK; //Beim reload werden unvollstaendige Bloecke automatisch wieder in den Buffer geladen
	    }

	    //Readbuffer schreiben
	    if(itemsInRead_>0){
		sd_-> write(buffer_, calcIdxOfFirstFreeBlock(),1);
	    }

	    //Variablen sichern
	    blockWrite<uint32_t>(tmpBlock,0,itemsInRead_);
	    blockWrite<uint32_t>(tmpBlock,1,itemsInWrite_);
	    blockWrite<uint32_t>(tmpBlock,2,idxRead_);
	    blockWrite<uint32_t>(tmpBlock,3,blocksOnSd_);
	    blockWrite<uint32_t>(tmpBlock,4,idxBeginSd_);

	    blockWrite<uint32_t>(tmpBlock,5,minBlock_);
	    blockWrite<uint32_t>(tmpBlock,6,maxBlock_);
	    blockWrite<uint32_t>(tmpBlock,7,sizeof(T));

	    uint32_t valCode=itemsInRead_+itemsInWrite_+idxRead_+blocksOnSd_+idxBeginSd_;
	    blockWrite<uint32_t>(tmpBlock,8,valCode);

	    sd_-> write(tmpBlock,minBlock_-1,1);

	}

	/**
	 * Setzt die Variablen der Queue auf die Defaultwerte
	 */
	void initNewQueue(uint32_t beginMem){	
	    itemsInRead_=0;
	    itemsInWrite_=0;
	    idxRead_=0;

	    blocksOnSd_=0;
	    idxBeginSd_=beginMem+1;
#ifdef INFO
	    debug_->debug("EXTERNALQUEUE INFO: inited new queue");
#endif
	}



	bool fillReadBuffer(){
	    if(blocksOnSd_<=0) {
		/**
		  * Es sind keine Bloecke auf SD, daher versuchen wir Elemente aus dem WriteBufer zu kopieren
		  */
		if(itemsInWrite_<=0) return false; //Funktioniert natuerlich nur, wenn da Elemente enthalten sind
		//Bloecke aus Write nach read schieben
		uint8_t blocksToMove=itemsInWrite_/MAX_ITEMS_PER_BLOCK+(itemsInWrite_%MAX_ITEMS_PER_BLOCK>0?1:0);
		moveBlocks(&buffer_[512],buffer_,blocksToMove);

		idxRead_=0;
		if(itemsInWrite_>MAX_ITEMS_PER_BLOCK){
		    itemsInRead_=MAX_ITEMS_PER_BLOCK;
		    itemsInWrite_-=MAX_ITEMS_PER_BLOCK;
		} else {
		    itemsInRead_=itemsInWrite_;
		    itemsInWrite_=0;
		}
	    } else {
		/**
		  * Wir muessen die Daten von der SD holen. Wir holen nur einen Block.
		  */
		sd_->read(buffer_, idxBeginSd_, 1);
		itemsInRead_=MAX_ITEMS_PER_BLOCK; //Wir holen immer nur volle Bloecke
		if(++idxBeginSd_>maxBlock_) idxBeginSd_=minBlock_; //Erster relevante Block auf SD um 1 erhoehen
		idxRead_=0; //Die Bloecke auf der SD sind geordnet
		--blocksOnSd_;
	    }
	    return true;
	}


	void removeFirst_read(T* x){
	    //Read ist nicht leer
	    blockRead<T>(buffer_,idxRead_,x);
	    if(itemsInRead_<=0){
		itemsInRead_=0;
		idxRead_=0;
	    } else {
		idxRead_=(idxRead_+1)%MAX_ITEMS_PER_BLOCK;
		--itemsInRead_;
	    }
	}
	void removeLast_read(T* x){
	    uint16_t idx = (idxRead_+itemsInRead_-1)%MAX_ITEMS_PER_BLOCK;
	    blockRead<T>(buffer_, idx ,x);
	    --itemsInRead_;
	}
	    

	void addLast_read(T x){
	    //read ist nicht voll
	    blockWrite<T>(buffer_, (idxRead_+itemsInRead_)%MAX_ITEMS_PER_BLOCK , x);
	    ++itemsInRead_;
	}
	void addFirst_read(T x){
	    //read ist nicht voll
	    if(idxRead_>0) --idxRead_;
	    else idxRead=MAX_ITEMS_PER_BLOCK-1;
	    blockWrite<T>(buffer_,idxRead,1);
	    ++itemsInRead_;
	}

	/**
	  * Fuegt ein Element in den WriteBuffer ein
	  */
	void addLast_write(T x){
	    blockWrite<T>(&buffer_[BLOCK_SIZE ], itemsInWrite_, x);
	    ++itemsInWrite_;
	}
	void removeLast_write(T* x){
	    blockRead<T>(&buffer_[BLOCK_SIZE], itemsInWrite_, x);
	    --itemsInWrite_;
	}

/**
  * FALSE wenn im WriteBuffer noch Platz ist.
  */
	bool isWriteBufferFull(){
	    return itemsInWrite_>=MAX_ITEMS_PER_BLOCK*(BUFFERSIZE-1);
	}
/**
  * false falls im ReadBuffer noch Platz ist.
  */
	bool isReadBufferFull(){
	    return itemsInRead_>=MAX_ITEMS_PER_BLOCK;
	}

/**
  * Leert den VOLLEN(!) WriteBuffer
  */
	bool flushWrite(){
	    //Vorbedingung WRITE FULL => ALLE BLOECKE VOLL
	    if(blocksOnSdLeft()<=BUFFERSIZE) return false; //Es ist genug platz auf der SD
	    if(blocksOnSd_<=0 && itemsInRead_<=0){ //erster Block von Write kann nach Read kopiert werden

		T* castedBufferRead = (T*) buffer_;
		T* castedBufferWrite = (T*) &buffer_[BLOCK_SIZE ];

		itemsInRead_=0; idxRead_=0;
		for(uint32_t i=0; i<MAX_ITEMS_PER_BLOCK; i++){ //Evtl schoener direkt ueber itemsInRead
		    castedBufferRead[i]=castedBufferWrite[i];
		}
		itemsInRead_=MAX_ITEMS_PER_BLOCK;
		itemsInWrite_-=MAX_ITEMS_PER_BLOCK;

		if(itemsInWrite_>0){ //restliche Bloecke auf SD schreiben
		    idxBeginSd_=minBlock_;

		    sd_->write(&buffer_[BLOCK_SIZE *2], minBlock_, BUFFERSIZE-2);

		    blocksOnSd_+= BUFFERSIZE-2;
		}
		itemsInWrite_=0;
	    } else { //kompletten WriteBuffer auf SD schreiben

		if(blocksOnSd_==0) idxBeginSd_=minBlock_;
		uint32_t max=maxBlock_-calcIdxOfNextFreeBlock()+1;//Maximal in einem Stueck schreibbare bloecke

		if(max>=BUFFERSIZE-1){//Der komplette Buffer kann in einem Stueck geschrieben werden.
		    sd_->write(&buffer_[BLOCK_SIZE ], calcIdxOfNextFreeBlock(), BUFFERSIZE-1);
		    blocksOnSd_+=BUFFERSIZE-1;
		    itemsInWrite_=0;
		} else {//Nur der vordere Teil kann in einem Stueck geschrieben werden. Der hintere wird nach vorne geschoben
		    uint32_t blocksLeft= BUFFERSIZE-1-max;
		    sd_->write(&buffer_[BLOCK_SIZE ], calcIdxOfNextFreeBlock(), max);
		    blocksOnSd_+=max;
		    itemsInWrite_-=max*MAX_ITEMS_PER_BLOCK;

		    moveBlocks(&buffer_[(max+1)*512],&buffer_[512],blocksLeft);
		}
	    }

	    return true;
	}


	/**
	 *Index vom naechsten freien Block, hinter den beschriebenen`
	 */
	uint32_t calcIdxOfNextFreeBlock(){//naechster Freier Block
	    uint32_t idxEnd= idxBeginSd_+blocksOnSd_;
	    if(idxEnd>maxBlock_){
		idxEnd-=(maxBlock_-minBlock_);
	    }
	    return idxEnd;
	}

	/**
	 * Index von erstem freien Block, vor den beschriebenen
	 */
	uint32_t calcIdxOfFirstFreeBlock(){
	    uint32_t idxBegin = idxBeginSd_-1;
	    if(idxBegin < minBlock_) idxBegin=maxBlock_;
	    return idxBegin;
	}

	uint32_t calcIdxOfLastBlock(){
	    uint32_t idxEnd= idxBeginSd_+blocksOnSd_-1;
	    if(idxEnd>maxBlock_){
		idxEnd-=(maxBlock_-minBlock_);
	    }
	    return idxEnd;
	}

	/**
	 * Anzahl der verlbeibenden freien Bloecke
	 */
	uint32_t blocksOnSdLeft(){
	    return maxBlock_-minBlock_+1-blocksOnSd_;
	}

	/**
	 * Liest ein Element aus Bloecken wie aus ein passendes Array. Blocksprunge werden ausgefuehrt.
	 */
	template<class S>
	    void blockRead(block_data_t* block, uint32_t idx, S* x){
		uint32_t maxPerBlock = BLOCK_SIZE /sizeof(S);
		S* castedBlock = (S*) &block[BLOCK_SIZE *(idx/maxPerBlock)];
		*x=castedBlock[idx%maxPerBlock];
	    }

	/**
	 * Schreibt ein Element passend in Bloecke
	 */
	template<class S>
	    void blockWrite(block_data_t* block, uint32_t idx, S x){
		uint32_t maxPerBlock = BLOCK_SIZE /sizeof(S);
		S* castedBlock = (S*) &block[BLOCK_SIZE *(idx/maxPerBlock)];
		castedBlock[idx%maxPerBlock]=x;
	    }

	void moveBlocks(block_data_t* from, block_data_t* to, uint8_t count){
	    for(uint8_t i=0;i<count; i++){
		for(uint16_t j=0; j<512; j++){
		    to[i*512+j]=from[i*512+j];
		}
	    }
	}

};
#endif

#ifndef EXTERNAL_QUEUE_HPP
#define EXTERNAL_QUEUE_HPP
#include <external_interface/external_interface.h>
#define BLOCK_SIZE_DEFINE Os::BlockMemory::BLOCK_SIZE

//#define DEBUG
//#define INFO

using namespace wiselib;
//14011050
/**
 * Eine extrem IO-effiziente Implementierung einer Queue fuer die Wiselib. Ihr Nachteil ist leider ein relativ hoher RAM-Verbrauch 
 * von mindestens 2 Bloecken, dafuer erreicht sie fast den minimal moeglichen IO-Durchschnitt pro Operation.
 * Die Queue ist standardmaessig persistent und kann nicht ueberlaufen. Als Folge dieser Schutzmassnahme ist 
 * es nicht immer Moeglich den Speicherplatz vollstaendig zu nutzen. 
 * Sollte man mehrere Queue im begrenzten RAM haben, so sollte man die Persistenz ausnutzen => 
 * die Queue zerstoeren und wiederherstellen bei Bedarf
 */
template<class T, uint8_t BUFFERSIZE=2, bool PERSISTENT=true>
class ExternalQueue{
    public:
	typedef wiselib::OSMODEL Os;
	typedef typename Os::BlockMemory::block_data_t block_data_t;
	typedef typename Os::BlockMemory::address_t address_t;

    private:
	const uint16_t MAX_ITEMS_PER_BLOCK = BLOCK_SIZE_DEFINE /sizeof(T);

	block_data_t buffer_[BUFFERSIZE*BLOCK_SIZE_DEFINE ]; //Der Buffer

	uint16_t itemsInRead_; //Anzahl der Elemente im Readbuffer
	uint16_t itemsInWrite_; //Anzahl der Elemente im Writebuffer
	uint16_t idxRead_; //Der Readbuffer ist eine Queue und hat einen rotierenden Anfang. Der Index zeigt auf das aelteste Element

	address_t blocksOnSd_; //Auf der SD gespeicherte VOLLSTAENDIGE Bloecke
	address_t idxBeginSd_; //Die Bloecke auf SD bilden eine Queue mit rotierendem Anfang. Der Index zeigt auf den aeltesten Block

	const address_t minBlock_; //Blockgrenze links
	const address_t maxBlock_; //Blockgrenze rechts

	Os::Debug::self_pointer_t debug_;
	Os::BlockMemory::self_pointer_t sd_;
    public:

	/**
	 * Konstruiert die Queue. 
	 *
	 * sd - Pointer auf das zu verwendende Blockinterface
	 * beginMem - Erster zu verwendende Block des Blockinterfaces
	 * endMem - Letzter zu verwendender Block des Blockinterfaces. Dieser Index wird noch verwendet.
	 * forceNew - Erzwingt das Erstellen einer neuen Queue. Die alte Queue dieses Speicherplatzes wird, falls sie vorhanden war, ueberschrieben
	 *
	 */
	ExternalQueue(Os::BlockMemory::self_pointer_t sd,address_t beginMem, address_t endMem, bool forceNew=false):sd_(sd),minBlock_(beginMem+1),maxBlock_(endMem){
	    if(BUFFERSIZE<2){
		debug_->debug("EXTERNAL_QUEUE ERROR: Tried to init with Buffersize %d, but Buffersize has to be at least 2");
		exit(1);
	    }
	    if(!PERSISTENT||forceNew){ //Neue Queue anlegen
		initNewQueue(beginMem);
	    } else { //Alte Queue versuchen zu reloaden
		sd_->read(buffer_, beginMem, 1);

		blockRead<uint16_t>(buffer_, 0, &itemsInRead_);
		blockRead<uint16_t>(buffer_, 4, &itemsInWrite_);
		blockRead<uint16_t>(buffer_, 8, &idxRead_);
		blockRead<address_t>(buffer_, 12, &blocksOnSd_);
		blockRead<address_t>(buffer_, 16, &idxBeginSd_);

		address_t tmpMinBlock; blockRead<address_t>(buffer_, 20, &tmpMinBlock);
		address_t tmpMaxBlock; blockRead<address_t>(buffer_, 24, &tmpMaxBlock);
		uint32_t tmpSizeof;   blockRead<uint32_t>(buffer_, 28, &tmpSizeof);
		uint32_t tmpValCode;  blockRead<uint32_t>(buffer_, 32, &tmpValCode);

		uint32_t valCode=itemsInRead_+itemsInWrite_+idxRead_+blocksOnSd_+idxBeginSd_;

		if(minBlock_!=tmpMinBlock || maxBlock_!=tmpMaxBlock || sizeof(T)!= tmpSizeof || valCode != tmpValCode){//Inkonsistent=>Neuer Stack
		    initNewQueue(beginMem);
		} else {
		    if(itemsInWrite_>0){
			sd_->read(&buffer_[BLOCK_SIZE_DEFINE], calcIdxOfNextFreeBlock(),1);
		    }
		    if(itemsInRead_>0){
			sd_->read(buffer_,calcIdxOfFirstFreeBlock(),1);
		    }
#ifdef INFO
		    debug_->debug("EXTERNAL_QUEUE INFO: reloaded old queue");
#endif
		}
	    }

	}


	~ExternalQueue(){
#ifdef INFO
	    debug_->debug("EXTERNAL_QUEUE INFO: destroyed queue");
#endif
	    if(PERSISTENT){
		flush(buffer_);
	    }
	}


	/**
	 * Fuegt ein Element an das Ende der Queue oder gibt FALSE zurueck.
	 */
	int offer(T x){//TODO groessere Sicherheit vor Datenverlust
	    int err = Os::SUCCESS;
	    if(blocksOnSdLeft()<BUFFERSIZE){
		return Os::ERR_NOMEM;
	    }
	    if(blocksOnSd_==0){
		if(itemsInWrite_>0 || isReadBufferFull()){
		    if(isWriteBufferFull()) flushWrite();
		    addLast_write(x);
		} else {
		    addLast_read(x);
		}
	    } else {
		if(isWriteBufferFull()) {
		    err = flushWrite();
		    if(err!=Os::SUCCESS) return err;
		}
		addLast_write(x);
	    }
	    return Os::SUCCESS;
	}

	/**
	 * Gibt das erste Element der Queue zurueck und entfernt es oder FALSE
	 */
	int poll(T* x){
	    int err = Os::SUCCESS;
	    if(itemsInRead_<=0){
		err = fillReadBuffer();
		if(err!=Os::SUCCESS){ return err; }
	    } 
	    removeFirst_read(x);
	    return Os::SUCCESS;
	}

	/**
	 * Gibt das erste Element der Queue zurueck ohne es zu entfernen oder FALSE
	 */
	int peek(T* x){//same as above
	    int err = Os::SUCCESS;
	    if(itemsInRead_<=0){
		err=fillReadBuffer();
		if(err!=Os::SUCCESS) return err;
	    } 
	    blockRead<T>(buffer_,idxRead_, x);
	    return Os::SUCCESS;
	}

	/**
	 * Gibt an, ob die Queue leer ist
	 */
	bool isEmpty(){
	    return !(blocksOnSd_>0 || itemsInRead_>0 || itemsInWrite_>0);
	}

	/**
	 * Gibt an, wie viele Elemente in der Queue sind
	 */
	uint64_t size(){
	    return blocksOnSd_*MAX_ITEMS_PER_BLOCK+itemsInWrite_+itemsInRead_;
	}

	/**
	 * Speichert den jetztigen Zustand der Queue. Sollten keine weiteren Operationen auf der Queue ausgefuehrt werden,
	 * so laesst sie sich garantiert wiederherstellen. 
	 */
	int flush(){
	    block_data_t tmpBlock[BLOCK_SIZE_DEFINE];
	    return flush(tmpBlock);
	}

    private:
	/**
	 * Fuehrt die Speicherung des jetztigen Zustandes der Queue aus. Der uebergebene Block wird zum schreiben auf die SD
	 * gebraucht und kann auch der Buffer sein. Der Buffer wird als erstes gesichert und braucht den temporaeren Block nicht.
	 */
	int flush(block_data_t *tmpBlock){
	    //Writebuffer schreiben
	    int err = Os::SUCCESS;
	    uint8_t fullBlocksToWrite=itemsInWrite_/MAX_ITEMS_PER_BLOCK;
	    uint8_t blocksToWrite=fullBlocksToWrite+(itemsInWrite_%MAX_ITEMS_PER_BLOCK>0?1:0);
	    if(blocksToWrite>0){
		address_t max=maxBlock_-(calcIdxOfNextFreeBlock()-1);//<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<
		if(max>=blocksToWrite){
		    err = sd_->write(&buffer_[BLOCK_SIZE_DEFINE ], calcIdxOfNextFreeBlock(), blocksToWrite);
		    if(err!=Os::SUCCESS) return err;

		    blocksOnSd_+=fullBlocksToWrite;
		} else {
		    err = sd_->write(&buffer_[BLOCK_SIZE_DEFINE ], calcIdxOfNextFreeBlock(), max);
		    if(err!=Os::SUCCESS) return err;
		    blocksOnSd_+=max;
		    err = sd_->write(&buffer_[BLOCK_SIZE_DEFINE +BLOCK_SIZE_DEFINE *max], calcIdxOfNextFreeBlock(), blocksToWrite-max);
		    if(err!=Os::SUCCESS) return err;
		    blocksOnSd_+=fullBlocksToWrite-max;
		}
		if(tmpBlock!=buffer_ && fullBlocksToWrite>0 && fullBlocksToWrite<blocksToWrite){
		    moveBlocks(&buffer_[(1+fullBlocksToWrite)*BLOCK_SIZE_DEFINE],&buffer_[BLOCK_SIZE_DEFINE],1);
		}
		itemsInWrite_%=MAX_ITEMS_PER_BLOCK; //Beim reload werden unvollstaendige Bloecke automatisch wieder in den Buffer geladen
	    }

	    //Readbuffer schreiben
	    if(itemsInRead_>0){
		err = sd_-> write(buffer_, calcIdxOfFirstFreeBlock(),1);
		if(err!=Os::SUCCESS) return err;
	    }

	    //Variablen sichern
	    blockWrite<uint16_t>(tmpBlock,0,itemsInRead_);
	    blockWrite<uint16_t>(tmpBlock,4,itemsInWrite_);
	    blockWrite<uint16_t>(tmpBlock,8,idxRead_);
	    blockWrite<address_t>(tmpBlock,12,blocksOnSd_);
	    blockWrite<address_t>(tmpBlock,16,idxBeginSd_);

	    blockWrite<address_t>(tmpBlock,20,minBlock_);
	    blockWrite<address_t>(tmpBlock,24,maxBlock_);
	    blockWrite<uint32_t>(tmpBlock,28,sizeof(T));

	    uint32_t valCode=itemsInRead_+itemsInWrite_+idxRead_+blocksOnSd_+idxBeginSd_;
	    blockWrite<uint32_t>(tmpBlock,32,valCode);

	    err = sd_->write(tmpBlock,minBlock_-1,1);
	    return err;

	}

	/**
	 * Setzt die Variablen der Queue auf die Defaultwerte
	 */
	void initNewQueue(address_t beginMem){	
	    itemsInRead_=0;
	    itemsInWrite_=0;
	    idxRead_=0;

	    blocksOnSd_=0;
	    idxBeginSd_=beginMem+1;
#ifdef INFO
	    debug_->debug("EXTERNAL_QUEUE INFO: inited new queue");
#endif
	}


	/**
	 * Fuellt den ReadBuffer durch einen Block von der SD
	 */
	int fillReadBuffer(){
	    if(blocksOnSd_<=0) {
		/**
		 * Es sind keine Bloecke auf SD, daher versuchen wir Elemente aus dem WriteBufer zu kopieren
		 */

		if(itemsInWrite_<=0) return Os::ERR_UNSPEC; //Funktioniert natuerlich nur, wenn da Elemente enthalten sind

		//Bloecke um einen nach vorne Schieben und damit den Readbuffer fuellen
		uint8_t blocksToMove=itemsInWrite_/MAX_ITEMS_PER_BLOCK+(itemsInWrite_%MAX_ITEMS_PER_BLOCK>0?1:0);//Wir wollen keine leeren Bloecke verschieben
		moveBlocks(&buffer_[BLOCK_SIZE_DEFINE],buffer_,blocksToMove);

		idxRead_=0;//Die Bloecke in Write sind geordnet

		if(itemsInWrite_>MAX_ITEMS_PER_BLOCK){//Es wurde ein voller Bock aus Write nach Read verschoben
		    itemsInRead_=MAX_ITEMS_PER_BLOCK;
		    itemsInWrite_-=MAX_ITEMS_PER_BLOCK;
		} else {//Es wurde nur ein partieller Block aus Write nach Read verschoben
		    itemsInRead_=itemsInWrite_;
		    itemsInWrite_=0;
		}

	    } else {
		/**
		 * Wir muessen die Daten von der SD holen. Wir holen nur einen Block.
		 */
		int err = sd_->read(buffer_, idxBeginSd_, 1);
		if(err!=Os::SUCCESS) return err;
		itemsInRead_=MAX_ITEMS_PER_BLOCK; //Wir holen immer nur volle Bloecke
		if(++idxBeginSd_>maxBlock_) idxBeginSd_=minBlock_; //Erster relevante Block auf SD um 1 erhoehen
		idxRead_=0; //Die Bloecke auf der SD sind geordnet
		--blocksOnSd_;
	    }
	    return Os::SUCCESS;
	}

	/**
	 * Entfernt ein Element vom Anfang des Readbuffers
	 * Vorbedingung: ReadBuffer nicht leer
	 */
	void removeFirst_read(T* x){
	    blockRead<T>(buffer_,idxRead_,x);
	    if(itemsInRead_<=0){
		itemsInRead_=0;
		idxRead_=0;
	    } else {
		idxRead_=(idxRead_+1)%MAX_ITEMS_PER_BLOCK;
		--itemsInRead_;
	    }
	}

	/**
	 * Fuegt ein Element an das Ende des Readbuffers ein
	 * Vorbedinung: Readbuffer nicht voll
	 */
	void addLast_read(T x){
	    blockWrite<T>(buffer_, (idxRead_+itemsInRead_)%MAX_ITEMS_PER_BLOCK , x);
	    ++itemsInRead_;
	}

	/**
	 * Fuegt ein Element in den WriteBuffer ein
	 */
	void addLast_write(T x){
	    blockWrite<T>(&buffer_[BLOCK_SIZE_DEFINE ], itemsInWrite_, x);
	    ++itemsInWrite_;
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
	 * Vorbedingung: WriteBuffer ist voll
	 * Nachbedingung: WriteBuffer ist um mindestens einen Block leerer
	 */
	int flushWrite(){
	    int err = Os::SUCCESS;
	    //  if(blocksOnSdLeft()<=BUFFERSIZE) return false; //Es ist nicht genug platz auf der SD
	    if(blocksOnSd_<=0 && itemsInRead_<=0){ //erster Block von Write kann nach Read kopiert werden
		moveBlocks(&buffer_[BLOCK_SIZE_DEFINE],buffer_,BUFFERSIZE-1);
		itemsInWrite_-=MAX_ITEMS_PER_BLOCK;
		itemsInRead_=MAX_ITEMS_PER_BLOCK;
		idxRead_=0;
	    } else { //kompletten WriteBuffer auf SD schreiben

		if(blocksOnSd_==0) idxBeginSd_=minBlock_;
		address_t max=maxBlock_-calcIdxOfNextFreeBlock()+1;//Maximal in einem Stueck schreibbare bloecke

		if(max>=BUFFERSIZE-1){//Der komplette Buffer kann in einem Stueck geschrieben werden.
		    err = sd_->write(&buffer_[BLOCK_SIZE_DEFINE ], calcIdxOfNextFreeBlock(), BUFFERSIZE-1);
		    if(err!=Os::SUCCESS) return err;
		    blocksOnSd_+=BUFFERSIZE-1;
		    itemsInWrite_=0;
		} else {//Nur der vordere Teil kann in einem Stueck geschrieben werden. Der hintere wird nach vorne geschoben
		    uint8_t blocksLeft= BUFFERSIZE-1-max;
		    err = sd_->write(&buffer_[BLOCK_SIZE_DEFINE ], calcIdxOfNextFreeBlock(), max);
		    if(err!=Os::SUCCESS) return err;
		    blocksOnSd_+=max;
		    itemsInWrite_-=max*MAX_ITEMS_PER_BLOCK;

		    moveBlocks(&buffer_[(max+1)*BLOCK_SIZE_DEFINE],&buffer_[BLOCK_SIZE_DEFINE],blocksLeft);
		}
	    }

	    return Os::SUCCESS;
	}


	/**
	 *Index vom naechsten freien Block, hinter den beschriebenen`
	 */
	address_t calcIdxOfNextFreeBlock(){//naechster Freier Block
	    address_t idxEnd= idxBeginSd_+blocksOnSd_;
	    if(idxEnd>maxBlock_){
		idxEnd-=((maxBlock_-minBlock_)+1);
	    }
	    return idxEnd;
	}

	/**
	 * Index von erstem freien Block, vor den beschriebenen
	 */
	address_t calcIdxOfFirstFreeBlock(){
	    address_t idxBegin = idxBeginSd_-1;
	    if(idxBegin < minBlock_){ idxBegin=maxBlock_;}
	    return idxBegin;
	}

	/**
	 * Anzahl der verlbeibenden freien Bloecke
	 */
	address_t blocksOnSdLeft(){
	    return (maxBlock_-minBlock_)+1-blocksOnSd_;
	}

	/**
	 * Liest ein Element aus Bloecken wie aus ein passendes Array. Blocksprunge werden ausgefuehrt.
	 */
	template<class S>
	    void blockRead(block_data_t* block, uint16_t idx, S* x){
		uint16_t maxPerBlock = BLOCK_SIZE_DEFINE /sizeof(S);
		S* castedBlock = (S*) &block[BLOCK_SIZE_DEFINE *(idx/maxPerBlock)];
		*x=castedBlock[idx%maxPerBlock];
	    }

	/**
	 * Schreibt ein Element passend in Bloecke
	 */
	template<class S>
	    void blockWrite(block_data_t* block, uint16_t idx, S x){
		uint16_t maxPerBlock = BLOCK_SIZE_DEFINE /sizeof(S);
		S* castedBlock = (S*) &block[BLOCK_SIZE_DEFINE *(idx/maxPerBlock)];
		castedBlock[idx%maxPerBlock]=x;
	    }

	void moveBlocks(block_data_t* from, block_data_t* to, uint8_t count){
	    for(uint8_t i=0;i<count; i++){
		for(uint16_t j=0; j<BLOCK_SIZE_DEFINE; j++){
		    to[i*BLOCK_SIZE_DEFINE+j]=from[i*BLOCK_SIZE_DEFINE+j];
		}
	    }
	}

};
#endif

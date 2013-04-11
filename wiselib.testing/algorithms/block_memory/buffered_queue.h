/**
 * Sehr effiziente Implementierung einer Queue fuer SD-Karten.
 * Ist Persistent und fuer (sehr)grosse Datenmengen geeignet.
 * Braucht mindestens zwei Bloecke im Arbeitsspeicher (durch Persistenz muess die Queue aber nicht dauerhaft im Speicher sein)
 * Author: Dominik Krupke
 * Das Wissen ueber die Implementierung von externen Stacks und Queues wurde auf 10 Seiten PDF festgehalten
 **/
 
#ifndef BUFFERED_QUEUE_H
#define BUFFERED_QUEUE_H
#include <external_interface/external_interface.h>
#define BLOCK_SIZE_DEFINE Os::BlockMemory::BLOCK_SIZE

//#define DEBUG
 
using namespace wiselib
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
;
template<class T, uint8_t BUFFERSIZE=2, bool PERSISTENT=true>
class BufferedQueue{
    public:
	typedef wiselib::OSMODEL Os;
	typedef typename Os::BlockMemory::block_data_t block_data_t;
	typedef typename Os::BlockMemory::address_t address_t;

    private:
	static const uint16_t MAX_ITEMS_PER_BLOCK = Os::BlockMemory::BLOCK_SIZE /sizeof(T);

	block_data_t buffer_[BUFFERSIZE*Os::BlockMemory::BLOCK_SIZE ]; //Der Buffer

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
	BufferedQueue(Os::BlockMemory::self_pointer_t sd,address_t beginMem, address_t endMem, bool forceNew=false):sd_(sd),minBlock_(beginMem+1),maxBlock_(endMem){
	    if(BUFFERSIZE<2){
		debug_->debug("EXTERNAL_QUEUE ERROR: Tried to init with Buffersize %d, but Buffersize has to be at least 2");
		exit(1);
	    }
	    if(!PERSISTENT||forceNew){ //Neue Queue anlegen
		initNewQueue(beginMem);
	    } else { //Alte Queue versuchen zu reloaden
		sd_->read(buffer_, beginMem, 1);

		blockRead<uint16_t>(buffer_, 0, &itemsInRead_);
		blockRead<uint16_t>(buffer_, 1, &itemsInWrite_);
		blockRead<uint16_t>(buffer_, 2, &idxRead_);
		uint32_t tmpSizeof;   blockRead<uint32_t>(buffer_, 3, &tmpSizeof);
		uint32_t tmpValCode;  blockRead<uint32_t>(buffer_, 4, &tmpValCode);

		uint64_t tmpBlocksOnSd;	blockRead<uint64_t>(buffer_, 8, &tmpBlocksOnSd); blocksOnSd_=(address_t)tmpBlocksOnSd;
		uint64_t tmpIdxBeginSd; blockRead<uint64_t>(buffer_, 12, &tmpIdxBeginSd); idxBeginSd_ = (address_t) tmpIdxBeginSd;

		uint64_t tmpMinBlock; blockRead<uint64_t>(buffer_, 16, &tmpMinBlock);
		uint64_t tmpMaxBlock; blockRead<uint64_t>(buffer_, 20, &tmpMaxBlock);

		uint32_t valCode=itemsInRead_+itemsInWrite_+idxRead_+blocksOnSd_+idxBeginSd_;

		if(minBlock_!=tmpMinBlock || maxBlock_!=tmpMaxBlock || sizeof(T)!= tmpSizeof || valCode != tmpValCode){//Inkonsistent=>Neuer Stack
		    initNewQueue(beginMem);
		} else {
		    if(itemsInWrite_>0){
			sd_->read(&buffer_[Os::BlockMemory::BLOCK_SIZE], calcIdxOfNextFreeBlock(),1);
		    }
		    if(itemsInRead_>0){
			sd_->read(buffer_,calcIdxOfFirstFreeBlock(),1);
		    }
#ifdef DEBUG
		    debug_->debug("EXTERNAL_QUEUE INFO: reloaded old queue");
#endif
		}
	    }

	}


	~BufferedQueue(){
#ifdef DEBUG
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
	    if(blocksOnSdLeft()<=BUFFERSIZE){
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
	    block_data_t tmpBlock[Os::BlockMemory::BLOCK_SIZE];
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
		    err = sd_->write(&buffer_[Os::BlockMemory::BLOCK_SIZE ], calcIdxOfNextFreeBlock(), blocksToWrite);
		    if(err!=Os::SUCCESS) return err;

		    blocksOnSd_+=fullBlocksToWrite;
		} else {
		    err = sd_->write(&buffer_[Os::BlockMemory::BLOCK_SIZE ], calcIdxOfNextFreeBlock(), max);
		    if(err!=Os::SUCCESS) return err;
		    blocksOnSd_+=max;
		    err = sd_->write(&buffer_[Os::BlockMemory::BLOCK_SIZE +Os::BlockMemory::BLOCK_SIZE *max], calcIdxOfNextFreeBlock(), blocksToWrite-max);
		    if(err!=Os::SUCCESS) return err;
		    blocksOnSd_+=fullBlocksToWrite-max;
		}
		if(tmpBlock!=buffer_ && fullBlocksToWrite>0 && fullBlocksToWrite<blocksToWrite){
		    moveBlocks(&buffer_[(1+fullBlocksToWrite)*Os::BlockMemory::BLOCK_SIZE],&buffer_[Os::BlockMemory::BLOCK_SIZE],1);
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
	    blockWrite<uint16_t>(tmpBlock,1,itemsInWrite_);
	    blockWrite<uint16_t>(tmpBlock,2,idxRead_);
	    blockWrite<uint32_t>(tmpBlock,3,sizeof(T));

	    uint32_t valCode=itemsInRead_+itemsInWrite_+idxRead_+blocksOnSd_+idxBeginSd_;
	    blockWrite<uint32_t>(tmpBlock,4,valCode);


	    blockWrite<uint64_t>(tmpBlock,8,(uint64_t)blocksOnSd_);
	    blockWrite<uint64_t>(tmpBlock,12,(uint64_t)idxBeginSd_);

	    blockWrite<uint64_t>(tmpBlock,16,(uint64_t)minBlock_);
	    blockWrite<uint64_t>(tmpBlock,20,(uint64_t)maxBlock_);
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
#ifdef DEBUG
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
		moveBlocks(&buffer_[Os::BlockMemory::BLOCK_SIZE],buffer_,blocksToMove);

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
	    blockWrite<T>(&buffer_[Os::BlockMemory::BLOCK_SIZE ], itemsInWrite_, x);
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
		moveBlocks(&buffer_[Os::BlockMemory::BLOCK_SIZE],buffer_,BUFFERSIZE-1);
		itemsInWrite_-=MAX_ITEMS_PER_BLOCK;
		itemsInRead_=MAX_ITEMS_PER_BLOCK;
		idxRead_=0;
	    } else { //kompletten WriteBuffer auf SD schreiben

		if(blocksOnSd_==0) idxBeginSd_=minBlock_;
		address_t max=maxBlock_-calcIdxOfNextFreeBlock()+1;//Maximal in einem Stueck schreibbare bloecke

		if(max>=BUFFERSIZE-1){//Der komplette Buffer kann in einem Stueck geschrieben werden.
		    err = sd_->write(&buffer_[Os::BlockMemory::BLOCK_SIZE ], calcIdxOfNextFreeBlock(), BUFFERSIZE-1);
		    if(err!=Os::SUCCESS) return err;
		    blocksOnSd_+=BUFFERSIZE-1;
		    itemsInWrite_=0;
		} else {//Nur der vordere Teil kann in einem Stueck geschrieben werden. Der hintere wird nach vorne geschoben
		    uint8_t blocksLeft= BUFFERSIZE-1-max;
		    err = sd_->write(&buffer_[Os::BlockMemory::BLOCK_SIZE ], calcIdxOfNextFreeBlock(), max);
		    if(err!=Os::SUCCESS) return err;
		    blocksOnSd_+=max;
		    itemsInWrite_-=max*MAX_ITEMS_PER_BLOCK;

		    moveBlocks(&buffer_[(max+1)*Os::BlockMemory::BLOCK_SIZE],&buffer_[Os::BlockMemory::BLOCK_SIZE],blocksLeft);
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
		uint16_t maxPerBlock = Os::BlockMemory::BLOCK_SIZE /sizeof(S);
		S* castedBlock = (S*) &block[Os::BlockMemory::BLOCK_SIZE *(idx/maxPerBlock)];
		*x=castedBlock[idx%maxPerBlock];
	    }

	/**
	 * Schreibt ein Element passend in Bloecke
	 */
	template<class S>
	    void blockWrite(block_data_t* block, uint16_t idx, S x){
		uint16_t maxPerBlock = Os::BlockMemory::BLOCK_SIZE /sizeof(S);
		S* castedBlock = (S*) &block[Os::BlockMemory::BLOCK_SIZE *(idx/maxPerBlock)];
		castedBlock[idx%maxPerBlock]=x;
	    }

	/**
	  * Verschiebt n Bloecke im Hauptspeicher
	  */
	void moveBlocks(block_data_t* from, block_data_t* to, uint8_t count){
	    for(uint8_t i=0;i<count; i++){
		for(uint16_t j=0; j<Os::BlockMemory::BLOCK_SIZE; j++){
		    to[i*Os::BlockMemory::BLOCK_SIZE+j]=from[i*Os::BlockMemory::BLOCK_SIZE+j];
		}
	    }
	}

};
#endif
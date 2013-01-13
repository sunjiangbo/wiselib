#ifndef EXTERNAL_QUEUE_HPP
#define EXTERNAL_QUEUE_HPP
#include <external_interface/external_interface.h>
#include <external_interface/arduino/arduino_sdcard.h>
#include <external_interface/arduino/arduino_debug.h>
#include <external_interface/arduino/arduino_clock.h>
typedef wiselib::OSMODEL Os;
typedef unsigned char block_data_t;
/**
 * Eine extrem IO-effiziente Implementierung einer Queue fuer die Wiselib. Ihr Nachteil ist leider ein relativ hoher RAM-Verbrauch 
 * von mindestens 2 Bloecken, dafuer erreicht sie fast den minimal moeglichen IO-Durchschnitt pro Operation.
 * Die Queue ist standardmaessig persistent und kann nicht ueberlaufen. Als Folge dieser Schutzmassnahme ist 
 * es nicht immer Moeglich den Speicherplatz vollstaendig zu nutzen. 
 * Sollte man mehrere Queue im begrenzten RAM haben, so sollte man die Persistenz ausnutzen und 
 * die Queue zerstoeren und wiederherstellen bei Bedarf
 */
template<class T, int BUFFERSIZE=2, bool PERSISTENT=true>
class ExternalQueue{
    private:
	wiselib::ArduinoSdCard<Os>* sd_;
	const unsigned int MAX_ITEMS_PER_BLOCK = 512/sizeof(T);

	block_data_t buffer_[BUFFERSIZE*512]; //Der Buffer

	unsigned int itemsInRead_; //Anzahl der Elemente im Readbuffer
	unsigned int itemsInWrite_; //Anzahl der Elemente im Writebuffer
	unsigned int idxRead_; //Der Readbuffer ist eine Queue und hat einen rotierenden Anfang. Der Index zeigt auf das aelteste Element

	unsigned int blocksOnSd_; //Auf der SD gespeicherte VOLLSTAENDIGE Bloecke
	unsigned int idxBeginSd_; //Die Bloecke auf SD bilden eine Queue mit rotierendem Anfang. Der Index zeigt auf den aeltesten Block

	const unsigned int minBlock_; //Blockgrenze links
	const unsigned int maxBlock_; //Blockgrenze rechts


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
	ExternalQueue(wiselib::ArduinoSdCard<Os>* sd,unsigned int beginMem, unsigned int endMem, bool forceNew=false):sd_(sd),minBlock_(beginMem+1),maxBlock_(endMem){
	    if(BUFFERSIZE<2){
		//ERROR TODO
	    }
	    if(!PERSISTENT||forceNew){ //Neue Queue anlegen
		initNewQueue(beginMem);
	    } else { //Alte Queue versuchen zu reloaden
		sd_->read(buffer_, beginMem, 1);

		blockRead<int>(buffer_, 0, &itemsInRead_);
		blockRead<int>(buffer_, 1, &itemsInWrite_);
		blockRead<int>(buffer_, 2, &idxRead_);
		blockRead<int>(buffer_, 3, &blocksOnSd_);
		blockRead<int>(buffer_, 4, &idxBeginSd_);

		unsigned int tmpMinBlock; blockRead<int>(buffer_, 5, &tmpMinBlock);
		unsigned int tmpMaxBlock; blockRead<int>(buffer_, 6, &tmpMaxBlock);
		unsigned int tmpSizeof;   blockRead<int>(buffer_, 7, &tmpSizeof);
		unsigned int tmpValCode;  blockRead<int>(buffer_, 8, &tmpValCode);

		unsigned int valCode=itemsInRead_+itemsInWrite_+idxRead_+blocksOnSd_+idxBeginSd_;

		if(minBlock_!=tmpMinBlock || maxBlock_!=tmpMaxBlock || sizeof(T)!= tmpSizeof || valCode != tmpValCode){//Inkonsistent=>Neuer Stack
		    initNewQueue(beginMem);
		} else {
		    debug_->debug("RELOADED OLD QUEUE");
		}
	    }
	}


	~ExternalQueue(){
	    if(PERSISTENT){
		//Writebuffer schreiben
		unsigned int fullBlocksToWrite=itemsInWrite_/MAX_ITEMS_PER_BLOCK;
		unsigned int blocksToWrite=fullBlocksToWrite+(itemsInWrite_%MAX_ITEMS_PER_BLOCK>0?1:0);
		if(blocksToWrite>0){
		    int max=maxBlock_-(calcIdxOfNextFreeBlock()-1);
		    if(max>=blocksToWrite){
			sd_->write(&buffer_[512], calcIdxOfNextFreeBlock(), blocksToWrite);
			blocksOnSd_+=fullBlocksToWrite;
		    } else {
			sd_->write(&buffer_[512], calcIdxOfNextFreeBlock(), max);
			blocksOnSd_+=max;
			sd_->write(&buffer_[512+512*max], calcIdxOfNextFreeBlock(), blocksToWrite-max);
			blocksOnSd_+=fullBlocksToWrite-max;
		    }
		    itemsInWrite_%=MAX_ITEMS_PER_BLOCK; //Beim reload werden unvollstaendige Bloecke automatisch wieder in den Buffer geladen
		}

		//Readbuffer schreiben
		if(itemsInRead_>0){
		    sd_-> write(buffer_, calcIdxOfFirstFreeBlock(),1);
		}

		//Variablen sichern
		blockWrite<int>(buffer_,0,itemsInRead_);
		blockWrite<int>(buffer_,1,itemsInWrite_);
		blockWrite<int>(buffer_,2,idxRead_);
		blockWrite<int>(buffer_,3,blocksOnSd_);
		blockWrite<int>(buffer_,4,idxBeginSd_);

		blockWrite<int>(buffer_,5,minBlock_);
		blockWrite<int>(buffer_,6,maxBlock_);
		blockWrite<int>(buffer_,7,sizeof(T));

		int valCode=itemsInRead_+itemsInWrite_+idxRead_+blocksOnSd_+idxBeginSd_;
		blockWrite<int>(buffer_,8,valCode);

		sd_-> write(buffer_,minBlock_-1,1);
	    }
	}



	bool push(T x){//TODO groessere Sicherheit vor Datenverlust
	    if(blocksOnSd_<=0){
		if(itemsInWrite_>0 || readBufferFull()){
		    if(writeBufferFull()) flushWrite();
		    pushWrite(x);
		} else {
		    pushRead(x);
		}
	    } else {
		if(writeBufferFull()) {
		    if(!flushWrite()) return false;
		}
		pushWrite(x);
	    }
	    return true;
	}

	bool pop(T* x){//Ergaenzung um einen float-param
	    if(itemsInRead_<=0){
		bool succ=fillReadBuffer();
		if(!succ) return false;
	    } 
	    popRead(x);
	    return true;
	}

	bool top(T* x){//same as above
	    if(itemsInRead_<=0){
		bool succ=fillReadBuffer();
		if(!succ) return false;
	    } 
	    blockRead<T>(buffer_,idxRead_, x);
	    return true;
	}

    private:

	/**
	 * Setzt die Variablen der Queue auf die Defaultwerte
	 */
	void initNewQueue(int beginMem){	
	    debug_->debug("INIT NEW QUEUE");
	    itemsInRead_=0;
	    itemsInWrite_=0;
	    idxRead_=0;

	    blocksOnSd_=0;
	    idxBeginSd_=beginMem+1;
	}



	bool fillReadBuffer(){
	    if(blocksOnSd_<=0) {
		if(itemsInWrite_<=0) return false;
		//Bloecke aus Write nach read schieben
		int blocksToMove=itemsInWrite_/MAX_ITEMS_PER_BLOCK+(itemsInWrite_%MAX_ITEMS_PER_BLOCK>0?1:0);
		for(int i=0; i<blocksToMove*512; i++){
		    buffer_[i]=buffer_[i+512];
		}
		idxRead_=0;
		if(itemsInWrite_>=MAX_ITEMS_PER_BLOCK){
		    itemsInRead_=MAX_ITEMS_PER_BLOCK;
		    itemsInWrite_-=MAX_ITEMS_PER_BLOCK;
		} else {
		    itemsInRead_=itemsInWrite_;
		    itemsInWrite_=0;
		}
	    } else {
		sd_->read(buffer_, idxBeginSd_, 1);
		itemsInRead_=MAX_ITEMS_PER_BLOCK; //Wir holen immer nur volle Bloecke
		if(++idxBeginSd_>maxBlock_) idxBeginSd_=minBlock_;
		idxRead_=0;
		--blocksOnSd_;
	    }
	    return true;
	}

	void popRead(T* x){
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

	void pushRead(T x){
	    //read ist nicht voll
	    blockWrite<T>(buffer_, (idxRead_+itemsInRead_)%MAX_ITEMS_PER_BLOCK , x);
	    ++itemsInRead_;
	}

	void pushWrite(T x){
	    blockWrite<T>(&buffer_[512], itemsInWrite_, x);
	    ++itemsInWrite_;
	}


	bool writeBufferFull(){
	    return itemsInWrite_>=MAX_ITEMS_PER_BLOCK*(BUFFERSIZE-1);
	}

	bool readBufferFull(){
	    return itemsInRead_>=MAX_ITEMS_PER_BLOCK;
	}


	bool flushWrite(){

	    //Vorbedingung WRITE FULL => ALLE BLOECKE VOLL
	    if(blocksOnSdLeft()<=BUFFERSIZE) return false; //Es ist genug platz auf der SD
	    if(blocksOnSd_<=0 && itemsInRead_<=0){ //erster Block von Write kann nach Read kopiert werden

		T* castedBufferRead = (T*) buffer_;
		T* castedBufferWrite = (T*) &buffer_[512];

		itemsInRead_=0; idxRead_=0;
		for(int i=0; i<MAX_ITEMS_PER_BLOCK; i++){ //Evtl schoener direkt ueber itemsInRead
		    castedBufferRead[i]=castedBufferWrite[i];
		}
		itemsInRead_=MAX_ITEMS_PER_BLOCK;
		itemsInWrite_-=MAX_ITEMS_PER_BLOCK;

		if(itemsInWrite_>0){ //restliche Bloecke auf SD schreiben
		    idxBeginSd_=minBlock_;

		    sd_->write(&buffer_[512*2], minBlock_, BUFFERSIZE-2);

		    blocksOnSd_+= BUFFERSIZE-2;
		}
		itemsInWrite_=0;
	    } else { //kompletten WriteBuffer auf SD schreiben

		if(blocksOnSd_==0) idxBeginSd_=minBlock_;
		int max=maxBlock_-calcIdxOfNextFreeBlock()+1;//Maximal in einem Stueck schreibbare bloecke
		if(max>=BUFFERSIZE-1){//Der komplette Buffer kann in einem Stueck geschrieben werden.
		    sd_->write(&buffer_[512], calcIdxOfNextFreeBlock(), BUFFERSIZE-1);
		    blocksOnSd_+=BUFFERSIZE-1;
		    itemsInWrite_=0;
		} else {//Nur der vordere Teil kann in einem Stueck geschrieben werden. Der hintere wird nach vorne geschoben
		    int blocksLeft= BUFFERSIZE-1-max;
		    sd_->write(&buffer_[512], calcIdxOfNextFreeBlock(), max);
		    blocksOnSd_+=max;
		    itemsInWrite_-=max*MAX_ITEMS_PER_BLOCK;

		    for(int i=0; i<blocksLeft*512; i++) //memcpy?
			buffer_[512+i]=buffer_[512*(max+1)+i];

		}
	    }

	    return true;
	}


	/**
	 *Index vom naechsten freien Block, hinter den beschriebenen`
	 */
	int calcIdxOfNextFreeBlock(){//naechster Freier Block
	    int idxEnd= idxBeginSd_+blocksOnSd_;
	    if(idxEnd>maxBlock_){
		idxEnd-=(maxBlock_-minBlock_);
	    }
	    return idxEnd;
	}

	/**
	 * Index von erstem freien Block, vor den beschriebenen
	 */
	int calcIdxOfFirstFreeBlock(){
	    int idxBegin = idxBeginSd_-1;
	    if(idxBegin < minBlock_) idxBegin=maxBlock_;
	    return idxBegin;
	}

	int blocksOnSdLeft(){
	    return maxBlock_-minBlock_+1-blocksOnSd_;
	}
	template<class S>
	    void blockRead(block_data_t* block, int idx, S* x){
		int maxPerBlock = 512/sizeof(S);
		S* castedBlock = (S*) &block[512*(idx/maxPerBlock)];
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

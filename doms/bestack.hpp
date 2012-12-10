#ifndef BESTACK_HPP
#define BESTACK_HPP

#include "virt_sd.hpp"

#define _BLOCKSIZE_ 512

template<class T> class memPtr{
    public: 
	int ptrA, ptrB;
	int beginMem_, endMem_;

	memPtr(int a=0, int b=0, int firstBlock=0, int lastBlock=0): ptrA(a), ptrB(b), beginMem_(firstBlock), endMem_(lastBlock){}

	void set(int a, int b, int firstBlock, int lastBlock){
	    ptrA=a; ptrB=b; beginMem_=firstBlock; endMem_=lastBlock;
	}

	bool operator++(){ 
	    if(ptrB>=512/sizeof(T)-1){
		ptrA++, ptrB=0; 
		if(ptrA>endMem_) ptrA=beginMem_;
		return true;
	    } else {
		ptrB++;
		return false;
	    }
	}

	bool operator--(){
	    if(ptrB<=0){
		ptrA--;
		ptrB=512/sizeof(T)-1;
		if(ptrA<beginMem_) ptrA=endMem_;
		return true;
	    } else {
		ptrB--;
		return false;
	    }
	}

	bool jumpOnIncrement(){
	    return (ptrB>=512/sizeof(T)-1);
	}
	bool jumpOnDecrement(){
	    return ptrB<=0;
	}
};



template <class T, int BUFFERSIZE> class beStack{

    public:
	beStack(int firstBlock, int lastBlock, VirtualSD *sd, bool reload=false): sd_(sd){
	    /*
	     * Erster Block persistenter Datenspeicher
	     * [0] Block, [1] Position im Block
	     */
	    BYTE bufferBlock[512];
	    T *wPtr = (T*)bufferBlock;
	    if(reload){
		sd_->read(firstBlock, bufferBlock);
		topPtr_.set(wPtr[0],wPtr[1],firstBlock+1, lastBlock);
	    } else {
		topPtr_.set(firstBlock+1, 0, firstBlock+1, lastBlock);
	    }
	    bufferitems_=0;
	}

	~beStack(){
	    safe();
	}

	bool push(T x){
	    if(bufferitems_>=BUFFERSIZE){ flush(); }
	    if(topPtr_.ptrA>=topPtr_.endMem_) {return false; }//Stack voll;
	    buffer_[bufferitems_++] = x;
	    std::cout << "Buffer: " << bufferitems_ <<std::endl;
	    return true;
	}

	bool pop(T *x){
	    if(bufferitems_<=0){ //keine Elemente im Buffer
		loadIntoBuffer();
	    }
	    if(bufferitems_<=0) return false; //SD ist leer
	    *x=buffer_[--bufferitems_];
	    return true;
	}

	bool peek(T *x){
	    bool ret = pop(x);
	    if(ret) push(*x);
	    return ret;
	}

	bool empty(){
	    if(bufferitems_>0) return false;
	    if(topPtr_.ptrB>0) return false;
	    if(topPtr_.ptrA>0) return false;
	    return true;
	}


    private:


	int loadIntoBuffer(){
	    if(empty() || bufferitems_>0) return 0;
	    --topPtr_;

	    BYTE blockBuffer[512];
	    T* wPtr = (T*)blockBuffer;

	    int itemsToCopy = (topPtr_.ptrB<BUFFERSIZE?topPtr_.ptrB:BUFFERSIZE);

	    bufferitems_=itemsToCopy;

	    while(itemsToCopy>0){
		buffer_[--itemsToCopy] = wPtr[topPtr_.ptrB];
		--topPtr_;
	    }
	}

	void flush(){
	    if(bufferitems_<=0) return; //nothing to flush
	    BYTE blockBuffer[512];
	    T* wPtr = (T*) blockBuffer;
	    sd_->read(topPtr_.ptrA, blockBuffer);

	    for(int i=0; i<bufferitems_; i++){
		wPtr[topPtr_.ptrB]=buffer_[i];
		if(topPtr_.jumpOnIncrement()){ //Es gab einen Blocksprung
		    sd_->write(topPtr_.ptrA,blockBuffer);
		}
		++topPtr_;
	    }
	    if(topPtr_.ptrB>0) sd_->write(topPtr_.ptrA,blockBuffer); //Nur schreiben, wenn noetig (ptrB==0 => Es wurde gerade erst geschrieben)
	    bufferitems_=0;
	}

	void safe(){
	    BYTE blockBuffer[512];
	    T* wPtr = (T*) blockBuffer;
	    wPtr[0]=topPtr_.ptrA;
	    wPtr[1]=topPtr_.ptrB;
	    sd_->write(topPtr_.beginMem_-1,blockBuffer);
	}


    private:
	T buffer_[BUFFERSIZE]; //Buffer fuer x Werte
	int bufferitems_; //Anzahl der Elemente im Buffer

	memPtr<T> topPtr_; //Der naechste freie platz
	VirtualSD *sd_;
};




#endif

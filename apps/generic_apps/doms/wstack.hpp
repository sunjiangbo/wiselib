/*
 * Dies ist meine erste Implementierung eines externen Speicheralgorithmus in Form eines Stacks. Die Idee ist die moegliche Uebergabe von Arrays. 
 * Das automatische Buffern ist eine weitere Moeglichkeit, hierbei muss ich aber noch die Kosten/Nutzen analysieren. Eine Implementierung ist geplant.
 * Die Queue die im zweiten Versuch entstand empfinde ich als 'schoener'
 */
#ifndef WSTACK_HPP
#define WSTACK_HPP

#include "virt_sd.hpp"

template <class T> class ext_Stack{
    VirtualSD* sd_;
    int mBegin_, mEnd_;
    int itemsPerBlock_;
    int ptra_, ptrb_;
    public:
    ext_Stack(int start, int stop, VirtualSD* sd):sd_(sd),mBegin_(start),mEnd_(stop){
	if(sizeof(T)>512) std::cerr << "INVALID GENERIC:BIGGER THAN 512 BYTES" << std::endl;
	itemsPerBlock_=512/sizeof(T);
	std::cout << itemsPerBlock_<<std::endl;
	ptra_=start;
	ptrb_=0;
    }

    bool push(T x){
	if(ptra_==mEnd_) { std::cerr<<"STACK OVERFLOW"<<std::endl; return false;}
	BYTE  buffer[512];
	sd_->read(ptra_, buffer);
	T* wPtr= (T*) buffer;
	wPtr[ptrb_] = x;
	sd_->write(ptra_, buffer);
	raise();
	return true;
    }
    int push(T x[], int size){
	int wCount=0;
	BYTE buffer[512];
	T* wPtr = (T*)buffer;
	sd_->read(ptra_,buffer);
	int oldPtra=ptra_;
	while(size>0){
	    if(ptra_>=mEnd_) { std::cerr<<"STACK OVERFLOW"<<std::endl; return wCount;}
	    wPtr[ptrb_]=*x;
	    raise();
	    if(oldPtra!=ptra_){
		sd_->write(oldPtra, buffer);
		sd_->read(ptra_, buffer);
		oldPtra=ptra_;
	    }
	    size--;
	    wCount++;
	    x++;
	}
	sd_->write(ptra_, buffer);
	return wCount;
    }

    bool pop(T& x){
	if(ptra_<=mBegin_ && ptrb_==0) { std::cerr << "Empty Pop" <<std::endl; return false;}
	lower();
	BYTE buffer[512];
	sd_->read(ptra_,buffer);
	T* tArray = (T*)buffer;
	x = tArray[ptrb_];
	return true;
    }
    int pop(T x[], int size){
	int ret=0;
	BYTE buffer[512];
	T* wPtr = (T*) buffer; if(ptrb_>0) sd_->read(ptra_,buffer);
	int ptraOld=ptra_;
	while(size>0){
	    lower();
	    if(ptra_<mBegin_) return ret;
	    if(ptraOld!=ptra_){ 
		sd_->read(ptra_,buffer); ptraOld=ptra_;
	    }
	    *(x++)=wPtr[ptrb_]; ret++;
	}
	return ret;
    }
    private:
    bool raise(){
	++ptrb_;
	if(ptrb_>=itemsPerBlock_){
	    ++ptra_;
	    ptrb_=0;
	}
    }
    bool lower(){
	--ptrb_;
	if(ptrb_<0){
	    ptra_--;
	    ptrb_=itemsPerBlock_-1;
	}
    }


};

#endif

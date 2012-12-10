/*
 * Dies ist eine Uebungsimplementierung einer Queue fuer externe Speicher. Der Vorteil dieser Variente liegt in der Moeglichkeit
 * Arrays statt einzelne Werte zu uebergeben und die Queue diese dann automatisch mit moeglichst wenig IOs verarbeiten zu lassen.
 * Dies ist keine Implemenatation fuer die Wiselib, sondern fuer Linux mittels des virt_sd Interfaces. Es wurde aber auf moeglichst
 * hohe Kompatibilitaet geachtet. 
 * Diese Implementierung ist schlecht/garnicht kommentiert und haelt sich evtl. nicht an alle Konventionen.
 */



#include "virt_sd.hpp"

template <class T>
class ext_Queue{
    public:
	class memPtr{
	    public:
		int ptrA, ptrB;
		int beginMem, endMem;
		memPtr(int a, int b,int &bm, int &em):ptrA(a), ptrB(b),beginMem(bm), endMem(em){}
		bool operator++(){ if(ptrB>=512/sizeof(T)-1){ ptrA++; ptrB=0; if(ptrA>=endMem) ptrA=beginMem; return true;} else ptrB++; return false;}
		bool operator--(){ if(ptrB<=0){ptrA--; ptrB=512/sizeof(T)-1; if(ptrA<0) ptrA=endMem-1; return true;} else ptrB--; return false;}
		//void operator<(memPtr x){if(ptrA<x.ptrA) return true; if(ptrA==x.ptrA && ptrB<x.ptrB) return true; return false; }
		void operator==(memPtr x){ return ptrA==x.ptrA && ptrB==x.ptrB;}
	};

	ext_Queue(int begin, int end, VirtualSD *sd):sd_(sd),beginMem(begin),endMem(end),begin(begin,0,begin,end),end(begin,0,begin,end){
	}


	bool push(T x){
	    BYTE buffer[512];
	    sd_->read(end.ptrA, buffer);
	    T* wPtr = (T*) buffer;
	    wPtr[end.ptrB]=x;
	    sd_->write(end.ptrA, buffer);
	    ++end;
	    return true;
	}

	int push(T x[], int length){
	    int wCount=0;
	    BYTE buffer[512];
	    T* wPtr = (T*) buffer;
	    sd_->read(end.ptrA, buffer);
	    while(length>0){
		wPtr[end.ptrB]=*x;
		if(++end){
		    --end;
		    sd_->write(end.ptrA, buffer);
		    ++end;
		    sd_->read(end.ptrA,buffer);
		}
		length--;
		x++;
		wCount++;
	    }
	    sd_->write(end.ptrA,buffer);
	    return wCount;
	}

	bool poll(T& x){
	    BYTE buffer[512];
	    T* wPtr = (T*) buffer;
	    sd_->read(begin.ptrA, buffer);
	    x=wPtr[begin.ptrB];
	    ++begin;
	    return true;
	}

	int poll(T x[], int length){
	    int wCount=0;
	    BYTE buffer[512];
	    T* wPtr = (T*) buffer;
	    sd_->read(begin.ptrA,buffer);
	    for(int i=0;i<length; i++){
		*x=wPtr[begin.ptrB];
		x++;
		wCount++;
		if(++begin){
		    sd_->read(begin.ptrA,buffer);
		}
	    }
	    return wCount;
	}

    private:
	VirtualSD *sd_;
	memPtr begin, end;
	int beginMem, endMem;
};

/*
 * Dies ist eine Implementierung einer virtuellen SD-Karte fuer Linux-Systeme (Eigentlich fuer alle) zum testen von externen Speicheralgorithmen.
 * Die Stats werden aus, auf meinen Erinnerungen von Messergebnissen basierenden, Daten berechnet, wobei die Peaks nicht beruecksichtigt werden.
 * Das Interface ist dem der Wiselib nachempfunden. Die Benutzung sollte selbsterklaerend sein.
 */
#ifndef VIRTSDCARD_HPP
#define	VIRTSDCARD_HPP
#include <iostream>
#include <stdlib.h>
typedef char BYTE;
typedef BYTE* BLOCK;

class VirtualSD{
    public:
	VirtualSD(int size);//size in blocks
	~VirtualSD();

	void write(int block, BLOCK x);
	void write(int block, BLOCK x, int length);

	void read(int block, BLOCK buffer);
	void read(int block, BLOCK buffer, int length);

	void resetStats();
	void printStats(){
	    std::cout << "Blocks Written: " << blocksWritten_ <<std::endl;
	    std::cout << "Blocks Read: " << blocksRead_ << std::endl;
	    std::cout << "IOs: " << ios_ <<std::endl;
	    std::cout << "Duration: " << duration_ <<std::endl;
	    std::cout << "AvgIO: " << duration_/ios_ << std::endl;
	}

	static void cpyBlock(BLOCK source, BLOCK dest, int length){
	    for(int i=0; i<length; i++) (dest)[i]=(source)[i];
	}
    private:
	BLOCK* memory;
	bool* isWritten;
	int size;

	int blocksWritten_;
	int blocksRead_;
	int ios_;
	int duration_;


};
#endif

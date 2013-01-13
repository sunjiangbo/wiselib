/*
 * Dies ist eine Implementierung einer virtuellen SD-Karte fuer Linux-Systeme (Eigentlich fuer alle) zum testen von externen Speicheralgorithmen.
 * Die Stats werden aus, auf meinen Erinnerungen von Messergebnissen basierenden, Daten berechnet, wobei die Peaks nicht beruecksichtigt werden.
 * Das Interface ist dem der Wiselib nachempfunden. Die Benutzung sollte selbsterklaerend sein.
 */
#ifndef VIRTSDCARD_HPP
#define	VIRTSDCARD_HPP
#include <iostream>
#include <stdlib.h>

typedef char block_data_t;
typedef int address_t;

class VirtualSD{
    public:
	VirtualSD(int size);//size in blocks
	VirtualSD();
	~VirtualSD();

	void write(block_data_t* buffer, address_t start_block, address_t blocks);

	void read(block_data_t* buffer, address_t start_block, address_t blocks);

    private:
	block_data_t*  memory;
	int size;

};

#endif

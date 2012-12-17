/*
 * Dies ist eine Implementierung einer virtuellen SD-Karte fuer Linux-Systeme (Eigentlich fuer alle) zum testen von externen Speicheralgorithmen.
 * Die Stats werden aus, auf meinen Erinnerungen von Messergebnissen basierenden, Daten berechnet, wobei die Peaks nicht beruecksichtigt werden.
 * Das Interface ist dem der Wiselib nachempfunden. Die Benutzung sollte selbsterklaerend sein.
 */
#ifndef __VIRTSDCARD_H__
#define	__VIRTSDCARD_H__
#include <iostream>
#include <stdlib.h>
#include "pc_os_model.h"

namespace wiselib {
class ArduinoOsModel;
}

namespace wiselib {

template<typename OsModel_P, int nrOfBlocks = 1000>
class VirtualSD {

public:
	typedef OsModel_P OsModel;
	typedef typename OsModel::block_data_t block_data_t;
	typedef typename OsModel::size_t size_type;
	typedef size_type address_t; /// always refers to a block number
	typedef VirtualSD self_type;
	typedef self_type* self_pointer_t;

	enum {
		SUCCESS = OsModel::SUCCESS, ERR_UNSPEC = OsModel::ERR_UNSPEC
	};

	VirtualSD() {
		//is this really necessary?
		for (int i = 0; i < nrOfBlocks; i++) {
			isWritten[i] = false;
		}
		resetStats();
	}

	int erase(address_t start_block, address_t blocks) {
		//TODO
		return SUCCESS;
	}

	int write(block_data_t* x, address_t block) {
		++ios_;
		++blocksWritten_;
		duration_ += 8;
		if (block < 0 || block >= nrOfBlocks) {
			std::cerr << "OVERFLOW VIRTUAL SD" << std::endl;
			return ERR_UNSPEC;
		}
		for (int i = 0; i < 512; i++)
		{
			memory[block][i] = x[i];
			isWritten[block] = true;
		}
		return SUCCESS;
	}

int write(block_data_t* x, address_t start_block, address_t blocks) {
	++ios_;
	duration_ += 4;

	for (int i = 0; i < blocks; i++) {
		write(start_block + i, x + 512 * i);
		duration_ -= 6;
		--ios_;
	}
	return SUCCESS;
}

int read(block_data_t* buffer, address_t block) {
	if (block < 0 || block >= nrOfBlocks) {
		std::cerr << "OVERFLOW VIRTUAL SD" << std::endl;
		return ERR_UNSPEC;
	}
	++ios_;
	++blocksRead_;
	duration_ += 4;
	//else if(!isWritten[block]) std::cerr << "READING EMPTY BLOCK" << std::endl;

	for (int i = 0; i < 512; i++)
		buffer[i] = memory[block][i];
	return SUCCESS;
}

int read(block_data_t* buffer, address_t block, address_t length) {
	++ios_;
	duration_ += 2;

	for (int i = 0; i < length; i++) {
		read(block + i, buffer + 512 * i);
		duration_ -= 2;
		--ios_;
	}
	return SUCCESS;
}

void resetStats() {
	blocksWritten_ = 0;
	blocksRead_ = 0;
	ios_ = 0;
	duration_ = 0;
}

void printStats() {
	std::cout << "Blocks Written: " << blocksWritten_ << std::endl;
	std::cout << "Blocks Read: " << blocksRead_ << std::endl;
	std::cout << "IOs: " << ios_ << std::endl;
	std::cout << "Duration: " << duration_ << std::endl;
	std::cout << "AvgIO: " << duration_ / ios_ << std::endl;
}

private:
block_data_t memory[nrOfBlocks][512];
bool isWritten[nrOfBlocks];

int blocksWritten_;
int blocksRead_;
int ios_;
int duration_;

};
} //NAMESPACE
#endif // __VIRTSDCARD_H__

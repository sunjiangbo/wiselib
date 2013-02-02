#ifndef EXMMU_HPP
#define EXMMU_HPP

//#include <external_interface/external_interface.h>

#include <math.h>
#include "external_stack.hpp"

#include <external_interface/external_interface.h>
#include <external_interface/arduino/arduino_sdcard.h>
#include <external_interface/arduino/arduino_debug.h>


/* One EXMMU administers as many data structures as you want. It is assigned a fixed section of the 
SD card to administer, within which there is exactly one virtual block size. The actual data-structures 
do all their communication with the SD-Card (Erase, Read, Write) via their EXMMU, which is handed to 
them as a parameter.*/

//TODO: make MY_BLOCK_SIZE and virtualization into template parameters

//TODO: Write a Wiselib test app, and make this program conform to it, so that you can use this program with everything from the wiselib.
//TODO: It is assumed that a block is at least ~40(?) Bytes in size. Write that in the documentation somewhere.

//TODO: Make the destructor to store the thing to disk, and a constructor which can restore the whole thing. Make some way to destroy the structure on the disk as well?
//TODO: Get rid of floats in the stack size equation if possible

//TODO: Make the EXMMU be defined based off of LO and HI values, and not on LO and TOTAL_VBLOCKS
//TODO: Note, all parameters which are known at compile time, should be made into template parameters, which probably means an overwhelming majority of the parameters... Make it so! :)

// Wise_testing utils meta.h for template magic with data types to ensure they are as small as possible for the given range

/* template<typename OsModel_P,
          typename Debug_P = typename OsModel_P::Debug>
class EXMMU {

	typedef OsModel_P OsModel;
	typedef OsModel::BlockMemory BlockMemory
	typedef Debug_P Debug;
	typedef typename BlockMemory::size_t size_t;
	typedef typename BlockMemory::block_data_t block_data_t;*/

// LO = absolute block number where the ROOT block of the EXMMU is placed.
// HI = absolute block number of the last block which is reserved for this MMU and its datastructures
// BLOCK_VIRTUALIZATION = # of physical blocks per virtual block
// MY_BLOCK_SIZE = size of a real block in bytes


using namespace wiselib;

template <typename OsModel_P, typename block_address_t, 
	int LO, int HI, typename Debug_P = typename OsModel_P::Debug,
	int BLOCK_VIRTUALIZATION=1, int MY_BLOCK_SIZE=512>

/*template <typename OsModel_P, typename block_address_t, 
	typename Debug_P = typename OsModel_P::Debug>*/
class EXMMU {

	typedef OsModel_P OsModel;
	typedef OsModel::BlockMemory BlockMemory;
	typedef Debug_P Debug;
	//typedef typename OsModel::block_data_t block_data_t;
	typedef typename BlockMemory::size_t size_t;
	typedef typename BlockMemory::block_data_t block_data_t;

private:
	/*int LO;
	int HI;
	int BLOCK_VIRTUALIZATION;
	int MY_BLOCK_SIZE;*/
	
	BlockMemory::self_pointer_t sd;
	int BLOCKS;
	
	int MMU_DATA_SIZE; 		// size (in real blocks) of the MMU on the block device
	int STACK_SIZE;			// in real blocks
	
	int TOTAL_VBLOCKS; 		// absolute block number of the last block administered by this EXMMU
	int FIRST_VBLOCK_AT;
	
	int highest_vblock; 		// virtual block number denoting highest block number which has ever been used
	
	
	bool persistent;

	ExternalStack<int> stack;

//TODO: Use the Sd card enum to ask for its block size (take a look at Henning's block interface stuff for examples)

public:
	/*Note that the STACK_SIZE is about 1% larger than it needs to be. +1 is added for the stack's metadata, 
	and another +1 is added as a kind of manual rounding up of the value produced by the division*/
	/*template <class block_address_t, int LO, int HI, int BLOCK_VIRTUALIZATION=1, int MY_BLOCK_SIZE=512>*/
	EXMMU(OsModel::BlockMemory::self_pointer_t sd, int LO, int HI, int BLOCK_VIRTUALIZATION = 1, int MY_BLOCK_SIZE = 512, bool restore=true, bool persistent=true, bool *restoreSuccess = 0) : 		
		
		/*LO(LO),
		HI(HI),
		BLOCK_VIRTUALIZATION(BLOCK_VIRTUALIZATION),
		MY_BLOCK_SIZE(MY_BLOCK_SIZE),*/
		
		sd(sd), 
		BLOCKS(HI - LO + 1), 
		
		MMU_DATA_SIZE(1),
		STACK_SIZE( BLOCKS * sizeof(block_address_t) / MY_BLOCK_SIZE + 1 + 1),
		
		TOTAL_VBLOCKS( (BLOCKS - MMU_DATA_SIZE - STACK_SIZE) / BLOCK_VIRTUALIZATION ), 
		FIRST_VBLOCK_AT(LO + MMU_DATA_SIZE + STACK_SIZE),
		
		highest_vblock(0), 
		persistent(persistent),		
		stack(sd, LO + MMU_DATA_SIZE, LO + MMU_DATA_SIZE + (STACK_SIZE-1), !restore)
	
		 { 
		
		/*TODO: There's a problem with the Stack, when restoring, the stack 
		appears to take its own meta-data block an read it as if it was the 
		data block, instead of looking one block further and reading the 
		actual data. Lookign at the outputs more clsoely, it seems that the 
		stack doesn't write its data to the SD at all for some reason...
		
		Increasing the value of STACK_SIZE by one, results in the stack actually
		writing its data block, as it should, but it still reads from it's
		meta-data block, so either the stack has a bug, or you are feeding it 
		wrong indeces where the reading is concerned. Check what the parameters
		for LO and HI actually mean, if the meta-data is included in that range,
		or if the meta-data has to go to LO-1 and the data goes into the region
		from LO to HI.
		
		*/
		if(restore) {
		
			uint32_t checkvalue = 42;
			uint32_t data[MY_BLOCK_SIZE/sizeof(uint32_t)];
		
			sd->read((block_data_t*) data, LO, 1);
			
			//if stored data matches the given parameters then the data is ok
			if(data[0] == checkvalue && data[1] == checkvalue && data[2] == BLOCK_VIRTUALIZATION 
			&& data[3] == TOTAL_VBLOCKS && data[4] == MY_BLOCK_SIZE && data[5] == MMU_DATA_SIZE 
			&& data[6] == STACK_SIZE && data[7] == highest_vblock && data[8] == FIRST_VBLOCK_AT 
			&& data[9] == HI) {
				
				if (restoreSuccess != 0) *restoreSuccess = true;
			} else {
				if (restoreSuccess != 0) *restoreSuccess = false;
			}
		
		} else {
			this->stack.push(0);
		}
	}

	~EXMMU() {
		printf("\nexmmu destructor...\n"); //TODO
		
		uint32_t checkvalue = 42;
		uint32_t data[MY_BLOCK_SIZE/sizeof(uint32_t)]; // = {0};
		
		for (int i = 0; i < MY_BLOCK_SIZE/sizeof(uint32_t); ++i) {
			data[i] = (uint32_t) 0;
		}
		
		
		
		if(persistent) {
			data[0] = checkvalue;
			data[1] = checkvalue;
			data[2] = BLOCK_VIRTUALIZATION;
			data[3] = TOTAL_VBLOCKS;
			data[4] = MY_BLOCK_SIZE;
			data[5] = MMU_DATA_SIZE;
			data[6] = STACK_SIZE;
			data[7] = highest_vblock;
			data[8] = FIRST_VBLOCK_AT;
			data[9] = HI;
		} else {
			for (int i = 0; i <= 9; ++i) {
				data[0] = 0;
			}
		
		}
		sd->write((block_data_t *) data, 0, 1);
	}

	bool block_alloc(int *vBlockNo) {
		int p;
		if (stack.pop(&p) == true) { // The stack contains a free memory block
			*vBlockNo = p;
			return true;
		}
	
		// The stack doesn't contain a free memory block, but memory space which has never been used yet, is available
		else if (highest_vblock < TOTAL_VBLOCKS - 1) { 
			highest_vblock++;
			*vBlockNo = highest_vblock;
			return true;
		}
		else return false; // There is no free memory
	}

	bool block_free(int vBlockNo) {
		if(vBlockNo >= 0 && vBlockNo < TOTAL_VBLOCKS) {
			stack.push(vBlockNo);
			return true;
		}
		else return false;	
	}

	/*bool erase(int start_vblock, int vblocks) {
		if(start_vblock >= 0 && start_vblock + vblocks < TOTAL_VBLOCKS) {
			sd->erase(vr(start_vblock), vblocks * BLOCK_VIRTUALIZATION);
			return true;
		}

		else return false;
	}*/
	
	bool read(block_data_t *buffer, int start_vblock, int vblocks) { 

		if(start_vblock >= 0 && start_vblock + vblocks < TOTAL_VBLOCKS) {
			for (int i = 0; i<BLOCK_VIRTUALIZATION; ++i) {
				// cast to char* needed to ensure that the increments function correctly
				sd->read((char*) buffer + i * MY_BLOCK_SIZE, vr(start_vblock), vblocks * BLOCK_VIRTUALIZATION); 
			}
			return true;
		}
		else return false;
	}


	bool write(block_data_t *buffer, int start_vblock, int vblocks) {
	
		if(start_vblock >= 0 && start_vblock + vblocks < TOTAL_VBLOCKS) { 
			sd->write(buffer, vr(start_vblock), vblocks * BLOCK_VIRTUALIZATION); 
			return true;
		}

		else return false;

		//return write(block_data_t *buffer, int start_block, int blocks);
	}

	/* converts real data block number to virtual block number. The given real block must be the first
	real block in the virtual block else the function returns FALSE and v is set to NULL. */
	bool rv(int r, int *v) {

		if((r - FIRST_VBLOCK_AT) % BLOCK_VIRTUALIZATION == 0) {
			*v = (r - FIRST_VBLOCK_AT) / BLOCK_VIRTUALIZATION;
			//printf("v is %d.\n", *v);
			return true;
		} else {
			v = NULL;
			//printf("v is undefinded.\n");
			return false;
		}
	}
	
	// converts virtual data block number to real block number
	int vr(int v) {
		//printf("v is %d and r is %d.\n", v, FIRST_VBLOCK_AT + v * BLOCK_VIRTUALIZATION);
		return FIRST_VBLOCK_AT + v * BLOCK_VIRTUALIZATION;
	}

	int get_STACK_SIZE() {
		return this->STACK_SIZE;
	} 

	int get_HI() {
		return this->HI;
	}
	
	void setPeristent(bool p) {
		this->persistent = p;
	}
	
	bool getPeristent() {
		return this->persistent;
	}
};

//

#endif

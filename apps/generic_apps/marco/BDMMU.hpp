#ifndef BDMMU_HPP
#define BDMMU_HPP
//#define DEBUG

#define BDMMU_DEBUG

#ifdef DEBUG
	#ifndef BDMMU_DEBUG
		#define BDMMU_DEBUG
	#endif
#endif

#include <external_interface/external_interface.h>
#include "external_stack.hpp"

/* One BDMMU administers as many data structures as you want. It is assigned a fixed section of the 
SD card to administer, within which there is exactly one virtual block size. The actual data-structures 
do all their communication with the SD-Card (Erase, Read, Write) via their BDMMU, which is handed to 
them as a parameter.*/

//TODO: Use the Sd card enum to ask for its block size (take a look at Henning's block interface stuff for examples)
//TODO: It is assumed that a block is at least ~40(?) Bytes in size. Write that in the documentation somewhere.
//TODO: Use Wise_testing utils meta.h for template magic with data types to ensure they are as small as possible for the given range
//TODO: Add debug messages to more methods
//TODO: Which variables should be constant? Make them so.
//TODO: Make all addresses into address_t

using namespace wiselib;

// LO = absolute block number where the ROOT block of the BDMMU is placed.
// HI = absolute block number of the last block which is reserved for this MMU and its datastructures
// BLOCK_VIRTUALIZATION = # of physical blocks per virtual block
// MY_BLOCK_SIZE = size of a real block in bytes

template <typename OsModel_P, 
	int LO, int HI, 
	typename Debug_P = typename OsModel_P::Debug,
	typename BlockMemory_P = typename OsModel_P::BlockMemory,
	int BLOCK_VIRTUALIZATION=1, int MY_BLOCK_SIZE=512>
	
class BDMMU {
	
	public:
		typedef OsModel_P OsModel;
		typedef Debug_P Debug;
		typedef BlockMemory_P BlockMemory;
	
		typedef typename Debug::self_pointer_t Debug_ptr;
		typedef typename BlockMemory::self_pointer_t BlockMem_ptr;
	
		typedef typename OsModel::size_t address_t;
		typedef typename BlockMemory::block_data_t block_data_t;
	
		/*Note that the STACK_SIZE is about 1% larger than it needs to be. +1 is added for the stack's metadata, another +1 is 
		added as a kind of manual rounding up of the value produced by the division, and +2 is for the BUFFERSIZE of the stack*/
		BDMMU(BlockMem_ptr bm_, Debug_ptr debug_, bool restore=true, bool persistent=true, bool *restoreSuccess = 0) : 		
		
			bm_(bm_), 
			debug_(debug_),
			BLOCKS(HI - LO + 1), 
		
			MMU_DATA_SIZE(1),
			/*STACK_SIZE( BLOCKS * sizeof(address_t) / MY_BLOCK_SIZE + 1 + 1 + 2),*/
			STACK_SIZE(BLOCKS/(MY_BLOCK_SIZE/sizeof(address_t) )+1+1+2),
		
			TOTAL_VBLOCKS( (BLOCKS - MMU_DATA_SIZE - STACK_SIZE) / BLOCK_VIRTUALIZATION ), 
			FIRST_VBLOCK_AT(LO + MMU_DATA_SIZE + STACK_SIZE),
			highest_vblock(0), 
		
			persistent(persistent),	
		
			stack(bm_, LO + MMU_DATA_SIZE, LO + MMU_DATA_SIZE + (STACK_SIZE-1), !restore)
	
		{ 
		
			if(restore) {
				debug_->debug("restore\n");
				uint32_t checkvalue = 42;
				uint32_t data[MY_BLOCK_SIZE/sizeof(uint32_t)];
		
				bm_->read((block_data_t*) data, LO, 1);
			
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

		~BDMMU() {
			#ifdef BDMMU_DEBUG
				debug_->debug("\n BDMMU destructor...\n");
			#endif
		
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
			bm_->write((block_data_t *) data, 0, 1);
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
			else {
				#ifdef BDMMU_DEBUG
					debug_->debug("There is no more free memory.\n");
				#endif
				return false; // There is no free memory
			}
		}

		bool block_free(int vBlockNo) {
			if(vBlockNo >= 0 && vBlockNo < TOTAL_VBLOCKS) {
				stack.push(vBlockNo);
				return true;
			}
			else {	
				#ifdef BDMMU_DEBUG
					debug_->debug("Attempted free a block with an invalid virtual block address.\n");
				#endif
				return false;
			}
		}

		bool erase(int start_vblock, int vblocks) {
			if(start_vblock >= 0 && start_vblock + vblocks < TOTAL_VBLOCKS) {
				bm_->erase(vr(start_vblock), vblocks * BLOCK_VIRTUALIZATION);
			
				#ifdef BDMMU_DEBUG
					debug_->debug("VIRTUAL BLOCKS: bm_->erase(%d, %d) | REAL BLOCKS: bm_->erase(%d, %d)\n", start_vblock, vblocks, vr(start_vblock), vblocks * BLOCK_VIRTUALIZATION);
				#endif
			
				return true;
			}
		
			else {
				#ifdef BDMMU_DEBUG
					debug_->debug("Attempted erase at invalid virtual block address(es).\n");
				#endif
				return false;
			}
		}
	
		bool read(block_data_t *buffer, int start_vblock, int vblocks) { 

			if(start_vblock >= 0 && start_vblock + vblocks < TOTAL_VBLOCKS) {
			
				bm_->read(buffer, vr(start_vblock), vblocks * BLOCK_VIRTUALIZATION);
			
				#ifdef BDMMU_DEBUG
					debug_->debug("VIRTUAL BLOCKS: bm_->read(buffer %d, %d) | REAL BLOCKS: bm_->read(buffer %d, %d)\n", start_vblock, vblocks, vr(start_vblock), vblocks * BLOCK_VIRTUALIZATION);
				#endif
			
				return true;
			}
		
		
			else {
				#ifdef BDMMU_DEBUG
					debug_->debug("Attempted read at invalid virtual block address(es).\n");
				#endif
				return false;
			}
		}


		bool write(block_data_t *buffer, int start_vblock, int vblocks) {
	
			if(start_vblock >= 0 && start_vblock + vblocks < TOTAL_VBLOCKS) { 
				bm_->write(buffer, vr(start_vblock), vblocks * BLOCK_VIRTUALIZATION); 
			
				#ifdef BDMMU_DEBUG
					debug_->debug("VIRTUAL BLOCKS: bm_->write(buffer %d, %d) | REAL BLOCKS: bm_->write(buffer %d, %d)\n", start_vblock, vblocks, vr(start_vblock), vblocks * BLOCK_VIRTUALIZATION);
				#endif
			
				return true;
			}
		
		
			else {
				#ifdef BDMMU_DEBUG
					debug_->debug("Attempted write at invalid virtual block address(es).\n");
				#endif
				return false;
			}
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
	
		int get_TOTAL_VBLOCKS() {
			return this->TOTAL_VBLOCKS;
		}
	
		void setPeristent(bool p) {
			this->persistent = p;
		}
	
		bool getPeristent() {
			return this->persistent;
		}
		
	private:
	
		BlockMem_ptr bm_;
		Debug_ptr debug_; 
		int BLOCKS;
	
		int MMU_DATA_SIZE; 		// size (in real blocks) of the MMU on the block device
		int STACK_SIZE;			// in real blocks
	
		int TOTAL_VBLOCKS; 		// absolute block number of the last block administered by this BDMMU
		int FIRST_VBLOCK_AT;
		int highest_vblock; 		// virtual block number denoting highest block number which has ever been used
	
		bool persistent;

		ExternalStack<int> stack;
};
	
			
	


    

//

#endif

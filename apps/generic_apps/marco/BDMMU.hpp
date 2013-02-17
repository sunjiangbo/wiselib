#ifndef BDMMU_HPP
#define BDMMU_HPP

#include <external_interface/external_interface.h>
//#include "external_stack.hpp"
#include "../doms/external_stack.hpp"
//#include "external_stack.hpp"

//#define DEBUG
//#define BDMMU_DEBUG

#ifdef DEBUG
	#ifndef BDMMU_DEBUG
		#define BDMMU_DEBUG
	#endif
#endif




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
	typename OsModel_P::size_t LO, 
	typename OsModel_P::size_t HI, 
	typename OsModel_P::size_t reserved = 0, 
	typename OsModel_P::size_t BLOCK_VIRTUALIZATION=1, 
	typename OsModel_P::size_t MY_BLOCK_SIZE=512,
	typename Debug_P = typename OsModel_P::Debug,
	typename BlockMemory_P = typename OsModel_P::BlockMemory>
	
class BDMMU {
	
	public:
		typedef OsModel_P OsModel;
		typedef Debug_P Debug;
		typedef BlockMemory_P BlockMemory;
	
		typedef typename Debug::self_pointer_t Debug_ptr;
		typedef typename BlockMemory::self_pointer_t BlockMem_ptr;
		
		typedef typename OsModel::size_t size_t;
		typedef typename OsModel::size_t address_t;
		typedef typename BlockMemory::block_data_t block_data_t;
	
		typedef BDMMU<OsModel, LO, HI, reserved, BLOCK_VIRTUALIZATION, 
			MY_BLOCK_SIZE, Debug, BlockMemory> self_type; //Note: MUST be public
		typedef self_type* self_pointer_t;//Note: MUST be public
		//typedef self_pointer_t MMU_ptr;
	
		/*Note that the STACK_SIZE is about 1% larger than it needs to be. +1 is added for the stack's metadata, another +1 is 
		added as a kind of manual rounding up of the value produced by the division, and +2 is for the BUFFERSIZE of the stack*/
		BDMMU(BlockMem_ptr bm_, Debug_ptr debug_, bool restore=true, bool persistent=true, int *restoreSuccess = 0) : 		
		
			bm_(bm_), 
			debug_(debug_),
			BLOCKS(HI - LO + 1), 
		
			MMU_DATA_SIZE(1),
			/*STACK_SIZE( BLOCKS * sizeof(address_t) / MY_BLOCK_SIZE + 1 + 1 + 2),*/
			STACK_SIZE(BLOCKS/(MY_BLOCK_SIZE/sizeof(address_t) )+1+1+2),
		
			TOTAL_VBLOCKS( (BLOCKS - MMU_DATA_SIZE - STACK_SIZE) / BLOCK_VIRTUALIZATION ), 
			FIRST_VBLOCK_AT(LO + MMU_DATA_SIZE + STACK_SIZE),
			next_vblock(reserved), 
		
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
				&& data[6] == STACK_SIZE && data[7] == next_vblock && data[8] == FIRST_VBLOCK_AT 
				&& data[9] == HI) {
				
					if (restoreSuccess != 0) *restoreSuccess = OsModel::SUCCESS;
				} else {
					if (restoreSuccess != 0) *restoreSuccess = OsModel::ERR_UNSPEC;
				}
		
			}
		}

		~BDMMU() {
			#ifdef BDMMU_DEBUG
				debug_->debug("\n BDMMU destructor...");
			#endif
		
			uint32_t checkvalue = 42;
			uint32_t data[MY_BLOCK_SIZE/sizeof(uint32_t)]; // = {0};
		
			for (uint8_t i = 0; i < MY_BLOCK_SIZE/sizeof(uint32_t); ++i) { //TODO use meta.h template magic here
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
				data[7] = next_vblock;
				data[8] = FIRST_VBLOCK_AT;
				data[9] = HI;
			} else {
				for (uint8_t i = 0; i <= 9; ++i) {
					data[0] = 0;
				}
		
			}
			
			int x = bm_->write((block_data_t *) data, 0, 1);
			
			#ifdef BDMMU_DEBUG
				if (x == OsModel::SUCCESS) {
					debug_->debug("Write to block memory device successful.\n");
				} else {
					debug_->debug("Write to block memory device failed.\n");
				}
			#endif
			
		}

		int block_alloc(address_t *vBlockNo) {
			address_t p;
			int r = stack.pop(&p);
			
			if (r == OsModel::SUCCESS) { // The stack contains a free memory block
				*vBlockNo = p;
				return OsModel::SUCCESS;
			}
			
			//The BlockDevice is funtioning correctly, but there are no block numbers of free blocks on the stack
			else if (r == OsModel::ERR_UNSPEC) {
				
				// The stack doesn't contain a free memory block, but memory space which has never been used yet, is available
				if (next_vblock < TOTAL_VBLOCKS) { 
					*vBlockNo = next_vblock;
					next_vblock++;
					return OsModel::SUCCESS;
				}
				else {
					#ifdef BDMMU_DEBUG
						debug_->debug("There is no more free memory.");
					#endif
					return OsModel::ERR_UNSPEC; // There is no free memory
				}
			}
			
			//IO Error or other hardware error occurred
			else return r;
		}

		int block_free(address_t vBlockNo) {
			if(vBlockNo >= 0 && vBlockNo < TOTAL_VBLOCKS) {
				return stack.push(vBlockNo);
			}
			else {	
				#ifdef BDMMU_DEBUG
					debug_->debug("Attempted free a block with an invalid virtual block address.\n");
				#endif
				return OsModel::ERR_UNSPEC;
			}
		}

		int erase(address_t start_vblock, address_t vblocks) {
			if(start_vblock >= 0 && start_vblock + vblocks < TOTAL_VBLOCKS) {
				
				#ifdef BDMMU_DEBUG
					debug_->debug("VIRTUAL BLOCKS: bm_->erase(%d, %d)", start_vblock, vblocks);
					debug_->debug("REAL BLOCKS: bm_->erase(%d, %d)\n", vr(start_vblock), vblocks * BLOCK_VIRTUALIZATION);
				#endif
			
				return bm_->erase(vr(start_vblock), vblocks * BLOCK_VIRTUALIZATION);
			}
		
			else {
				#ifdef BDMMU_DEBUG
					debug_->debug("Attempted erase at invalid virtual block address(es).\n");
				#endif
				return OsModel::ERR_UNSPEC;
			}
		}
	
		int read(block_data_t *buffer, address_t start_vblock, address_t vblocks = 1) { 

			if(start_vblock >= 0 && start_vblock + vblocks < TOTAL_VBLOCKS) {
			
				#ifdef BDMMU_DEBUG
					debug_->debug("VIRTUAL BLOCKS: bm_->read(buffer %d, %d)", start_vblock, vblocks);
					debug_->debug("REAL BLOCKS: bm_->read(buffer %d, %d)\n", vr(start_vblock), vblocks * BLOCK_VIRTUALIZATION);
				#endif
			
				return bm_->read(buffer, vr(start_vblock), vblocks * BLOCK_VIRTUALIZATION);
			}
		
		
			else {
				#ifdef BDMMU_DEBUG
					debug_->debug("Attempted read at invalid virtual block address(es).\n");
				#endif
				return OsModel::ERR_UNSPEC;
			}
		}


		bool write(block_data_t *buffer, address_t start_vblock, address_t vblocks = 1) {
	
			if(start_vblock >= 0 && start_vblock + vblocks < TOTAL_VBLOCKS) { 
				
				#ifdef BDMMU_DEBUG
					debug_->debug("VIRTUAL BLOCKS: bm_->write(buffer %d, %d)", start_vblock, vblocks);
					debug_->debug("REAL BLOCKS: bm_->write(buffer %d, %d)\n", vr(start_vblock), vblocks * BLOCK_VIRTUALIZATION);
				#endif
			
				return bm_->write(buffer, vr(start_vblock), vblocks * BLOCK_VIRTUALIZATION); 
			}
		
		
			else {
				#ifdef BDMMU_DEBUG
					debug_->debug("Attempted write at invalid virtual block address(es).\n");
				#endif
				return OsModel::ERR_UNSPEC;
			}
		}

		/* converts real data block number to virtual block number. The given real block must be the first
		real block in the virtual block else the function returns FALSE and v is set to NULL. */
		bool rv(address_t r, address_t *v) {

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
		address_t vr(address_t v) {
			
			#ifdef BDMMU_DEBUG
				//debug_->debug("FUNCTION VR: FIRST_VBLOCK_AT=%u + v=%u * BLOCK_VIRTUALIZATION=%u", FIRST_VBLOCK_AT, v, BLOCK_VIRTUALIZATION);
				debug_->debug("FIRST_VBLOCK_AT: %u", FIRST_VBLOCK_AT);
				debug_->debug("v: %u", v);
				debug_->debug("BLOCK_VIRTUALIZATION: %u", BLOCK_VIRTUALIZATION);
				debug_->debug("FIRST_VBLOCK_AT + v * BLOCK_VIRTUALIZATION = %u", FIRST_VBLOCK_AT + v * BLOCK_VIRTUALIZATION);
			#endif
			return FIRST_VBLOCK_AT + (v * BLOCK_VIRTUALIZATION);
		}

		int get_STACK_SIZE() {
			return this->STACK_SIZE;
		} 

		address_t get_HI() {
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
		
		//TODO: Make all the const variables into enums
		BlockMem_ptr bm_;		// pointer to this MMU's BlockMemory object
		Debug_ptr debug_; 		// Pointer to a debug object for debug messages
		const size_t BLOCKS;		// Total number of virtual blocks available in this MMU
	
		const size_t MMU_DATA_SIZE; 	// size (in real blocks) of the MMU on the block device
		const size_t STACK_SIZE;	// in real blocks
	
		const size_t TOTAL_VBLOCKS;	// absolute block number of the last block administered by this BDMMU
		const address_t FIRST_VBLOCK_AT;
		address_t next_vblock; 		// virtual block number denoting highest block number which has ever been used
	
		bool persistent;

		ExternalStack<address_t> stack;
};
	
			
	


    

//
#ifdef BDMMU_DEBUG
	#undef BDMMU_DEBUG
#endif

#endif

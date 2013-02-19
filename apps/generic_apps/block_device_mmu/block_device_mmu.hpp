#ifndef BLOCK_DEVICE_MMU_HPP
#define BLOCK_DEVICE_MMU_HPP

#include <external_interface/external_interface.h>
#include "../doms/external_stack.hpp"

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

//TODO: It is assumed that a block is at least ~40(?) Bytes in size. Write that in the documentation somewhere.
//TODO: Use Wise_testing utils meta.h for template magic with data types to ensure they are as small as possible for the given range

using namespace wiselib;
	
	
template <typename OsModel_P, 
	typename BlockMemory_P = typename OsModel_P::BlockMemory,
	typename Debug_P = typename OsModel_P::Debug>
	
struct BDMMU_Template_Wrapper {
			
	template <typename BlockMemory_P::address_t LO, 
		typename BlockMemory_P::address_t HI,
		typename OsModel_P::size_t reserved = 0, 
		typename OsModel_P::size_t BLOCK_VIRTUALIZATION=1>

	class BDMMU {

		public:
			typedef OsModel_P OsModel;
			typedef Debug_P Debug;
			typedef BlockMemory_P BlockMemory;

			typedef typename Debug::self_pointer_t Debug_ptr;
			typedef typename BlockMemory::self_pointer_t BlockMem_ptr;

			typedef typename OsModel::size_t size_t;
			typedef typename BlockMemory::address_t block_address_t;
			typedef typename BlockMemory::block_data_t block_data_t;
			
			typedef BDMMU<LO, HI, reserved, BLOCK_VIRTUALIZATION> self_type; //Note: MUST be public
			typedef self_type* self_pointer_t;//Note: MUST be public
			//typedef self_pointer_t MMU_ptr;

			/*Note that the STACK_SIZE is about 1% larger than it needs to be. +1 is added for the stack's metadata, another +1 is 
			added as a kind of manual rounding up of the value produced by the division, and +2 is for the BUFFERSIZE of the stack*/
			
			BDMMU(BlockMem_ptr bm_, Debug_ptr debug_, bool restore=true, bool persistent=true, int *restoreSuccess = 0) : 		

				bm_(bm_), 
				debug_(debug_),
				
				/*BLOCKS(HI - LO + 1), 
				MMU_DATA_SIZE(1),
				STACK_SIZE(BLOCKS/(BlockMemory::BLOCK_SIZE/sizeof(block_address_t) )+1+1+2),

				TOTAL_VBLOCKS( (BLOCKS - MMU_DATA_SIZE - STACK_SIZE) / BLOCK_VIRTUALIZATION ), 
				FIRST_VBLOCK_AT(LO + MMU_DATA_SIZE + STACK_SIZE),*/
				next_vblock(reserved), 

				persistent(persistent),	

				stack(bm_, LO + MMU_DATA_SIZE, LO + MMU_DATA_SIZE + (STACK_SIZE-1), !restore)

			{ 

				if(restore) {
					debug_->debug("restore\n");
					uint32_t checkvalue = 42;
					uint32_t data[BlockMemory::BLOCK_SIZE/sizeof(uint32_t)];

					bm_->read((block_data_t*) data, LO, 1);

					//if stored data matches the given parameters then the data is ok
					if(data[0] == checkvalue && data[1] == checkvalue && data[2] == BLOCK_VIRTUALIZATION 
					&& data[3] == TOTAL_VBLOCKS && data[4] == BlockMemory::BLOCK_SIZE && data[5] == MMU_DATA_SIZE 
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

				block_address_t checkvalue = 42;
				block_address_t data[BlockMemory::BLOCK_SIZE/sizeof(block_address_t)]; // = {0};

				for (unsigned int i = 0; i < BlockMemory::BLOCK_SIZE/sizeof(block_address_t); ++i) { //TODO use meta.h template magic here
					data[i] = (block_address_t) 0;
				}

				if(persistent) {
					data[0] = checkvalue;
					data[1] = checkvalue;
					data[2] = BLOCK_VIRTUALIZATION;
					data[3] = TOTAL_VBLOCKS;
					data[4] = BlockMemory::BLOCK_SIZE;
					data[5] = MMU_DATA_SIZE;
					data[6] = STACK_SIZE;
					data[7] = next_vblock;
					data[8] = FIRST_VBLOCK_AT;
					data[9] = HI;
				} else {
					for (unsigned int i = 0; i <= 9; ++i) {
						data[0] = 0;
					}

				}

				#ifdef BDMMU_DEBUG
				int x = 
				#endif

				bm_->write((block_data_t *) data, 0, 1);

				#ifdef BDMMU_DEBUG
					if (x == OsModel::SUCCESS) {
						debug_->debug("BDMMU Destructor: Write to block memory device successful.\n");
					} else {
						debug_->debug("BDMMU Destructor: Write to block memory device failed.\n");
					}
				#endif

			}
			
			int block_alloc(block_address_t *vBlockNo) {
				block_address_t p;
				int r = stack.pop(&p);

				if (r == OsModel::SUCCESS) { // The stack contains a free memory block
					*vBlockNo = p;
					#ifdef BDMMU_DEBUG
						debug_->debug("block_alloc: success(%u)", *vBlockNo);
					#endif
					return OsModel::SUCCESS;
				}

				//The BlockDevice is funtioning correctly, but there are no block numbers of free blocks on the stack
				else {
	
					if(stack.isEmpty()) {
						// The stack doesn't contain a free memory block, but memory space which has never been used yet, is available
						if (next_vblock < TOTAL_VBLOCKS) { 
							*vBlockNo = next_vblock;
							next_vblock++;
							
							#ifdef BDMMU_DEBUG
								debug_->debug("block_alloc: success(%u)", *vBlockNo);
							#endif
							
							return OsModel::SUCCESS;
						}
						else {
							#ifdef BDMMU_DEBUG
								debug_->debug("block_alloc: no free memory");
							#endif
							return OsModel::ERR_NOMEM;
						}
					} else {
						#ifdef BDMMU_DEBUG
							debug_->debug("block_alloc: IO error.");
						#endif
		
						return OsModel::ERR_IO;
					}
				}

			}

			int block_free(block_address_t vBlockNo) {
				if(vBlockNo >= 0 && vBlockNo < TOTAL_VBLOCKS) {
	
					int r = stack.push(vBlockNo);
	
					if (r == OsModel::SUCCESS) {
						#ifdef BDMMU_DEBUG
							debug_->debug("block_free(%u): success", vBlockNo);
						#endif
						return OsModel::SUCCESS;
					} else {
		
						#ifdef BDMMU_DEBUG
							debug_->debug("block_free(%u): IO error.", vBlockNo);
						#endif
						return OsModel::ERR_IO;
					}
				}
				else {	
					#ifdef BDMMU_DEBUG
						debug_->debug("block_free(%u): invalid argument", vBlockNo);
					#endif
					return OsModel::ERR_UNSPEC;
				}
			}

			int erase(block_address_t start_vblock, block_address_t vblocks) {
				if(start_vblock >= 0 && start_vblock + vblocks < TOTAL_VBLOCKS) {
	
					int r = bm_->erase(vr(start_vblock), vblocks * BLOCK_VIRTUALIZATION);
	
					if (r == OsModel::SUCCESS) { 
						#ifdef BDMMU_DEBUG
							debug_->debug("erase(%u, %u): success", start_vblock, vblocks);
						#endif
						return OsModel::SUCCESS;
					}
	
					else {
						#ifdef BDMMU_DEBUG
							debug_->debug("erase(%u, %u): IO error", start_vblock, vblocks);
						#endif
						return OsModel::ERR_IO;
					}
	
				}

				else {
					#ifdef BDMMU_DEBUG
						debug_->debug("erase(%u, %u): invalid arguments", start_vblock, vblocks);
					#endif
					return OsModel::ERR_UNSPEC;
				}
			}

			int read(block_data_t *buffer, block_address_t start_vblock, block_address_t vblocks = 1) { 

				if(start_vblock >= 0 && start_vblock + vblocks < TOTAL_VBLOCKS) {

					int r = bm_->read(buffer, vr(start_vblock), vblocks * BLOCK_VIRTUALIZATION);
	
					if (r == OsModel::SUCCESS) {
						#ifdef BDMMU_DEBUG
							debug_->debug("read(buffer, %u, %u): success", start_vblock, vblocks);
						#endif
						return OsModel::SUCCESS;
					} else {
						#ifdef BDMMU_DEBUG
							debug_->debug("read(buffer, %u, %u): IO Error", start_vblock, vblocks);
						#endif
						return OsModel::ERR_IO;
					}
				}


				else {
					#ifdef BDMMU_DEBUG
						debug_->debug("read(buffer, %u, %u): invalid arguments", start_vblock, vblocks);
					#endif
					return OsModel::ERR_UNSPEC;
				}
			}


			int write(block_data_t *buffer, block_address_t start_vblock, block_address_t vblocks = 1) {

				if(start_vblock >= 0 && start_vblock + vblocks < TOTAL_VBLOCKS) { 
	
					int r = bm_->write(buffer, vr(start_vblock), vblocks * BLOCK_VIRTUALIZATION); 
	
					if (r == OsModel::SUCCESS) {
						#ifdef BDMMU_DEBUG
						debug_->debug("write(buffer, %u, %u): success", start_vblock, vblocks);
						#endif
						return OsModel::SUCCESS;
					} else {
						#ifdef BDMMU_DEBUG
						debug_->debug("write(buffer, %u, %u): IO error", start_vblock, vblocks);
						#endif
						return OsModel::ERR_IO;
					}
				}


				else {
					#ifdef BDMMU_DEBUG
						debug_->debug("write(buffer, %u, %u): invalid arguments", start_vblock, vblocks);
						#endif
					return OsModel::ERR_UNSPEC; //EINVAL would be good
				}
			}

			/* converts real data block number to virtual block number. The given real block must be the first
			real block in the virtual block else the function returns FALSE and v is set to NULL. */
			bool rv(block_address_t r, block_address_t *v) {

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
			block_address_t vr(block_address_t v) {
				return FIRST_VBLOCK_AT + (v * BLOCK_VIRTUALIZATION);
			}

			/*size_t get_STACK_SIZE() {
				return this->STACK_SIZE;
			} 

			block_address_t get_LO() {
				return LO;
			}

			block_address_t get_HI() {
				return HI;
			}

			size_t get_TOTAL_VBLOCKS() {
				return this->TOTAL_VBLOCKS;
			}*/

			block_address_t get_reserved() {
				return reserved;
			}

			void setPeristent(bool p) {
				this->persistent = p;
			}

			bool isPeristent() {
				return this->persistent;
			}

			int sizeofStack() {
				return sizeof(ExternalStack<block_address_t, 1>);
			}

		private:

			BlockMem_ptr bm_;		// pointer to this MMU's BlockMemory object
			Debug_ptr debug_; 		// Pointer to a debug object for debug messages
			
			block_address_t next_vblock; 		// virtual block number denoting highest block number which has ever been used
			bool persistent;

			ExternalStack<block_address_t, 1> stack;
			
		public:
			/* These constants were not placed into an enum because an enum uses only int values, 
			which may overflow if values from a larger datatype are used*/
			static const size_t BLOCKS = HI - LO + 1;			// Total number of virtual blocks available in this MMU
			static const size_t MMU_DATA_SIZE = 1; 		// size (in real blocks) of the MMU on the block device
			
			// in real blocks
			static const size_t STACK_SIZE = BLOCKS/(BlockMemory::BLOCK_SIZE/sizeof(block_address_t) )+1+1+2;
			
			// absolute block number of the last block administered by this BDMMU
			static const size_t TOTAL_VBLOCKS = (BLOCKS - MMU_DATA_SIZE - STACK_SIZE) / BLOCK_VIRTUALIZATION;
			
			
			static const block_address_t FIRST_VBLOCK_AT = LO + MMU_DATA_SIZE + STACK_SIZE;
		
			enum {
				VIRTUAL_BLOCK_SIZE = BLOCK_VIRTUALIZATION * BlockMemory::BLOCK_SIZE
			};
		
	};
	
};


//
#ifdef BDMMU_DEBUG
	#undef BDMMU_DEBUG
#endif

#endif

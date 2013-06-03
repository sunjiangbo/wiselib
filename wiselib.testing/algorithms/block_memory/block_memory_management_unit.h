#ifndef BLOCK_MEMORY_MANAGEMENT_UNIT_H
#define BLOCK_MEMORY_MANAGEMENT_UNIT_H

#include <external_interface/external_interface.h>
#include <algorithms/block_memory/buffered_stack.h>

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

//TODO: It is asserted that a block must be at least 64 Bytes in size. Write that in the documentation somewhere.
//TODO: Use Wise_testing utils meta.h for template magic with data types to ensure they are as small as possible for the given range
//TODO: Change the design so that generic virtual block sizes are possible (both smaller and larger than a given block)
//TODO: Write a stack which doesn't have its own buffer, but just writes everything to the SD directly
/*TODO: Clean up the code to get rid of all that really explicit output after thorough testing. You may want to wait until
you've written an unboffered stack and switched to that though...*/

using namespace wiselib;
	
	
template <typename OsModel_P, 
	typename BlockMemory_P = typename OsModel_P::BlockMemory,
	typename Debug_P = typename OsModel_P::Debug>
	
class BDMMU {
   public:		
	template <typename BlockMemory_P::address_t LO, 
		typename BlockMemory_P::address_t HI,
		typename OsModel_P::size_t reserved = 0, 
		typename OsModel_P::size_t BLOCK_VIRTUALIZATION=1>

	class BDMMU_i {

		public:
			typedef OsModel_P OsModel;
			
			typedef Debug_P Debug;
			typedef BlockMemory_P BlockMemory;
			
			typedef BDMMU_i<LO, HI, reserved, BLOCK_VIRTUALIZATION> self_type;
			typedef self_type* self_pointer_t;
			typedef typename BlockMemory::block_data_t block_data_t;
			typedef typename BlockMemory::address_t address_t;

			typedef typename Debug::self_pointer_t Debug_ptr;
			typedef typename BlockMemory::self_pointer_t BlockMem_ptr;
			typedef typename OsModel::size_t size_t;
			
			enum {
				BLOCK_SIZE = BlockMemory::BLOCK_SIZE * BLOCK_VIRTUALIZATION,
				NO_ADDRESS = BlockMemory::NO_ADDRESS,
			};
			
			enum {	
				SUCCESS = BlockMemory::SUCCESS, 
				ERR_IO = BlockMemory::ERR_IO,
				ERR_NOMEM = BlockMemory::ERR_NOMEM,
				ERR_UNSPEC = BlockMemory::ERR_UNSPEC,
			};
			
			int init(BlockMem_ptr bm, Debug_ptr debug, bool restore=true, bool persistent=true) {
				this->bm_ = bm;
				this->debug_ = debug;
				this->next_vblock_ = reserved;
				this->persistent_ = persistent;	

				stack_.init(bm_, debug_, LO + MMU_DATA_SIZE, LO + MMU_DATA_SIZE + (STACK_SIZE-1), !restore);
				
				assert(BlockMemory::BLOCK_SIZE >= 64); //TODO: Just do this using a static assert instead? Do static asserts work on all platforms?

				if(restore) {
					debug_->debug("restore\n");
					uint32_t checkvalue = 42;
					uint32_t data[BlockMemory::BLOCK_SIZE/sizeof(uint32_t)];

					bm_->read((block_data_t*) data, LO, 1);

					//if stored data matches the given parameters then the data is ok
					if(data[0] == checkvalue && data[1] == checkvalue && data[2] == BLOCK_VIRTUALIZATION 
					&& data[3] == TOTAL_VBLOCKS && data[4] == BlockMemory::BLOCK_SIZE && data[5] == MMU_DATA_SIZE 
					&& data[6] == STACK_SIZE && data[7] == next_vblock_ && data[8] == FIRST_VBLOCK_AT 
					&& data[9] == HI) {
	
						return SUCCESS;
					} else {
						return ERR_UNSPEC;
					}

				}
				return SUCCESS;
			}
			
			
			/*BDMMU_i(BlockMem_ptr bm_, Debug_ptr debug_, bool restore=true, bool persistent=true, int *restoreSuccess = 0) : 		

				bm_(bm_), 
				debug_(debug_),
				next_vblock_(reserved), 

				persistent_(persistent),	

				stack_.init(bm_, debug_, LO + MMU_DATA_SIZE, LO + MMU_DATA_SIZE + (STACK_SIZE-1), !restore);

			{ 
				assert(BlockMemory::BLOCK_SIZE >= 64); //TODO: Just do this using a static assert instead? Do static asserts work on all platforms?

				if(restore) {
					debug_->debug("restore\n");
					uint32_t checkvalue = 42;
					uint32_t data[BlockMemory::BLOCK_SIZE/sizeof(uint32_t)];

					bm_->read((block_data_t*) data, LO, 1);

					//if stored data matches the given parameters then the data is ok
					if(data[0] == checkvalue && data[1] == checkvalue && data[2] == BLOCK_VIRTUALIZATION 
					&& data[3] == TOTAL_VBLOCKS && data[4] == BlockMemory::BLOCK_SIZE && data[5] == MMU_DATA_SIZE 
					&& data[6] == STACK_SIZE && data[7] == next_vblock_ && data[8] == FIRST_VBLOCK_AT 
					&& data[9] == HI) {
	
						if (restoreSuccess != 0) *restoreSuccess = SUCCESS;
					} else {
						if (restoreSuccess != 0) *restoreSuccess = ERR_UNSPEC;
					}

				}
			}*/
			
			//TODO: You have to create a method to shut the MMU down with data persistence..

			/*~BDMMU_i() {
				#ifdef BDMMU_DEBUG
					debug_->debug("\n BDMMU destructor...");
				#endif
				
				
				address_t checkvalue = 42;
				address_t data[BlockMemory::BLOCK_SIZE/sizeof(address_t)]; // = {0};

				for (unsigned int i = 0; i < BlockMemory::BLOCK_SIZE/sizeof(address_t); ++i) { //TODO use meta.h template magic here
					data[i] = (address_t) 0;
				}

				if(persistent_) {
					data[0] = checkvalue;
					data[1] = checkvalue;
					data[2] = BLOCK_VIRTUALIZATION;
					data[3] = TOTAL_VBLOCKS;
					data[4] = BlockMemory::BLOCK_SIZE;
					data[5] = MMU_DATA_SIZE;
					data[6] = STACK_SIZE;
					data[7] = next_vblock_;
					data[8] = FIRST_VBLOCK_AT;
					data[9] = HI;
				} else {
					for (unsigned int i = 0; i <= 9; ++i) {
						data[0] = 0;
					}

				}
debug_->debug("foo");
				#ifdef BDMMU_DEBUG
				int x = 
				#endif

				bm_->write((block_data_t *) data, 0, 1);
debug_->debug("bar");
				#ifdef BDMMU_DEBUG
					if (x == SUCCESS) {
						debug_->debug("BDMMU Destructor: Write to block memory device successful.\n");
					} else {
						debug_->debug("BDMMU Destructor: Write to block memory device failed.\n");
					}
				#endif

			}*/
			
			int block_alloc(address_t *vBlockNo) {
				address_t p;
				int r = stack_.pop(&p);

				if (r == SUCCESS) { // The stack contains a free memory block
					*vBlockNo = p;
					#ifdef BDMMU_DEBUG
						debug_->debug("block_alloc: success(%u)", *vBlockNo);
					#endif
					return SUCCESS;
				}

				//The BlockDevice is funtioning correctly, but there are no block numbers of free blocks on the stack
				else {
	
					if(stack_.isEmpty()) {
						// The stack doesn't contain a free memory block, but memory space which has never been used yet, is available
						if (next_vblock_ < TOTAL_VBLOCKS) { 
							*vBlockNo = next_vblock_;
							next_vblock_++;
							
							#ifdef BDMMU_DEBUG
								debug_->debug("block_alloc: success(%u)", *vBlockNo);
							#endif
							
							return SUCCESS;
						}
						else {
							#ifdef BDMMU_DEBUG
								debug_->debug("block_alloc: no free memory");
							#endif
							return ERR_NOMEM;
						}
					} else {
						#ifdef BDMMU_DEBUG
							debug_->debug("block_alloc: IO error.");
						#endif
		
						return ERR_IO;
					}
				}

			}
			
			//TODO: Should you set the vBlockNo to NO_ADDRESS when you are done? Otherwise make a block_free() const
			int block_free(address_t vBlockNo) {
				if(vBlockNo >= 0 && vBlockNo < TOTAL_VBLOCKS) {
	
					int r = stack_.push(vBlockNo);
	
					if (r == SUCCESS) {
						#ifdef BDMMU_DEBUG
							debug_->debug("block_free(%u): success", vBlockNo);
						#endif
						return SUCCESS;
					} else {
		
						#ifdef BDMMU_DEBUG
							debug_->debug("block_free(%u): IO error.", vBlockNo);
						#endif
						return ERR_IO;
					}
				}
				else {	
					#ifdef BDMMU_DEBUG
						debug_->debug("block_free(%u): invalid argument", vBlockNo);
					#endif
					return ERR_UNSPEC;
				}
			}

			void init(){} //TODO: Henning wrote somethign about this method having to reset algorithms to thier initial state? Read that again...

			int read(block_data_t *buffer, address_t start_vblock, address_t vblocks = 1) { 

				if(start_vblock >= 0 && start_vblock + vblocks < TOTAL_VBLOCKS) {

					int r = bm_->read(buffer, vr(start_vblock), vblocks * BLOCK_VIRTUALIZATION);
	
					if (r == SUCCESS) {
						#ifdef BDMMU_DEBUG
							debug_->debug("read(buffer, %u, %u): success", start_vblock, vblocks);
						#endif
						return SUCCESS;
					} else {
						#ifdef BDMMU_DEBUG
							debug_->debug("read(buffer, %u, %u): IO Error", start_vblock, vblocks);
						#endif
						return ERR_IO;
					}
				}


				else {
					#ifdef BDMMU_DEBUG
						debug_->debug("read(buffer, %u, %u): invalid arguments", start_vblock, vblocks);
					#endif
					return ERR_UNSPEC;
				}
			}


			int write(block_data_t *buffer, address_t start_vblock, address_t vblocks = 1) {

				if(start_vblock >= 0 && start_vblock + vblocks < TOTAL_VBLOCKS) { 
	
					int r = bm_->write(buffer, vr(start_vblock), vblocks * BLOCK_VIRTUALIZATION); 
	
					if (r == SUCCESS) {
						#ifdef BDMMU_DEBUG
						debug_->debug("write(buffer, %u, %u): success", start_vblock, vblocks);
						#endif
						return SUCCESS;
					} else {
						#ifdef BDMMU_DEBUG
						debug_->debug("write(buffer, %u, %u): IO error", start_vblock, vblocks);
						#endif
						return ERR_IO;
					}
				}


				else {
					#ifdef BDMMU_DEBUG
						debug_->debug("write(buffer, %u, %u): invalid arguments", start_vblock, vblocks);
						#endif
					return ERR_UNSPEC; //EINVAL would be good
				}
			}

			int erase(address_t start_vblock, address_t vblocks) {
				if(start_vblock >= 0 && start_vblock + vblocks < TOTAL_VBLOCKS) {
	
					int r = bm_->erase(vr(start_vblock), vblocks * BLOCK_VIRTUALIZATION);
	
					if (r == SUCCESS) { 
						#ifdef BDMMU_DEBUG
							debug_->debug("erase(%u, %u): success", start_vblock, vblocks);
						#endif
						return SUCCESS;
					}
	
					else {
						#ifdef BDMMU_DEBUG
							debug_->debug("erase(%u, %u): IO error", start_vblock, vblocks);
						#endif
						return ERR_IO;
					}
				}

				else {
					#ifdef BDMMU_DEBUG
						debug_->debug("erase(%u, %u): invalid arguments", start_vblock, vblocks);
					#endif
					return ERR_UNSPEC;
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
				return FIRST_VBLOCK_AT + (v * BLOCK_VIRTUALIZATION);
			}

			/*size_t get_STACK_SIZE() {
				return this->STACK_SIZE;
			} 

			address_t get_LO() {
				return LO;
			}

			address_t get_HI() {
				return HI;
			}

			size_t get_TOTAL_VBLOCKS() {
				return this->TOTAL_VBLOCKS;
			}*/

			address_t get_reserved() {
				return reserved;
			}

			void setPeristent(bool p) {
				this->persistent_ = p;
			}

			bool isPeristent() {
				return this->persistent_;
			}

			int sizeofStack() {
				return sizeof(BufferedStack<address_t, 1>);
			}

		private:

			BlockMem_ptr bm_;		// pointer to this MMU's BlockMemory object
			Debug_ptr debug_; 		// Pointer to a debug object for debug messages
			
			address_t next_vblock_; 		// virtual block number denoting highest block number which has ever been used
			bool persistent_;

			BufferedStack<address_t, 1> stack_;
			
		public:
			/* These constants were not placed into an enum because an enum uses only int values, 
			which may overflow if values from a larger datatype are used*/
			static const size_t BLOCKS = HI - LO + 1;	// Total number of virtual blocks available in this MMU
			static const size_t MMU_DATA_SIZE = 1; 		// size (in real blocks) of the MMU on the block device
			
			/* 
			 * Size of Stack in real blocks. Note that the STACK_SIZE is about 1% larger 
			 * than it needs to be. +1 is added for the stack's metadata, another +1 is added 
			 * as a kind of manual rounding up of the value produced by the division, and +2 
			 * is for the BUFFERSIZE of the stack
			 */
			static const size_t STACK_SIZE = BLOCKS/(BlockMemory::BLOCK_SIZE/sizeof(address_t) )+1+1+2;
			
			// absolute block number of the last block administered by this BDMMU
			static const size_t TOTAL_VBLOCKS = (BLOCKS - MMU_DATA_SIZE - STACK_SIZE) / BLOCK_VIRTUALIZATION;
			
			
			static const address_t FIRST_VBLOCK_AT = LO + MMU_DATA_SIZE + STACK_SIZE;
		
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

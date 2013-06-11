/*
 * Test application for BDMMU
 */

//#define USE_RAM_BLOCK_MEMORY 1 //TODO: Get rid of this once you've fixed the PC_OsModel's Block memory
#define DEBUG

#include <external_interface/external_interface.h>
#include <algorithms/block_memory/block_memory_management_unit.h>

#define DEF_TEST_MINIMAL_OUTPUT
//#define DEF_TEST_EXPLICIT_OUTPUT

typedef wiselib::OSMODEL OsModel;

//TODO: How about some Doxygen Documentation?
//TODO: Write a persistence test to ensure data is stored in a persistent manner when shutting down datastruct

class BDMMUTestApp
{
   public:
	typedef typename BDMMU<OsModel, OsModel::BlockMemory, OsModel::Debug>::BDMMU_i<0, 11, 1, 1> MMU_0_t;
	typedef typename OsModel::size_t size_t;
	typedef typename MMU_0_t::address_t address_t;
	
	enum {	
		NO_ADDRESS = MMU_0_t::NO_ADDRESS,
		SUCCESS = MMU_0_t::SUCCESS, 
		ERR_IO = MMU_0_t::ERR_IO,
		ERR_NOMEM = MMU_0_t::ERR_NOMEM,
		ERR_UNSPEC = MMU_0_t::ERR_UNSPEC,
	};
	
	bool error_code(int code, bool print = false) {
		
		switch (code) {
			
			case SUCCESS:
				if(print) debug_->debug("SUCCESS \n");
				return true;
			case ERR_IO:
				if(print) debug_->debug("ERR_IO \n");
				return false;
			case ERR_NOMEM:
				if(print) debug_->debug("ERR_NOMEM \n");
				return false;
			case ERR_UNSPEC:
				if(print) debug_->debug("ERR_UNSPEC \n");
				return false;
			default:
				if(print) debug_->debug("unknown error or error code \n");
				return false;
		}
	}
	
	/* Allocate 'number_tries' virtual blocks and store their block numbers in the array allocated_blocks. Note 
	that if no more blocks are available a magic value, NO_ADDRESS is written into the array, as a NULL value. */
	template <typename MMU>
	bool test_batch_block_alloc(MMU mmu, size_t* allocated_blocks, size_t number_tries, size_t number_expected) {
		
		debug_->debug("test_batch_block_alloc = ");
		
		size_t number_successes = 0;
		int status = -2;
		
		for (size_t i = 0; i < number_tries; i++) {

			status = mmu.block_alloc(&allocated_blocks[i]);

			if (status == SUCCESS) {				
				++number_successes;
			} else if (i >= number_expected && status == ERR_NOMEM) {
				allocated_blocks[i] = NO_ADDRESS;
			} else {
				allocated_blocks[i] = NO_ADDRESS;
				return error_code(status, true);
			}
		}
		
		return (number_successes == number_expected ? error_code(SUCCESS, true) : error_code(-2, true)); 
	}
	
	/* Read 'number' virtual block numbers from 'allocated_blocks' and free those blocks. NO_ADDRESS is interpreted
	like a NULL value, resulting in the free operation not being executed for that entry. */
	template <typename MMU>
	bool test_batch_block_free(MMU mmu, size_t* allocated_blocks, size_t number_tries, size_t number_expected) {
	
		debug_->debug("test_batch_block_free = ");
		
		size_t number_successes = 0;
		int status = -2;
	
		for (size_t i = 0; i < number_tries; i++) {
		
			if (allocated_blocks[i] != NO_ADDRESS) {
		
				status = mmu.block_free(allocated_blocks[i]);
		
				if (status == SUCCESS) {
					++number_successes;
				} else {
					return error_code(status, true);
				}
			}
		}
		
		return (number_successes == number_expected ? error_code(SUCCESS, true) : error_code(-2, true));
	}
	
	template <typename MMU>
	bool test_erase_write_read(MMU mmu, address_t start_address, size_t length_in_bytes) {
		
		/*size_t buffer_size = length_in_bytes/MMU::BLOCK_SIZE;
		while (length_in_bytes > buffer_size) { // TODO: More elegant fix here
			buffer_size += MMU::BLOCK_SIZE;
		}*/

		debug_->debug("test_erase_write_read = ");
		
		size_t blocks = (length_in_bytes % MMU::BLOCK_SIZE == 0) ? (length_in_bytes/MMU::BLOCK_SIZE) : (length_in_bytes/MMU::BLOCK_SIZE +1);
		size_t buffer_size = blocks * MMU::BLOCK_SIZE;
		
		unsigned char txt_in[buffer_size];
		unsigned char txt_out[buffer_size];
		
		//TODO: You may be able to use "memset" here...
		for (size_t i = 0; i < buffer_size; ++i) {
			txt_in[i] = '1';
			txt_out[i] = '0';
		}
		
		int status = mmu.erase(start_address, blocks);
		if (!error_code(status)) return error_code(status, true);
		
		status = mmu.write(txt_in, start_address, blocks);
		if (!error_code(status)) return error_code(status, true);
		
		status = mmu.read(txt_out, start_address, blocks);
		if (!error_code(status)) return error_code(status, true);
		
		for (size_t i = 0; i < buffer_size; ++i) {
			if (txt_in[i] != txt_out[i]) return error_code(-2, true);
		}
		
		return error_code(SUCCESS, true);
	}

	
	void init( OsModel::AppMainParameter& value )
	{
		//GENERAL SETUP
		debug_ = &wiselib::FacetProvider<OsModel, OsModel::Debug>::get_facet( value );
		debug_->debug("\n\n >>> Launching application! <<< \n\n" );
		sd_ = &wiselib::FacetProvider<OsModel, OsModel::BlockMemory>::get_facet(value);
		printf("sd_ = %x\n", sd_);
		sd_->init();
	
		//MMU SETUP
//		MMU_0_t mmu_0(sd_, debug_, false, true);
//		MMU_0_t mmu_0();
		mmu_0.init(sd_, debug_, false, true);
		
		//PRINT MMU PROPERTIES
			/*debug_->debug("sd_ = %x", sd_);
			debug_->debug("sizeof(MMU_0_t) = %u", sizeof(MMU_0_t));
			debug_->debug("sizeof(MMU_0_t.stack) = %u", mmu_0.sizeofStack());
			debug_->debug("sizeof(MMU_0_t::block_address_t) = %u", sizeof(typename MMU_0_t::block_address_t));*/
		debug_->debug("Stack size = %d", mmu_0.STACK_SIZE);
		debug_->debug("MMU administers %d virtual block(s).", mmu_0.TOTAL_VBLOCKS);
		debug_->debug("Reserved %d virtual block(s) for special purposes. (Starting from virtual block address 0.)\n", mmu_0.get_reserved());
		
		bool all_tests_successful = true;
		bool test0 = true;
		bool test1 = true;
		bool test2 = true;
		
		//TESTING
		size_t number_of_blocks = 5;
		size_t allocated_blocks[number_of_blocks];
	
		test0 = test_batch_block_alloc<MMU_0_t>(mmu_0, allocated_blocks, number_of_blocks, 5);
		test1 = test_batch_block_free<MMU_0_t>(mmu_0, allocated_blocks, number_of_blocks, 5);
		test2 = test_erase_write_read<MMU_0_t>(mmu_0, 0, MMU_0_t::BLOCK_SIZE);
		
		all_tests_successful = test0 && test1 && test2;
		
		//PRINT OVERALL RESULT
		debug_->debug("all_tests_successful = %d", all_tests_successful);
	
//		sd_->printStats();
	
		//SHUTDOWN
		exit(0);
		
	}
	
	
	
	// --------------------------------------------------------------------
	
   private:

	OsModel::Debug::self_pointer_t debug_;
	OsModel::BlockMemory::self_pointer_t sd_;
	MMU_0_t mmu_0;
};
// --------------------------------------------------------------------------
wiselib::WiselibApplication<OsModel, BDMMUTestApp> mmu_test;
// --------------------------------------------------------------------------
void application_main( OsModel::AppMainParameter& value )
{
	mmu_test.init( value );
}

#ifdef DEF_TEST_EXPLICIT_OUTPUT 
	#undef DEF_TEST_EXPLICIT_OUTPUT
#endif //DEF_TEST_EXPLICIT_OUTPUT

#ifdef DEF_TEST_MINIMAL_OUTPUT 
	#undef DEF_TEST_MINIMAL_OUTPUT
#endif //DEF_TEST_MINIMAL_OUTPUT

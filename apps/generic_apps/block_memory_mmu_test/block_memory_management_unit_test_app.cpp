/*
 * Test application for BDMMU
 */

#define USE_RAM_BLOCK_MEMORY 1 //TODO: Get rid of this once you've fixed the PC_OsModel's Block memory
//#define DEBUG

#include <external_interface/external_interface.h>
#include <algorithms/block_memory/block_memory_management_unit.h>

#define DEF_TEST_MINIMAL_OUTPUT
//#define DEF_TEST_EXPLICIT_OUTPUT

typedef wiselib::OSMODEL OsModel;

//TODO: How about some Doxygen Documentation?

class BDMMUTestApp
{
   public:
	typedef typename BDMMU_Template_Wrapper<OsModel, OsModel::BlockMemory, OsModel::Debug>::BDMMU<0, 11, 1, 1> MMU_0_t;
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
		
		size_t number_successes = 0;
		
		for (size_t i = 0; i < number_tries; i++) {

			if (mmu.block_alloc(&allocated_blocks[i]) == SUCCESS) {
			
				#ifdef DEF_TEST_EXPLICIT_OUTPUT
				debug_->debug("Allocated virtual block: %u.", allocated_blocks[i]);
				debug_->debug("Its real block number is: %u.\n", mmu.vr(allocated_blocks[i]));
				#endif //DEF_TEST_EXPLICIT_OUTPUT
				
				++number_successes;
			} else {
				allocated_blocks[i] = NO_ADDRESS;
				
				#ifdef DEF_TEST_EXPLICIT_OUTPUT
				debug_->debug("Virtual block could not be allocated.\n");
				#endif //DEF_TEST_EXPLICIT_OUTPUT

			}
		}
		
		bool success = (number_successes == number_expected) ? true : false; 
		
		#ifdef DEF_TEST_MINIMAL_OUTPUT
		debug_->debug("test_batch_block_alloc = %d", success);
		#endif //DEF_TEST_MINIMAL_OUTPUT
		
		return success;
	}
	
	/* Read 'number' virtual block numbers from 'allocated_blocks' and free those blocks. NO_ADDRESS is interpreted
	like a NULL value, resulting in the free operation not being executed for that entry. */
	template <typename MMU>
	bool test_batch_block_free(MMU mmu, size_t* allocated_blocks, size_t number_tries, size_t number_expected) {
	
		size_t number_successes = 0;
	
		for (size_t i = 0; i < number_tries; i++) {
		
			if (allocated_blocks[i] != NO_ADDRESS) {
		
				if (mmu.block_free(allocated_blocks[i]) == SUCCESS) {
					
					#ifdef DEF_TEST_EXPLICIT_OUTPUT
					debug_->debug("Freed virtual block: %u", allocated_blocks[i]);
					#endif //DEF_TEST_EXPLICIT_OUTPUT
					
					++number_successes;
				} else {
					#ifdef DEF_TEST_EXPLICIT_OUTPUT
					debug_->debug("Virtual block %u could not be freed.", allocated_blocks[i]);
					#endif //DEF_TEST_EXPLICIT_OUTPUT
				}
			} else {
				#ifdef DEF_TEST_EXPLICIT_OUTPUT
				debug_->debug("No virtual block to free.");
				#endif //DEF_TEST_EXPLICIT_OUTPUT
			}
		}
		
		bool success = (number_successes == number_expected) ? true : false; 
		
		#ifdef DEF_TEST_MINIMAL_OUTPUT
		debug_->debug("test_batch_block_free = %d", success);
		#endif //DEF_TEST_MINIMAL_OUTPUT
		
		return success;
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
	
	/* 1024 byte array
	unsigned char txt_in[] = "1000000 1000001 1000002 1000003 1000004 1000005 1000006 1000007 1000008 1000009 1000010 1000011 1000012 1000013 1000014 1000015 1000016 1000017 1000018 1000019 1000020 1000021 1000022 1000023 1000024 1000025 1000026 1000027 1000028 1000029 1000030 1000031 1000032 1000033 1000034 1000035 1000036 1000037 1000038 1000039 1000040 1000041 1000042 1000043 1000044 1000045 1000046 1000047 1000048 1000049 1000050 1000051 1000052 1000053 1000054 1000055 1000056 1000057 1000058 1000059 1000060 1000061 1000062 1000063 1000064 1000065 1000066 1000067 1000068 1000069 1000070 1000071 1000072 1000073 1000074 1000075 1000076 1000077 1000078 1000079 1000080 1000081 1000082 1000083 1000084 1000085 1000086 1000087 1000088 1000089 1000090 1000091 1000092 1000093 1000094 1000095 1000096 1000097 1000098 1000099 1000100 1000101 1000102 1000103 1000104 1000105 1000106 1000107 1000108 1000109 1000110 1000111 1000112 1000113 1000114 1000115 1000116 1000117 1000118 1000119 1000120 1000121 1000122 1000123 1000124 1000125 1000126 ENDMES.";
	512 byte array
	unsigned char txt_in[] = "1000000 1000001 1000002 1000003 1000004 1000005 1000006 1000007 1000008 1000009 1000010 1000011 1000012 1000013 1000014 1000015 1000016 1000017 1000018 1000019 1000020 1000021 1000022 1000023 1000024 1000025 1000026 1000027 1000028 1000029 1000030 1000031 1000032 1000033 1000034 1000035 1000036 1000037 1000038 1000039 1000040 1000041 1000042 1000043 1000044 1000045 1000046 1000047 1000048 1000049 1000050 1000051 1000052 1000053 1000054 1000055 1000056 1000057 1000058 1000059 1000060 1000061 1000062 ENDMES.";
	*/
	
	
	
	
	void init( OsModel::AppMainParameter& value )
	{
		//GENERAL SETUP
		debug_ = &wiselib::FacetProvider<OsModel, OsModel::Debug>::get_facet( value );
		debug_->debug("\n\n >>> Launching application! <<< \n\n" );
		sd_ = &wiselib::FacetProvider<OsModel, OsModel::BlockMemory>::get_facet(value);
		sd_->init();
		
		bool all_tests_successful = true;
		bool test0 = true;
		bool test1 = true;
		bool test2 = true;
		bool test3 = true;
	
		//MMU SETUP
		MMU_0_t mmu_0(sd_, debug_, false, true);
		
		//PRINT MMU PROPERTIES
			/*debug_->debug("sd_ = %x", sd_);
			debug_->debug("sizeof(MMU_0_t) = %u", sizeof(MMU_0_t));
			debug_->debug("sizeof(MMU_0_t.stack) = %u", mmu_0.sizeofStack());
			debug_->debug("sizeof(MMU_0_t::block_address_t) = %u", sizeof(typename MMU_0_t::block_address_t));*/
		debug_->debug("Stack size = %d", mmu_0.STACK_SIZE);
		debug_->debug("MMU administers %d virtual block(s).", mmu_0.TOTAL_VBLOCKS);
		debug_->debug("Reserved %d virtual block(s) for special purposes. (Starting from virtual block address 0.)\n", mmu_0.get_reserved());

		//TESTING
		size_t number_of_blocks = 5;
		size_t allocated_blocks[number_of_blocks];

		test0 = test_batch_block_alloc<MMU_0_t>(mmu_0, allocated_blocks, number_of_blocks, 5);
		test1 = test_batch_block_free<MMU_0_t>(mmu_0, allocated_blocks, number_of_blocks, 5);
		test2 = test_erase_write_read<MMU_0_t>(mmu_0, 0, MMU_0_t::BLOCK_SIZE);
		
		all_tests_successful = test0 && test1 && test2 && test3;
		
		//PRINT OVERALL RESULT
		debug_->debug("all_tests_successful = %d", all_tests_successful);
		
		//SHUTDOWN
		exit(0);
		
	}
	
	
	
	// --------------------------------------------------------------------
	
   private:

	OsModel::Debug::self_pointer_t debug_;
	OsModel::BlockMemory::self_pointer_t sd_;
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

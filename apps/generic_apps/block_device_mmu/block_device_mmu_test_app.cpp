/*
 * Test application for BDMMU
 */

#define USE_RAM_BLOCK_MEMORY 1
//#define DEBUG

#include <external_interface/external_interface.h>
#include "block_device_mmu.hpp"
 
#define DEF_BLOCK_NUMBER_NULL_VALUE 999999999
#define DEF_TEST_MINIMAL_OUTPUT
//#define DEF_TEST_EXPLICIT_OUTPUT

typedef wiselib::OSMODEL OsModel;

class BDMMUTestApp
{
   public:
   
   	typedef typename OsModel::size_t size_t;
   	typedef typename OsModel::size_t address_t;
   	
   	/* Allocate 'number_tries' virtual blocks and store their block numbers in the array allocated_blocks. Note 
   	that if no more blocks are available a magic value, DEF_BLOCK_NUMBER_NULL_VALUE which is preprocessor defined
   	is written into the array, as a NULL value. */
   	template <typename MMU>
   	bool test_batch_block_alloc(MMU mmu, size_t* allocated_blocks, size_t number_tries, size_t number_expected) {
		
		size_t number_successes = 0;
		
		for (size_t i = 0; i < number_tries; i++) {

			if (mmu.block_alloc(&allocated_blocks[i]) == OsModel::SUCCESS) {
			
				#ifdef DEF_TEST_EXPLICIT_OUTPUT
				debug_->debug("Allocated virtual block: %u.", allocated_blocks[i]);
				debug_->debug("Its real block number is: %u.\n", mmu.vr(allocated_blocks[i]));
				#endif //DEF_TEST_EXPLICIT_OUTPUT
				
				++number_successes;
			} else {
				allocated_blocks[i] = DEF_BLOCK_NUMBER_NULL_VALUE;
				
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
   	
   	/* Read 'number' virtual block numbers from 'allocated_blocks' and free those blocks. There is a preprocessor
   	defined DEF_BLOCK_NUMBER_NULL_VALUE, which is interpreted as a NULL value in the array, resulting in the free
   	operation not being executed for that entry. */
   	template <typename MMU>
   	bool test_batch_block_free(MMU mmu, size_t* allocated_blocks, size_t number_tries, size_t number_expected) {
   	
   		size_t number_successes = 0;
   	
		for (size_t i = 0; i < number_tries; i++) {
		
			if (allocated_blocks[i] != DEF_BLOCK_NUMBER_NULL_VALUE) {
		
				if (mmu.block_free(allocated_blocks[i]) == OsModel::SUCCESS) {
					
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
   	bool test_write_read_512(MMU mmu, size_t start_address) {
   		//512 byte arrays
		unsigned char txt_in[] = "1000000 1000001 1000002 1000003 1000004 1000005 1000006 1000007 1000008 1000009 1000010 1000011 1000012 1000013 1000014 1000015 1000016 1000017 1000018 1000019 1000020 1000021 1000022 1000023 1000024 1000025 1000026 1000027 1000028 1000029 1000030 1000031 1000032 1000033 1000034 1000035 1000036 1000037 1000038 1000039 1000040 1000041 1000042 1000043 1000044 1000045 1000046 1000047 1000048 1000049 1000050 1000051 1000052 1000053 1000054 1000055 1000056 1000057 1000058 1000059 1000060 1000061 1000062 ENDMES.";
		unsigned char txt_out[512];
		
		int write = mmu.write(txt_in, start_address, 1);
		int read = mmu.read(txt_out, start_address, 1);
		bool wr_success = true;
		
		for (size_t i = 0; i < 1024; i++) {
			if (txt_in[i] != txt_out[i]) wr_success = false;
		}
		
		#ifdef DEF_TEST_EXPLICIT_OUPUT
		debug_->debug("Write status = %d\n", write);
		debug_->debug("Read status = %d\n", read);
		#endif //DEF_TEST_EXPLICIT_OUPUT
		
		#ifdef DEF_TEST_MINIMAL_OUTPUT
		debug_->debug("test_write_read_512(%d) = %d", start_address, wr_success);
		debug_->debug("= %d", wr_success);
		#endif //DEF_TEST_MINIMAL_OUTPUT
				
		return wr_success;
   	}
   	
   	template <typename MMU>
   	bool test_write_read_1024(MMU mmu, size_t start_address) {
   		//1024 byte arrays
		unsigned char txt_in[] = "1000000 1000001 1000002 1000003 1000004 1000005 1000006 1000007 1000008 1000009 1000010 1000011 1000012 1000013 1000014 1000015 1000016 1000017 1000018 1000019 1000020 1000021 1000022 1000023 1000024 1000025 1000026 1000027 1000028 1000029 1000030 1000031 1000032 1000033 1000034 1000035 1000036 1000037 1000038 1000039 1000040 1000041 1000042 1000043 1000044 1000045 1000046 1000047 1000048 1000049 1000050 1000051 1000052 1000053 1000054 1000055 1000056 1000057 1000058 1000059 1000060 1000061 1000062 1000063 1000064 1000065 1000066 1000067 1000068 1000069 1000070 1000071 1000072 1000073 1000074 1000075 1000076 1000077 1000078 1000079 1000080 1000081 1000082 1000083 1000084 1000085 1000086 1000087 1000088 1000089 1000090 1000091 1000092 1000093 1000094 1000095 1000096 1000097 1000098 1000099 1000100 1000101 1000102 1000103 1000104 1000105 1000106 1000107 1000108 1000109 1000110 1000111 1000112 1000113 1000114 1000115 1000116 1000117 1000118 1000119 1000120 1000121 1000122 1000123 1000124 1000125 1000126 ENDMES.";
		unsigned char txt_out[1024];
		
		int write = mmu.write(txt_in, start_address, 1);
		int read = mmu.read(txt_out, start_address, 1);
		bool wr_success = true;
		
		for (size_t i = 0; i < 1024; i++) {
			if (txt_in[i] != txt_out[i]) wr_success = false;
		}
		
		#ifdef DEF_TEST_EXPLICIT_OUPUT
		debug_->debug("Write status = %d\n", write);
		debug_->debug("Read status = %d\n", read);
		#endif //DEF_TEST_EXPLICIT_OUPUT
		
		#ifdef DEF_TEST_MINIMAL_OUTPUT
		debug_->debug("test_write_read_1024(%d) = %d", (int)start_address, (int)wr_success);
		#endif //DEF_TEST_MINIMAL_OUTPUT
		
		return wr_success;
   	}
   	
   	
   	
   	
   	
	void init( OsModel::AppMainParameter& value )
	{
		//GENERAL SETUP
		debug_ = &wiselib::FacetProvider<OsModel, OsModel::Debug>::get_facet( value );
		debug_->debug("\n\n >>> Launching application! <<< \n\n" );
		sd_ = &wiselib::FacetProvider<OsModel, OsModel::BlockMemory>::get_facet(value);
		sd_->init();
		bool all_tests_successful = true;
		
		//MMU SETUP
		typedef typename BDMMU_Template_Wrapper<OsModel, OsModel::BlockMemory, OsModel::Debug>::BDMMU<0, 11, 1, 2> MMU_0_t; //typedef BDMMU<OsModel, 0, 11, 1, 2, 512, OsModel::Debug, OsModel::BlockMemory> MMU_0_t;
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

		all_tests_successful &= test_batch_block_alloc<MMU_0_t>(mmu_0, allocated_blocks, number_of_blocks, 2);
		all_tests_successful &=	test_batch_block_free<MMU_0_t>(mmu_0, allocated_blocks, number_of_blocks, 2);
		all_tests_successful &=	test_write_read_1024<MMU_0_t>(mmu_0, 0); //TODO: Write a generic method which takes block_size as a parameter, which generates itself a suitable char[] to test with
		
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

#ifdef DEF_BLOCK_NUMBER_NULL_VALUE
	#undef DEF_BLOCK_NUMBER_NULL_VALUE
#endif //DEF_BLOCK_NUMBER_NULL_VALUE

#ifdef DEF_TEST_EXPLICIT_OUTPUT 
	#undef DEF_TEST_EXPLICIT_OUTPUT
#endif //DEF_TEST_EXPLICIT_OUTPUT

#ifdef DEF_TEST_MINIMAL_OUTPUT 
	#undef DEF_TEST_MINIMAL_OUTPUT
#endif //DEF_TEST_MINIMAL_OUTPUT

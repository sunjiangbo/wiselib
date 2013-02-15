#define USE_RAM_BLOCK_MEMORY 1

/*
 * Test application for BDMMU
 */
#include <external_interface/external_interface.h>
#include "BDMMU.hpp"
//#include <external_interface/default_return_values.h>


typedef wiselib::OSMODEL OsModel;

class BDMMUTestApp
{
   public:
   
	void init( OsModel::AppMainParameter& value )
	{
		debug_ = &wiselib::FacetProvider<OsModel, OsModel::Debug>::get_facet( value );
	
		debug_->debug("\n\n >>> Launching application! <<< \n\n" );
		
		sd_ = &wiselib::FacetProvider<OsModel, OsModel::BlockMemory>::get_facet(value);
		sd_->init();
		
		BDMMU<OsModel, 0, 10, 2, 2, 512, OsModel::Debug, OsModel::BlockMemory> mmu(sd_, debug_, false, true);
		
		int blocks_to_allocate = 5;
		int b[blocks_to_allocate];
		
		debug_->debug("Stack size = %d\n", mmu.get_STACK_SIZE());

		//Allocate all
		for (int i = 0; i < blocks_to_allocate; i++) {

			if (mmu.block_alloc(&b[i])) {
				debug_->debug("Allocated virtual block: %d. Its real block number is: %d.\n", b[i], mmu.vr(b[i]));
			} else {
				b[i] = -999;
				debug_->debug("Virtual block could not be allocated.\n");
			}
		}

		//Free all
		for (int i = 0; i < blocks_to_allocate; i++) {

			if (b[i] != -999) {
		
				if (mmu.block_free(b[i])) {
					debug_->debug("Freed virtual block: %d \n", b[i]);
				} else {
					debug_->debug("Virtual block %d could not be freed.\n", b[i]);
				}
			} else {
				debug_->debug("No virtual block to free.\n");
			}
		}
		
		/*int persistence_test_block1 = 99;
		int persistence_test_block2 = 99;
		mmu.block_alloc(&persistence_test_block1);
		mmu.block_alloc(&persistence_test_block2);
		debug_->debug("Allocated virtual block %d \n", persistence_test_block1);
		debug_->debug("Allocated virtual block %d \n", persistence_test_block2);
		
		mmu.block_free(persistence_test_block1);
		debug_->debug("Freed virtual block %d \n", persistence_test_block1);*/
		
		
		//1024 byte arrays
		unsigned char txt_in[] = "1000000 1000001 1000002 1000003 1000004 1000005 1000006 1000007 1000008 1000009 1000010 1000011 1000012 1000013 1000014 1000015 1000016 1000017 1000018 1000019 1000020 1000021 1000022 1000023 1000024 1000025 1000026 1000027 1000028 1000029 1000030 1000031 1000032 1000033 1000034 1000035 1000036 1000037 1000038 1000039 1000040 1000041 1000042 1000043 1000044 1000045 1000046 1000047 1000048 1000049 1000050 1000051 1000052 1000053 1000054 1000055 1000056 1000057 1000058 1000059 1000060 1000061 1000062 1000063 1000064 1000065 1000066 1000067 1000068 1000069 1000070 1000071 1000072 1000073 1000074 1000075 1000076 1000077 1000078 1000079 1000080 1000081 1000082 1000083 1000084 1000085 1000086 1000087 1000088 1000089 1000090 1000091 1000092 1000093 1000094 1000095 1000096 1000097 1000098 1000099 1000100 1000101 1000102 1000103 1000104 1000105 1000106 1000107 1000108 1000109 1000110 1000111 1000112 1000113 1000114 1000115 1000116 1000117 1000118 1000119 1000120 1000121 1000122 1000123 1000124 1000125 1000126 ENDMES.";
		
		debug_->debug("txt_in:\n");
		
		/*for (int i = 0; i < 9; ++i) {
			debug_->debug("	 %s\n", &txt_in[120*i]);
		}*/
		
		//debug_->debug("Write status = %d\n", mmu.write(txt_in, persistence_test_block2, 1));
		debug_->debug("Write status = %d\n", mmu.write(txt_in, 0, 1));
		unsigned char txt_out[1024];
		
		//512 byte arrays
		//unsigned char txt_in[] = "1000000 1000001 1000002 1000003 1000004 1000005 1000006 1000007 1000008 1000009 1000010 1000011 1000012 1000013 1000014 1000015 1000016 1000017 1000018 1000019 1000020 1000021 1000022 1000023 1000024 1000025 1000026 1000027 1000028 1000029 1000030 1000031 1000032 1000033 1000034 1000035 1000036 1000037 1000038 1000039 1000040 1000041 1000042 1000043 1000044 1000045 1000046 1000047 1000048 1000049 1000050 1000051 1000052 1000053 1000054 1000055 1000056 1000057 1000058 1000059 1000060 1000061 1000062 ENDMES.";
		//unsigned char txt_out[512];
		
		
		
		/*debug_->debug("Write status = %d\n", mmu.write(txt_in, persistence_test_block2, 2));
		
		debug_->debug("\nDestroying old MMU...\n ");
		mmu.~BDMMU();
		
		debug_->debug("\n\n\n ------------------- Creating new MMU... -------------------\n\n\n ");
		BDMMU<OsModel, 0, 8, OsModel::Debug, 1, 512> m_res(sd_, debug_, true, true);
		
		//Allocate all blocks
		for (int i = 0; i < blocks_to_allocate; i++) {

			if (mmu.block_alloc(&b[i])) {
				debug_->debug("Allocated virtual block: %d. Its real block number is: %d.\n", b[i], mmu.vr(b[i]));
			} else {
				b[i] = -999;
				debug_->debug("Virtual block could not be allocated.\n");
			}
		}
		
		debug_->debug("Read status = %d\n", mmu.read(txt_in, persistence_test_block2, 2));
		
	*/
		//Erase Test
		/*debug_->debug("Erase & rewind... ;)\n");
		mmu.erase(0,1);*/
	
	
		debug_->debug("Read status = %d\n", mmu.read(txt_out, 0, 1));
		
		
		bool rw_success = true;
		
		for (int i = 0; i < 1024; i++) {
			if (txt_in[i] != txt_out[i]) rw_success = false;
		}	
		
		
		
		debug_->debug("txt_out:\n");
		
		/*for (int i = 0; i < 9; ++i) {
			debug_->debug("	 %s\n", &txt_out[120*i]);
		}*/
		
		debug_->debug("rw_sucess = %d\n", rw_success);
	
		//mmu_res.block_free(persistence_test_block2);
		
		debug_->debug("free vBlock 20 = %d", mmu.block_free(20));
		
		//debug_->debug("Test: ERR_IO_HATES_YOU = %d\n", return_value_test());
		
		
		exit(0);
		
	}
	
	int return_value_test() {
		return OsModel::ERR_IO_HATES_YOU;
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

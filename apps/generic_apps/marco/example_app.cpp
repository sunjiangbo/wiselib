/*
 * Simple Wiselib Example
 */
#include <external_interface/external_interface.h>
#include <external_interface/arduino/arduino_sdcard.h>
#include <external_interface/arduino/arduino_debug.h>
#include "EXMMU_0_4.hpp"

typedef wiselib::OSMODEL OsModel;

class ExampleApplication
{
   public:
	void init( Os::AppMainParameter& value )
	{
		debug_ = &wiselib::FacetProvider<Os, Os::Debug>::get_facet( value );
	
		debug_->debug( "Hello World from Example Application!\n" );
		
		sd_ = &wiselib::FacetProvider<Os, Os::BlockMemory>::get_facet(value);
		sd_->init();
		
		/*TODO: What do you need to put into the template parameters here so that 
		OS stuff is put there correctly? Check in an example app somewhere?*/
		
		EXMMU<OsModel, int, OsModel::Debug> m(sd_, 0, 6, 1, 512, false);
		
		int blocks_to_allocate = 5;
		int b[blocks_to_allocate];
		
		debug_->debug("Stack size = %d\n", m.get_STACK_SIZE());

		//Allocate all
		for (int i = 0; i < blocks_to_allocate; i++) {

			if (m.block_alloc(&b[i])) {
				debug_->debug("Allocated virtual block: %d. Its real block number is: %d.\n", b[i], m.vr(b[i]));
			} else {
				b[i] = -999;
				debug_->debug("Virtual block could not be allocated.\n");
			}
		}

		//Free all
		for (int i = 0; i < blocks_to_allocate; i++) {

			if (b[i] != -999) {
		
				if (m.block_free(b[i])) {
					debug_->debug("Freed virtual block: %d \n", b[i]);
				} else {
					debug_->debug("Virtual block %d could not be freed.\n", b[i]);
				}
			} else {
				debug_->debug("No virtual block to free.\n");
			}
		}
		
	}
      
      // --------------------------------------------------------------------
      
   private:

	Os::Debug::self_pointer_t debug_;
	Os::BlockMemory::self_pointer_t sd_;
//	ArduinoSdCard<Os> sd_;
};
// --------------------------------------------------------------------------
wiselib::WiselibApplication<Os, ExampleApplication> example_app;
// --------------------------------------------------------------------------
void application_main( Os::AppMainParameter& value )
{
	example_app.init( value );
}

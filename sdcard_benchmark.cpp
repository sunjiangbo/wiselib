#include <external_interface/external_interface.h>
#include <external_interface/arduino/arduino_sdcard.h>
#include <external_interface/arduino/arduino_debug.h>
#include <external_interface/arduino/arduino_clock.h>

#define NR_OF_BLOCKS_TO_TEST 8

typedef wiselib::OSMODEL Os;

class App {
	public:
		
		// NOTE: this uses up a *lot* of RAM, way too much for uno!
		enum { BlOCK_SIZE = 512 , BLOCKS_IN_1GB=2097152};
		
		void init(Os::AppMainParameter& value) {

			debug_ = &wiselib::FacetProvider<Os, Os::Debug>::get_facet( value );
			clock_ = &wiselib::FacetProvider<Os, Os::Clock>::get_facet( value );


			debug_->debug( "Starting SD Card Benchmark" );
			benchmark_sd();
		}
		
		void benchmark_sd() {
			// initialize test buffer
			for(Os::size_t i=0; i<BlOCK_SIZE*NR_OF_BLOCKS_TO_TEST; i++) {
				test_buffer_[i] = 255;
			}
			
			int r = Os::SUCCESS;
			

			sd_.init();
			
			debug_->debug("writing...");
			for(int i = 0; i < 20000; i+=NR_OF_BLOCKS_TO_TEST)
			{
				Os::Clock::time_t start = clock_->time();
				r = sd_.write(test_buffer_, i, NR_OF_BLOCKS_TO_TEST );
				if(r != Os::SUCCESS) break;
				Os::Clock::time_t end = clock_->time();
				debug_->debug("%d %d",i, (end - start));
			}
			
			if(r != Os::SUCCESS) { debug_->debug("Error %d", r); }
		}
		
	private:
		//static Os::Debug dbg;
		Os::Debug::self_pointer_t debug_;
		Os::Clock::self_pointer_t clock_;
		Os::block_data_t test_buffer_[BlOCK_SIZE*NR_OF_BLOCKS_TO_TEST];
		wiselib::ArduinoSdCard<Os> sd_;
};

//Os::Debug App::dbg = Os::Debug();

wiselib::WiselibApplication<Os, App> app;
void application_main( Os::AppMainParameter& value ) {
	app.init(value);
}

#include <external_interface/external_interface.h>
#include <external_interface/arduino/arduino_debug.h>
#include <external_interface/arduino/arduino_sdcard.h>
#include "HashMap.h"
#include "util/serialization/simple_types.h"
#define NR_OF_BLOCKS_TO_TEST 8

typedef wiselib::OSMODEL Os;

class App {
public:

	// NOTE: this uses up a *lot* of RAM, way too much for uno!
	enum {
		BlOCK_SIZE = 512, BLOCKS_IN_1GB = 2097152
	};

	void init(Os::AppMainParameter& value) {
		debug_ = &wiselib::FacetProvider<Os, Os::Debug>::get_facet(value);

		testHashMap();
	}

	void testHashMap() {
		wiselib::HashMap hashMap(debug_);
		debug_->debug("testing the hashmap now");
		hashMap.putEntry(0, "dies ist ein test");
		debug_->debug("Write first entry");
		hashMap.putEntry(1, "ein zweiter test");
		debug_->debug("Write second entry");




		char buffer[BlOCK_SIZE];
		//debug_->debug(buffer);
		hashMap.getEntry(0, buffer);
		//debug_->debug(buffer);
		hashMap.getEntry(1, buffer);
		//debug_->debug(buffer);
	}

private:
	//static Os::Debug dbg;
	Os::Debug::self_pointer_t debug_;
	//wiselib::ArduinoSdCard<Os> sd;
};

//Os::Debug App::dbg = Os::Debug();

wiselib::WiselibApplication<Os, App> app;
void application_main(Os::AppMainParameter& value) {
	app.init(value);
}

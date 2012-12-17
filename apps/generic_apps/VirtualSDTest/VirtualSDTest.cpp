#include <external_interface/external_interface.h>
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
		bm_ = &wiselib::FacetProvider<Os, Os::BlockMemory>::get_facet(value);
		testVirtualSD();
	}

	void testVirtualSD()
	{
		Os::block_data_t buffer[BlOCK_SIZE];

		int data = 42;
		char *string = "Hello World";

		int generalOffset = 55;

		int writePos = 0;
		writePos += wiselib::write<Os, Os::block_data_t, int>(buffer + writePos + generalOffset, data);
		writePos += wiselib::write<Os, Os::block_data_t, char*>(buffer + writePos + generalOffset, string);

		bm_->write(buffer, 5);

		Os::block_data_t buffer2[BlOCK_SIZE];

		bm_->read(buffer2, 5);

		int readingPos = 0;
		debug_->debug("%d was read", wiselib::read<Os, Os::block_data_t, int>(buffer2 + readingPos + generalOffset));
		readingPos += sizeof(int);
		debug_->debug("%s was read", wiselib::read<Os, Os::block_data_t, char*>(buffer2 + readingPos + generalOffset));
	}

private:
	Os::Debug::self_pointer_t debug_;
	Os::BlockMemory::self_pointer_t bm_;
};

wiselib::WiselibApplication<Os, App> app;
void application_main(Os::AppMainParameter& value) {
	app.init(value);
}

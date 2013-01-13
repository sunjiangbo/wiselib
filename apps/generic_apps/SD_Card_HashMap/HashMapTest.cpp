#include <external_interface/external_interface.h>
#include "util/serialization/simple_types.h"
#include "HashMap.h"
#include "Block.h"
#define NR_OF_BLOCKS_TO_TEST 8

typedef wiselib::OSMODEL Os;


struct Message
{
	char* string[5];
	float pi;
};

class App {
public:

	// NOTE: this uses up a *lot* of RAM, way too much for uno!
	enum {
		BlOCK_SIZE = 512, BLOCKS_IN_1GB = 2097152
	};

	void init(Os::AppMainParameter& value) {
		debug_ = &wiselib::FacetProvider<Os, Os::Debug>::get_facet(value);
		sd = &wiselib::FacetProvider<Os, Os::BlockMemory>::get_facet(value);

		testHashMap();
		//testBlock();
		exit(0);
	}

	Message makeMessage(const char* string)
	{
		Message m;
		memcpy(m.string, string, 20);
		m.pi = 3.141592;
		return m;
	}

	void printMessage(Message m)
	{
		debug_->debug("%s", m.string);
		debug_->debug("%f", m.pi);
	}

	void testBlock()
	{
		wiselib::Block<int, Message> b(0, sd);
		debug_->debug("size of message: %d", sizeof(Message));
		debug_->debug("Number of entries In block: %d", b.getNumValues());
		debug_->debug("Max number of Values: %d", b.maxNumValues());

		Message m1, m2, m3;
		m1 = makeMessage("ABCDEFGHIJ");
		m2 = makeMessage("TES^T123");
		m3 = makeMessage("M3");

		b.insertValue(3, m1);
		b.insertValue(2, m2);
		b.insertValue(1, m3);

		sd->printASCIIOutputBytes(0,0);

		debug_->debug("Value with key 1:");
		printMessage(b.getValueByKey(1));

		debug_->debug("");

		debug_->debug("Value with id 0:");
		printMessage(b.getValueByID(0));


		b.writeBack();

		sd->printASCIIOutputBytes(0,0);

		wiselib::Block<int, Message> b2(0, sd);
		debug_->debug("");
		printMessage(b2.getValueByKey(3));
		//if(b.containsKey(33)) debug_->debug("It contains key 33");

		sd->printASCIIOutputBytes(0,0);
	}

	void testHashMap()
	{
		Message m1, m2, m3;
		m1 = makeMessage("ABCDEFGHIJ");
		m2 = makeMessage("TES^T123");
		m3 = makeMessage("M3");

		wiselib::HashMap<int, Message> hashMap(debug_, sd);

		hashMap.putEntry(0, m1);
		//sd->printASCIIOutputBytes(0, 1);
		hashMap.putEntry(1, m2);
		//sd->printASCIIOutputBytes(0, 1);

		//sd->printASCIIOutputBytes(0, 1);

		printMessage(hashMap.getEntry(0));
		printMessage(hashMap.getEntry(1));
	}

private:
	//static Os::Debug dbg;
	Os::Debug::self_pointer_t debug_;
	Os::BlockMemory::self_pointer_t sd;
};

//Os::Debug App::dbg = Os::Debug();

wiselib::WiselibApplication<Os, App> app;
void application_main(Os::AppMainParameter& value) {
	app.init(value);
}

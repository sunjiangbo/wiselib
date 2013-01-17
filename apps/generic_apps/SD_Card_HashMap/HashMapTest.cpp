//#define DEBUG
#include <external_interface/external_interface.h>
#include "util/serialization/simple_types.h"
#include "HashMap.h"
#include "Block.h"
#define NR_OF_BLOCKS_TO_TEST 8

typedef wiselib::OSMODEL Os;


struct Message
{
	int m1;
	int m2;
	//char* string[5];
	//float pi;
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

		//testBlockRemoving();
		simpleTestHashMap();
		testBlockRemoving();
		//testBlock();
		sequentialTestHashMap();
		exit(0);
	}

	/*Message makeMessage(const char* string)
	{
		Message m;
		memcpy(m.string, string, 5);
		m.pi = 3.141592;
		return m;
	}*/

	Message makeMessage(int m1, int m2)
	{
		Message m;
		m.m1 = m1;
		m.m2 = m2;
		return m;
	}

	void printMessage(Message m)
	{
		debug_->debug("m1: %d\nm2: %d", m.m1, m.m2);
		//debug_->debug("%s", m.string);
		//debug_->debug("%f", m.pi);
	}

	void printBlock(wiselib::Block<int, Message> b)
	{
		int numValues = b.getNumValues();
		debug_->debug("\n\nBlock:__________");
		for(int i = 0; i < numValues; i++)
		{
			printMessage(b.getValueByID(i));
			debug_->debug("________________");
		}
	}

	void testBlockRemoving()
	{
		wiselib::Block<int, Message> b(0, sd);
		Message m1, m2, m3;
		m1 = makeMessage(4, 2);
		m2 = makeMessage(99, 100);
		m3 = makeMessage(23, 24);

		b.insertValue(1, m1);
		b.insertValue(2, m3);
		b.insertValue(3, m2);

		printBlock(b);

		b.removeValue(2);

		printBlock(b);
	}

	void testBlock()
	{
		wiselib::Block<int, Message> b(0, sd);
		debug_->debug("size of message: %d", sizeof(Message));
		debug_->debug("Number of entries In block: %d", b.getNumValues());
		debug_->debug("Max number of Values: %d", b.maxNumValues());

		Message m1, m2, m3;
		m1 = makeMessage(4, 2);
		m2 = makeMessage(99, 100);
		m3 = makeMessage(23, 24);

		b.insertValue(3, m1);
		b.insertValue(2, m2);
		b.insertValue(1, m3);

		//sd->printASCIIOutputBytes(0,0);

		debug_->debug("Value with key 1:");
		printMessage(b.getValueByKey(1));

		debug_->debug("");

		debug_->debug("Value with id 0:");
		printMessage(b.getValueByID(0));


		b.writeBack();

		//sd->printASCIIOutputBytes(0,0);

		wiselib::Block<int, Message> b2(0, sd);
		debug_->debug("");
		printMessage(b2.getValueByKey(3));
		//if(b.containsKey(33)) debug_->debug("It contains key 33");

		//sd->printASCIIOutputBytes(0,0);
	}

	void simpleTestHashMap()
	{
		wiselib::HashMap<int, Message> hashMap(debug_, sd);
		Message m1, m2, m3;
		m1 = makeMessage(4, 2);
		m2 = makeMessage(99, 100);
		m3 = makeMessage(23, 24);

		//First simple insertion and retrieval test
		debug_->debug("We insert 1st entry:");
		hashMap.putEntry(0, m1);
		debug_->debug("");
		debug_->debug("We insert 2nd entry:");
		hashMap.putEntry(1, m2);

		debug_->debug("\n");

		debug_->debug("we read 1st entry:");
		printMessage(hashMap.getEntry(0));
		debug_->debug("");
		debug_->debug("We read 2nd entry:");
		printMessage(hashMap.getEntry(1));
	}

	void sequentialTestHashMap()
	{
		wiselib::HashMap<int, Message, 0, 50> hashMap(debug_, sd);
		Message m1, m2, m3;
		m1 = makeMessage(4, 2);
		m2 = makeMessage(99, 100);
		m3 = makeMessage(23, 24);

		for(int i = 0; i < 300; i+=3)
		{
			hashMap.putEntry(i + 0, m1);
			hashMap.putEntry(i + 1, m2);
			hashMap.putEntry(i + 2, m3);
			debug_->debug("%3d:", i);
			//sd->printASCIIOutputBlocks(0, 50);
		}
		//sd->printStats();
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

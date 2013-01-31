//define DEBUG
#include <external_interface/external_interface.h>
#include "util/serialization/simple_types.h"
#include "HashMap.h"
#include "Block.h"
#include "Stopwatch.h"
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

		sd->init();
		//sequentialTestHashMap();

		exit(0);
	}

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


	void sequentialTestHashMap()
	{
		allStopwatch.startMeasurement();
		wiselib::HashMap<int, Message, 0, 25> hashMap(debug_, sd);
		Message m1, m2, m3;
		m1 = makeMessage(0xFFFFFFFF, 0xFFFFFFFF - 1);
		m2 = makeMessage(99, 100);
		m3 = makeMessage(23, 24);

		for(int i = 0; i < 1000; i++)
		{
			if(!hashMap.putEntry(i + 0, m1))
			{
				debug_->debug("could not insert element %d", i);
				debug_->debug("Failed at load factor: %f", hashMap.getLoadFactor());
				return;
			}
			//debug_->debug("%3d:", i);
			//sd->printASCIIOutputBlocks(0, 50);
		}
		allStopwatch.stopMeasurement();

		unsigned long int durationAll = allStopwatch.getAllTime();
		//unsigned long int durationIORead = readIOStopwatch.getAllTime();
		//unsigned long int durationIOWrite = writeIOStopwatch.getAllTime();
		//unsigned long int durationIO = durationIORead + durationIOWrite;
		unsigned long int durationIO = IOStopwatch.getAllTime();

		//debug_->debug("Complete duration: %u", durationAll);
		//debug_->debug("Read IO duration: %u", durationIORead);
		//debug_->debug("Write IO duration: %u", durationIOWrite);
		//debug_->debug("IO duration: %u", durationIO);

		/*unsigned long iorp = (durationIORead*1.0/durationAll) * 100.0;
		unsigned long iowp = (durationIOWrite*1.0/durationAll) * 100.0;
		unsigned long iop = (durationIO*1.0/durationAll) * 100.0;
		debug_->debug("Read IO's: %u of the time", iorp);
		debug_->debug("Write IO's: %u of the time", iowp);
		debug_->debug("IO's: %u of the time", iop);
		*/
		//sd->printStats();
		//sd->printGraphBytes(0, 50);
	}

private:

	Os::Debug::self_pointer_t debug_;
	Os::BlockMemory::self_pointer_t sd;
};


wiselib::WiselibApplication<Os, App> app;
void application_main(Os::AppMainParameter& value) {
	app.init(value);
}

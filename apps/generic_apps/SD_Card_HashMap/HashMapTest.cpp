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

		//testBlockRemoving();
		//simpleTestHashMap();
		//testBlockRemoving();
		//testBlock();
		sd->init();
		//sequentialTestHashMap();
		maxLoadFactorTest();
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
		debug_->debug("Value with id 0:");
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

	struct SmallValue
	{
		int load;
	};

	struct MiddleValue
	{
		int load1;
		long int load2;
		long int load3;
		long int load4;
	};

	struct BigValue
	{
		int load1;
		long int load2;
		long int load3;
		long int load4;
		long int load5;
		long int load6;
		long int load7;
		long int load8;
		long int load9;
		long int load10;
	};

	void maxLoadFactorTest()
	{
		SmallValue small;
		MiddleValue middle;
		BigValue big;

		wiselib::HashMap<int, SmallValue, 0, 100> smallValueMap(debug_, sd);
		wiselib::HashMap<int, MiddleValue, 100, 200> middleValueMap(debug_, sd);
		wiselib::HashMap<int, BigValue, 200, 300> bigValueMap(debug_, sd);

		int counter = 0;
		while(smallValueMap.putEntry(counter, small))
			counter++;
		debug_->debug("Failed with small values at %f load factor", smallValueMap.getLoadFactor());

		counter = 0;
		while(middleValueMap.putEntry(counter, middle))
			counter++;
		debug_->debug("Failed with middle values at %f load factor", middleValueMap.getLoadFactor());

		counter = 0;
		while(bigValueMap.putEntry(counter, big))
			counter++;
		debug_->debug("Failed with big values at %f load factor", bigValueMap.getLoadFactor());
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
	//static Os::Debug dbg;
	Os::Debug::self_pointer_t debug_;
	Os::BlockMemory::self_pointer_t sd;
};

//Os::Debug App::dbg = Os::Debug();

wiselib::WiselibApplication<Os, App> app;
void application_main(Os::AppMainParameter& value) {
	app.init(value);
}

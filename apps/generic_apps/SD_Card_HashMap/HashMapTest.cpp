//define DEBUG
#include <external_interface/external_interface.h>
#include "util/serialization/simple_types.h"
#include "HashMap.h"
#include "Block.h"
#include "BlockIterator.h"
#include "Stopwatch.h"
#include "HashMapIterator.h"
#include "HashFunctionProvider.h"
#include <stdio.h>

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

	struct Value1
	{
		long load;
	};

	struct Value2
	{
		long load1;
		long load2;
	};

	struct Value3
	{
		long load1;
		long load2;
		long load3;
	};

	struct Value4
	{
		long load1;
		long load2;
		long load3;
		long load4;
	};

	struct Value5
	{
		long load1;
		long load2;
		long load3;
		long load4;
		long load5;
	};

	struct Value6
	{
		long load1;
		long load2;
		long load3;
		long load4;
		long load5;
		long load6;
	};

	struct Value7
	{
		long load1;
		long load2;
		long load3;
		long load4;
		long load5;
		long load6;
		long load7;
	};

	struct Value8
	{
		long load1;
		long load2;
		long load3;
		long load4;
		long load5;
		long load6;
		long load7;
		long load8;
	};

	struct Value9
	{
		long load1;
		long load2;
		long load3;
		long load4;
		long load5;
		long load6;
		long load7;
		long load8;
		long load9;
	};

	struct Value10
	{
		long load1;
		long load2;
		long load3;
		long load4;
		long load5;
		long load6;
		long load7;
		long load8;
		long load9;
		long load10;
	};

	void init(Os::AppMainParameter& value) {
		debug_ = &wiselib::FacetProvider<Os, Os::Debug>::get_facet(value);
		sd = &wiselib::FacetProvider<Os, Os::BlockMemory>::get_facet(value);

		sd->init();
		if(!blockTest()) debug_->debug("blockTest failed!");
		if(!blockIteratorTest()) debug_->debug("blockIteratorTest failed!");
		if(!hashMapTest()) debug_->debug("hashMapTest failed!");
		if(!hashMapIteratorTest()) debug_->debug("hashMapTestIterator failed!");

//		maxLoadFactorTest();
//		generateGNUPLOTScripts();
		//generateFillupAnimation();
		exit(0);
	}

	bool blockTest()
	{
		sd->reset();
		wiselib::Block<int, long> block(0, sd);

		if(block.getNumValues() != 0)
		{
			debug_->debug("A new block should contain 0 values!");
			return false;
		}

		const int initialLen = 8;
		long values1[initialLen]	= {1, 1, 2, 3, 5, 8, 13, 21};
		int keys1[initialLen]		= {8, 7, 6, 5, 4, 3,  2,  1};

		// --- test if insertion works ---
		for(int i = 0; i < initialLen; ++i)
			if(block.insertValue(keys1[i], values1[i]) == BLOCK_FULL)
			{
				debug_->debug("Block was full while it should not be!");
				return false;
			}

		if(!blockContainsValues(&block, values1, initialLen))
		{
			debug_->debug("The block did not contain the values in the order it was supposed to after insertion!");
			return false;
		}

		if(block.getNumValues() != initialLen)
		{
			debug_->debug("The block failed to count its elements!");
			return false;
		}

		// --- test if contains key works ---

		for(int i = 0; i < initialLen; i++)
			if(!block.containsKey(keys1[i]))
			{
				debug_->debug("The block.containsKey claims that the key %d is not in the block while it should be in there", keys1[i]);
				return false;
			}

		for(int i = 9; i < 20; i++)
			if(block.containsKey(i))
			{
				debug_->debug("The block.containsKey claims that the block contains %d while it should not!", i);
				return false;
			}

		// --- test if deletion works ---

		// --- deletion in the middle
		long values2[initialLen - 1]	= {1, 1, 2, 3, 21, 8, 13};
		int keys2[initialLen - 1]		= {8, 7, 6, 5,  1, 3,  2};

		block.removeValue(4);
		if(!blockContainsValues(&block, values2, initialLen -1))
		{
			debug_->debug("The block did not contain the values in the order it was supposed to after deletion in the middle!");
			return false;
		}

		if(block.getNumValues() != initialLen - 1)
		{
			debug_->debug("The block failed to count its elements!");
			return false;
		}

		// --- deletion at the beginning ---
		long values3[initialLen - 2]	= {13, 1, 2, 3, 21, 8};
		int keys3[initialLen - 2]		= { 2, 7, 6, 5,  1, 3};

		block.removeValue(8);
		if(!blockContainsValues(&block, values3, initialLen - 2))
		{
			debug_->debug("The block did not contain the values in the order it was supposed to after deletion at the beginning!");
			return false;
		}

		if(block.getNumValues() != initialLen - 2)
		{
			debug_->debug("The block failed to count its elements!");
			return false;
		}

		// --- deletion at the end ---
		long values4[initialLen - 3]	= {13, 1, 2, 3, 21};
		int keys4[initialLen - 3]		= { 2, 7, 6, 5,  1};

		block.removeValue(3);
		if(!blockContainsValues(&block, values4, initialLen - 3))
		{
			debug_->debug("The block did not contain the values in the order it was supposed to after deletion at the end!");
			return false;
		}

		if(block.getNumValues() != initialLen - 3)
		{
			debug_->debug("The block failed to count its elements!");
			return false;
		}

		// --- writing to the block interface and reading again, also reading the meta info ---
		block.setNextBlock(5);
		block.setPrevBlock(7);

		block.writeBack();

		wiselib::Block<int, long> readBlock(0, sd);
		if(readBlock.getNextBlock() != 5)
		{
			debug_->debug("The block did not properly store the next block");
			return false;
		}
		if(readBlock.getPrevBlock() != 7)
		{
			debug_->debug("The block did not properly store the prev block");
			return false;
		}

		if(block.getNumValues() != initialLen - 3)
		{
			debug_->debug("The block failed to count its elements after reading again from the sd card!");
			return false;
		}

		if(!blockContainsValues(&readBlock, values4, initialLen - 3))
		{
			debug_->debug("The block could not retrieve the proper values after writing and reading from sd");
			return false;
		}

		sd->reset();
		// --- test appending ---
		{
			wiselib::Block<int, long> b1(1, sd);
			wiselib::Block<int, long> b2(2, sd);
			wiselib::Block<int, long> b3(3, sd);

			b1.append(&b2);
			b2.append(&b3);

			b1.writeBack();
			b2.writeBack();
			b3.writeBack();

			b1.initFromSD();
			b2.initFromSD();
			b3.initFromSD();

			if(b1.hasPrevBlock() == true || b1.hasNextBlock() == false || b1.getNextBlock() != 2)
			{
				debug_->debug("appending on b1 failed!");
				return false;
			}

			if(b2.hasPrevBlock() == false || b2.hasNextBlock() == false || b2.getPrevBlock() != 1 || b2.getNextBlock() != 3)
			{
				debug_->debug("appending on b2 failed!");
				return false;
			}

			if(b3.hasPrevBlock() == false || b3.hasNextBlock() == true || b3.getPrevBlock() != 2)
			{
				debug_->debug("appending on b3 failed!");
				return false;
			}
		}

		// --- test removing from block chain ---
		sd->reset();
		// --- removing block at the beginning of the chain---
		{
			wiselib::Block<int, long> b1(1, sd);
			wiselib::Block<int, long> b2(2, sd);
			wiselib::Block<int, long> b3(3, sd);

			b1.append(&b2);
			b2.append(&b3);

			b1.writeBack();
			b2.writeBack();
			b3.writeBack();

			b1.removeFromChain();
			b1.writeBack();

			b1.initFromSD();
			b2.initFromSD();
			b3.initFromSD();

			if(b1.hasNextBlock() || b1.hasPrevBlock())
			{
				debug_->debug("removing on b1 failed! at the beginning");
				return false;
			}
			if(b2.hasNextBlock() == false || b2.hasPrevBlock() || b2.getNextBlock() != 3)
			{
				debug_->debug("removing on b2 failed! at the beginning");
				debug_->debug("b2.nextBlock: %d, b2.prevBlock: %d", b2.getNextBlock(), b2.getPrevBlock());
				//return false;
			}
			if(b3.hasNextBlock() || b3.hasPrevBlock() == false || b3.getPrevBlock() != 2)
			{
				debug_->debug("removing on b3 failed! at the beginning");
				debug_->debug("b3.nextBlock: %d, b3.prevBlock: %d", b3.getNextBlock(), b3.getPrevBlock());
				return false;
			}
		}
		sd->reset();
		// --- remove block in the middle of the chain ---
		{
			wiselib::Block<int, long> b1(1, sd);
			wiselib::Block<int, long> b2(2, sd);
			wiselib::Block<int, long> b3(3, sd);
			b1.append(&b2);
			b2.append(&b3);
			b1.writeBack();
			b2.writeBack();
			b3.writeBack();

			b2.removeFromChain();
			b2.writeBack();

			b1.initFromSD();
			b2.initFromSD();
			b3.initFromSD();

			if(b1.hasNextBlock() == false || b1.hasPrevBlock() || b1.getNextBlock() != 3)
			{
				debug_->debug("removing on b1 failed! in the middle");
				return false;
			}
			if(b2.hasNextBlock()|| b2.hasPrevBlock())
			{
				debug_->debug("removing on b2 failed! in the middle");
				return false;
			}
			if(b3.hasNextBlock() || b3.hasPrevBlock() == false || b3.getPrevBlock() != 1)
			{
				debug_->debug("removing on b3 failed! in the middle");
				return false;
			}
		}

		sd->reset();
		// --- remove block at the end of the chain---
		{
			wiselib::Block<int, long> b1(1, sd);
			wiselib::Block<int, long> b2(2, sd);
			wiselib::Block<int, long> b3(3, sd);
			b1.append(&b2);
			b2.append(&b3);

			b1.writeBack();
			b2.writeBack();
			b3.writeBack();

			b3.removeFromChain();
			b3.writeBack();

			b1.initFromSD();
			b2.initFromSD();
			b3.initFromSD();

			if(b1.hasNextBlock() == false || b1.hasPrevBlock() || b1.getNextBlock() != 2)
			{
				debug_->debug("removing on b1 failed! at the end");
				return false;
			}
			if(b2.hasNextBlock()|| b2.hasPrevBlock() == false || b2.getPrevBlock() != 1)
			{
				debug_->debug("removing on b2 failed! at the end");
				return false;
			}
			if(b3.hasNextBlock() || b3.hasPrevBlock())
			{
				debug_->debug("removing on b3 failed! at the end");
				return false;
			}
		}

		sd->reset();
		// --- remove free floating block from the chain ---
		{
			wiselib::Block<int, long> b1(1, sd);
			b1.writeBack();

			b1.removeFromChain();
			b1.writeBack();

			if(b1.hasNextBlock() || b1.hasPrevBlock())
			{
				debug_->debug("Removing a free floating block from the chain failed!");
				return false;
			}
		}

		debug_->debug("Block test succeeds!");

		return true;
	}

	bool blockIteratorTest()
	{
		sd->reset();
		wiselib::Block<int, long> block(0, sd);
		const int initialLen = 8;
		long values1[initialLen]	= {1, 1, 2, 3, 5, 8, 13, 21};
		int keys1[initialLen]		= {8, 7, 6, 5, 4, 3,  2,  1};

		long arrayChecksum = 0;
		for(int i = 0; i < initialLen; i++)
		{
			block.insertValue(keys1[i], values1[i]);
			arrayChecksum += values1[i];
		}

		wiselib::BlockIterator<int, long> it(&block);

		int counter = 0;
		long blockChecksum = 0;
		while(!it.reachedEnd())
		{
			if(*it != values1[counter])
			{
				debug_->debug("The block iterator gave wrong values! at position %d", counter);
				return false;
			}
			blockChecksum += *it;
			++it;
			++counter;
		}

		if(counter < initialLen)
		{
			debug_->debug("The iterator ran only through %d elements while the block contains %d elements!", counter, initialLen);
			return false;
		}

		if(counter > initialLen)
		{
			debug_->debug("The iterator ran through %d elements while the block contains %d elements!", counter, initialLen);
			return false;
		}

		if(arrayChecksum != blockChecksum)
		{
			debug_->debug("Apparently the iterator id not iterate over all the elements, because the two checksums did not match up!");
			return false;
		}
		debug_->debug("Block Iterator test succeeds!");
		return true;
	}

	bool hashMapTest()
	{
		// --- simple insertion and retrieval of known values test ---
		sd->reset();
		wiselib::HashMap<int, long> hashMap(debug_, sd, &wiselib::HashFunctionProvider<Os, int>::fnv, 0, 100);
		const int initialLen = 8;
		long values1[initialLen]	= {1, 1, 2, 3, 5, 8, 13, 21};
		int keys1[initialLen]		= {8, 7, 6, 5, 4, 3,  2,  1};

		for(int i = initialLen - 1; i >= 0; i--) //fill in backwards
		{
			hashMap.putEntry(keys1[i], values1[i]);
		}

		for(int i = 0; i < initialLen; i++) //retrieve forwards
			if(hashMap[keys1[i]] != values1[i])
			{
				debug_->debug("Simple insertion and retrieval test on the hashmap failed!");
				return false;
			}

		// --- contains key method test ---
		for(int i = 0; i < initialLen; i++)
			if(!hashMap.containsKey(keys1[i]))
			{
				debug_->debug("Contains key gave a false negative!");
				return false;
			}

		for(int i = 9; i < 500; i++)
			if(hashMap.containsKey(i))
			{
				debug_->debug("Contains key gave a false positive on the key %d, apparently it has the value %d!", i, hashMap[i]);
				return false;
			}

		// --- test deletions ---

		hashMap.removeEntry(1);
		hashMap.removeEntry(2);
		hashMap.removeEntry(3);

		// --- are the others still there? ---

		for(int i = 0; i < initialLen - 3; i++)
			if(hashMap[keys1[i]] != values1[i])
			{
				debug_->debug("hashMap.removeEntry removed to much!");
				return false;
			}

		// --- are the deleted ones really away? ---
		for(int i = 1; i <= 3; i++)
			if(hashMap.containsKey(i))
			{
				debug_->debug("The hashmap still contains key %d while it was removed just before!", i);
				return false;
			}

		debug_->debug("Hash Map test succeeds!");
		return true;
	}

	bool hashMapIteratorTest()
	{
		sd->reset();
		wiselib::HashMap<int, long> hashMap(debug_, sd, &wiselib::HashFunctionProvider<Os, int>::fnv, 0, 100);
		const int initialLen = 8;
		long values1[initialLen]	= {1, 1, 2, 3, 5, 8, 13, 21};
		int keys1[initialLen]		= {8, 7, 6, 5, 4, 3,  2,  1};

		long arrayChecksum = 0;
		long hashMapChecksum = 0;

		for(int i = initialLen - 1; i >= 0; i--) //fill in backwards
		{
			hashMap.putEntry(keys1[i], values1[i]);
			arrayChecksum += values1[i];
		}

		wiselib::HashMapIterator<int, long> it(hashMap.getFirstUsedBlock(), sd);
		int elementCounter = 0;

		while(!it.reachedEnd())
		{
			if(!arrayContainsValue(values1, initialLen, *it))
			{
				debug_->debug("The HashMapIterator returned the value %d that was not supposed to be in the HashMap!", *it);
				return false;
			}
			hashMapChecksum += *it;
			++it;
			++elementCounter;
		}

		if(elementCounter < initialLen)
		{
			debug_->debug("The iterator ran only through %d elements while the hashmap contains %d elements!", elementCounter, initialLen);
			return false;
		}

		if(elementCounter > initialLen)
		{
			debug_->debug("The iterator ran through %d elements while the hashmap contains %d elements!", elementCounter, initialLen);
			return false;
		}

		if(arrayChecksum != hashMapChecksum)
		{
			debug_->debug("Apparently the iterator id not iterate over all the elements, because the two checksums did not match up!");
			return false;
		}


		debug_->debug("HashMapIterator test succeeds!");
		return true;
	}

	bool arrayContainsValue(long array[], int arrayLen, long value)
	{
		for(int i = 0; i < arrayLen; i++)
			if(array[i] == value) return true;

		return false;
	}


	bool blockContainsValues(wiselib::Block<int, long> *b, long values[], int numValues)
	{
		for(int i = 0; i < numValues; ++i)
		{
			if(b->getValueByID(i) != values[i])
			{
				return false;
			}
		}
		return true;
	}

	void maxLoadFactorTest()
	{
		for(int hashFunction = 0; hashFunction <=2; ++hashFunction)
		{
			char filename[100];
			sprintf(filename, "hashFunction%d.dat", hashFunction);
			FILE* file = fopen(filename, "w");
			fprintf(file, "#numblocks loadfactor\n");
			for(int numBlocks = 1; numBlocks < 700; ++numBlocks)
			{
				sd->reset();
				wiselib::HashMap<long, Value1> hashMap1(debug_, sd, wiselib::HashFunctionProvider<Os, long>::getHashFunction(hashFunction), 0, numBlocks);
				fillupHashMap<Value1>(&hashMap1);
				float lf1 = hashMap1.getLoadFactor();

				sd->reset();
				wiselib::HashMap<long, Value2> hashMap2(debug_, sd, wiselib::HashFunctionProvider<Os, long>::getHashFunction(hashFunction), 0, numBlocks);
				fillupHashMap<Value2>(&hashMap2);
				float lf2 = hashMap2.getLoadFactor();

				sd->reset();
				wiselib::HashMap<long, Value3> hashMap3(debug_, sd, wiselib::HashFunctionProvider<Os, long>::getHashFunction(hashFunction), 0, numBlocks);
				fillupHashMap<Value3>(&hashMap3);
				float lf3 = hashMap3.getLoadFactor();

				sd->reset();
				wiselib::HashMap<long, Value4> hashMap4(debug_, sd, wiselib::HashFunctionProvider<Os, long>::getHashFunction(hashFunction), 0, numBlocks);
				fillupHashMap<Value4>(&hashMap4);
				float lf4 = hashMap4.getLoadFactor();

				sd->reset();
				wiselib::HashMap<long, Value5> hashMap5(debug_, sd, wiselib::HashFunctionProvider<Os, long>::getHashFunction(hashFunction), 0, numBlocks);
				fillupHashMap<Value5>(&hashMap5);
				float lf5 = hashMap5.getLoadFactor();

				sd->reset();
				wiselib::HashMap<long, Value6> hashMap6(debug_, sd, wiselib::HashFunctionProvider<Os, long>::getHashFunction(hashFunction), 0, numBlocks);
				fillupHashMap<Value6>(&hashMap6);
				float lf6 = hashMap6.getLoadFactor();

				sd->reset();
				wiselib::HashMap<long, Value7> hashMap7(debug_, sd, wiselib::HashFunctionProvider<Os, long>::getHashFunction(hashFunction), 0, numBlocks);
				fillupHashMap<Value7>(&hashMap7);
				float lf7 = hashMap7.getLoadFactor();

				sd->reset();
				wiselib::HashMap<long, Value8> hashMap8(debug_, sd, wiselib::HashFunctionProvider<Os, long>::getHashFunction(hashFunction), 0, numBlocks);
				fillupHashMap<Value8>(&hashMap8);
				float lf8 = hashMap8.getLoadFactor();

				sd->reset();
				wiselib::HashMap<long, Value9> hashMap9(debug_, sd, wiselib::HashFunctionProvider<Os, long>::getHashFunction(hashFunction), 0, numBlocks);
				fillupHashMap<Value9>(&hashMap9);
				float lf9 = hashMap9.getLoadFactor();

				sd->reset();
				wiselib::HashMap<long, Value10> hashMap10(debug_, sd, wiselib::HashFunctionProvider<Os, long>::getHashFunction(hashFunction), 0, numBlocks);
				fillupHashMap<Value10>(&hashMap10);
				float lf10 = hashMap10.getLoadFactor();

				fprintf(file, "%d %f %f %f %f %f %f %f %f %f %f\n", numBlocks, lf1, lf2, lf3, lf4, lf5, lf6, lf7, lf8, lf9, lf10);
				debug_->debug("analyzing with %d blocks", numBlocks);
			}
			fclose(file);
		}

	}

	void generateGNUPLOTScripts()
	{
		for(int hashFunction = 0; hashFunction <= 4; ++hashFunction)
		{
			char filename[100];
			sprintf(filename, "comValSizes%d.dat", hashFunction);
			FILE* file = fopen(filename, "w");
			fprintf(file, "set term x11\n");
			fprintf(file, "h%d = './hashFunction%d.dat'\n", hashFunction, hashFunction);
			fprintf(file, "set style line 2  lc rgb '#0025ad' lt 1 lw 0.5 # --- blue\n");
			fprintf(file, "set style line 3  lc rgb '#0042ad' lt 1 lw 0.5 #      .\n");
			fprintf(file, "set style line 4  lc rgb '#0060ad' lt 1 lw 0.5 #      .\n");
			fprintf(file, "set style line 5  lc rgb '#007cad' lt 1 lw 0.5 #      .\n");
			fprintf(file, "set style line 6  lc rgb '#0099ad' lt 1 lw 0.5 #      .\n");
			fprintf(file, "set style line 7  lc rgb '#00ada4' lt 1 lw 0.5 #      .\n");
			fprintf(file, "set style line 8  lc rgb '#00ad88' lt 1 lw 0.5 #      .\n");
			fprintf(file, "set style line 9  lc rgb '#00ad6b' lt 1 lw 0.5 #      .\n");
			fprintf(file, "set style line 10 lc rgb '#00ad4e' lt 1 lw 0.5 #      .\n");
			fprintf(file, "set style line 11 lc rgb '#00ad31' lt 1 lw 0.5 #      .\n");
			fprintf(file, "set style line 12 lc rgb '#00ad14' lt 1 lw 0.5 #      .\n");
			fprintf(file, "set style line 13 lc rgb '#09ad00' lt 1 lw 0.5 # --- green\n");
			fprintf(file, "plot for [n=2:11] './hashFunction%d.dat' using 1:n title '(n*4)bytes' with lines ls n", hashFunction );
			fprintf(file, "\n");
			fclose(file);
		}

	}

	template<typename ValueType>
	void fillupHashMap(wiselib::HashMap<long, ValueType>* hashMap)
	{
		ValueType dummyValue;
		long counter = 0;
		while(hashMap->putEntry(counter, dummyValue) == OK)
		{
			counter++;
		}

	}

	void generateFillupAnimation()
	{
		wiselib::HashMap<int, long> hashMap(debug_, sd, &wiselib::HashFunctionProvider<Os, int>::fnv, 0, 50);

		int counter = 0;
		long value = 0xFFFFFFFFL;
		while(hashMap.putEntry(counter, value) == OK)
		{
			char filename[20];
			sprintf(filename, "bild%d.dot", counter);
			FILE* file = fopen(filename, "w");
			sd->printGraphBytes(0, 100, file);
			fclose(file);
			debug_->debug("we inserted %d items", counter);
			counter++;
		}
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

	void sequentialTestHashMap()
	{
		allStopwatch.startMeasurement();
		wiselib::HashMap<int, Message> hashMap(debug_, sd, 0, 25);
		Message m1, m2, m3;
		m1 = makeMessage(0xFFFFFFFF, 0xFFFFFFFF - 1);
		m2 = makeMessage(99, 100);
		m3 = makeMessage(23, 24);

		for(int i = 0; i < 1000; i++)
		{
			//if(!hashMap.putEntry(i + 0, m1))
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

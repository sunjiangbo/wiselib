//define DEBUG
#define USE_FILE_BLOCK_MEMORY 1

#include <external_interface/external_interface.h>
#include "util/serialization/simple_types.h"
#include "hash_map.h"
#include "block.h"
#include "block_iterator.h"
#include "stopwatch.h"
#include "hash_map_iterator.h"
#include "hash_function_provider.h"
#include <stdio.h>


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
//		generateGNUPLOTData();
//		generateFillupAnimation();
		fillWithGoethe();

//		if(!stupidSDCardTest()) debug_->debug("stupidSDCardTest failed!");
//		if(!blockTest()) debug_->debug("blockTest failed!");
//		if(!blockIteratorTest()) debug_->debug("blockIteratorTest failed!");
//		if(!hashMapTest()) debug_->debug("hashMapTest failed!");
//		if(!hashMapIteratorTest()) debug_->debug("hashMapTestIterator failed!");
//		if(!reverseFNVTest()) debug_->debug("reverseFNVTest failed!");
//		maxLoadFactorTest();
//		generateGNUPLOTScripts();
		//generateFillupAnimation();
		exit(0);
	}

	uint32_t fib(uint16_t i)
	{
		if(i < 3)
			return 1;
		else
			return fib(i-1) + fib(i-2);
	}

	bool reverseFNVTest()
	{
		sd->erase(0, 1000);
		wiselib::HashMap<Os::size_t, uint16_t> hashMap(debug_, sd, &wiselib::HashFunctionProvider<Os, Os::size_t>::fnv, 0, 1000);

		wiselib::HashFunctionProvider<Os, uint16_t>::hashFunction myfnv = wiselib::HashFunctionProvider<Os, uint16_t>::fnv;
		uint16_t i = 0;
		for(; hashMap.putEntry(myfnv(i), i) == Os::SUCCESS; i++)
		{

		}

		--i;

		for(; i > 0; i--)
		{
			size_t fnvKey = myfnv(i);
			uint16_t readValue;
			if(hashMap.getEntry(fnvKey, &readValue) != Os::SUCCESS)
			{
				debug_->debug("the hash map did not contain the origin for the hash %u while it should", fnvKey);
				return false;
			}
			if(readValue != i)
			{
				debug_->debug("according to the hashmap %u reversehashes to %u. But it reversehashes to %u", fnvKey, readValue, i);
				return false;
			}
		}

		debug_->debug("reverseFNVTest test succeeds");
		return true;
	}

	bool stupidSDCardTest()
	{
		//Make sure that the sd card gets wiped
		if(sd->erase(0, 1000) != Os::SUCCESS)
		{
			debug_->debug("Failed to erase the first 1000 blocks of the sd card!");
			return false;
		}

		Os::block_data_t buffer[sd->BLOCK_SIZE]; //create a buffer with the right size
		for(int i = 0; i < sd->BLOCK_SIZE; ++i) buffer[i] = 0; //make sure that the buffer really only contains zeros!

		long value = 123456789; //create a magic value

		wiselib::write<Os, Os::block_data_t, long>(buffer, value); //write the magic value to the buffer using wiselib magic

		if(sd->write(buffer, 2) != Os::SUCCESS) //write the buffer to block 2
		{
			debug_->debug("failed to write onto the sd card");
			return false;
		}

		for(int i = 0; i < sd->BLOCK_SIZE; ++i) buffer[i] = 0; //again make sure that the buffer really only contains zeros

		if(sd->read(buffer, 2) != Os::SUCCESS) //read the buffer again from the sd card
		{
			debug_->debug("failed to read from the sd card");
			return false;
		}

		long read = wiselib::read<Os, Os::block_data_t, long>(buffer); //retrieve the magic value from the buffer

		if(value != read) //compare with original magic value //< -----------------------------------    this fails
		{
			debug_->debug("failed to write and retrieve a simple value!");
			return false;
		}
		debug_->debug("stupidSDCardTest succeeds!");
		return true;
	}

	bool blockTest()
	{
		const int initialLen = 8;

		{
			sd->erase(0, 1000);
			wiselib::Block<int, long> block(0, sd);

			if(block.getNumValues() != 0)
			{
				debug_->debug("A new block should contain 0 values!");
				return false;
			}

			long values1[initialLen]	= {1, 1, 2, 3, 5, 8, 13, 21};
			int keys1[initialLen]		= {8, 7, 6, 5, 4, 3,  2,  1};

			// --- test if re-insertino / updating works ---
			for(int i = 0; i < initialLen; ++i)
				if(block.insertValue(keys1[i], values1[i]) == Os::ERR_NOMEM)
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

			long values2[initialLen]	= {1, 1, 2, 3, 5, 8, 13, 22};
			int keys2[initialLen]		= {8, 7, 6, 5, 4, 3,  2,  1};

			block.insertValue(keys2[initialLen-1], values2[initialLen-1]);

			if(!blockContainsValues(&block, values2, initialLen))
			{
				debug_->debug("The block did not contain the values in the order it was supposed to after re-insertion!");
				return false;
			}

			if(block.getNumValues() != initialLen)
			{
				debug_->debug("The block failed to count its elements after re-insertion!");
				return false;
			}
		}

		{
			//sd->reset();
			sd->erase(0, 1000);
			wiselib::Block<int, long> block(0, sd);

			if(block.getNumValues() != 0)
			{
				debug_->debug("A new block should contain 0 values!");
				return false;
			}

			long values1[initialLen]	= {1, 1, 2, 3, 5, 8, 13, 21};
			int keys1[initialLen]		= {8, 7, 6, 5, 4, 3,  2,  1};

			// --- test if insertion works ---
			for(int i = 0; i < initialLen; ++i)
				if(block.insertValue(keys1[i], values1[i]) == Os::ERR_NOMEM)
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

			// --- test contains key for false positives ---
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
			if(block.writeBack() != Os::SUCCESS)
			{
				debug_->debug("Writhing back the block to the sd card failed");
				return false;
			}
		}

		{
			wiselib::Block<int, long> readBlock(0, sd);
			long values4[initialLen - 3]	= {13, 1, 2, 3, 21};
			int keys4[initialLen - 3]		= { 2, 7, 6, 5,  1};

			if(!blockContainsValues(&readBlock, values4, initialLen - 3))
			{
				debug_->debug("The block could not retrieve the proper values after writing and reading from sd");
				return false;
			}

			if(readBlock.getNextBlock() != 5)
			{
				debug_->debug("The block did not properly store the next block after reading again from the sd card!");
				return false;
			}
			if(readBlock.getPrevBlock() != 7)
			{
				debug_->debug("The block did not properly store the prev block after reading again from the sd card!");
				return false;
			}

			if(readBlock.getNumValues() != initialLen - 3)
			{
				debug_->debug("The block failed to count its elements after reading again from the sd card!");
				return false;
			}


			//sd->reset();
			sd->erase(0, 1000);

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
		}

		// --- test removing from block chain ---
		//sd->reset();
		sd->erase(0, 1000);
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
		//sd->reset();
		sd->erase(0, 1000);
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

		//sd->reset();
		sd->erase(0, 1000);
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

		//sd->reset();
		sd->erase(0, 1000);
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
		//sd->reset();
		sd->erase(0, 1000);
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

		wiselib::Block<int, long>::iterator it = block.begin();

		int counter = 0;
		long blockChecksum = 0;
		while(it != block.end())
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
		//sd->reset();
		sd->erase(0, 1000);
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
		//sd->reset();
		sd->erase(0, 1000);
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

		wiselib::HashMap<int, long>::iterator it = hashMap.begin();


		int elementCounter = 0;

		while(it != hashMap.end())
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
			if((*b)[i] != values[i])
			{
				return false;
			}
		}
		return true;
	}
/*
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
				//sd->reset();
		sd->erase(0, 1000);
				wiselib::HashMap<long, Value1> hashMap1(debug_, sd, wiselib::HashFunctionProvider<Os, long>::getHashFunction(hashFunction), 0, numBlocks);
				fillupHashMap<Value1>(&hashMap1);
				float lf1 = hashMap1.getLoadFactor();

				//sd->reset();
		sd->erase(0, 1000);
				wiselib::HashMap<long, Value2> hashMap2(debug_, sd, wiselib::HashFunctionProvider<Os, long>::getHashFunction(hashFunction), 0, numBlocks);
				fillupHashMap<Value2>(&hashMap2);
				float lf2 = hashMap2.getLoadFactor();

				//sd->reset();
		sd->erase(0, 1000);
				wiselib::HashMap<long, Value3> hashMap3(debug_, sd, wiselib::HashFunctionProvider<Os, long>::getHashFunction(hashFunction), 0, numBlocks);
				fillupHashMap<Value3>(&hashMap3);
				float lf3 = hashMap3.getLoadFactor();

				//sd->reset();
		sd->erase(0, 1000);
				wiselib::HashMap<long, Value4> hashMap4(debug_, sd, wiselib::HashFunctionProvider<Os, long>::getHashFunction(hashFunction), 0, numBlocks);
				fillupHashMap<Value4>(&hashMap4);
				float lf4 = hashMap4.getLoadFactor();

				//sd->reset();
		sd->erase(0, 1000);
				wiselib::HashMap<long, Value5> hashMap5(debug_, sd, wiselib::HashFunctionProvider<Os, long>::getHashFunction(hashFunction), 0, numBlocks);
				fillupHashMap<Value5>(&hashMap5);
				float lf5 = hashMap5.getLoadFactor();

				//sd->reset();
		sd->erase(0, 1000);
				wiselib::HashMap<long, Value6> hashMap6(debug_, sd, wiselib::HashFunctionProvider<Os, long>::getHashFunction(hashFunction), 0, numBlocks);
				fillupHashMap<Value6>(&hashMap6);
				float lf6 = hashMap6.getLoadFactor();

				//sd->reset();
		sd->erase(0, 1000);
				wiselib::HashMap<long, Value7> hashMap7(debug_, sd, wiselib::HashFunctionProvider<Os, long>::getHashFunction(hashFunction), 0, numBlocks);
				fillupHashMap<Value7>(&hashMap7);
				float lf7 = hashMap7.getLoadFactor();

				//sd->reset();
		sd->erase(0, 1000);
				wiselib::HashMap<long, Value8> hashMap8(debug_, sd, wiselib::HashFunctionProvider<Os, long>::getHashFunction(hashFunction), 0, numBlocks);
				fillupHashMap<Value8>(&hashMap8);
				float lf8 = hashMap8.getLoadFactor();

				//sd->reset();
		sd->erase(0, 1000);
				wiselib::HashMap<long, Value9> hashMap9(debug_, sd, wiselib::HashFunctionProvider<Os, long>::getHashFunction(hashFunction), 0, numBlocks);
				fillupHashMap<Value9>(&hashMap9);
				float lf9 = hashMap9.getLoadFactor();

				//sd->reset();
		sd->erase(0, 1000);
				wiselib::HashMap<long, Value10> hashMap10(debug_, sd, wiselib::HashFunctionProvider<Os, long>::getHashFunction(hashFunction), 0, numBlocks);
				fillupHashMap<Value10>(&hashMap10);
				float lf10 = hashMap10.getLoadFactor();

				fprintf(file, "%d %f %f %f %f %f %f %f %f %f %f\n", numBlocks, lf1, lf2, lf3, lf4, lf5, lf6, lf7, lf8, lf9, lf10);
				debug_->debug("analyzing with %d blocks", numBlocks);
			}
			fclose(file);
		}

	}
*/
	/*
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
	*/

	template<typename ValueType>
	void fillupHashMap(wiselib::HashMap<long, ValueType>* hashMap)
	{
		ValueType dummyValue;
		long counter = 0;
		while(hashMap->putEntry(counter, dummyValue) == Os::SUCCESS)
		{
			counter++;
		}

	}
/*
	void generateGNUPLOTData()
	{
		FILE* file = fopen("blockmem.dat", "w");
		sd->erase(0, 1000);
		wiselib::HashMap<long, Value3> hashMap4(debug_, sd, wiselib::HashFunctionProvider<Os, long>::fnv, 0, 300);

		Value3 dummy;
		for(long i = 0; i < 300; i++)
		{
			hashMap4.putEntry(i, dummy);
		}

//		fillupHashMap<Value3>(&hashMap4);


//		sd->printGNUPLOTOutputBytes(0, 100, file, colorizeBlock<wiselib::Block<long, Value3> >);
		fclose(file);
	}*/

	template<typename BlockType>
	static int colorizeBlock(Os::size_t block, int position, int value)
	{
		if(value == 0)
			return 0;

		if(position < sizeof(typename BlockType::header))
			return value/2;



		int posInKVPair = (position - sizeof(typename BlockType::header)) % sizeof(typename BlockType::keyValuePair);

		if(posInKVPair < sizeof(typename BlockType::KeyType))
			return value/2 + 255;
		else
			return value/2 + 512;
	}
/*
	void generateFillupAnimation()
	{
		wiselib::HashMap<int, long> hashMap(debug_, sd, &wiselib::HashFunctionProvider<Os, int>::fnv, 0, 50);

		int counter = 0;
		long value = 0xFFFFFFFFL;
		int imageCounter = 0;
		while(hashMap.putEntry(counter, value) == Os::SUCCESS)
		{
			if(counter % 5 == 0)
			{
				char filename[20];
				sprintf(filename, "bild%04d.dot", imageCounter);
				imageCounter++;
				FILE* file = fopen(filename, "w");
				sd->printGNUPLOTOutputBytes(0, 50, file, colorizeBlock<wiselib::Block<int, long> >);
				fclose(file);
			}
			if(counter == 300)
			{
				FILE* file = fopen("dummy.dot", "w");
				sd->printGraphBytes(3, 10, file);
				fclose(file);
			}
//			debug_->debug("we inserted %d items", counter);
			counter++;
		}
	}*/


	void printBlock(wiselib::Block<int, long> b)
	{

		int numValues = b.getNumValues();
		debug_->debug("\n\nBlock:__________");
		for(int i = 0; i < numValues; i++)
		{
			debug_->debug("%d",b[i]);
		}
		debug_->debug("________________");
	}

	void printRawBlock(size_t nr)
	{
		debug_->debug("Block %d:", nr);
		Os::block_data_t buffer[sd->BLOCK_SIZE];
		sd->read(buffer, nr);
		for(int i = 0; i < 50; i++) //only the first 50 bytes
			debug_->debug("%3d", ((int)buffer[i]));
	}

	struct goetheline
	{
		char line[62];
	};

	struct goetheword
	{
		char word[30];
		uint16_t idx;

		bool operator==(const goetheword& o) const
		{
			for(int i = 0; i < 30; ++i)
				if(o.word[i] != word[i])
					return false;
			if(o.idx == idx)
				return true;
			else
				return false;
		}
	};

	void fillWithGoethe()
	{
		wiselib::HashMap<uint16_t, goetheline> linesHM(debug_, sd, &wiselib::HashFunctionProvider<Os, uint16_t>::fnv, 0, 2000);
		wiselib::HashMap<goetheword, uint16_t> wordsHM(debug_, sd, &wiselib::HashFunctionProvider<Os, goetheword>::fnv, 2000, 20000);

		wiselib::Block<uint16_t, goetheline> lineBlock(1, sd);

		wiselib::Block<uint16_t, goetheline>::iterator it = lineBlock.begin();

		FILE *fp;
		fp = fopen("lines.txt", "r");
		uint16_t lineNumber = 0;
		while(! feof(fp))
		{
			char line[62];
			fgets(line, 62, fp);

			goetheline gl;
			for(int i = 0; i < 62; i++)
				gl.line[i] = line[i];

			//printf("lineNumber: %d \n line: %s\n\n",lineNumber, line);
			lineNumber++;
			if(linesHM.putEntry(lineNumber-1, gl) != Os::SUCCESS)
			{
				debug_->debug("the hash map is too small");
				break;
			}
			printf("line: %d", lineNumber);
		}
		fclose(fp);

		fp = fopen("words.txt", "r");
		while(! feof(fp))
		{
			char line[62];
			fgets(line, 62, fp);
			char word[30];

			sscanf(line, "%d %s", &lineNumber, word);
			int idx = 0;

			goetheword gw;
			for(int i = 0; i < 30; i++)
				gw.word[i] = word[i];

			for(int i = strlen(word); i < 30; i++)
				gw.word[i] = '\0';

			gw.idx = 0;

			while(wordsHM.containsKey(gw))
			{
				gw.idx++;
			}

			if(wordsHM.putEntry(gw, lineNumber) != Os::SUCCESS)
			{
				debug_->debug("words HM full at line %d!", lineNumber);
				break;
			}

			printf("line: %d  word: %s\n", lineNumber, gw.word);
		}
		fclose(fp);


		debug_->debug("");

		lookForWord("Kern", linesHM, wordsHM);
		lookForWord("Religion", linesHM, wordsHM);
		lookForWord("Kunst", linesHM, wordsHM);



//		sd->printASCIIOutputBytes(0, 1);
	}

	void lookForWord(char* word, wiselib::HashMap<uint16_t, goetheline> linesHM, wiselib::HashMap<goetheword, uint16_t> wordsHM)
	{
		goetheword gw;

		for(int i = 0; i < strlen(word); i++)
			gw.word[i] = word[i];
		for(int i = strlen(word); i < 30; i++)
			gw.word[i] = '\0';


		gw.idx = 0;

		uint16_t occ = 3;

		debug_->debug("looking for %s ...", word, occ);

		int ret = wordsHM.getEntry(gw, &occ);
		debug_->debug("%s is at line %d", word, occ);

		if(ret == Os::NO_VALUE)
			debug_->debug("no value was found");

		debug_->debug("%d: %s\n", occ, linesHM[occ].line);
	}

private:
	Os::Debug::self_pointer_t debug_;
	Os::BlockMemory::self_pointer_t sd;
};

wiselib::WiselibApplication<Os, App> app;
void application_main(Os::AppMainParameter& value) {
	app.init(value);
}

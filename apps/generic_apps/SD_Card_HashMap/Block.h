/*
 * Block.h
 *
 *  Created on: Jan 8, 2013
 *      Author: Maximilian Ernestus
 */

#ifndef BLOCK_H_
#define BLOCK_H_

#include <external_interface/external_interface.h>
#include "util/serialization/simple_types.h"

#include "Stopwatch.h"


namespace wiselib {

typedef OSMODEL Os;

template<typename KeyType_P, typename ValueType_P, int blocksize = 512>
class Block
{

public:
	typedef KeyType_P KeyType;
	typedef ValueType_P ValueType;

	typedef struct
	{
		long pi;
		uint8_t numKVPairs;
	} header;

	typedef struct
	{
		KeyType key;
		ValueType value;
	} keyValuePair;

	Block(long int nr, Os::BlockMemory::self_pointer_t sd)
	{
		this->blockNr = nr;
		this->sd = sd;

		IOStopwatch.startMeasurement();
		sd->read(rawData, nr); //read the raw data from the sd card
		IOStopwatch.stopMeasurement();

		head = read<Os, Os::block_data_t, header>(rawData);

		//If the block has not been used yet
		if(head.pi != 123456789)
		{
#ifdef DEBUG
			printf("Block %d has not been used yet\n", nr);
#endif
			head.pi = 123456789;
			head.numKVPairs = 0;
		}
#ifdef DEBUG
		else
			printf("Block %d contains %d elements\n", nr, head.numKVPairs);
#endif
	}

	/*header getHeader()
	{
		return read<Os, Os::block_data_t, header>(rawData);
	}

	void setHeader(header& h)
	{
		write<Os, Os::block_data_t, header>(rawData, h);
	}*/

	ValueType getValueByID(uint8_t id)
	{
		return getKVPairByID(id).value;
	}

	ValueType getValueByKey(KeyType key)
	{
		for(int i = 0; i < getNumValues(); i++)
		{
			keyValuePair kvPair = getKVPairByID(i);
			if(kvPair.key == key)
				return kvPair.value;
		}
#ifdef DEBUG
		printf("Could not get the value for key %d because it is not in this block\n", key);
#endif
	}

	bool containsKey(KeyType key)
	{
		return getIDForKey(key) != -1;
	}

	bool containsID(uint8_t id)
	{
		return id < getNumValues() && id >= 0;
	}

	bool insertValue(KeyType key, ValueType& value)
	{
		if(getNumValues() < maxNumValues())
		{
			keyValuePair pair;
			pair.key = key;
			pair.value = value;
			write<Os, Os::block_data_t, keyValuePair>(rawData + computePairOffset(getNumValues()) , pair);
			head.numKVPairs = getNumValues() + 1;
#ifdef DEBUG
			printf("we inserted key %d into block %d\n", key, blockNr);
#endif
			return true;
		}
		else
		{
#ifdef DEBUG
			printf("Could not insert value with key %d becuase the block is full\n", key);
#endif
			return false;
		}
	}

	bool removeValue(KeyType key)
	{
		int startID = getIDForKey(key);
		if(startID == -1)
			return false;

		for(int i = startID; i < head.numKVPairs-1; i++)
		{
			moveKVPair(i+1, i);
		}
		head.numKVPairs--;
		return true;
	}

	int getNumValues()
	{
		return head.numKVPairs;
	}

	int maxNumValues()
	{
		return (blocksize - sizeof(header)) / sizeof(keyValuePair);
	}

	bool writeBack()
	{
		write<Os, Os::block_data_t, header>(rawData, head);
		IOStopwatch.startMeasurement();
		bool s = sd->write(rawData, blockNr) == Os::SUCCESS;
		IOStopwatch.stopMeasurement();
		return s;
	}

private:
	Os::block_data_t rawData[blocksize];
	Os::size_t blockNr;
	header head;

	Os::BlockMemory::self_pointer_t sd;

	keyValuePair getKVPairByID(uint8_t id)
	{
		if(id < getNumValues() && id >= 0)
		{
#ifdef DEBUG
			printf("reading id %d from pos %d\n", id, computePairOffset(id));
#endif
			return read<Os, Os::block_data_t, keyValuePair>(rawData + computePairOffset(id));
		}
#ifdef DEBUG
		else
			printf("tried to acess id %d which does not exist!\n", id);
#endif
	}

	int computePairOffset(uint8_t id)
	{
		return sizeof(header) + id * sizeof(keyValuePair);
	}

	uint8_t getIDForKey(KeyType k)
	{
		for(int i = 0; i < head.numKVPairs; i++)
		{
			if(getKVPairByID(i).key == k)
				return i;
		}
		return -1;
	}

	void moveKVPair(uint8_t fromID, uint8_t toID)
	{
		keyValuePair p = getKVPairByID(fromID);
		insertKVPairAtID(p, toID);
	}

	void insertKVPairAtID(keyValuePair kvp, uint8_t id)
	{
		write<Os, Os::block_data_t, keyValuePair>(rawData + computePairOffset(id) , kvp);
	}
};

}// NS WISELIB

#endif /* BLOCK_H_ */

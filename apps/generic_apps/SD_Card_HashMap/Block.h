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
		int numKVPairs;
	} header;

	typedef struct
	{
		KeyType key;
		ValueType value;
	} keyValuePair;

	Block(int nr, Os::BlockMemory::self_pointer_t sd)
	{
		this->blockNr = nr;
		this->sd = sd;

		readIOStopwatch.startMeasurement();
		sd->read(rawData, nr); //read the raw data from the sd card
		readIOStopwatch.stopMeasurement();

		//If the block has not been used yet
		if(getHeader().pi != 123456789)
		{
#ifdef DEBUG
			printf("Block %d has not been used yet\n", nr);
#endif
			header newHeader;
			newHeader.pi = 123456789;
			newHeader.numKVPairs = 0;
			setHeader(newHeader);
		}
#ifdef DEBUG
		else
			printf("Block %d contains %d elements\n", nr, getHeader().numKVPairs);
#endif
	}

	header getHeader()
	{
		return read<Os, Os::block_data_t, header>(rawData);
	}

	void setHeader(header& h)
	{
		write<Os, Os::block_data_t, header>(rawData, h);
	}

	ValueType getValueByID(int id)
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
		printf("Could not get the value for key %d becaus it is not in this block\n", key);
#endif
	}

	bool containsKey(KeyType key)
	{
		return getIDForKey(key) != -1;
	}

	bool containsID(int id)
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
			header h = getHeader();
			h.numKVPairs = getNumValues() + 1;
			setHeader(h);
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
		header h = getHeader();
		for(int i = startID; i < h.numKVPairs-1; i++)
		{
			moveKVPair(i+1, i);
		}
		h.numKVPairs--;
		setHeader(h);
		return true;
	}

	int getNumValues()
	{
		return getHeader().numKVPairs;
	}

	int maxNumValues()
	{
		return (blocksize - sizeof(header)) / sizeof(keyValuePair);
	}

	bool writeBack()
	{
		writeIOStopwatch.startMeasurement();
		bool s = sd->write(rawData, blockNr) == Os::SUCCESS;
		writeIOStopwatch.stopMeasurement();
		return s;
	}

private:
	Os::block_data_t rawData[blocksize];
	int blockNr;
	//int numKVPairs;
	Os::BlockMemory::self_pointer_t sd;

	keyValuePair getKVPairByID(int id)
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

	int computePairOffset(int id)
	{
		return sizeof(header) + id * sizeof(keyValuePair);
	}

	int getIDForKey(KeyType k)
	{
		header h = getHeader();
		for(int i = 0; i < h.numKVPairs; i++)
		{
			if(getKVPairByID(i).key == k)
				return i;
		}
		return -1;
	}

	void moveKVPair(int fromID, int toID)
	{
		keyValuePair p = getKVPairByID(fromID);
		insertKVPairAtID(p, toID);
	}

	void insertKVPairAtID(keyValuePair kvp, int id)
	{
		write<Os, Os::block_data_t, keyValuePair>(rawData + computePairOffset(id) , kvp);
	}
};

}// NS WISELIB

#endif /* BLOCK_H_ */

/*
 * Block.h
 *
 *  Created on: Jan 8, 2013
 *      Author: maximilian
 */

#ifndef BLOCK_H_
#define BLOCK_H_

#include <external_interface/external_interface.h>
#include "util/serialization/simple_types.h"

namespace wiselib {

typedef OSMODEL Os;

template<typename keyType, typename valueType, int blocksize = 512>
class Block
{
	typedef struct
	{
		long pi;
		int numKVPairs;
	} header;

	typedef struct
	{
		keyType key;
		valueType value;
	} keyValuePair;

public:

	Block(int nr, Os::BlockMemory::self_pointer_t sd)
	{
		this->blockNr = nr;
		this->sd = sd;

		sd->read(rawData, nr); //read the raw data from the sd card

		//If the block has not been used yet
		if(getHeader().pi != 123456789)
		{
			printf("Block %d has not been used yet\n", nr);
			header newHeader;
			newHeader.pi = 123456789;
			newHeader.numKVPairs = 0;
			setHeader(newHeader);
		}
		else
			printf("Block %d contains %d elements\n", nr, getHeader().numKVPairs);
	}

	header getHeader()
	{
		return read<Os, Os::block_data_t, header>(rawData);
	}

	void setHeader(header& h)
	{
		write<Os, Os::block_data_t, header>(rawData, h);
	}

	valueType getValueByID(int id)
	{
		return getKVPairByID(id).value;
	}

	valueType getValueByKey(keyType key)
	{
		for(int i = 0; i < getNumValues(); i++)
		{
			keyValuePair kvPair = getKVPairByID(i);
			if(kvPair.key == key)
				return kvPair.value;
		}
		printf("Could not get the value for key %d becaus it is not in this block\n", key);
	}

	bool containsKey(keyType key)
	{
		for(int i = 0; i < getNumValues(); i++)
		{
			if(getKVPairByID(i).key == key) return true;
		}
		return false;
	}

	bool containsID(int id)
	{
		return id < getNumValues() && id >= 0;
	}

	bool insertValue(keyType key, valueType& value)
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
			printf("we inserted key %d into block %d\n", key, blockNr);
			return true;
		}
		else
		{
			printf("Could not insert value with key %d becuase the block is full\n", key);
			return false;
		}
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
		return sd->write(rawData, blockNr) == Os::SUCCESS;
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
			printf("reading id %d from pos %d\n", id, computePairOffset(id));
			return read<Os, Os::block_data_t, keyValuePair>(rawData + computePairOffset(id));
		}
		else
			printf("tried to acess id %d which does not exist!\n", id);
	}

	int computePairOffset(int id)
	{
		return sizeof(header) + id * sizeof(keyValuePair);
	}
};

}// NS WISELIB

#endif /* BLOCK_H_ */

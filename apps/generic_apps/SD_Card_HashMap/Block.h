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
#include "ReturnTypes.h"

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
		Os::size_t nextBlock;
		Os::size_t prevBlock;
	} header;

	typedef struct
	{
		KeyType key;
		ValueType value;
	} keyValuePair;

	Block(Os::size_t nr, Os::BlockMemory::self_pointer_t sd)
	{
		this->blockNr = nr;
		this->sd = sd;

		initFromSD();
	}

	void initFromSD()
	{
		IOStopwatch.startMeasurement();
		sd->read(rawData, blockNr); //read the raw data from the sd card
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
			head.nextBlock = this->blockNr;
			head.prevBlock = this->blockNr;
		}
#ifdef DEBUG
		else
			printf("Block %d contains %d elements\n", nr, head.numKVPairs);
#endif
	}

	Os::size_t getNextBlock()
	{
		return head.nextBlock;
	}

	Os::size_t getPrevBlock()
	{
		return head.prevBlock;
	}

	void setNextBlock(Os::size_t nextBlock)
	{
		head.nextBlock = nextBlock;
	}

	void setPrevBlock(Os::size_t prevBlock)
	{
		head.prevBlock = prevBlock;
	}

	bool hasNextBlock()
	{
		return head.nextBlock != blockNr;
	}

	bool hasPrevBlock()
	{
		return head.prevBlock != blockNr;
	}

	bool isEmpty()
	{
		return getNumValues() == 0;
	}

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

	returnTypes insertValue(KeyType key, ValueType& value)
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
			return OK;
		}
		else
		{
#ifdef DEBUG
			printf("Could not insert value with key %d becuase the block is full\n", key);
#endif
			return BLOCK_FULL;
		}
	}

	returnTypes removeValue(KeyType key) //TODO: not tested yet
	{
		int valueID = getIDForKey(key);
		if(valueID == -1)
			return NO_VALUE_FOR_THAT_KEY;

		//we just put the last element in place of the current element.
		moveKVPair(head.numKVPairs -1, valueID);
		head.numKVPairs--;
		return OK;
	}

	int getNumValues()
	{
		return head.numKVPairs;
	}

	int maxNumValues()
	{
		return (blocksize - sizeof(header)) / sizeof(keyValuePair);
	}

	returnTypes writeBack()
	{
		write<Os, Os::block_data_t, header>(rawData, head);
		IOStopwatch.startMeasurement();
		bool s = sd->write(rawData, blockNr) == Os::SUCCESS;
		IOStopwatch.stopMeasurement();
		if(s)
			return OK;
		else
			return SD_ERROR;
	}

	void append(Block* nextBlock)
	{
		nextBlock->setPrevBlock(this->blockNr);
		this->setNextBlock(nextBlock->blockNr);
	}

	returnTypes removeFromChain()
	{
		if(!hasPrevBlock()) //special case if it is the first block
		{
			Block<KeyType, ValueType> nextBlock(head.nextBlock, sd);
			nextBlock.setPrevBlock(head.nextBlock); //we set the prev part of the next block to "null"

			head.nextBlock = blockNr;
			head.prevBlock = blockNr;

			if(nextBlock.writeBack() == SD_ERROR)
				return SD_ERROR;
			else
				return OK;
		}

		if(!hasNextBlock()) //special case if it is the last block
		{
			Block<KeyType, ValueType> prevBlock(head.prevBlock, sd);
			prevBlock.setNextBlock(head.prevBlock); //we set the next part of the prev block to "null"

			head.nextBlock = blockNr;
			head.prevBlock = blockNr;

			if(prevBlock.writeBack() == SD_ERROR)
				return SD_ERROR;
			else
				return OK;
		}

		//all the other special cases:

		Block<KeyType, ValueType> prevBlock(head.prevBlock, sd);
		Block<KeyType, ValueType> nextBlock(head.nextBlock, sd);
		prevBlock.append(&nextBlock);
		head.nextBlock = blockNr;
		head.prevBlock = blockNr;
		if(prevBlock.writeBack() == SD_ERROR || nextBlock.writeBack() == SD_ERROR)
			return SD_ERROR;
		else
			return OK;
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


	int16_t getIDForKey(KeyType k) //we use a int16_t instead of uint_8 in case we want to return -1;
	{
		for(int i = 0; i < head.numKVPairs; ++i)
		{
			if(getKVPairByID(i).key == k)
				return i;
		}
		return -1;
	}

	void moveKVPair(uint8_t fromID, uint8_t toID)
	{
		if(fromID != toID)
		{
			keyValuePair p = getKVPairByID(fromID);
			insertKVPairAtID(p, toID);
		}
	}

	void insertKVPairAtID(keyValuePair kvp, uint8_t id)
	{
		write<Os, Os::block_data_t, keyValuePair>(rawData + computePairOffset(id) , kvp);
	}
};

}// NS WISELIB

#endif /* BLOCK_H_ */

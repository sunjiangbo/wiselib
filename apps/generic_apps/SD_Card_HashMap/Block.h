/*
 * Block.h
 *  A block cache for a BlockDevice with advanced IO methods
 *  Created on: Jan 8, 2013
 *      Author: Maximilian Ernestus
 */

#ifndef BLOCK_H_
#define BLOCK_H_

#include <external_interface/external_interface.h>
#include "util/serialization/simple_types.h"
#include <util/meta.h>

#include "BlockIterator.h"
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
		KeyType key;
		ValueType value;
	} keyValuePair;

//	typedef SmallUint<(blocksize - sizeof(header)) / sizeof(keyValuePair)>  index_t; //TODO: how to solve this with template magic? we have circular references there!
	typedef typename SmallUint<(blocksize - (sizeof(Os::size_t) * 2 + sizeof(long) + 8)) / sizeof(keyValuePair)>::t index_t;
	typedef typename SmallUint<blocksize>::t posInBlock_t;
//	typedef uint16_t index_t;

	typedef BlockIterator<Block<KeyType, ValueType, blocksize> > iterator;
	/*
	 * The header to be stored at the beginning of each block
	 */
	typedef struct
	{
		long pi;
		index_t numKVPairs;
		Os::size_t nextBlock;
		Os::size_t prevBlock;
	} header;


	/*
	 * Creates a new block based on the block number and the block memory.
	 * Also loads the block data from the block memory.
	 */
	Block(Os::size_t nr, Os::BlockMemory::self_pointer_t sd)
	: sd(sd), blockNr(nr)
	{
//		(blocksize - sizeof(header)) / sizeof(keyValuePair);

		initFromSD();
	}

	void initFromSD()
	{
#ifdef SPEED_MEASUREMENT
		IOStopwatch.startMeasurement();
#endif
		sd->read(rawData, blockNr); //read the raw data from the sd card
#ifdef SPEED_MEASUREMENT
		IOStopwatch.stopMeasurement();
#endif
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

	iterator end()
	{
		return iterator(this, getNumValues());
	}

	iterator begin()
	{
		return iterator(this, 0);
	}


	int getValueByID(index_t id, ValueType* value)
	{
		if(id < getNumValues() && id >= 0)
		{
			*value = getKVPairByID(id).value;
			return Os::SUCCESS;
		}
		else
			return Os::NO_VALUE;
	}

	/*
	 * Returns the value by ID, not by Key like the hash map. Unspecified behavior if the index is out of range!
	 */
	const ValueType operator[](index_t idx)
	{
		ValueType value;
		getValueByID(idx, &value);
		return value;
	}

	int getValueByKey(KeyType key, ValueType* value)
	{
		for(index_t i = 0; i < getNumValues(); i++)
		{
			keyValuePair kvPair = getKVPairByID(i);
			if(kvPair.key == key)
			{
				*value = kvPair.value;
				return Os::SUCCESS;
			}
		}
		return Os::NO_VALUE;
	}

	bool containsKey(KeyType key)
	{
		return getIDForKey(key) != -1;
	}

	bool containsID(uint8_t id)
	{
		return id < getNumValues() && id >= 0;
	}

	int insertValue(KeyType key, ValueType& value)
	{
		int16_t pos = getIDForKey(key); //define at what position the pair should be inserted
		if(pos == -1) //if the key was not yet inserted into the block we append it to the end and increase the number of elements in the block
		{
			if(getNumValues() < maxNumValues()) //we only insert if the block is not full
			{
				pos = getNumValues();
				++head.numKVPairs;
			}
			else
				return Os::ERR_NOMEM;
		}
		keyValuePair pair;
		pair.key = key;
		pair.value = value;
		insertKVPairAtID(pair, pos);
		return Os::SUCCESS;
	}

	int removeValue(KeyType key)
	{
		index_t valueID = getIDForKey(key);
		if(valueID == -1)
			return Os::NO_VALUE;

		//we just put the last element in place of the current element.
		moveKVPair(head.numKVPairs - 1, valueID);
		head.numKVPairs--;
		return Os::SUCCESS;
	}

	index_t getNumValues()
	{
		return head.numKVPairs;
	}

	int maxNumValues()
	{
		return (blocksize - sizeof(header)) / sizeof(keyValuePair);
	}

	int writeBack()
	{
		write<Os, Os::block_data_t, header>(rawData, head); //Writing the header back to the buffer
#ifdef SPEED_MEASUREMENT
		IOStopwatch.startMeasurement();
#endif
		int s = sd->write(rawData, blockNr);
#ifdef SPEED_MEASUREMENT
		IOStopwatch.stopMeasurement();
#endif
		return s;
	}

	void append(Block* nextBlock)
	{
		nextBlock->setPrevBlock(this->blockNr);
		this->setNextBlock(nextBlock->blockNr);
	}


	int removeFromChain()
	{
		if(!hasPrevBlock() && !hasNextBlock()) //special case if we are not connected anyways not really necessary but it saves some io's
		{
			return Os::SUCCESS;
		}

		if(!hasPrevBlock()) //special case if it is the first block
		{
			Block<KeyType, ValueType> nextBlock(head.nextBlock, sd);
			nextBlock.setPrevBlock(head.nextBlock); //we set the prev part of the next block to "null"

			head.nextBlock = blockNr;
			head.prevBlock = blockNr;

			return nextBlock.writeBack();
		}

		if(!hasNextBlock()) //special case if it is the last block
		{
			Block<KeyType, ValueType> prevBlock(head.prevBlock, sd);
			prevBlock.setNextBlock(head.prevBlock); //we set the next part of the prev block to "null"

			head.nextBlock = blockNr;
			head.prevBlock = blockNr;

			return prevBlock.writeBack();
		}

		//all the other special cases:

		Block<KeyType, ValueType> prevBlock(head.prevBlock, sd);
		Block<KeyType, ValueType> nextBlock(head.nextBlock, sd);
		prevBlock.append(&nextBlock);
		head.nextBlock = blockNr; //TODO: I want a "disconnect from next"/"disconnect from tail" for this
		head.prevBlock = blockNr;
		if(prevBlock.writeBack() == Os::SUCCESS && nextBlock.writeBack() == Os::SUCCESS)
			return Os::SUCCESS;
		else
			return Os::ERR_IO_HATES_YOU;
	}

private:
	Os::block_data_t rawData[blocksize];
	Os::BlockMemory::self_pointer_t sd;
	Os::size_t blockNr;
	header head;


	keyValuePair getKVPairByID(index_t id)
	{
		return read<Os, Os::block_data_t, keyValuePair>(rawData + computePairOffset(id));
	}

	posInBlock_t computePairOffset(index_t id)
	{
		return sizeof(header) + id * sizeof(keyValuePair);
	}

	/**
	 * Returns the id for the given key or -1 if the key does not exist.
	 */
	int16_t getIDForKey(KeyType k) //we use a int16_t instead of uint_8 in case we want to return -1;
	{
		for(index_t i = 0; i < head.numKVPairs; ++i)
		{
			if(getKVPairByID(i).key == k)
				return i;
		}
		return -1;
	}

	void moveKVPair(index_t fromID, index_t toID)
	{
		if(fromID != toID)
		{
			keyValuePair p = getKVPairByID(fromID);
			insertKVPairAtID(p, toID);
		}
	}

	void insertKVPairAtID(keyValuePair kvp, index_t id)
	{
		write<Os, Os::block_data_t, keyValuePair>(rawData + computePairOffset(id) , kvp);
	}
};

}// NS WISELIB

#endif /* BLOCK_H_ */

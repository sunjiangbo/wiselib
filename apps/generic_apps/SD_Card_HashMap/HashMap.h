/*
 * HashMap.h
 *
 *  Created on: Nov 29, 2012
 *      Author: Maximilian Ernestus
 */

#ifndef HASHMAP_H_
#define HASHMAP_H_

#include <external_interface/external_interface.h>
#include <util/serialization/serialization.h>
#include <algorithms/hash/fnv.h>
#include "Block.h"
#include "ReturnTypes.h"

namespace wiselib {


typedef OSMODEL Os;

template<typename KeyType_P, typename ValueType_P>
class HashMap {

public:
	typedef KeyType_P KeyType;
	typedef ValueType_P ValueType;

	typedef Fnv32<Os>::hash_t hash;
	typedef Fnv32<Os>::block_data_t block_data;


	HashMap(Os::Debug::self_pointer_t debug_, Os::BlockMemory::self_pointer_t sd, int fromBlock = 0, int toBlock = 100)
	: insertedElements(0), fromBlock(fromBlock), toBlock(toBlock), lastNewBlock(0), firstBlock(0), currentState(INTACT)
	{
		this->debug_ = debug_;
		this->sd = sd;
		sd->init();
	}

	returnTypes putEntry(KeyType key, ValueType& value)
	{
		size_t blockNr = computeHash(key);
		Block<KeyType, ValueType> block(blockNr, sd);
		if(block.insertValue(key, value) == BLOCK_FULL) return HASHMAP_FULL;

		if(insertedElements == 0)
		{
			firstBlock = blockNr;
		}
		else
		{
			Block<KeyType, ValueType> headBlock(lastNewBlock, sd);
			headBlock.append(&block);
			if(headBlock.writeBack() == SD_ERROR) currentState = BROKEN;

			lastNewBlock = blockNr;
		}

		returnTypes writingToSd = block.writeBack();
		if(writingToSd == OK)
			insertedElements++;
		else
			currentState = BROKEN;
		return writingToSd;
	}

	ValueType getEntry(KeyType key)
	{
		Block<KeyType, ValueType> block(computeHash(key), sd);
		return block.getValueByKey(key);
	}

	returnTypes removeEntry(KeyType key)
	{
		Block<KeyType, ValueType> block(computeHash(key), sd);
		if(block.removeValue(key) == NO_VALUE_FOR_THAT_KEY) return NO_VALUE_FOR_THAT_KEY;

		if(block.isEmpty())
		{
			Block<KeyType, ValueType> prevBlock(block.getPrevBlock(), sd);
			Block<KeyType, ValueType> nextBlock(block.getNextBlock(), sd);
			block.disconnect();
			prevBlock.append(nextBlock);
			if(prevBlock.writeBack() != OK) currentState = BROKEN;
			if(nextBlock.writeBack() != OK) currentState = BROKEN;
		}

		returnTypes writingToSd = block.writeBack();
		if(writingToSd == OK)
			insertedElements--;
		else
			currentState = BROKEN;
		return writingToSd;
	}

	const ValueType operator[](KeyType idx)
	{
		return getEntry(idx);
	}

	bool containsKey(KeyType key)
	{
		Block<KeyType, ValueType> block(computeHash(key), sd);
		return block.containsKey(key);
	}

	float getLoadFactor()
	{
		Block<KeyType, ValueType> block(computeHash(0), sd);
		int valuesPerBlock = block.maxNumValues();
		unsigned long int maxElements = valuesPerBlock * (toBlock - fromBlock);
		return ((float)insertedElements)/maxElements;
	}

private:
	Os::size_t computeHash(KeyType key)
	{
		//return (key % (toBlock - fromBlock)) + fromBlock;
		return (Fnv32<Os>::hash((const block_data*) &key, sizeof(key)) % (toBlock - fromBlock)) + fromBlock;
	}

	Os::BlockMemory::self_pointer_t sd;
	Os::Debug::self_pointer_t debug_;

	//better data types here!!!
	int insertedElements;
	int fromBlock, toBlock;

	size_t lastNewBlock;
	size_t firstBlock;

	enum hashMapState{INTACT, BROKEN};

	hashMapState currentState;
};

} //NS wiselib

#endif /* HASHMAP_H_ */

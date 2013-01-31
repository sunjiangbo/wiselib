/*
 * HashMap.h
 *
 *	Hash Map implementation based on a block device. Also supports iterators.
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
	typedef size_t (*hashFunction)(KeyType);


	/*
	 * Creates a new hashmap that operates on a given block memory, between given block numbers and uses a given hashFunction.
	 *
	 */
	HashMap(Os::Debug::self_pointer_t debug_, Os::BlockMemory::self_pointer_t sd, hashFunction hash, int fromBlock = 0, int toBlock = 100)
	: hashFunc(hash), insertedElements(0), fromBlock(fromBlock), toBlock(toBlock), lastNewBlock(0), firstBlock(0), currentState(INTACT)
	{
		this->debug_ = debug_;
		this->sd = sd;
		sd->init();
	}

	/*
	 * Inserts a new key value pair into the hashmap.
	 * Returns SD_ERROR if there was an IO problem and HASHMAP_FULL when there is no space left in the hashmap.
	 */
	returnTypes putEntry(KeyType key, ValueType& value)
	{
		size_t blockNr = computeHash(key);
		Block<KeyType, ValueType> block(blockNr, sd);
		if(block.insertValue(key, value) == BLOCK_FULL) return HASHMAP_FULL;

		if(insertedElements == 0) //it was the first block that we ever inserted
		{
			firstBlock = blockNr;
			lastNewBlock = blockNr;
		}
		else
		{
			if(block.getNumValues() == 1) //we just used an empty block for the first time
			{
				Block<KeyType, ValueType> headBlock(lastNewBlock, sd);
				headBlock.append(&block);
				if(headBlock.writeBack() == SD_ERROR) currentState = BROKEN;
				lastNewBlock = blockNr;
			}
		}

		returnTypes writingToSd = block.writeBack();
		if(writingToSd == OK)
			insertedElements++;
		else
			currentState = BROKEN;
		return writingToSd;
	}

	/*
	 * Retrives an entry from the hashmap based on a given key.
	 * Only keys that are already in the hashmap might be retrieved, otherwise the behavior is undefined.
	 */
	ValueType getEntry(KeyType key)
	{
		Block<KeyType, ValueType> block(computeHash(key), sd);
		return block.getValueByKey(key);
	}

	/*
	 * Removes an entry from the hashmap based on a given key.
	 * Returns NO_VALUE_FOR_THAT_KEY if the hashmap did not contain the key.
	 * Return SD_ERROR if there was an IO error.
	 */
	returnTypes removeEntry(KeyType key)
	{
		Block<KeyType, ValueType> block(computeHash(key), sd);
		if(block.removeValue(key) == NO_VALUE_FOR_THAT_KEY) return NO_VALUE_FOR_THAT_KEY;

		if(block.isEmpty())
		{
			if(block.removeFromChain() == SD_ERROR) currentState = BROKEN;
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

	/*
	 * Checks if a given key exists in the hashmap.
	 */
	bool containsKey(KeyType key)
	{
		Block<KeyType, ValueType> block(computeHash(key), sd);
		return block.containsKey(key);
	}

	/*
	 * Computes the load factor of the hashmap. The overhead caused by storing the keys and the block headers is ignored!
	 */
	float getLoadFactor()
	{
		Block<KeyType, ValueType> block(computeHash(0), sd);
		int valuesPerBlock = block.maxNumValues();
		unsigned long int maxElements = valuesPerBlock * (toBlock - fromBlock);
		return ((float)insertedElements)/maxElements;
	}

	/*
	 * Returns the address of the first block that was used.
	 * Use this method to create your iterator!
	 */
	size_t getFirstUsedBlock()
	{
		return firstBlock;
	}

private:
	Os::size_t computeHash(KeyType key)
	{
		return (hashFunc(key) % (toBlock - fromBlock)) + fromBlock;
		//return (key % (toBlock - fromBlock)) + fromBlock;
		//return (Fnv32<Os>::hash((const block_data*) &key, sizeof(key)) % (toBlock - fromBlock)) + fromBlock;
	}

	hashFunction hashFunc;

	Os::BlockMemory::self_pointer_t sd;
	Os::Debug::self_pointer_t debug_;

	//TODO: better data types here!!!
	int insertedElements;
	size_t fromBlock, toBlock;

	size_t lastNewBlock;
	size_t firstBlock;

	//TODO: somehow react to a broken state
	enum hashMapState{INTACT, BROKEN};
	hashMapState currentState;
};

} //NS wiselib

#endif /* HASHMAP_H_ */

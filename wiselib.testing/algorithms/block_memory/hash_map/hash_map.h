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

#include <algorithms/block_memory/hash_map/hash_map_block.h>
#include <algorithms/block_memory/hash_map/hash_map_iterator.h>

namespace wiselib {


typedef OSMODEL Os;

template<typename KeyType_P, typename ValueType_P>
class HashMap {

public:
	typedef KeyType_P KeyType;
	typedef ValueType_P ValueType;

	typedef Os::size_t size_t;
	typedef Fnv32<Os>::hash_t hash;
	typedef Fnv32<Os>::block_data_t block_data;
	typedef size_t (*hashFunction)(KeyType);
	typedef HashMapIterator<HashMap<KeyType, ValueType> > iterator;


	/*
	 * Creates a new hashmap that operates on a given block memory,
	 * between given block numbers and uses a given hashFunction.
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
	 * @return:
	 * 	Os::ERR_NOMEM if the hash map is full
	 * 	The block interfaces write error code otherwise
	 */
	int putEntry(KeyType key, ValueType& value)
	{
		size_t blockNr = computeHash(key);
		Block<KeyType, ValueType> block(blockNr, sd);
		if(block.insertValue(key, value) == Os::ERR_NOMEM) return Os::ERR_NOMEM;

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
				if(headBlock.writeBack() == Os::ERR_UNSPEC) currentState = BROKEN;
				lastNewBlock = blockNr;
			}
		}

		int writingToSd = block.writeBack();
		if(writingToSd == Os::SUCCESS)
			insertedElements++;
		else
			currentState = BROKEN;
		return writingToSd;
	}

	/*
	 * Retrieves an entry from the hash map based on a given key
	 * @return:
	 * 	Os::SUCCESS if everything went ok,
	 *  Os::NO_VALUE if no value could be found for the given key
	 */
	int getEntry(KeyType key, ValueType* value)
	{
		Block<KeyType, ValueType> block(computeHash(key), sd);
		return block.getValueByKey(key, value);
	}

	/*
	 * Removes an entry from the hashmap based on a given key.
	 * @return:
	 * 	OS::NO_VALUE if the key was not in the hash map.
	 * 	The writing error code of the block memory otherwise
	 */
	int removeEntry(KeyType key)
	{
		Block<KeyType, ValueType> block(computeHash(key), sd);
		if(block.removeValue(key) == Os::NO_VALUE) return Os::NO_VALUE;

		if(block.isEmpty())
		{
			if(block.removeFromChain() == Os::ERR_UNSPEC) currentState = BROKEN;
		}

		int writingToSd = block.writeBack();
		if(writingToSd == Os::SUCCESS)
			insertedElements--;
		else
			currentState = BROKEN;
		return writingToSd;
	}

	/*
	 * Retrives an entry from the hashmap based on a given key.
	 * Only keys that are already in the hashmap might be retrieved,
	 * otherwise the return value will be empty.
	 */
	const ValueType operator[](KeyType key)
	{
		ValueType value;
		getEntry(key, &value);
		return value;
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
		unsigned long int maxElements = valuesPerBlock * (toBlock - fromBlock - 1);
		return ((float)insertedElements)/maxElements;
	}

	/*
	 * For the iterator concept
	 */

	iterator begin()
	{
		return iterator(firstBlock, sd);
	}

	int end()
	{
		return 42;
	}

	int writeMetadata()
	{
		Os::block_data_t buffer[sd->BLOCK_SIZE];
		int pos = 0;
		pos += write<Os, Os::block_data_t, int>(buffer + pos, insertedElements);
		pos += write<Os, Os::block_data_t, size_t>(buffer + pos, lastNewBlock);
		pos += write<Os, Os::block_data_t, size_t>(buffer + pos, firstBlock);
		pos += write<Os, Os::block_data_t, hashMapState>(buffer + pos, currentState);
		return sd->write(buffer, fromBlock);
	}

	int readMetadata()
	{
		Os::block_data_t buffer[sd->BLOCK_SIZE];
		int s = sd->read(buffer, fromBlock);
		if(s == Os::SUCCESS)
		{
			int pos = 0;
			read<Os, Os::block_data_t, int>(buffer + pos, insertedElements);
			pos += sizeof(int);
			read<Os, Os::block_data_t, size_t>(buffer + pos, lastNewBlock);
			pos += sizeof(size_t);
			read<Os, Os::block_data_t, size_t>(buffer + pos, firstBlock);
			pos += sizeof(size_t);
			read<Os, Os::block_data_t, hashMapState>(buffer + pos, currentState);
			pos += sizeof(hashMapState);
		}
		return s;
	}

private:
	Os::size_t computeHash(KeyType key)
	{
		return (hashFunc(key) % (toBlock - (fromBlock + 1))) + fromBlock + 1; //The first block is for meta data
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
	/*
	 * Keeps track of the linkage state of the blocks.
	 * If something messed up while linking together the blocks the state will go to BROKEN.
	 * As of know there is no way to recover from this state and nothing will happen if the
	 * hash map is broken. Only the iterator breaks anyways so who cars?
	 */
	enum hashMapState{INTACT, BROKEN};
	hashMapState currentState;
};

} //NS wiselib

#endif /* HASHMAP_H_ */

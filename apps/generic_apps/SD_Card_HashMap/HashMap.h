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

#define FULLBLOCK 42
#define BLOCKSIZE 512



namespace wiselib {


typedef OSMODEL Os;

template<typename KeyType_P, typename ValueType_P, int fromBlock = 0, int toBlock = 100>
class HashMap {

public:
	typedef KeyType_P KeyType;
	typedef ValueType_P ValueType;

	typedef Fnv32<Os>::hash_t hash;
	typedef Fnv32<Os>::block_data_t block_data;


	HashMap(Os::Debug::self_pointer_t debug_, Os::BlockMemory::self_pointer_t sd) : insertedElements(0)
	{
		this->debug_ = debug_;
		this->sd = sd;
		sd->init();
	}

	bool putEntry(KeyType key, ValueType& value)
	{
		Block<KeyType, ValueType> block(computeHash(key), sd);
		bool insertSuccess = block.insertValue(key, value);
		if(insertSuccess)
		{
			insertedElements++;
			return block.writeBack();
		}
		else
			return false;
	}

	ValueType getEntry(KeyType key)
	{
		Block<KeyType, ValueType> block(computeHash(key), sd);
		return block.getValueByKey(key);
	}

	bool removeEntry(KeyType key)
	{
		Block<KeyType, ValueType> block(computeHash(key), sd);
		bool sucess = block.removeValue(key);
		if(sucess)
			block.writeBack();
		return sucess;
	}

	const ValueType& operator[](KeyType idx)
	{
		return getEntry(idx);
	}

	float getLoadFactor()
	{
		Block<KeyType, ValueType> block(computeHash(0), sd);
		int valuesPerBlock = block.maxNumValues();
		unsigned long int maxElements = valuesPerBlock * (toBlock - fromBlock);
		return ((float)insertedElements)/maxElements;
	}

private:
	hash computeHash(KeyType key)
	{
		return (Fnv32<Os>::hash((const block_data*) &key, sizeof(key)) % (toBlock - fromBlock)) + fromBlock;
	}

	Os::BlockMemory::self_pointer_t sd;
	Os::Debug::self_pointer_t debug_;

	unsigned long int insertedElements;
};

} //NS wiselib

#endif /* HASHMAP_H_ */

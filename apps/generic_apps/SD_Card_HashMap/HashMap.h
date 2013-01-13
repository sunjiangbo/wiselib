/*
 * HashMap.h
 *
 *  Created on: Nov 29, 2012
 *      Author: maximilian
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
typedef Fnv32<Os>::hash_t hash;
typedef Fnv32<Os>::block_data_t block_data;

template<typename keyType, typename valueType>
class HashMap {


public:
	HashMap(Os::Debug::self_pointer_t debug_, Os::BlockMemory::self_pointer_t sd)
	{
		this->debug_ = debug_;
		this->sd = sd;
		sd->init();
	}

	bool putEntry(keyType key, valueType& value)
	{
		Block<keyType, valueType> block(computeHash(key), sd);
		bool insertSuccess = block.insertValue(key, value);
		if(insertSuccess)
			return block.writeBack();
		else
			return false;
	}

	valueType getEntry(keyType key)
	{
		Block<keyType, valueType> block(computeHash(key), sd);
		return block.getValueByKey(key);
	}

private:
	hash computeHash(keyType key)
	{
		return Fnv32<Os>::hash((const block_data*) &key, sizeof(key));
	}

	Os::BlockMemory::self_pointer_t sd;
	Os::Debug::self_pointer_t debug_;
};

} //NS wiselib

#endif /* HASHMAP_H_ */

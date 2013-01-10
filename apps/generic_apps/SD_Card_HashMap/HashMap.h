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
#include "Block.h"

#define FULLBLOCK 42
#define BLOCKSIZE 512



namespace wiselib {


typedef OSMODEL Os;

template<typename valueType>
class HashMap {

public:
	HashMap(Os::Debug::self_pointer_t debug_, Os::BlockMemory::self_pointer_t sd)
	{
		this->debug_ = debug_;
		this->sd = sd;
		sd->init();
	}

	bool putEntry(int key, valueType& value)
	{
		Block<valueType> block(hash(key), sd);
		bool insertSuccess = block.insertValue(key, value);
		if(insertSuccess)
			return block.writeBack();
		else
			return false;
	}

	valueType getEntry(int key)
	{
		Block<valueType> block(hash(key), sd);
		return block.getValueByKey(key);
	}

private:
	int hash(int key)
	{
		return key; //for testing only!!!
	}

	Os::BlockMemory::self_pointer_t sd;
	Os::Debug::self_pointer_t debug_;
};

} //NS wiselib

#endif /* HASHMAP_H_ */

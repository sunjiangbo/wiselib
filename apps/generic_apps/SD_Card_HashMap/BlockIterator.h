/*
 * BlockIterator.h
 *
 *  Created on: Jan 27, 2013
 *      Author: maximilian
 */

#ifndef BLOCKITERATOR_H_
#define BLOCKITERATOR_H_

#include "Block.h"

namespace wiselib {


template<typename KeyType_P, typename ValueType_P>
class BlockIterator
{
	typedef KeyType_P KeyType;
	typedef ValueType_P ValueType;
public:

	BlockIterator(Block<KeyType, ValueType>* block) :  currentPos(0), block(block)
	{

	}

	ValueType operator*() const
	{
		return block->getValueByID(currentPos);
	}

	BlockIterator<KeyType, ValueType>& operator++()
	{
		++currentPos;
		return *this;
	}

	bool hasNext()
	{
		return currentPos < block->getNumValues() -1;
	}


private:
	uint8_t currentPos;
	Block<KeyType, ValueType>* block;
};

}  // namespace wiselib

#endif /* BLOCKITERATOR_H_ */

/*
 * HashMapIterator.h
 *
 *  Created on: Jan 27, 2013
 *      Author: maximilian
 */

#ifndef HASHMAPITERATOR_H_
#define HASHMAPITERATOR_H_


namespace wiselib {

template<typename KeyType_P, typename ValueType_P>
class HashMapIterator
{
	typedef KeyType_P KeyType;
	typedef ValueType_P ValueType;
public:

	HashMapIterator(size_t beginBlock, Os::BlockMemory::self_pointer_t sd)
	: sd(sd), currentBlock(Block<KeyType, ValueType>(beginBlock, sd)), blockIterator(BlockIterator<KeyType, ValueType>(&currentBlock)), dead(false)
	{

	}

	ValueType operator*() const
	{
		return *blockIterator;
	}

	HashMapIterator<KeyType, ValueType>& operator++()
	{
		++blockIterator;
		if(blockIterator.reachedEnd())
		{
			if(currentBlock.hasNextBlock()) //we reached the end of the block
			{
				currentBlock = Block<KeyType, ValueType>(currentBlock.getNextBlock(), sd);
				blockIterator = BlockIterator<KeyType, ValueType>(&currentBlock);
			}
			else //we reached the end of the hashmap
			{
				this->dead = true;
			}
		}
		return *this;
	}

	bool reachedEnd()
	{
		return this->dead;
	}

private:
	Os::BlockMemory::self_pointer_t sd;
	Block<KeyType, ValueType> currentBlock;
	BlockIterator<KeyType, ValueType> blockIterator;
	bool dead;
};

} //NS wiselib
#endif /* HASHMAPITERATOR_H_ */

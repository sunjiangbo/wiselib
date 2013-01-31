/*
 * HashMapIterator.h
 *  This class can be used to iterate over the block that were filled by a hashmap.
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

	/*
	 * Creates a new iterator starting at the beginBlock.
	 * Because all metadata is stored within the blocks no instance of the hashmap is required.
	 */
	HashMapIterator(size_t beginBlock, Os::BlockMemory::self_pointer_t sd)
	: sd(sd), currentBlock(Block<KeyType, ValueType>(beginBlock, sd)), blockIterator(BlockIterator<KeyType, ValueType>(&currentBlock)), dead(false)
	{

	}

	ValueType operator*() const
	{
		return *blockIterator; //we just forward the hard work to the underlying BlockIterator
	}

	HashMapIterator<KeyType, ValueType>& operator++()
	{
		++blockIterator;
		if(blockIterator.reachedEnd())//we reached the end of the block
		{
			if(currentBlock.hasNextBlock()) //load the next block if there is one
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

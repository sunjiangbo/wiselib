/*
 * HashMapIterator.h
 *  This class can be used to iterate over the block that were filled by a hash map.
 *  Created on: Jan 27, 2013
 *      Author: maximilian
 */

#ifndef HASHMAPITERATOR_H_
#define HASHMAPITERATOR_H_

#include <algorithms/block_memory/hash_map/hash_map_block.h>

namespace wiselib {

template<typename HashmapType_P>
class HashMapIterator
{
	typedef HashmapType_P HashMapType;
	typedef typename HashMapType::KeyType KeyType;
	typedef typename HashMapType::ValueType ValueType;
	typedef Block<KeyType, ValueType> BlockType;

public:

	/*
	 * Creates a new iterator starting at the beginBlock.
	 * Because all metadata is stored within the blocks no instance of the hashmap is required.
	 */
	HashMapIterator(size_t beginBlock, Os::BlockMemory::self_pointer_t sd)
	: sd(sd),
	  currentBlock(BlockType(beginBlock, sd)),
	  blockIterator(currentBlock.begin()),
	  dead(false)
	{
	}

	ValueType operator*() const
	{
		return *blockIterator; //we just forward the hard work to the underlying BlockIterator
	}

	HashMapIterator<HashMapType>& operator++()
	{
		if(dead)
		{
			return *this;
		}

		++blockIterator;
		if(blockIterator == currentBlock.end())//we reached the end of the block
		{
			if(currentBlock.hasNextBlock()) //load the next block if there is one
			{
				currentBlock = BlockType(currentBlock.getNextBlock(), sd);
				blockIterator = currentBlock.begin();
			}
			else //we reached the end of the hashmap
			{
				this->dead = true;
			}
		}

		return *this;
	}

	bool operator==(const HashMapIterator<HashMapType>& o) const
	{
		return o.blockIterator == blockIterator;
	}

	bool operator==(const int& o) const
	{
		return dead;
	}

	bool operator!=(const HashMapIterator<HashMapType>& o) const
	{
		return !(this == o);
	}

	bool operator!=(const int& o) const
	{
		return !dead;
	}

private:
	Os::BlockMemory::self_pointer_t sd;
	BlockType currentBlock;
	typename BlockType::iterator blockIterator;
	bool dead;
};

} //NS wiselib
#endif /* HASHMAPITERATOR_H_ */

/*
 * BlockIterator.h
 * An iterator for the block class.
 *  Created on: Jan 27, 2013
 *      Author: maximilian
 */

#ifndef BLOCKITERATOR_H_
#define BLOCKITERATOR_H_

namespace wiselib {

template<typename KeyType_P, typename ValueType_P, int blocksize>
class Block;

template<typename BlockType_P>
class BlockIterator
{
	typedef BlockType_P BlockType;
	typedef typename BlockType::KeyType KeyType;
	typedef typename BlockType::ValueType ValueType;
	typedef typename BlockType::index_t index_t;

public:

	/*
	 * Creates a new Block iterator for a given block at a given position.
	 */
	BlockIterator(BlockType* block, index_t position) :  currentPos(position), block(block)
	{
	}

	ValueType operator*() const
	{
		return (*block)[currentPos];
	}

	BlockIterator<BlockType>& operator++()
	{
		++currentPos;
		return *this;
	}

	/*
	 * Checks for the same position and the same block.
	 */
	bool operator==(const BlockIterator<BlockType>& o) const
	{
		return o.currentPos == currentPos && o.block == block;
	}

	bool operator!=(const BlockIterator<BlockType>& o) const
	{
		return !(o.currentPos == currentPos && o.block == block);
	}


private:
	index_t currentPos;
	BlockType* block;
};

}  // namespace wiselib

#endif /* BLOCKITERATOR_H_ */

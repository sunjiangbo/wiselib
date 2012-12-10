/*
 * HashMap.cpp
 *
 *  Created on: Nov 29, 2012
 *      Author: maximilian
 */

#include "HashMap.h"

namespace wiselib {

HashMap::HashMap(Os::Debug::self_pointer_t debug_)
{
	this->debug_ = debug_;
	sd.init();
}

bool HashMap::putEntry(int key, char value[])
{
	if(strlen(value) > BLOCKSIZE - (sizeof(size_t) + sizeof(Os::block_data_t)))
		return false;

	debug_->debug("Size of the value does not exceed the block size!");
	int block = hash(key);
	/*if(isBlockFull(block))
	{
		debug_->debug("Sorry, the block is already occupied");
		return false;
	}*/


	debug_->debug("checked if block is occupied: it is not");
	Os::block_data_t writingBuffer[BLOCKSIZE];

	header head;
	head.isUsed = 42;
	head.length = strlen(value);
	writeHeader(&head, writingBuffer);
	writeString(value, writingBuffer, sizeof(header));
	int r = sd.write(writingBuffer, block, 1);
	if(r != Os::SUCCESS)
		debug_->debug("there was an error while writing on the sd card!");
	return r == Os::SUCCESS;
}

header HashMap::readHeader(Os::block_data_t buffer[])
{
	header h;
	h.isUsed = read<Os, Os::block_data_t, unsigned char>( buffer );
	h.length = read<Os, Os::block_data_t, size_t>( buffer + sizeof(unsigned char));
	debug_->debug("header.isUsed: " + h.isUsed);
	debug_->debug("header.length: " + h.length);
	return h;
}

void HashMap::writeHeader(header* h, Os::block_data_t buffer[])
{
	write<Os, Os::block_data_t, unsigned char>(buffer, h->isUsed);
	write<Os, Os::block_data_t, size_t>(buffer + sizeof(unsigned char), h->length);
}

void HashMap::writeString(char str[], Os::block_data_t buffer[], int offset)
{
	memcpy(buffer + offset, str, strlen(str));
}

void HashMap::readString(char str[], Os::block_data_t buffer[], int offset)
{
	memcpy(str, buffer + offset, sizeof(str));
}

bool HashMap::getEntry(int key, char buffer[])
{
	Os::block_data_t readingBuffer[BLOCKSIZE];
	if(sd.read(readingBuffer, hash(key), 1) != Os::SUCCESS)
	{
		debug_->debug("There was a failure reading the sd card");
		return false;
	}

	header head = readHeader(readingBuffer);


	if(head.length > strlen(buffer))
	{
		debug_->debug("the buffer for the value is too small");
		return false;
	}

	readString(buffer, readingBuffer, sizeof(header));

	return true;
}

bool HashMap::isBlockFull(int block)
{
	Os::block_data_t readingBuffer[BLOCKSIZE];
	sd.read(readingBuffer, block, 1);

	return readingBuffer[0] == 42;
}

int HashMap::hash(int key)
{
	return key; //for testing only!!!
}

} //namespace wiselib

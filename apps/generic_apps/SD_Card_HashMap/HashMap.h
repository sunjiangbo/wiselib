/*
 * HashMap.h
 *
 *  Created on: Nov 29, 2012
 *      Author: maximilian
 */

#ifndef HASHMAP_H_
#define HASHMAP_H_

#include <external_interface/external_interface.h>
#include <external_interface/arduino/arduino_sdcard.h>
#include <external_interface/arduino/arduino_debug.h>
#include <util/serialization/serialization.h>

#define FULLBLOCK 42
#define BLOCKSIZE 512



namespace wiselib {
typedef struct
{
	unsigned char isUsed;
	size_t length;
} header;

typedef wiselib::OSMODEL Os;

class HashMap {
public:
	HashMap(Os::Debug::self_pointer_t debug_);
	bool putEntry(int key, char value[]);
	bool getEntry(int key, char value[]);
	bool isBlockFull(int block);
private:
	ArduinoSdCard<Os> sd;
	header readHeader(Os::block_data_t buffer[]);
	void writeHeader(header* h, Os::block_data_t buffer[]);
	void writeString(char str[], Os::block_data_t buffer[], int offset);
	void readString(char str[], Os::block_data_t buffer[], int offset);
	int hash(int key);
	Os::Debug::self_pointer_t debug_;
};

} //namespace wiselib

#endif /* HASHMAP_H_ */

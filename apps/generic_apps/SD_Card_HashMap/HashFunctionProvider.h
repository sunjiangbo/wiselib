/*
 * HashFunctionProvider.h
 *
 *  Created on: Jan 30, 2013
 *      Author: maximilian
 */

#ifndef HASHFUNCTIONPROVIDER_H_
#define HASHFUNCTIONPROVIDER_H_

#include <algorithms/hash/fnv.h>

namespace wiselib
{
template<
		typename OsModel_P,
		typename KeyType_P
	>
	class HashFunctionProvider
	{
	public:
		typedef OsModel_P OsModel;
		typedef KeyType_P KeyType;
//		typedef typename OsModel::size_t size_t;
		typedef Fnv32<Os>::block_data_t block_data;

		typedef size_t (*hashFunction)(KeyType);
		//typedef Fnv32<Os>::block_data_t block_data;

		static hashFunction getHashFunction(int id)
		{
			switch(id)
			{
			case 0:
				return &identity; //simple identity function
			case 1:
				return &fnv; //fnv32
			case 2:
				return &sfh; // super fast hash
			case 3:
				return &additive; //additive hash
			case 4:
				return &rotating; //rotating hash
			}
		}


		static size_t identity(KeyType key)
		{
			return size_t(key);
		}

		static size_t fnv(KeyType key)
		{
			return size_t(Fnv32<Os>::hash((const block_data*) &key, sizeof(key)));
		}

		static size_t sfh(KeyType key)
		{
			return SuperFastHash((const char *) &key, sizeof(key));
		}

		#define get16bits(d) (*((const uint16_t *) (d)))
		static size_t SuperFastHash (const char * data, int len) {
		size_t hash = len, tmp;
		int rem;

		    if (len <= 0 || data == NULL) return 0;

		    rem = len & 3;
		    len >>= 2;

		    /* Main loop */
		    for (;len > 0; len--) {
		        hash  += get16bits (data);
		        tmp    = (get16bits (data+2) << 11) ^ hash;
		        hash   = (hash << 16) ^ tmp;
		        data  += 2*sizeof (uint16_t);
		        hash  += hash >> 11;
		    }

		    /* Handle end cases */
		    switch (rem) {
		        case 3: hash += get16bits (data);
		                hash ^= hash << 16;
		                hash ^= ((signed char)data[sizeof (uint16_t)]) << 18;
		                hash += hash >> 11;
		                break;
		        case 2: hash += get16bits (data);
		                hash ^= hash << 11;
		                hash += hash >> 17;
		                break;
		        case 1: hash += (signed char)*data;
		                hash ^= hash << 10;
		                hash += hash >> 1;
		                break;
		    }

		    /* Force "avalanching" of final 127 bits */
		    hash ^= hash << 3;
		    hash += hash >> 5;
		    hash ^= hash << 4;
		    hash += hash >> 17;
		    hash ^= hash << 25;
		    hash += hash >> 6;

		    return hash;
		}

		static size_t additive(KeyType key)
		{
			char *castedKey = (char *) &key;
			int len = sizeof(KeyType);
			size_t magicPrime = 0x1000193UL; //stolen from fnv32
			size_t hash;
			int i;
			for (hash=len, i=0; i<len; ++i)
				hash += castedKey[i];
		  return size_t(hash % magicPrime);
		}

		static size_t rotating(KeyType key)
		{
			size_t hash;
			size_t magicPrime = 0x1000193UL; //stolen from fnv32
			int i = 0;
			int len = sizeof(KeyType);
			char *castedKey = (char *)&key;
			for (hash=len, i=0; i<len; ++i)
				hash = (hash<<4)^(hash>>28)^castedKey[i];
		  return size_t(hash % magicPrime);
		}


	};
} //ns wiselib


#endif /* HASHFUNCTIONPROVIDER_H_ */

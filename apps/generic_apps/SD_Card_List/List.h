/**
THINGS FOR PRESI DO TO
ITERATOR CONCEPT (NEXT PREV; BUFFER)
WISELIB COMPATIBLE
ALLOW CHANGE ON FILL STATE OF BLOCKS FOR MERGE/SPLIT
*/



/* Required FEATURES for list
- handle both small values and big values (directly in block save and pointer to block with value) - pointer version not yet here. its not that much to add once this version works though.
- handle both keyed and unkeyed values
- independend from blocksize
- (allow for blocksize to be unknown before compiling and automatically sort out if simple or complex list type is better)
- allow for PREV and NEXT and well as indexed and keyed access
- allow to be reconstructed from disk (with a SHORT check if there really is a list there. Nothing complex)
*/
#ifndef __memlist_H__
#define	__memlist_H__

#include "../block_device_mmu/block_device_mmu.hpp"

namespace wiselib {

template<typename OsModel_P, typename BDMMU_P, typename KeyType_P, 
	typename ValueType_P, 
	typename OsModel_P::size_t blocksize = 512, bool loadFromDisk = false,
	typename OsModel_P::size_t buffersize = 1,
	typename OsModel_P::size_t mergeVal = 75, //if 2 blocks combined into one would be less full than 75% - merge them when removing
	typename OsModel_P::size_t splitVal = 80, //if 2 blocks are more than 75% full - split them when inserting (each of the 3 wouldbe 50% full)
	typename Debug_P = typename OsModel_P::Debug>
	/*typename BlockMemory_P = typename OsModel_P::BlockMemory*/
	
class List{	
	typedef OsModel_P Os;
	typedef Debug_P Debug;
	typedef BDMMU_P BDMMU;
	
	typedef typename BDMMU::self_pointer_t MMU_ptr;
	typedef typename Debug::self_pointer_t Debug_ptr;
	typedef typename BDMMU::block_data_t block_data_t;

	typedef typename BDMMU::block_address_t block_address_t;
	typedef typename Os::size_t CounterType;

	typedef KeyType_P KeyType;
	typedef ValueType_P ValueType;
 

	public: //TODO: private

		//const CounterType HEAD = 0 //where the head of the list is located. since we get our own virtual memory partition, we can set this to 0.
		
		struct buffer_instance{
			block_data_t data[blocksize];
			block_address_t last_read; //position of last block read
			uint32_t last_id; //index of first element in last block read
		};	
		buffer_instance buffer[buffersize];
		CounterType lastbuffer;
		uint32_t totalCount; //amount of objs in list
		//here come options:
		CounterType key_size;
		bool useKeys;
		CounterType maxElements;

		//Os::Debug::self_pointer_t 

		Debug_ptr debug_;
		MMU_ptr mmu;
		
		//MemoryMap * mmu;
		/* information about HEAD block: int-><0 [current elements in this block - always 0 (for searches)> ptr-><first block> ptr-><last block> int-><current elements in list><max elements per block><use keys><key_size(unreduced)><value_size><blocksize>
		*/
		/* Format of 1 block: int-><amount objects in this block> ptr-><next block> ptr-><previous block> [data] 
		  getting element i in a block is at position 0 + int_size + 2 * ptr_size + i * value_size
		*/

		static const CounterType ptr_size = sizeof(block_address_t);
		static const CounterType cnt_size = sizeof(CounterType);
		static const CounterType value_size = sizeof(ValueType);
		//all
		static const CounterType offsetForward = cnt_size;
		static const CounterType offsetBackward = offsetForward + ptr_size;
		//data only
		static const CounterType offsetData = offsetBackward + ptr_size;
		//head only
		static const CounterType offsetTotalCounter = offsetBackward + ptr_size;
		static const CounterType offsetMax = offsetTotalCounter + cnt_size;
		static const CounterType offsetUseKey = offsetMax + cnt_size;
		static const CounterType offsetKeySize = offsetUseKey + sizeof(bool);
		static const CounterType offsetValueSize = offsetKeySize + cnt_size;
		static const CounterType offsetBlockSize = offsetValueSize + cnt_size;



	void readForward(CounterType bufferPos){

		buffer[bufferPos].last_read = read<Os, block_data_t, block_address_t> (buffer[bufferPos].data + offsetForward);
		buffer[bufferPos].last_id = buffer[bufferPos].last_id + read<Os, block_data_t, uint32_t>(buffer[bufferPos].data);
		mmu->read(buffer[bufferPos].data, buffer[bufferPos].last_read);
		if (buffer[bufferPos].last_read == 0){
			buffer[bufferPos].last_id = 0;
		}
	}

	void readBackward(CounterType bufferPos){
		if (buffer[bufferPos].last_read == 0){
			buffer[bufferPos].last_id = totalCount;
		}
		buffer[bufferPos].last_read = read<Os, block_data_t, block_address_t>(buffer[bufferPos].data + offsetBackward);
		mmu->read(buffer[bufferPos].data, buffer[bufferPos].last_read);
		buffer[bufferPos].last_id = buffer[bufferPos].last_id - read<Os, block_data_t, uint32_t>(buffer[bufferPos].data);
	}

	CounterType calcBlocks(CounterType amount){
		if (amount < (maxElements * mergeVal) / 100){ //it fills 1 block to 75%
			return 1;
		}
		else if (amount < (maxElements * splitVal) / 50){ //it would fill 2 blocks to 75% - or 3 to 50%
			return 2;
		}	
		else {//3 blocks needed 
			return 3;
		}
	}

	void rebalance (CounterType bufferPos){
		//find most suited partner 	
		//our wished for fill state is 2/3 after this op
		//total amount should be closest to a multiple of (maxElements * 2/3). which multiple doesnt matter, just that it is as close as possible.
		if (buffer[bufferPos].last_read == 0) return; //Dont do anything if this is about the HEAD block.

		CounterType data_amount = read<Os, block_data_t, CounterType>(buffer[bufferPos].data);
		block_address_t data2_pos = read<Os, block_data_t, block_address_t>(buffer[bufferPos].data + offsetBackward);
		block_address_t data3_pos = read<Os, block_data_t, block_address_t>(buffer[bufferPos].data + offsetForward);
	
		if (data2_pos == 0 && data3_pos == 0 && calcBlocks(data_amount) == 1){ //special case - last block in the list is getting smaller
			if (data_amount == 0){ //collapse empty block
			}
			return; //nothing (else) to do here
		}

		//partner search:
		block_data_t  data2[blocksize];
		block_data_t  data3[blocksize];
	
		mmu->read(data2, data2_pos);
		CounterType data2_amount = read<Os, block_data_t, CounterType>(data2);
		CounterType blocks  = calcBlocks(data_amount + data2_amount);
		CounterType d2_closeness  = abs(((data_amount + data2_amount)/ blocks) - (maxElements * 2) / 3);
	
		mmu->read(data3, data3_pos);
		CounterType data3_amount = read<Os, block_data_t, CounterType>(data3);
		blocks = calcBlocks(data_amount + data3_amount);
		CounterType d3_closeness = abs(((data_amount + data3_amount)/ blocks) - (maxElements * 2) / 3);

		block_data_t * first;
		block_data_t * second;
		block_data_t * third;

		if (((d2_closeness < d3_closeness) || (data3_pos == 0)) && (data2_pos != 0)){ //use d2 either if its better or if its the only available
			//d2 is BEFORE d
			buffer[bufferPos].last_read = data2_pos;
			debug_->debug("Split - set last_read to %d", data2_pos);	
		
			buffer[bufferPos].last_id = buffer[bufferPos].last_id - data2_amount;
			first = data2;
			second = buffer[bufferPos].data;
			third = data3;
			blocks  = calcBlocks(data_amount + data2_amount);	
		}else if (((d2_closeness >= d3_closeness) || (data2_pos == 0)) && (data3_pos != 0)){ 
			//d3 is BEHIND d
			first = buffer[bufferPos].data;
			second = data3;
			third = data2;
		}
		else {
			//Special case - both sides are HEAD. create a new block first, then reassign (blocks = 2)
			//note - this cannot be called because the block is small as that case is already handled top
			blocks = 2;
			first = buffer[bufferPos].data;
			second = data2;
		//	third = data3; // wont be used
	
		debug_->debug("Split - There is only one Block. Creating new and reassigning.");	
			block_address_t newpos; 
			mmu->block_alloc(&newpos);//request new adress	
	
			write<Os, block_data_t, block_address_t>(first + offsetForward, newpos);//write new adress on this block
			//mmu->write(last_read, data); //will be written later
			mmu->read(data3,0);
			write<Os, block_data_t, block_address_t>(data3 + offsetBackward, newpos); //write new end pointer on HEAD
			mmu->write(data3,0);
		
			//prepare the new block (counter = 0, pre pointer, last pointer)
			for (block_address_t i = 0; i < blocksize; i++){
				second[i] = 0;
			}
			write<Os, block_data_t, block_address_t>(second + offsetBackward, buffer[bufferPos].last_read); //write back pointer
			//forward pointer to HEAD is already 0, no action needed
			//now were ready to go on and treat this block as if it already existed on the card

		debug_->debug("Extra Block %d created. LR= %d LID=%d cnt=%d", newpos, buffer[bufferPos].last_read, buffer[bufferPos].last_id, data_amount);
			}
			//now that we decided who to play with, we decide what to do (depends on blocks)
			if (blocks == 1){ //merge first and second.
				CounterType amount1 = read<Os, block_data_t, CounterType>(first);
				CounterType amount2 = read<Os, block_data_t, CounterType>(second);
				for (CounterType i = 0; i < amount2 * (key_size + value_size); i++){
					first[offsetData + amount1 * (key_size + value_size) + i] = second[offsetData + i];
				}
				amount1 = amount1 + amount2;
				write<Os, block_data_t, CounterType>(first, amount1); //write new Counter data
		

				block_address_t ptr1 = read<Os, block_data_t, block_address_t>(second + offsetBackward);
				block_address_t ptr2 = read<Os, block_data_t, block_address_t>(first + offsetForward);
				
				block_address_t ptrfwd = read<Os, block_data_t, block_address_t>(second + offsetForward);

				write<Os, block_data_t, block_address_t>(first + offsetForward, ptrfwd); //write new pointer
				mmu->write(first, ptr1); //store first
		
				mmu->read(third, ptrfwd);
				write<Os, block_data_t, block_address_t>(third + offsetBackward, ptr1);
				mmu->write(third, ptrfwd); //make sure second is out of the list

				mmu->block_free(read<Os, block_data_t, block_address_t>(first + offsetForward)); //free second on sd card
				mmu->read(buffer[bufferPos].data, ptr1); //finally make sure data has the correct content.
				debug_->debug("Merged Block %d and %d. The second is the one set free", ptr1, ptr2);		
	
			}
			else if (blocks == 2){ //reassign elements. third isnt used

				CounterType amount1 = read<Os, block_data_t, CounterType>(first);
				CounterType amount2 = read<Os, block_data_t, CounterType>(second);
				block_address_t ptr1 = read<Os, block_data_t, block_address_t>(second + offsetBackward);
				block_address_t ptr2 = read<Os, block_data_t, block_address_t>(first + offsetForward);

				CounterType rmElem = 0;		
				CounterType avg = (amount1 + amount2) / 2; 
				if (amount2 > avg){
					//first move all elements to their block, THEN go and adjust the other block
					while (amount2 > avg){
						if (useKeys){
							KeyType key = read<Os, block_data_t, KeyType>(
									second + offsetData + rmElem * (key_size + value_size));
							write<Os, block_data_t, KeyType>
								(first + offsetData + amount1 * (key_size + value_size), key);
					
						}
						ValueType val = read<Os, block_data_t, ValueType>
							(second + offsetData + rmElem * (key_size + value_size) + key_size);
						write<Os, block_data_t, ValueType>
							(first + offsetData + amount1 * (key_size + value_size) + key_size, val);
						rmElem = rmElem + 1;
						amount1 = amount1 + 1;
						amount2 = amount2 - 1;
					}
					for (CounterType i = 0; i < amount2 * (key_size + value_size); i++){
						second[offsetData + i] = second[offsetData + i + rmElem * (key_size + value_size)];
					}
				}
				else{
					//since we have to put in front of block2, first make space
					for (CounterType i = 0; i < amount2 * (key_size + value_size); i++){
						second[offsetData + avg * (key_size + value_size) - i - 1] = 
				 second[offsetData + amount2 * (key_size + value_size) - i - 1]; //shove all amount2 elements in block2 behind by amount1-amount2
					}
					while (amount2 < avg){
			//debug_->debug("reassigning elements: a1=%d; a2=%d", amount1, amount2);	//works now
						if (useKeys){
							KeyType key = read<Os, block_data_t, KeyType>(
									first + offsetData + (avg + rmElem - 1) * (key_size + value_size));
							write<Os, block_data_t, KeyType>
								(second + offsetData + rmElem * (key_size + value_size), key);
					
						}
						ValueType val = read<Os, block_data_t, ValueType>
							(first + offsetData + (avg + rmElem) * (key_size + value_size) + key_size);
						write<Os, block_data_t, ValueType>
							(second + offsetData + rmElem * (key_size + value_size) + key_size, val);
						rmElem = rmElem + 1;
						amount2 = amount2 + 1;
						amount1 = amount1 - 1;
					}
				}
				write<Os, block_data_t, CounterType>(first, amount1);
				write<Os, block_data_t, CounterType>(second, amount2);
				
				mmu->write(first, ptr1);
				mmu->write(second, ptr2);
				mmu->read(buffer[bufferPos].data, ptr1);	
		debug_->debug("reassign completed between %d and %d. Filled: %d + %d %d", ptr1, ptr2, buffer[bufferPos].last_id, amount1, amount2);	
			}
			else if (blocks == 3){ //3 - way split
				//first create new 3rd block - that one we will put between first and second. (no changing external pointers)
				block_address_t ptr1 = read<Os, block_data_t, block_address_t>(second + offsetBackward);
				block_address_t ptr2 = read<Os, block_data_t, block_address_t>(first + offsetForward);
				block_address_t ptr3; 
				int a = mmu->block_alloc(&ptr3);
		debug_->debug("a: %d, ptr3: %d", a, ptr3);	
		
				write<Os, block_data_t, block_address_t>(first + offsetForward, ptr3);
				write<Os, block_data_t, block_address_t>(second + offsetBackward, ptr3); 
				write<Os, block_data_t, block_address_t>(third + offsetForward, ptr2); 
				write<Os, block_data_t, block_address_t>(third + offsetBackward, ptr1); 
				//The block has been inserted into the list now. 
				CounterType amount1 = read<Os, block_data_t, CounterType>(first);
				CounterType amount2 = read<Os, block_data_t, CounterType>(second);
				CounterType amount3 = 0;
				CounterType avg = (amount1 + amount2 + 1) / 3; //if not +1 the new block may end up with avg+2 entries - this way its avg+1 on either third or both first and second - which is best distribution possible
				CounterType rmElem = 0;
				//One of the Blocks needs to be at least half filled, the other full for this to start - we can use that to optimise a bit since we will only shuffle from first and/or second to third
				while (avg < amount1){
						if (useKeys){
							KeyType key = read<Os, block_data_t, KeyType>(
									first + offsetData + (avg + rmElem) * (key_size + value_size));
							write<Os, block_data_t, KeyType>
								(third + offsetData + amount3 * (key_size + value_size), key);
					
						}
						ValueType val = read<Os, block_data_t, ValueType>
							(first + offsetData + (avg + rmElem) * (key_size + value_size) + key_size);
						write<Os, block_data_t, ValueType>
							(third + offsetData + amount3 * (key_size + value_size) + key_size, val);
						amount3 = amount3 + 1;
						amount1 = amount1 - 1;
						rmElem = rmElem + 1;
				}
				rmElem = 0;
				while (avg < amount2){
						if (useKeys){
							KeyType key = read<Os, block_data_t, KeyType>(
									second + offsetData + (rmElem) * (key_size + value_size));
							write<Os, block_data_t, KeyType>
								(third + offsetData + amount3 * (key_size + value_size), key);
					
						}
						ValueType val = read<Os, block_data_t, ValueType>
							(second + offsetData + rmElem * (key_size + value_size) + key_size);
						write<Os, block_data_t, ValueType>
							(third + offsetData + amount3 * (key_size + value_size) + key_size, val);
						amount3 = amount3 + 1;
						amount2 = amount2 - 1;
						rmElem = rmElem + 1;
				}
				//now reposition values
				for (CounterType i = 0; i < amount2 * (key_size + value_size); i++){
					second[offsetData + i] = second[offsetData + i + rmElem * (key_size + value_size)];
				}
				write<Os, block_data_t, CounterType>(first, amount1);
				write<Os, block_data_t, CounterType>(second, amount2);
				write<Os, block_data_t, CounterType>(third, amount3);
		
				//save and done!
				mmu->write(first, ptr1);
				mmu->write(second, ptr2);
				mmu->write(third, ptr3);
				mmu->read(buffer[bufferPos].data, ptr1); //prepare data again.
				debug_->debug("Did the split: New block %d between %d and %d. Filled: %d + %d %d %d",  ptr3, ptr1, ptr2, buffer[bufferPos].last_id, amount1, amount2, amount3);		
	
	
			}
			else {
				//TODO: Error
			}
		}
	
		bool scrollBuffer(uint32_t index, CounterType bufferPos){//scroll this buffer to pos x
			if (index >= totalCount){	
				return false; //element not in list - trying to insert at the end will call add() instead
			}
			return true; //TODO
		}
		bool scrollList(uint32_t index, CounterType& bufferPos){ //scroll a buffer to pos x
			//Figure out how to scroll: from a current buffer or start, forward or backward (this cuts search worst & average case by 2, costing a single pointer in each block and 1 CounterType in ram)
			if (index >= totalCount){	
				return false; //element not in list - trying to insert at the end will call add() instead
			}
			
			CounterType b = buffersize;
			CounterType diff = index;
			bool FW = true;
			if (totalCount < diff + index){
				diff = totalCount - index;
				FW = false;
			}
			//check if we are already there on any buffer while checking out distances
			for (CounterType i = 0; i < buffersize; i++){
				if (index >=buffer[i].last_id && index < buffer[i].last_id + read<Os, block_data_t, CounterType>(buffer[i].data))
				{
					bufferPos = i;
					return true;
				}
				else if (index > buffer[i].last_id) //forward from this 
				{
					if (index - (buffer[i].last_id + read<Os, block_data_t, CounterType>(buffer[i].data)) < diff){
						diff = index - buffer[i].last_id;
						FW = true;
						b = i;
					}
				}
				else 
				{
					if (buffer[i].last_id - index < diff){
						diff = buffer[i].last_id - index;
						FW = false;
						b = i;
					} 
				}
			}		
			if (b == buffersize){
				lastbuffer++;
				if (lastbuffer == buffersize) lastbuffer = 0;
				b = lastbuffer;
				buffer[b].last_read = 0;
				if (FW)			
					buffer[b].last_id = 0;
				else
					buffer[b].last_id = totalCount;
				mmu->read(buffer[b].data, 0); //prepare data again.
			}	
			bufferPos = b;
			if (FW){
				while (buffer[bufferPos].last_id + read<Os, block_data_t, CounterType>(buffer[bufferPos].data) <= index){
					readForward(bufferPos);
				}
				return true;
			}
			else{

				while (index < buffer[bufferPos].last_id){
					readBackward(bufferPos);
				}
				return true;
			}
		}

	public:

		List(Debug_ptr debug_, MMU_ptr mmu) : debug_(debug_), mmu(mmu){
			
			debug_->debug("Memory map created");
			if (loadFromDisk){//try to restore
				block_data_t data[blocksize];
				mmu->read(data, 0); //this should be our head
				//read & verify some values - if it fails we have to create like below //TODO
				return;
			}
			//create a new list
			//mmu->block_alloc(); //This was originally to reserve the 0th block fo rthe head, this is now handled by the MMU though
			debug_->debug("HEAD reserved");
			key_size  = sizeof(KeyType);
			useKeys = true;
			totalCount = 0;
			maxElements = (blocksize - (2 * ptr_size + cnt_size)) / (key_size + value_size);
			debug_->debug("maxElements is %d", maxElements);
		
			if (maxElements < 1) return; //TODO error
			debug_->debug("Preparing to write new, empty head");
			for (CounterType i = 0; i < buffersize; i++){
				buffer[i].last_read = 0;
				buffer[i].last_id = 0;
			}			
			for (block_address_t i = 0; i < blocksize; i++){
				buffer[0].data[i] = 0;
			} 		
			lastbuffer = 0;	
		/* information about HEAD block: int-><0 [current elements in this block - always 0 (for searches)> ptr-><first block> ptr-><last block> int-><current elements in list><max elements per block><use keys><key_size(unreduced)><value_size><blocksize>
		*/	
			CounterType bSize = blocksize;
			CounterType valsize = value_size; //saved value cannot be constant
			write<Os, block_data_t, CounterType>(buffer[0].data + offsetMax, maxElements);
			write<Os, block_data_t, bool>(buffer[0].data + offsetUseKey, useKeys);
			write<Os, block_data_t, CounterType>(buffer[0].data + offsetKeySize, key_size);
			write<Os, block_data_t, CounterType>(buffer[0].data + offsetValueSize, valsize);
			write<Os, block_data_t, CounterType>(buffer[0].data + offsetBlockSize, bSize);

			mmu->write(buffer[0].data, 0, 1);
		}

		void setKeyUse(bool value){
			if (totalCount == 0)
			{
				useKeys = value;
				if (useKeys)
					key_size = sizeof(KeyType);
				else
					key_size = 0;
				maxElements = (blocksize - 3 * ptr_size) / (key_size + value_size);
			}
			else
			{
				//TODO:error();
			}
		}

		void insertByKey(KeyType key, ValueType& value , KeyType insertKey){
			insertByIndex(value, key, getIndex(insertKey));
		}	



		void insertByIndex(KeyType key, ValueType value, uint32_t index = 0){ //insert element before index - insert(e,5) will then have index 5

			if (totalCount == index){add(key, value); return;} //there is special code to handle empty list in add - cheap way of saving lines
			CounterType bufferPos = 0;			
			scrollList(index, bufferPos);
			//debug_->debug("inserting %d at %d into block %d", value, index, buffer[bufferPos].last_read);	
				
			//now data is the correct block - if it is full, we need to split	
			CounterType amount;
			read<Os, block_data_t, CounterType>(buffer[bufferPos].data, amount);
			if (amount == maxElements){
		//debug_->debug("Doing the balance for %d with LID %d", last_read, last_id);
				debug_->debug("rebalance when inserting %d at %d", value, index);	
				rebalance(bufferPos);
				scrollList(index, bufferPos);
				amount = read<Os, block_data_t, CounterType>(buffer[bufferPos].data);

			}
			//index right now is global, recalculate it to an index in this block
			index = index - buffer[bufferPos].last_id;
			for (block_address_t i = offsetData + (amount) * (key_size + value_size) - 1; i >= offsetData + index * (key_size + value_size); i--){
				buffer[bufferPos].data[i + (key_size + value_size)] = buffer[bufferPos].data[i];
			}
			if (useKeys){
				write<Os, block_data_t, KeyType>(buffer[bufferPos].data + offsetData + index * (key_size + value_size), key); //write key
			}		
			write<Os, block_data_t, ValueType>(buffer[bufferPos].data + offsetData + index * (key_size + value_size) + key_size, value); //write value
	
	
			amount = amount + 1;
			totalCount = totalCount + 1;
			write<Os, block_data_t, CounterType>(buffer[bufferPos].data, amount); //write amount
			mmu->write(buffer[bufferPos].data, buffer[bufferPos].last_read); //and save it all
		//debug_->debug("index %d, last_read %d, LID %d, tcount %d", index, last_read, last_id, totalCount);
	
		}

		void add(KeyType key, ValueType value){ //add element to list
			CounterType bufferPos = 0;			
			scrollList(totalCount - 1, bufferPos);
			
			CounterType amount = read<Os, block_data_t, CounterType>(buffer[bufferPos].data);
			if ((!(amount < maxElements)) || buffer[bufferPos].last_read == 0){ //there is no space, make a new block OR there are no elements in the list		
				debug_->debug("Add: No space in Block. Making new one. Oh and the new id is %d", buffer[bufferPos].last_id + amount);	
				block_address_t newpos; 
				mmu->block_alloc(&newpos);//request new adress
				write<Os, block_data_t, block_address_t>(buffer[bufferPos].data + offsetForward, newpos);//write new adress on this block
				mmu->write(buffer[bufferPos].data, buffer[bufferPos].last_read);
			
				mmu->read(buffer[bufferPos].data, 0);
				write<Os, block_data_t, block_address_t>(buffer[bufferPos].data + offsetBackward, newpos); //write new end pointer on HEAD
				mmu->write(buffer[bufferPos].data, 0);
		
				//prepare the new block (counter = 0, pre pointer, last pointer)
				for (block_address_t i = 0; i < blocksize; i++){
					buffer[bufferPos].data[i] = 0;
				}
				write<Os, block_data_t, block_address_t>(buffer[bufferPos].data + offsetBackward, buffer[bufferPos].last_read); //write back pointer
				//forward pointer to HEAD is already 0, no action needed
	
				buffer[bufferPos].last_read = newpos;
				buffer[bufferPos].last_id = buffer[bufferPos].last_id + amount;
				amount = 0;
			}
		
			//now we are in the correct block
			if (useKeys){
				write<Os, block_data_t, KeyType>(buffer[bufferPos].data + offsetData + amount * (key_size + value_size), key); //write key
			}
			write<Os, block_data_t, ValueType>(buffer[bufferPos].data + offsetData + amount * (key_size + value_size) + key_size, value); //write value
			amount = amount + 1;
			totalCount = totalCount + 1;
			write<Os, block_data_t, CounterType>(buffer[bufferPos].data, amount); //write amount
			mmu->write(buffer[bufferPos].data, buffer[bufferPos].last_read); //and save it all
			//debug_->debug("index %d, last_read %d, LID %d, tcount %d", index, last_read, last_id, totalCount);
	
		}
		void removeByIndex(block_address_t pos){
			
			CounterType bufferPos = 0;			
			scrollList(pos, bufferPos);
		
			CounterType amount = read<Os, block_data_t, CounterType>(buffer[bufferPos].data);
		
			pos = pos - buffer[bufferPos].last_id;
			for (block_address_t i = 0; i < (amount - (pos + 1)) * (key_size + value_size); i++){ 
				buffer[bufferPos].data[offsetData + pos * (key_size + value_size) + i] 
					= buffer[bufferPos].data[offsetData + (pos + 1)* (key_size + value_size) + i] ;
			}
			
		
			amount = amount - 1;
			totalCount = totalCount - 1;
			write<Os, block_data_t, CounterType>(buffer[bufferPos].data, amount); //write amount#
	
		
			//check if removal would trigger a merge (counter - 1 <  max / 3)
			if (amount < maxElements / 3){
		
				rebalance(bufferPos);
			}
			else {mmu->write(buffer[bufferPos].data, buffer[bufferPos].last_read);}

		
		}
		void removeByKey(KeyType key){
			remove(getIndex(key));
		}

		block_address_t getIndexByKey(KeyType key){
			//go through the list, checking each element. Start at last_read, going forwards jumping over 0.
			lastbuffer++; if (lastbuffer == buffersize) lastbuffer = 0;			
			
			mmu->read(buffer[lastbuffer].data, buffer[lastbuffer].last_read);
			block_address_t start = buffer[lastbuffer].last_read;
			bool doOnce = true;
			while (buffer[lastbuffer].last_read != start || doOnce){
				doOnce = false;
				CounterType amount = read<Os, block_data_t, CounterType>(buffer[lastbuffer].data);
				for (CounterType i = 0; i < amount; i++){ 
					KeyType key2 = read<Os, block_data_t, KeyType>(buffer[lastbuffer].data + offsetData + (key_size + value_size) * i);
					if (key == key2){
						return buffer[lastbuffer].last_id + i;
					}
				}
				readForward(lastbuffer);
			}
		}

		void clear(){ //remove all elements
			mmu->read(buffer[0].data, 0);
			buffer[0].last_read = 0;
			readForward(0);
			while (buffer[0].last_read != 0){ //go through the list and open all blocks you find up for reassign (memMap)
				mmu->block_free(buffer[0].last_read);
				readForward(0);
			}
			totalCount = 0;
			write<Os, block_data_t, block_address_t>(buffer[0].data, 0);
			write<Os, block_data_t, block_address_t>(buffer[0].data, 0); 
			write<Os, block_data_t, CounterType>(buffer[0].data + offsetTotalCounter, totalCount);

			mmu->write(buffer[0].data, 0);
			//TODO clear buffers;
		}

		KeyType getKeyByIndex(uint32_t index){
	
			//find position i
			//return key
			CounterType bufferPos = 0;			
			
			scrollList(index , bufferPos);

			return read<Os, block_data_t, KeyType>(buffer[bufferPos].data + offsetData + (index - buffer[bufferPos].last_id) * (key_size + value_size));
		}

		ValueType getValueByKey(KeyType key){ //returns element e, if in list
			getByIndex(getIndex(key));
		}
		ValueType getValueByIndex(uint32_t index){ //return element i
			//find position i
			//return
			CounterType bufferPos = 0;			
			
			scrollList(index , bufferPos);

			return read<Os, block_data_t, ValueType>(buffer[bufferPos].data + offsetData + (index - buffer[bufferPos].last_id) * (key_size + value_size) + key_size);
		}

		void sync(){ //saves the status of the list on the disk //TODO
			block_data_t data[blocksize];
			mmu->read(data, 0);
			debug_->debug("read HEAD");
			//TODO: Make sure all is saved
			CounterType c = 0;
			write<Os, block_data_t, CounterType>(data, c); //write emptly slot
			write<Os, block_data_t, uint32_t>(data + offsetTotalCounter, totalCount); //write amount
			write<Os, block_data_t, bool>(data + offsetUseKey, useKeys); //write amount
			mmu->write(data, 0);
			debug_->debug("Wrote HEAD");
	
		}
}; //end class List
} //end namespace

#endif

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

//#include "../marco/BDMMU.hpp"

namespace wiselib {

/* A memoryMap is a manager for external data structures. It serves two main purposes: 
* First, it manages free and used blocks and allows data structures to reserve space without writing into another structures space
* Second, it allows the program it is created by to recover data structures after a restart without having to hardcode the head positions for all of them
*/
class MemoryMap {

private:

typedef OSMODEL Os;
Os::size_t HEAD; //first block of the map
Os::size_t size;
Os::size_t blocksize; //bits in a block
//1 bit / (remaining) block - usage[]
Os::size_t a;
Os::BlockMemory::self_pointer_t sd;
public:

MemoryMap(Os::size_t block_start, Os::size_t block_end, Os::size_t blocksize, Os::BlockMemory::self_pointer_t sd){
a = 0;
this->sd = sd;
sd->init();
}
Os::size_t reserveBlock(Os::size_t size = 1){
a = a + 1;
return a - 1;
}
void free(Os::size_t adress){
}
void read(Os::size_t adress, Os::block_data_t data[]){
	sd->read(data, adress, 1);
}
void write(Os::size_t adress, Os::block_data_t data[]){
	sd->write(data, adress, 1);
}
Os::size_t getBlockSize(){
	return 512;
}
};

template<typename OsModel_P, typename KeyType_P, typename ValueType_P,
typename CounterType_P, typename PointerType_P, 
typename OsModel_P::size_t fromBlock, typename OsModel_P::size_t toBlock, typename OsModel_P::size_t blocksize = 512, bool loadFromDisk = false>
   class List{	
	typedef OsModel_P Os;
	typedef typename Os::Debug Debug;
	typedef typename Debug::self_pointer_t Debug_ptr;	
	
	typedef typename Os::block_data_t block_data_t; //TODO
	typedef KeyType_P KeyType;
	typedef ValueType_P ValueType;
	typedef PointerType_P PointerType;
	typedef PointerType_P CounterType;
		

public:

//const Os::size_t HEAD = 0 //where the head of the list is located. since we get our own virtual memory partition, we can set this to 0.

CounterType totalCount; //amount of objs in list
//here come options:
CounterType key_size;
bool useKeys;
CounterType maxElements;

//Os::Debug::self_pointer_t 

Debug_ptr debug_;
PointerType last_read; //position of last block read
CounterType last_id; //index of first element in last block read

MemoryMap * mem;
/* information about HEAD block: int-><0 [current elements in this block - always 0 (for searches)> ptr-><first block> ptr-><last block> int-><current elements in list><max elements per block><use keys><key_size(unreduced)><value_size><blocksize>
*/
/* Format of 1 block: int-><amount objects in this block> ptr-><next block> ptr-><previous block> [data] 
  getting element i in a block is at position 0 + int_size + 2 * ptr_size + i * value_size
*/

static const CounterType ptr_size = sizeof(PointerType);
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



void readForward(Os::block_data_t data[blocksize]){
	last_read = read<Os, Os::block_data_t, PointerType>(data + offsetForward);
	last_id = last_id + read<Os, Os::block_data_t, CounterType>(data);
	mem->read(last_read, data);
	if (last_read == 0){
		last_id = 0;
	}
}

void readBackward(Os::block_data_t data[blocksize]){
	if (last_read == 0){
		last_id = totalCount;
	}
	last_read = read<Os, Os::block_data_t, PointerType>(data + offsetBackward);
	mem->read(last_read, data);
	last_id = last_id - read<Os, Os::block_data_t, CounterType>(data);
}

CounterType calcBlocks(CounterType amount){
	if (amount < (maxElements * 3) / 4){ //it fills 1 block to 75%
		return 1;
	}
	else if (amount < (maxElements * 8) / 5){ //it would fill 2 blocks to 75% - or 3 to 50%
		return 2;
	}	
	else {//3 blocks needed 
		return 3;
	}
}

void rebalance (Os::block_data_t data[blocksize]){
	//find most suited partner 	
	//our wished for fill state is 2/3 after this op
	//total amount should be closest to a multiple of (maxElements * 2/3). which multiple doesnt matter, just that it is as close as possible.
	if (last_read == 0) return; //Dont do anything if this is about the HEAD block.

	CounterType data_amount = read<Os, Os::block_data_t, CounterType>(data);
	PointerType data2_pos = read<Os, Os::block_data_t, PointerType>(data + offsetBackward);
	PointerType data3_pos = read<Os, Os::block_data_t, PointerType>(data + offsetForward);
	
	if (data2_pos == 0 && data3_pos == 0 && calcBlocks(data_amount) == 1){ //special case - last block in the list is getting smaller
		if (data_amount == 0){ //collapse empty block
		}
		return; //nothing (else) to do here
	}

	//partner search: 
	Os::block_data_t  data2[blocksize];
	Os::block_data_t  data3[blocksize];
	
	mem->read(data2_pos, data2);
	CounterType data2_amount = read<Os, Os::block_data_t, CounterType>(data2);
	CounterType blocks  = calcBlocks(data_amount + data2_amount);
	CounterType d2_closeness  = abs(((data_amount + data2_amount)/ blocks) - (maxElements * 2) / 3);
	
	mem->read(data3_pos, data3);
	CounterType data3_amount = read<Os, Os::block_data_t, CounterType>(data3);
	blocks = calcBlocks(data_amount + data3_amount);
	CounterType d3_closeness = abs(((data_amount + data3_amount)/ blocks) - (maxElements * 2) / 3);

	Os::block_data_t * first;
	Os::block_data_t * second;
	Os::block_data_t * third;

	if (((d2_closeness < d3_closeness) || (data3_pos == 0)) && (data2_pos != 0)){ //use d2 either if its better or if its the only available
		//d2 is BEFORE d
		last_read = data2_pos;
		last_id = last_id - data2_amount;
		first = data2;
		second = data;
		third = data3;
		blocks  = calcBlocks(data_amount + data2_amount);	
	}else if (((d2_closeness >= d3_closeness) || (data2_pos == 0)) && (data3_pos != 0)){ 
		//d3 is BEHIND d
		first = data;
		second = data3;
		third = data2;
	}
	else {
		//Special case - both sides are HEAD. create a new block first, then reassign (blocks = 2)
		//note - this cannot be called because the block is small as that case is already handled top
		blocks = 2;
		first = data;
		second = data2;
	//	third = data3; // wont be used
	
	debug_->debug("Split - There is only one Block. Creating new and reassigning.");	
		PointerType newpos = mem->reserveBlock();//request new adress	
	
		write<Os, Os::block_data_t, PointerType>(first + offsetForward, newpos);//write new adress on this block
		//mem->write(last_read, data); //will be written later
		mem->read(0,data3);
		write<Os, Os::block_data_t, PointerType>(data3 + offsetBackward, newpos); //write new end pointer on HEAD
		mem->write(0,data3);
		
		//prepare the new block (counter = 0, pre pointer, last pointer)
		for (Os::size_t i = 0; i < blocksize; i++){
			second[i] = 0;
		}
		write<Os, Os::block_data_t, PointerType>(second + offsetBackward, last_read); //write back pointer
		//forward pointer to HEAD is already 0, no action needed
		//now were ready to go on and treat this block as if it already existed on the card

debug_->debug("Extra Block %d created. LR= %d LID=%d cnt=%d", newpos, last_read, last_id, data_amount);
	}
	//now that we decided who to play with, we decide what to do (depends on blocks)
	if (blocks == 1){ //merge first and second.
		CounterType amount1 = read<Os, Os::block_data_t, CounterType>(first);
		CounterType amount2 = read<Os, Os::block_data_t, CounterType>(second);
		for (CounterType i = 0; i < amount2 * (key_size + value_size); i++){
			first[offsetData + amount1 * (key_size + value_size) + i] = second[offsetData + i];
		}
		amount1 = amount1 + amount2;
		write<Os, Os::block_data_t, CounterType>(first, amount1); //write new Counter data
		

		PointerType ptr1 = read<Os, Os::block_data_t, PointerType>(second + offsetBackward);
		PointerType ptrfwd = read<Os, Os::block_data_t, PointerType>(second + offsetForward);

		write<Os, Os::block_data_t, PointerType>(first + offsetForward, ptrfwd); //write new pointer
		mem->write(ptr1, first); //store first
		
		mem->read(ptrfwd, third);
		write<Os, Os::block_data_t, PointerType>(third + offsetBackward, ptr1);
		mem->write(ptrfwd, third); //make sure second is out of the list

		mem->free(read<Os, Os::block_data_t, PointerType>(first + offsetForward)); //free second on sd card
		mem->read(ptr1, data); //finally make sure data has the correct content.
		debug_->debug("Merged some");		
	
	}
	else if (blocks == 2){ //reassign elements. third isnt used

		CounterType amount1 = read<Os, Os::block_data_t, CounterType>(first);
		CounterType amount2 = read<Os, Os::block_data_t, CounterType>(second);
		PointerType ptr1 = read<Os, Os::block_data_t, PointerType>(second + offsetBackward);
		PointerType ptr2 = read<Os, Os::block_data_t, PointerType>(first + offsetForward);

		CounterType rmElem = 0;		
		CounterType avg = (amount1 + amount2) / 2; 
		if (amount2 > avg){
			//first move all elements to their block, THEN go and adjust the other block
			while (amount2 > avg){
				if (useKeys){
					KeyType key = read<Os, Os::block_data_t, KeyType>(
							second + offsetData + rmElem * (key_size + value_size));
					write<Os, Os::block_data_t, KeyType>
						(first + offsetData + amount1 * (key_size + value_size), key);
					
				}
				ValueType val = read<Os, Os::block_data_t, ValueType>
					(second + offsetData + rmElem * (key_size + value_size) + key_size);
				write<Os, Os::block_data_t, ValueType>
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
					KeyType key = read<Os, Os::block_data_t, KeyType>(
							first + offsetData + (avg + rmElem - 1) * (key_size + value_size));
					write<Os, Os::block_data_t, KeyType>
						(second + offsetData + rmElem * (key_size + value_size), key);
					
				}
				ValueType val = read<Os, Os::block_data_t, ValueType>
					(first + offsetData + (avg + rmElem) * (key_size + value_size) + key_size);
				write<Os, Os::block_data_t, ValueType>
					(second + offsetData + rmElem * (key_size + value_size) + key_size, val);
				rmElem = rmElem + 1;
				amount2 = amount2 + 1;
				amount1 = amount1 - 1;
			}
		}
		write<Os, Os::block_data_t, CounterType>(first, amount1);
		write<Os, Os::block_data_t, CounterType>(second, amount2);
				
		mem->write(ptr1, first);
		mem->write(ptr2, second);
		mem->read(ptr1, data);	
debug_->debug("reassign completed between %d and %d. Filled: %d + %d %d", ptr1, ptr2, last_id, amount1, amount2);	
	}
	else if (blocks == 3){ //3 - way split
		//first create new 3rd block - that one we will put between first and second. (no changing external pointers)
		PointerType ptr1 = read<Os, Os::block_data_t, PointerType>(second + offsetBackward);
		PointerType ptr2 = read<Os, Os::block_data_t, PointerType>(first + offsetForward);
		PointerType ptr3 = mem->reserveBlock();
		
		write<Os, Os::block_data_t, PointerType>(first + offsetForward, ptr3);
		write<Os, Os::block_data_t, PointerType>(second + offsetBackward, ptr3); 
		write<Os, Os::block_data_t, PointerType>(third + offsetForward, ptr2); 
		write<Os, Os::block_data_t, PointerType>(third + offsetBackward, ptr1); 
		//The block has been inserted into the list now. 
		CounterType amount1 = read<Os, Os::block_data_t, CounterType>(first);
		CounterType amount2 = read<Os, Os::block_data_t, CounterType>(second);
		CounterType amount3 = 0;
		CounterType avg = (amount1 + amount2 + 1) / 3; //if not +1 the new block may end up with avg+2 entries - this way its avg+1 on either third or both first and second - which is best distribution possible
		CounterType rmElem = 0;
		//One of the Blocks needs to be at least half filled, the other full for this to start - we can use that to optimise a bit since we will only shuffle from first and/or second to third
		while (avg < amount1){
				if (useKeys){
					KeyType key = read<Os, Os::block_data_t, KeyType>(
							first + offsetData + (avg + rmElem) * (key_size + value_size));
					write<Os, Os::block_data_t, KeyType>
						(third + offsetData + amount3 * (key_size + value_size), key);
					
				}
				ValueType val = read<Os, Os::block_data_t, ValueType>
					(first + offsetData + (avg + rmElem) * (key_size + value_size) + key_size);
				write<Os, Os::block_data_t, ValueType>
					(third + offsetData + amount3 * (key_size + value_size) + key_size, val);
				amount3 = amount3 + 1;
				amount1 = amount1 - 1;
				rmElem = rmElem + 1;
		}
		rmElem = 0;
		while (avg < amount2){
				if (useKeys){
					KeyType key = read<Os, Os::block_data_t, KeyType>(
							second + offsetData + (rmElem) * (key_size + value_size));
					write<Os, Os::block_data_t, KeyType>
						(third + offsetData + amount3 * (key_size + value_size), key);
					
				}
				ValueType val = read<Os, Os::block_data_t, ValueType>
					(second + offsetData + rmElem * (key_size + value_size) + key_size);
				write<Os, Os::block_data_t, ValueType>
					(third + offsetData + amount3 * (key_size + value_size) + key_size, val);
				amount3 = amount3 + 1;
				amount2 = amount2 - 1;
				rmElem = rmElem + 1;
		}
		//now reposition values
		for (CounterType i = 0; i < amount2 * (key_size + value_size); i++){
			second[offsetData + i] = second[offsetData + i + rmElem * (key_size + value_size)];
		}
		write<Os, Os::block_data_t, CounterType>(first, amount1);
		write<Os, Os::block_data_t, CounterType>(second, amount2);
		write<Os, Os::block_data_t, CounterType>(third, amount3);
		
		//save and done!
		mem->write(ptr1, first);
		mem->write(ptr2, second);
		mem->write(ptr3, third);
		mem->read(ptr1, data); //prepare data again.
		debug_->debug("Did the split: New block %d between %d and %d. Filled: %d + %d %d %d",  ptr3, ptr1, ptr2, last_id, amount1, amount2, amount3);		
	
	
	}
	else {
		//TODO: Error
	}
}

bool scrollList(CounterType index,Os::block_data_t data[blocksize]){ //requires lastRead to be in data
	//Figure out how to scroll: from current or start, forward or backward (this cuts search worst & average case by 2, costing a single pointer in each block and 1 Os::size_t in ram)
	//first check if were already there
	if (index >=last_id && index < last_id + read<Os, Os::block_data_t, CounterType>(data)){
		return true;
	}
	else if (index > totalCount){	
		return false; //element not in list - special case for insert: index == totalCount needs to be found-> always the last block (go back from head)
	}
	else if (index == totalCount){	
		mem->read(0, data);
		last_read = 0;
		last_id = totalCount;
		readBackward(data);
		return false; //element not in list , however scroll to the back to allow insert - in case of full block this results in split and another scroll to the end.
	}
	if (last_id > 2 * index || (totalCount + last_id - index) > (index)){ //need to go forward
		if (last_id > 2 * index){
			//closest to the start, go from there
			mem->read(0, data);
			last_read = 0;
			last_id = 0;
		}
		while (last_id + read<Os, Os::block_data_t, CounterType>(data) <= index){
			readForward(data);
		}
		return true;
	}
	else{
		//need to go backwards 
		if ((totalCount + last_id - index) < (index)){
			//closest to the start, go from there
			mem->read(0, data);
			last_read = 0;
			last_id = totalCount;
		}
		while (index < last_id){
			readBackward(data);
		}
		return true;
	}
}

public:

List(Os::Debug::self_pointer_t debug_, Os::BlockMemory::self_pointer_t sd){
	
	
	this-> debug_ = debug_;	
	mem = new MemoryMap(fromBlock, toBlock, blocksize, sd);
	debug_->debug("Memory map created");
	if (loadFromDisk){//try to restore
		Os::block_data_t data[blocksize];
		mem->read(0, data); //this should be our head
		//read & verify some values - if it fails we have to create like below
		return;
	}
	//create a new list
	mem->reserveBlock();
	debug_->debug("HEAD reserved");
	key_size  = sizeof(KeyType);
	useKeys = true;
	totalCount = 0;
	maxElements = (blocksize - (2 * ptr_size + cnt_size)) / (key_size + value_size);
	debug_->debug("maxElements is %d", maxElements);
		
	if (maxElements < 1) return; //TODO error
	debug_->debug("Preparing to write new, empty head");
	last_read = 0;
	last_id = 0;
	Os::block_data_t data[blocksize];

	for (Os::size_t i = 0; i < blocksize; i++){
		data[i] = 0;
	}
/* information about HEAD block: int-><0 [current elements in this block - always 0 (for searches)> ptr-><first block> ptr-><last block> int-><current elements in list><max elements per block><use keys><key_size(unreduced)><value_size><blocksize>
*/	
	CounterType valueSize = sizeof(ValueType);
	CounterType bSize = blocksize;
	write<Os, Os::block_data_t, CounterType>(data + offsetMax, maxElements);
	write<Os, Os::block_data_t, bool>(data + offsetUseKey, useKeys);
	write<Os, Os::block_data_t, CounterType>(data + offsetKeySize, key_size);
	write<Os, Os::block_data_t, CounterType>(data + offsetValueSize, valueSize);
	write<Os, Os::block_data_t, CounterType>(data + offsetBlockSize, bSize);

	mem->write(0, data);

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



void insertByIndex(KeyType key, ValueType value, CounterType index = 0){ //insert element before index - insert(e,5) will then have index 5

	Os::block_data_t data[blocksize];
	if ((totalCount == 0) && (index == 0)){add(key, value); return;} //there is special code to handle empty list in add - cheap way of saving lines
	mem->read(last_read, data);
	scrollList(index, data);
	//now data is the correct block - if it is full, we need to split	
	CounterType amount;
	read<Os, Os::block_data_t, CounterType>(data, amount);
	if (amount == maxElements){
//debug_->debug("Doing the balance for %d with LID %d", last_read, last_id);
	
		rebalance(data);
		scrollList(index, data);
		amount = read<Os, Os::block_data_t, CounterType>(data);

	}
	//index right now is global, recalculate it to an index in this block
	index = index - last_id;
	for (Os::size_t i = offsetData + (amount) * (key_size + value_size) - 1; i >= offsetData + index * (key_size + value_size); i--){
		data[i + (key_size + value_size)] = data[i];
	}
	if (useKeys){
		write<Os, Os::block_data_t, KeyType>(data + offsetData + index * (key_size + value_size), key); //write key
	}		
	write<Os, Os::block_data_t, ValueType>(data + offsetData + index * (key_size + value_size) + key_size, value); //write value
	
	
	amount = amount + 1;
	totalCount = totalCount + 1;
	write<Os, Os::block_data_t, CounterType>(data, amount); //write amount
	mem->write(last_read, data); //and save it all
//debug_->debug("index %d, last_read %d, LID %d, tcount %d", index, last_read, last_id, totalCount);
	
}

void add(KeyType key, ValueType value){ //add element to list
	Os::block_data_t data[blocksize];
	mem->read(0, data);
	last_read = 0;
	readBackward(data);
	CounterType amount = read<Os, Os::block_data_t, CounterType>(data);
	if ((!(amount < maxElements)) || last_read == 0){ //there is no space, make a new block OR there are no elements in the list		
		debug_->debug("Add: No space in Block. Making new one. Oh and the new id is %d", last_id + amount);	
		CounterType newpos = mem->reserveBlock();//request new adress
		write<Os, Os::block_data_t, PointerType>(data + offsetForward, newpos);//write new adress on this block
		mem->write(last_read, data);
		mem->read(0,data);
		write<Os, Os::block_data_t, PointerType>(data + offsetBackward, newpos); //write new end pointer on HEAD
		mem->write(0,data);
		
		//prepare the new block (counter = 0, pre pointer, last pointer)
		for (Os::size_t i = 0; i < blocksize; i++){
			data[i] = 0;
		}
		write<Os, Os::block_data_t, PointerType>(data + offsetBackward, last_read); //write back pointer
		//forward pointer to HEAD is already 0, no action needed
	
		last_read = newpos;
		last_id = last_id + amount;
		amount = 0;
	}
		
	//now we are in the correct block
	if (useKeys){
		write<Os, Os::block_data_t, KeyType>(data + offsetData + amount * (key_size + value_size), key); //write key
	}
	write<Os, Os::block_data_t, ValueType>(data + offsetData + amount * (key_size + value_size) + key_size, value); //write value
	amount = amount + 1;
	totalCount = totalCount + 1;
	write<Os, Os::block_data_t, CounterType>(data, amount); //write amount
	mem->write(last_read, data); //and save it all
	//debug_->debug("index %d, last_read %d, LID %d, tcount %d", index, last_read, last_id, totalCount);
	
}
void removeByIndex(Os::size_t pos){
	Os::block_data_t data[blocksize];
	mem->read(last_read, data);
	scrollList(pos, data);
	CounterType amount = read<Os, Os::block_data_t, CounterType>(data);
	pos = pos - last_id;
	for (Os::size_t i = 0; i < (amount - (pos + 1)) * (key_size + value_size); i++){ 
		data[offsetData + pos * (key_size + value_size) + i] 
			= data[offsetData + (pos + 1)* (key_size + value_size) + i] ;
	}
	amount = amount - 1;
	totalCount = totalCount - 1;
	write<Os, Os::block_data_t, CounterType>(data, amount); //write amount
	//check if removal would trigger a merge (counter - 1 <  max / 3)
	if (amount < maxElements / 3){
		rebalance(data);
	}
	else {mem->write(last_read, data);}
}
void removeByKey(KeyType key){
	remove(getIndex(key));
}

Os::size_t getIndexByKey(KeyType key){
	//go through the list, checking each element. Start at last_read, going forwards jumping over 0.
	Os::block_data_t data[blocksize];
	mem->read(last_read, data);
	Os::size_t start = last_read;
	bool doOnce = true;
	while (last_read != start || doOnce){
		doOnce = false;
		CounterType amount = read<Os, Os::block_data_t, CounterType>(data);
		for (CounterType i = 0; i < amount; i++){ 
			KeyType key2 = read<Os, Os::block_data_t, KeyType>(data + offsetData + (key_size + value_size) * i);
			if (key == key2){
				return last_id + i;
			}
		}
	}
}

void clear(){ //remove all elements
	Os::block_data_t data[blocksize];
	mem->read(0, data);
	readForward(data);
	while (last_read != 0){ //go through the list and open all blocks you find up for reassign (memMap)
		mem->free(last_read);
		readForward(data);
	}
	totalCount = 0;
	write<Os, Os::block_data_t, PointerType>(data, 0);
	write<Os, Os::block_data_t, PointerType>(data, 0); 
	write<Os, Os::block_data_t, CounterType>(data + offsetTotalCounter, totalCount);

	mem->write(0,data);
}

KeyType getKeyByIndex(CounterType index){
	
	//find position i
	//return key
	Os::block_data_t data[blocksize];
	mem->read(last_read, data);
	scrollList(index , data);

	return read<Os, Os::block_data_t, KeyType>(data + offsetData + (index - last_id) * (key_size + value_size));
}

ValueType getValueByKey(KeyType key){ //returns element e, if in list
	getByIndex(getIndex(key));
}
ValueType getValueByIndex(CounterType index){ //return element i
	//find position i
	//return
	Os::block_data_t data[blocksize];	
	mem->read(last_read, data);
	scrollList(index , data);
	return read<Os, Os::block_data_t, ValueType>(data + offsetData + (index - last_id) * (key_size + value_size) + key_size);
}

void sync(){ //saves the status of the list on the disk
	Os::block_data_t data[blocksize];
	mem->read(0, data);
	debug_->debug("read HEAD");
	//TODO: Make sure all is saved
	CounterType c = 0;
	write<Os, Os::block_data_t, CounterType>(data, c); //write emptly slot
	write<Os, Os::block_data_t, CounterType>(data + offsetTotalCounter, totalCount); //write amount
	write<Os, Os::block_data_t, bool>(data + offsetUseKey, useKeys); //write amount
	mem->write(0,data);
	debug_->debug("Wrote HEAD");
	
}
}; //end class List
} //end namespace

#endif

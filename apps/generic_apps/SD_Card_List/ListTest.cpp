//#define DEBUG
#define USE_RAM_BLOCK_MEMORY 1
//#define BDMMU_DEBUG
#include <external_interface/external_interface.h>
#include "util/serialization/simple_types.h"
#define LIST_VIRT
//#define LIST_DEBUG
#include "List.h"

#include "../block_device_mmu/block_device_mmu.hpp"
#define NR_OF_BLOCKS_TO_TEST 8

#include <time.h>
#include <stdlib.h> 
using namespace wiselib;


typedef OSMODEL Os;

class App {
     public:

	

	struct Message
	{
		char* string[5];
		Os::size_t pi;
	};

		// NOTE: this uses up a *lot* of RAM, way too much for uno!
	enum {
		BlOCK_SIZE = 512, BLOCKS_IN_1GB = 2097152
	};

	void init(Os::AppMainParameter& value) {
		debug_ = &FacetProvider<Os, Os::Debug>::get_facet(value);
		sd = &FacetProvider<Os, Os::BlockMemory>::get_facet(value);
		sd->init();
		//debug_->debug("sd = %x", sd);
		//testAdd();
		//testInsertByIndex();
		//testRemoveIndex();
		//testPersistenz();  //list itself works!
		//testGetKeyIndexBothWays();
		generateFillupAnimation();
//testKeyUse (on all tests)
	//debug_->debug("About to exit");
		
		exit(0);
	}

	void generateFillupAnimation()
	{
		typedef typename BDMMU_Template_Wrapper<Os, Os::BlockMemory, Os::Debug>::BDMMU<0, 50, 1, 1> MMU_0_t;
		MMU_0_t mmu_0(sd, debug_, false, true);
		List<Os, MMU_0_t, int, int, false> list(debug_, &mmu_0);

		srand (345);//time(NULL));

		long value = 0xFFFFFFFFL;
		int imageCounter = 0;
		for (int i = 0; i < 800; i++){
			list.add(-1, i); //key a number with itself
			if(i % 15 == 0)
			{
				char filename[20];
				sprintf(filename, "bild%04d.dot", imageCounter);
				imageCounter++;
				FILE* file = fopen(filename, "w");
				sd->printGNUPLOTOutputBytes(0, 50, file);
				fclose(file);
			}
		}
		
		int count = 800;
		for (int i = 800; i < 6000; i++)
		{

	
				
			int a = rand() % count;
			//debug_->debug("pulled %d", a);
			if (2 * a < count){
				list.removeByIndex(a);
				count--;
			}
			else{
				list.insertByIndex(-1,i,a);
				count++;
			}

			if(i % 5 == 0)
			{
				char filename[20];
				sprintf(filename, "bild%04d.dot", imageCounter);
				imageCounter++;
				FILE* file = fopen(filename, "w");
				sd->printGNUPLOTOutputBytes(0, 50, file);
				fclose(file);
			}
		}
	}


	Message makeMessage(const char* string)
	{
		Message m;
		memcpy(m.string, string, 20);
		m.pi = 3;
		return m;
	}
	void printMessage(Message m)
	{
		debug_->debug("%s", m.string);
		debug_->debug("%d", m.pi);
	}

	/*void testAdd(){ //Test assumes getValueByIndex works fine - works!
		debug_->debug("Official Test start here");
		debug_->debug("Testing: Add");
		
		typedef BDMMU<Os, 0, 10, 1, 1, 512, Os::Debug, Os::BlockMemory> MMU_0_t;
		MMU_0_t mmu_0(debug_, sd, false, true);
		
		List<Os, MMU_0_t, int, int, Os::size_t, Os::size_t, 100, 200, 512, false> list(debug_, &mmu_0);
			
		//this list saves numbers
		debug_->debug("List created, filling with numbers");		
		for (int i = 0; i < 200; i++){
			list.add(i, i); //key a number with itself
		}
		debug_->debug("List should contain the first 1000 numbers (0-999)");		
		for (int i = 0; i < 200; i++){
			int j = list.getValueByIndex(i);
			if (j != i)
			{
				debug_->debug("But there is a mistake with number %d - it says %d instead", i, j);		
				debug_->debug("Aborting");		return;
			
			}
		}
	
		debug_->debug("Test finished succesfully");		
	}*/
 	void testPersistenz(){ 
		debug_->debug("Official Test start here");
		debug_->debug("Testing: Persistenz");
		
		//typedef BDMMU<Os, 0, 20, 1, 1, 512, Os::Debug, Os::BlockMemory> MMU_0_t; //Old signature
		

		typedef typename BDMMU_Template_Wrapper<Os, Os::BlockMemory, Os::Debug>::BDMMU<0, 50, 1, 1> MMU_0_t;
		{
			MMU_0_t mmu_0(sd, debug_, false, true);
			{
				List<Os, MMU_0_t, int, int, false> list(debug_, &mmu_0);
				debug_->debug("List created. It is %d byte large", sizeof(list));		
				for (int i = 0; i < 1000; i++){
					list.add(i, i); //key a number with itself
				}
				list.sync();
			}
			{
				List<Os, MMU_0_t, int, int, true> list(debug_, &mmu_0);
				debug_->debug("2nd List created. Filling another 1k", sizeof(list));		
				for (int i = 1000; i < 2000; i++){
					list.add(i, i); //key a number with itself
				}
			
				list.sync();
			}
		}
		{
			MMU_0_t mmu_0(sd, debug_, true, true);
			List<Os, MMU_0_t, int, int, true> list(debug_, &mmu_0);
			debug_->debug("3rd List created. Reading");		
			for (int i = 0; i < 2000; i++){
				int j = list.getValueByIndex(i);
				if (j != i)
				{
					debug_->debug("But there is a mistake with number %d - it says %d instead", i, j);		
					debug_->debug("Aborting");		
					return;
				}
			}
		}
		debug_->debug("Persistence Test finished succesfully.");
		
	}

	 void testRemoveIndex(){ //Test assumes add & getValueByIndex works fine
		debug_->debug("Official Test start here");
		debug_->debug("Testing: Remove");
		
		//typedef BDMMU<Os, 0, 50, 1, 1, 512, Os::Debug, Os::BlockMemory> MMU_0_t; //Old signature
		

		typedef typename BDMMU_Template_Wrapper<Os, Os::BlockMemory, Os::Debug>::BDMMU<0, 50, 1, 1> MMU_0_t;

		MMU_0_t mmu_0(sd, debug_, false, true);
		
		List<Os, MMU_0_t, int, int, false> list(debug_, &mmu_0);
		
		
		//List<Os, int, int, Os::size_t, Os::size_t, 100, 200, 512, false> list(debug_, sd);
			//this list saves numbers
		debug_->debug("List created. It is %d byte large", sizeof(list));		
		for (int i = 0; i < 1000; i++){
			list.add(i, i); //key a number with itself
		}
		debug_->debug("List Filled, Removin 500");
		for (int i = 0; i < 500; i++){
			list.removeByIndex(0); //remove first 500 nrs
			
		
		}
		debug_->debug("500 removed, removing even numbers");
		for (int i = 0; i < 250; i++){
			list.removeByIndex(i); //remove every second number
		}
		debug_->debug("List should contain 250 uneven numbers (501-999)");		
		for (int i = 0; i < 250; i++){
			int j = list.getValueByIndex(i);
			if (j != (i * 2) + 501)
			{
				debug_->debug("But there is a mistake with number %d - it says %d instead", i, j);		
				debug_->debug("Aborting");		
				return;
				
			}else{	
				
			}
		}
	
		debug_->debug("RM Test finished succesfully. Inserting numbers again");
		for (int i = 0; i < 250; i++){
			
			list.insertByIndex(2 * i + 500, 2 * i + 500, i * 2); //insert all back in
			 
		}
		debug_->debug("List should contain 500 numbers (500-999)");		
		for (int i = 0; i < 500; i++){
			int j = list.getValueByIndex(i);
			if (j != i + 500)
			{
				debug_->debug("But there is a mistake with number %d - it says %d instead", i, j);		
				debug_->debug("Aborting");		
				return;
				
			}
		}
		debug_->debug("insert Test finished succesfully.");
		list.sync();
	}

	/*void testInsertByIndex(){ //Test assumes add & getValueByIndex works fine - works!
		debug_->debug("Official Test start here");
		debug_->debug("Testing: Insert");
	
		List<Os, unsigned int, int, int, int, 100, 200, 512, false> list(debug_, sd);

		debug_->debug("List created, filling with numbers");
	#define INSERT_TEST_AMOUNT 10000
		for ( int i = 0; i < INSERT_TEST_AMOUNT; i++){
			list.insertByIndex(i + INSERT_TEST_AMOUNT, i + INSERT_TEST_AMOUNT, i); //key a number with itself
	//debug_->debug("List Filled, %d, %d, %d", list.last_read, list.last_id, list.totalCount);
	
		}
		debug_->debug("List Filled, %d, %d, %d", list.last_read, list.last_id, list.totalCount);
		//should look <100, 0, 101, 1, ...>
		for ( int i = 0; i < INSERT_TEST_AMOUNT; i++){
			 int j = list.getValueByIndex(i);
			if (j != i + INSERT_TEST_AMOUNT)
			{
				debug_->debug("But there is a mistake with index %d - it says %d instead of %d", i, j, i);		
				debug_->debug("Aborting");debug_->debug("LR %d, LID %d", list.last_read, list.last_id);
				return;
			
			}//else debug_->debug("Correct: index %d - it says %d instead of %d", 2 * i, j, i + INSERT_TEST_AMOUNT);
		}

		debug_->debug("Test finished succesfully");
	}*/

	/*void testGetKeyIndexBothWays(){ //assumes add works as it should
		debug_->debug("Official Test start here");
		debug_->debug("Testing: Add");
		List<Os, int, int, Os::size_t, Os::size_t, 100, 200, 512, false> list(debug_, sd);
			//this list saves numbers
		debug_->debug("List created, filling with numbers");		
		for (int i = 0; i < 10000; i++){
			list.add(i * 2, i); //use numbers twice as large as the keys
		}
		debug_->debug("List should contain 10000 keys (0-19998)");		
		for (int i = 0; i < 10000; i++){
			int j = list.getKeyByIndex(i);
			if (j != i * 2)
			{
				debug_->debug("But there is a mistake with key number %d - it says %d instead of %", i, j, i * 2);		
				debug_->debug("Aborting");		return;
			
			}
		}
		debug_->debug("Now looking at indices belonging to keys");		
		for (int i = 0; i < 10000; i++){
			int j = list.getIndexByKey(i * 2);
			if (j != i)
			{
				debug_->debug("But there is a mistake with key %d - it says it has index %d instead of %", i * 2, j, i);		
				debug_->debug("Aborting");		return;
			
			}
		}
		debug_->debug("Test finished succesfully");	
	}*/
	
	/*void test(){
		debug_->debug("Official Test start here");
		List<Os, int, Message, Os::size_t, Os::size_t, 100, 200, 512, false> list(debug_, sd);

		debug_->debug("List created");		
		Message m1, m2, m3;
		m1 = makeMessage("ABCDEFGHIJ");
		m2 = makeMessage("TES^T123");
		m3 = makeMessage("M3");
		debug_->debug("adding stuff to the list");			
		list.add(2,m2);
		list.add(3,m3);	
		debug_->debug("added, reading now");
		printMessage(list.getValueByIndex(0));
		printMessage(list.getValueByIndex(1));
		debug_->debug("%d", list.getKeyByIndex(1));

		list.insertByIndex(1, m1, 1);
		printMessage(list.getValueByIndex(0));
		printMessage(list.getValueByIndex(1));
		printMessage(list.getValueByIndex(2));
	}*/

    private:
	//static Os::Debug dbg;
	Os::Debug::self_pointer_t debug_;
	Os::BlockMemory::self_pointer_t sd;
};

//Os::Debug App::dbg = Os::Debug();

wiselib::WiselibApplication<Os, App> app;
void application_main(Os::AppMainParameter& value) {
	app.init(value);
}
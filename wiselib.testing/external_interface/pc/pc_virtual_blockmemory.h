#ifndef __VIRTSDCARD_H__
#define	__VIRTSDCARD_H__

#include <iostream>
#include <stdio.h>
#include <stdlib.h>
#include <stdint.h>
#include "pc_os_model.h"

namespace wiselib {

/*
 * @brief A virtual bock memory for the PC os model. It implements the block memory interface.
 * @author Dominik Krupke
 * @author Maximilian Ernestus
 * @author Marco Nikander
 * 
 * This is an implementation of a virtual SD-Card for PCs, for testing of external memory algorithms and 
 * applications. The data is stored in a array in ram; though inefficient this is acceptable for test purposes. 
 * The IO-stats are collect calculated using test-data collected during testing of various SD-Cards on 
 * Arduino µCs over SPI.
 * 
Reads, writes, and Erase-Block erase 
operations, which are required when performing an erase or 
when writing to a new block, are counted. The assumption is 
made that the erase block which is erased is empty (other 
than the data to be erased), so that no additional write 
operations are needed. In reality once the card starts 
getting a bit fuller this is not the case, so the results 
obtained from this counting scheme should be viewed as a 
lower bound where IOs are concerned.

 * @tparam OsModel_P specifies the os model to use (pc os model in this case)
 * @tparam nrOfBlocks specifies the number of blocks we want to use (default 1000)
 * @tparam blocksize the size of the blocks you want to use (deault 512)
 */

//TODO: use memcopy in read, write erase instead of for loops to clean up code a bit
/*TODO: Redo IO & IO-time calculations. Make an enum for the seperate times and values, and use the enum 
to calc the stats*/

template<typename OsModel_P, unsigned int nrOfBlocks = 1000, unsigned int blocksize = 512, unsigned int eraseBlockSize = 128>
class VirtualSD {

public:

	typedef OsModel_P OsModel;
	typedef VirtualSD self_type;
	typedef self_type* self_pointer_t;
	typedef typename OsModel::block_data_t block_data_t;
	typedef typename OsModel::size_t address_t;

	typedef unsigned int io_t;

	enum {
		BLOCK_SIZE = blocksize, ///< size of block in byte (usable payload)
	};
			
	enum { 
		NO_ADDRESS = (address_t)(-1), ///< address_t value denoting an invalid address
	};
	
	enum {
		SUCCESS = OsModel::SUCCESS, 
		ERR_IO = OsModel::ERR_IO,
		ERR_NOMEM = OsModel::ERR_NOMEM,
		ERR_UNSPEC = OsModel::ERR_UNSPEC,
	};
	
	//BLOCK MEMORY CONCEPT
	
	int init(){
		static bool initialized = false;
		
		if(initialized == false) {
			memset(isWritten, false, nrOfBlocks);
			memset(memory, 0, sizeof(memory[0][0]) * BLOCK_SIZE * nrOfBlocks);
			initialized = true;
		}

		resetStats();
		return SUCCESS;
	}
	
	int init(typename OsModel::AppMainParameter& value){ 
	
		return init();
	}

	int read(block_data_t* buffer, address_t addr, address_t blocks = 1) {

		if(checkInputRange(addr, blocks) == SUCCESS) {
			
			memcpy(buffer, memory + TRUE_BLOCK_SIZE * addr, TRUE_BLOCK_SIZE * blocks);
			reads_ += blocks;
			ioCalls_++;
			return SUCCESS;
		
		} else {
			printf("READ: ");
			return checkInputRange(addr, blocks, true);
		}
	}

	int write(block_data_t* buffer, address_t addr, address_t blocks = 1) { 

		if(checkInputRange(addr, blocks) == SUCCESS) {
			memcpy(memory + TRUE_BLOCK_SIZE * addr, buffer, TRUE_BLOCK_SIZE * blocks);
			writes_ += blocks;
			
			/*Note that for a normal SD card, in order for a write of one or more blocks to be done, 
			an entire erase block must first be erased and the bytes then set to their respective 
			values, hence the eraseBlockErases counter is also incremented here.*/
			eraseBlockErases_ += (blocks % eraseBlockSize == 0 ? blocks/eraseBlockSize : blocks/eraseBlockSize + 1); 
			ioCalls_++;
			return SUCCESS;	
		} else {
			printf("WRITE: ");
			return checkInputRange(addr, blocks, true);
		}
	}

	int erase(address_t addr, address_t blocks = 1) {
		
		if(checkInputRange(addr, blocks) == SUCCESS) {
			memset(memory + TRUE_BLOCK_SIZE * addr, 0u, TRUE_BLOCK_SIZE * blocks);
			eraseBlockErases_ += (blocks % eraseBlockSize == 0 ? blocks/eraseBlockSize : blocks/eraseBlockSize + 1);
			ioCalls_++;
			return SUCCESS;
		} else {
			printf("ERASE: ");
			return checkInputRange(addr, blocks, true);
		}
	}

	





	//EXTRA FUNCTIONALITY

	int checkInputRange(address_t addr, address_t blocks, bool print = false) {
		
		if (addr + blocks - 1 >= nrOfBlocks) {
			if (print) {
				printf("Overflow virtual SD. Attempted to access blocks %ju - %ju\n", (uintmax_t) addr, (uintmax_t)(addr + blocks -1));
			}
			
			return ERR_UNSPEC;
		} else if (NO_ADDRESS - blocks < addr) {

			if (print) {
				printf("Either (1) Attempted to access invalid address (address == NO_ADDRESS) OR (2) the address datatype is overflowing!\n");
			}
			return ERR_UNSPEC;
		} else return SUCCESS;
	}

	/*
	 * Resets all the counters that keep track of the IO's
	 */
	void resetStats() {
		reads_ = 0;
		writes_ = 0;
		eraseBlockErases_ = 0;
		ioCalls_ = 0;
	}

	/*
	 * Prints usage statistics about the virtual sd card to the console.
	 */
	void printStats() {
		printf("Blocks read: %ju\n", (uintmax_t) reads_);
		printf("Blocks written: %ju\n", (uintmax_t) writes_);
		printf("EraseBlock erases: %ju\n", (uintmax_t) eraseBlockErases_);
		printf("IO Calls: %ju\n", (uintmax_t) ioCalls_);
	}

	/*
	 * Prints out a graphviz graph displaying every single byte.
	 * Non-null bytes are colored red so it is easier to see what parts are used.
	 * @param fromBlock The number of the block where to start displaying.
	 * @param toBlock The number of the block where to stop displaying.
	 */
	void printGraphBytes(int fromBlock, int toBlock, FILE* f)
	{
		if(fromBlock < 0 || toBlock > nrOfBlocks) return;
		fprintf(f, "digraph BM { \n\t node [shape=none, margin=0]; \n");
		fprintf(f, "\t sdcard [label=<");
		printHTMLTableBytes(fromBlock, toBlock, f);
		fprintf(f, ">];\n");
		fprintf(f, "}\n");
	}

	/*
	 * Prints out a graphiz graph displaying the specified blocks.
	 * Used blocks are marked red.
	 * @param fromBlock The number of the block where to start displaying.
	 * @param toBlock The number of the block where to stop displaying.
	 */
	void printGraphBlocks(int fromBlock, int toBlock, FILE* f)
	{
		if(fromBlock < 0 || toBlock > nrOfBlocks) return;

		fprintf(f, "digraph BM {\n\t node [shape=none, margin=0];\n");
		fprintf(f, "\t sdcard [label=<");
		printHTMLTableBlocks(fromBlock, toBlock, f);
		fprintf(f, ">]\n");
		fprintf(f, "}\n");
	}

	/*
	 * Helper for printGraphBytes
	 */
	void printHTMLTableBytes(int fromBlock, int toBlock, FILE* f)
	{
		if(fromBlock < 0 || toBlock > nrOfBlocks) return;
		fprintf(f, "<table border=\"0\" cellborder=\"1\" cellspacing=\"0\">");
		for(int block = fromBlock; block <= toBlock; block++)
		{
			fprintf(f, "\n\t\t<tr>\n");
			for(int j = 0; j < blocksize; j++)
			{
				fprintf(f, "\t\t\t<td bgcolor=\"%s\">%3d</td>\n", ((int)memory[block][j] == 0 ? "#FFFFFF" : "#FF0000"), (int)memory[block][j]);
				//std::cout << "\t\t\t<td bgcolor=\"" << ((int)memory[block][j] == 0 ? "#FFFFFF" : "#FF0000") << "\">" << " " << "</td>\n";
			}
			fprintf(f, "\t\t</tr>\n");
		}
		fprintf(f, "</table>");
	}

	/*
	 * Helper for printGraphBlocks
	 */
	void printHTMLTableBlocks(int fromBlock, int toBlock, FILE* f)
	{
		if(fromBlock < 0 || toBlock > nrOfBlocks) return;
		fprintf(f, "<table border=\"0\" cellborder=\"1\" cellspacing=\"0\">");
		fprintf(f, "\n\t\t<tr>\n");
		for(int block = fromBlock; block <= toBlock; block++)
		{
			fprintf(f, "\t\t\t<td bgcolor=\"%s\">%d</td>\n", (!isWritten[block] ? "#FFFFFF" : "#FF0000"), isWritten[block]);
			//std::cout << "\t\t\t<td bgcolor=\"" << ((int)memory[block][j] == 0 ? "#FFFFFF" : "#FF0000") << "\">" << " " << "</td>\n";

		}
		fprintf(f, "\t\t</tr>\n");
		fprintf(f, "</table>");
	}

	void printASCIIOutputBytes(int fromBlock, int toBlock)
	{
		if(fromBlock < 0 || toBlock > nrOfBlocks) return;
		for(int block = fromBlock; block <= toBlock; block++)
		{
			for(int j = 0; j < 20; j++)
			{
				printf("%3d|", ((int)memory[block][j]));
			}
			printf("\n");
		}
	}

	void printASCIIOutputBlocks(int fromBlock, int toBlock)
	{
		if(fromBlock < 0 || toBlock > nrOfBlocks) return;
		for(int block = fromBlock; block <= toBlock; block++)
		{
			printf("|%s", isWritten[block] ? "1" : " ");
		}
		printf("|\n");
	}

	void printGNUPLOTOutputBytes(address_t fromBlock, address_t toBlock, FILE* f)
	{
		if(fromBlock < 0 || toBlock > nrOfBlocks) return;
		for(address_t block = fromBlock; block < toBlock; ++block)
			{
				for(int j = 0; j < blocksize; j++)
				{
//					fprintf(f, "%s ", ((int)memory[block][j] == 0 ? "1" : "0"));
					fprintf(f, "%d ", ((int)memory[block][j]));
				}
				fprintf(f, "\n");
			}
	}

	typedef int (*colorfunc)(address_t, int, int);

	void printGNUPLOTOutputBytes(address_t fromBlock, address_t toBlock, FILE* f, colorfunc colorize)
	{
		if(fromBlock < 0 || toBlock > nrOfBlocks) return;

		for(address_t block = fromBlock; block < toBlock; ++block)
			{
				for(int j = 0; j < blocksize; j++)
				{
//					fprintf(f, "%s ", ((int)memory[block][j] == 0 ? "1" : "0"));
					fprintf(f, "%d ", colorize(block, j, (int)memory[block][j]));
				}
				fprintf(f, "\n");
			}
	}

	void reset()
	{
		memset(0u, false, nrOfBlocks);
		erase(0, nrOfBlocks);
		resetStats();
	}

	void dumpToFile(FILE* f)
	{
		for(int i = 0; i < nrOfBlocks; ++i)
		{
			fwrite(memory[i], blocksize, 1, f);
		}
	}

private:
	block_data_t memory[nrOfBlocks][blocksize];
	bool isWritten[nrOfBlocks];

	/*Unless the datatype of memory[][] is changed (usually block_data_t of size 1), TRUE_BLOCK_SIZE
	is equal to BLOCK_SIZE*/
	enum {
		TRUE_BLOCK_SIZE = sizeof(memory[0][0]) * BLOCK_SIZE, 
	};

/*	int blocksWritten_;
	int blocksRead_;
	int ios_;
	int duration_;*/

	io_t reads_ = 0;
	io_t writes_ = 0;
	io_t eraseBlockErases_ = 0;
	io_t ioCalls_ = 0;
};
} //NAMESPACE
#endif // __VIRTSDCARD_H__

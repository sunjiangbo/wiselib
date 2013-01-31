/*
 * Dies ist eine Implementierung einer virtuellen SD-Karte fuer Linux-Systeme (Eigentlich fuer alle) zum testen von externen Speicheralgorithmen.
 * Die Stats werden aus, auf meinen Erinnerungen von Messergebnissen basierenden, Daten berechnet, wobei die Peaks nicht beruecksichtigt werden.
 * Das Interface ist dem der Wiselib nachempfunden. Die Benutzung sollte selbsterklaerend sein.
 */
#ifndef __VIRTSDCARD_H__
#define	__VIRTSDCARD_H__
#include <iostream>
#include <stdio.h>
#include <stdlib.h>
#include "pc_os_model.h"

namespace wiselib {

/*
* @brief A virtual bock memory for the PC os model. It implements the block memory interface.
* @author Maximilian Ernestus
*
* The data is stored in the ram (huge waste but who cares, it is just used for debugging)
* It was written to debug programs, that
* @tparam OsModel_P specifies the os model to use (pc os model in this case)
* @tparam nrOfBlocks specifies the number of blocks we want to use (default 1000)
* @tparam blocksize the size of the blocks you want to use (deault 512)
*/
template<typename OsModel_P, int nrOfBlocks = 1000, int blocksize = 512>
class VirtualSD {

public:
	/*
	 * weird wiselib stuff comes here
	 */
	typedef OsModel_P OsModel;
	typedef typename OsModel::block_data_t block_data_t;
	typedef typename OsModel::size_t size_type;
	typedef size_type address_t; /// always refers to a block number
	typedef VirtualSD self_type;
	typedef self_type* self_pointer_t;
	enum {
		SUCCESS = OsModel::SUCCESS, ERR_UNSPEC = OsModel::ERR_UNSPEC,
		BLOCK_SIZE = blocksize
	};


	/*
	 * Generates
	 */
	VirtualSD() {
		for (int i = 0; i < nrOfBlocks; i++) {
			isWritten[i] = false;
		}
		resetStats();
	}

	/*
	 * Not implemented yet! Who wants this anyways?
	 */
	int erase(address_t start_block, address_t blocks) {
		for(unsigned int i = 0; i < blocks; i++)
		{
			for(unsigned int j = 0; j < blocksize; j++)
				memory[i + start_block][j] = 0;
		}

		return SUCCESS;
	}

	/*
	 * Does nothing, just to comply with the other sd card interface
	 */
	void init()
	{

	}

	/*
	 * Writes some data to a block.
	 * @param x An array of data to be written. The array is assumed to be of size *blocksize*.
	 * @param block the number of the block to write into.
	 */
	int write(block_data_t* x, address_t block) {
		++ios_;
		++blocksWritten_;
		duration_ += 8;
		if (block < 0 || block >= nrOfBlocks) {
			std::cerr << "OVERFLOW VIRTUAL SD" << std::endl;
			return ERR_UNSPEC;
		}
		for (int i = 0; i < blocksize; i++)
		{
			memory[block][i] = x[i];
			isWritten[block] = true;
		}
		return SUCCESS;
	}

	/*
	 * Writes data from a big array into multiple sucessive blocks.
	 * @param x The array of data to be written. The array is assumed to be of size blocks*blocksize.
	 * @start_block The number of the block where to start writing the data.
	 * @blocks The numer of blocks to write into.
	 */
	int write(block_data_t* x, address_t start_block, address_t blocks) {
		++ios_;
		duration_ += 4;

		for (unsigned int i = 0; i < blocks; i++) {
			write(x + blocksize * i, start_block + i);
			duration_ -= 6;
			--ios_;
		}
		return SUCCESS;
	}

	/*
	 * Reads some data from a block and stores it into a buffer.
	 * @param buffer a pointer to a buffer to place the data into.
	 * @param block the block to read from.
	 */
	int read(block_data_t* buffer, address_t block) {
		if (block < 0 || block >= nrOfBlocks) {
			printf("OVERFLOW VIRTUAL SD\n");
			return ERR_UNSPEC;
		}
		++ios_;
		++blocksRead_;
		duration_ += 4;
		//else if(!isWritten[block]) std::cerr << "READING EMPTY BLOCK" << std::endl;

		for (int i = 0; i < blocksize; i++)
			buffer[i] = memory[block][i];
		return SUCCESS;
	}

	/*
	 * Reads data from multiple sucessive blocks at once and stores it into a buffer.
	 * @param buffer The buffer to store the data into. It is assumed to be of lenth length*blocksize
	 * @param block The number of the block where to start reading from.
	 * @param length the number of blocks to read.
	 */
	int read(block_data_t* buffer, address_t block, address_t length) {
		++ios_;
		duration_ += 2;

		for (unsigned int i = 0; i < length; i++) {
			read(buffer + blocksize * i, block + i);
			duration_ -= 2;
			--ios_;
		}
		return SUCCESS;
	}

	/*
	 * Resets all the counters that keep track of the IO's
	 */
	void resetStats() {
		blocksWritten_ = 0;
		blocksRead_ = 0;
		ios_ = 0;
		duration_ = 0;
	}

	/*
	 * Prints usage statistics about the virtual sd card to the console.
	 */
	void printStats() {
		printf("Blocks Written: %d \n", blocksWritten_);
		printf("Blocks Read: %d\n", blocksRead_ );
		printf("IOs: %d\n", ios_);
		printf("Duration: %d\n", duration_);
		printf("AvgIO: %d\n", duration_ / ios_);
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
			for(int j = 0; j < blocksize; j++)
			{
				printf("%3d | ", ((int)memory[block][j]));
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

	void reset()
	{
		for (int unsigned i = 0; i < nrOfBlocks; i++)
			isWritten[i] = false;
		resetStats();

		erase(0, nrOfBlocks);
	}

private:
	block_data_t memory[nrOfBlocks][blocksize];
	bool isWritten[nrOfBlocks];
	int blocksWritten_;
	int blocksRead_;
	int ios_;
	int duration_;

};
} //NAMESPACE
#endif // __VIRTSDCARD_H__

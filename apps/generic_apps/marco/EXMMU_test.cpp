#include "EXMMU_0_4.hpp"
#include "virt_sd.cpp"
#include <stdio.h>
#include <cstring>
#include <string>

int main() {
	typedef char* block_data_t; //TODO make block_data_t a non-pointer type
	
	VirtualSD sd(100);

	bool restoreSuccess = false;
	EXMMU<int, 0, 6, 1, 512> m(&sd, false);
	

	int blocks_to_allocate = 5;
	int b[blocks_to_allocate];

	printf("Stack size = %d\n", m.get_STACK_SIZE());

	//Allocate all
	for (int i = 0; i < blocks_to_allocate; i++) {

		if (m.block_alloc(&b[i])) {
			printf("Allocated virtual block: %d. Its real block number is: %d.\n", b[i], m.vr(b[i]));
		} else {
			b[i] = -999;
			printf("Virtual block could not be allocated.\n");
		}
	}

	//Free all
	for (int i = 0; i < blocks_to_allocate; i++) {

		if (b[i] != -999) {
		
			if (m.block_free(b[i])) {
				printf("Freed virtual block: %d \n", b[i]);
			} else {
				printf("Virtual block %d could not be freed.\n", b[i]);
			}
		} else {
			printf("No virtual block to free.\n");
		}
	}

	//Allocate all
	/*for (int i = 0; i < blocks_to_allocate; i++) {

		if (m.block_alloc(&b[i])) {
			printf("Allocated virtual block: %d. Its real block number is: %d.\n", b[i], m.vr(b[i]));
		} else {
			b[i] = -999;
			printf("Virtual block could not be allocated.\n");
		}
	}*/
	
	/*std::string text = "1000000 1000001 1000002 1000003 1000004 1000005 1000006 1000007 1000008 1000009 1000010 1000011 1000012 1000013 1000014 1000015 1000016 1000017 1000018 1000019 1000020 1000021 1000022 1000023 1000024 1000025 1000026 1000027 1000028 1000029 1000030 1000031 1000032 1000033 1000034 1000035 1000036 1000037 1000038 1000039 1000040 1000041 1000042 1000043 1000044 1000045 1000046 1000047 1000048 1000049 1000050 1000051 1000052 1000053 1000054 1000055 1000056 1000057 1000058 1000059 1000060 1000061 1000062 1000063 1000064 1000065 1000066 1000067 1000068 1000069 1000070 1000071 1000072 1000073 1000074 1000075 1000076 1000077 1000078 1000079 1000080 1000081 1000082 1000083 1000084 1000085 1000086 1000087 1000088 1000089 1000090 1000091 1000092 1000093 1000094 1000095 1000096 1000097 1000098 1000099 1000100 1000101 1000102 1000103 1000104 1000105 1000106 1000107 1000108 1000109 1000110 1000111 1000112 1000113 1000114 1000115 1000116 1000117 1000118 1000119 1000120 1000121 1000122 1000123 1000124 1000125 1000126 ENDMES."; //exactly 1023 chars, which when appended with an EOL makes 1024 bytes
	
	char *buffer_in = new char [text.length()+1];
	char *buffer_out = new char [text.length()+1];
	
  	std::strcpy(buffer_in, text.c_str());
	
	printf("tag1\n");
	for (int i = 0; i < 1024; i++) {
		printf("%c", buffer_in[i]);
	}
	
	printf("Write status = %d\n", m.write(buffer_in, 0, 2));
	printf("Read status = %d\n", m.read(buffer_out, 0, 2));
	
	printf("tag2\n");
	for (int i = 0; i < 1024; i++) {
		printf("%c", buffer_out[i]);
	}*/
	printf("\n");
	
	
	int persistence_test_block1 = 99;
	int persistence_test_block2 = 99;
	m.block_alloc(&persistence_test_block1);
	m.block_alloc(&persistence_test_block2);
	printf("Allocated virtual block %d \n", persistence_test_block1);
	printf("Allocated virtual block %d \n", persistence_test_block2);
	//m.block_free(persistence_test_block1);
	printf("\nDestroying old MMU...\n ");
	m.~EXMMU();
	printf("\n\n\n ------------------- Creating new MMU... -------------------\n\n\n ");
	EXMMU<int, 0, 6, 1, 512> m_res(&sd, true);
	//Allocate all blocks
	for (int i = 0; i < blocks_to_allocate; i++) {

		if (m.block_alloc(&b[i])) {
			printf("Allocated virtual block: %d. Its real block number is: %d.\n", b[i], m.vr(b[i]));
		} else {
			b[i] = -999;
			printf("Virtual block could not be allocated.\n");
		}
	}
	
	m_res.block_free(persistence_test_block2);
	
}

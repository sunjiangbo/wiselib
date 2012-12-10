#include "virt_sd.hpp"

VirtualSD::VirtualSD(int size):size(size){
    memory = new BLOCK[size];
    isWritten = new bool[size];
    for(int i=0; i<size; i++){
	memory[i]= new BYTE[512];
	isWritten[i]=false;
    }
    resetStats();
}

VirtualSD::~VirtualSD(){
    for(int i=0; i<size; i++) free( memory[i]);
    free( memory);
    free(isWritten);
}

void VirtualSD::write(int block, BLOCK x){
    ++ios_;
    ++blocksWritten_;
    duration_+=8;

    if(block<0 || block>=size) std::cerr << "OVERFLOW VIRTUAL SD"<<std::endl;
    else{ for(int i=0; i<512; i++) memory[block][i]=x[i]; isWritten[block]=true;}
}

void VirtualSD::write(int block, BLOCK x, int length){
    ++ios_;
    duration_+=4;

    for(int i=0; i<length; i++){
	write(block+i, x+512*i);
	duration_-=6;
	--ios_;
    }

}

void VirtualSD::read(int block, BLOCK buffer){
    ++ios_;
    ++blocksRead_;
    duration_+=4;
    if(block<0 || block>=size) std::cerr << "OVERFLOW VIRTUAL SD"<< std::endl;
    //else if(!isWritten[block]) std::cerr << "READING EMPTY BLOCK" << std::endl;
    else for(int i=0; i<512; i++) buffer[i]=memory[block][i];
}

void VirtualSD::read(int block, BLOCK buffer, int length){
    ++ios_;
    duration_+=2;

    for(int i=0; i<length; i++){
	read(block+i, buffer+512*i);
	duration_-=2;
	--ios_;
    }
}


void VirtualSD::resetStats(){
    blocksWritten_ = 0;
    blocksRead_=0;
    ios_=0;
    duration_=0;
}


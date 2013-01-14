#include "virt_sd.hpp"
//#include "external_stack2.hpp"
#include "external_stack_item2.hpp"

VirtualSD::VirtualSD(int size):size(size){
    memory = new block_data_t[512*size];
    for(int i=0; i<size; i++) memory[i]=0;
}
VirtualSD::VirtualSD(){
    memory = new block_data_t[512*1000];
   }
VirtualSD::~VirtualSD(){
    free(memory);
}
void VirtualSD::write(block_data_t* buffer, address_t start_block, address_t blocks){
    std::cout << ">>>>>>>>>>>VirtualSD::write("<<"____" <<","<<start_block<<","<<blocks<<")\n";
    for(int i=0; i<blocks*512;i++){
	memory[start_block*512+i]=buffer[i];
    }
    std::cout << ">>>>>>Block Written:\n\n";
    long* castedMem = (long*) &memory[start_block*512];
    for(int i=0; i*sizeof(long)<512; i++){
	std::cout << "  " << castedMem[i];
    }
    std::cout <<"\n<<<<<<<<<<<<<<<<<<\n\n";

}

void VirtualSD::read(block_data_t* buffer, address_t start_block, address_t blocks){
    std::cout << ">>>>>>>>>>VirtualSD::read("<<start_block<<","<<blocks<<")\n";
    for(int i=0; i<blocks*512; i++){
	buffer[i]=memory[start_block*512+i];
    }
}

int main(){
    VirtualSD sd;
    
    ExternalStackItem<long, 10,8> es(&sd, 0,20);
    for(long i=1; i<20; i++){
	es.push(i);
    }
    for(long j=1; j<20; j++){
	long x=-1;
	es.pop(&x);
	std::cout << x <<std::endl;
    }
}
/*
bool test(ExternalStack<long, 3>* es){
    int testvar=1000;
    for(int i=0; i<1000; i++) es->push(i);
    for(int i=1000-1; i>=0; i--){
	long x;
	es->pop(&x);
	if(x!=i){
	    std::cout << ">>>>>>>>>ERROR<<<<<<<<<<<<<\nXXXXXXXXXXXXX\nXXXXXXX";
	    exit(1);
	    return false;
	}
    }
    return true;
}
*//*
int main(){
    ExternalQueue<long, 3> es;
    long write=0;
    long read=0;

	for(int i=10; i<10000; i++){
	    for(int j=0; j<i; j++) {
		if(!es.push(write++)) {
		    std::cout << "QUEUE FULL\n";
		    exit(1);
		}
	    }
	    for(int j=0; j<(i*3)/4; j++) {
		long x=0;
		if(!es.pop(&x)){
		    std::cout <<"?\n";
		    exit(1);
		}
		//std::cout << x <<std::endl;
		if(x!=read++) {
		    std::cout<<"error"<<x<<" " <<read-1<<std::endl;
		    exit(1);
		}
	    }
	}



    */
    //for(int i=0; i<1000; i++) es.push(i);
    /*for(int i=0; i<1000; i++) {
	if(!test(&es)) std::cout << ">>>>>>>>>>>>> ERROR\n";
	long x;
	es.pop(&x);
    }*//*
    std::cout << "END\n";
}*/







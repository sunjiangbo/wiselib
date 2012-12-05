//#include <unistd>
#include <iostream>
#include <time.h>
#include "sd_card.hpp"
#include <unistd.h>
#include <string.h>
#include "virt_sd.hpp"
#include "wstack.hpp"
#include "wqueue.hpp"
using namespace std;
int main(){
    VirtualSD sd(100);
    ext_Stack<int> s(10,99,&sd);
    int ia[] = {4,2,6,4,8,3,6,1,9,7,3,6,2,8,8,1,2,3,4,5,6,7,8,0};
    int io[25]; io[24]=0;
    s.push(ia, 24);
    s.pop(io,24);
    for(int i=0; ia[i]!=0; i++){
	std::cout << io[i]<<" ";

    }
    std::cout << std::endl;
    sd.printStats();
sd.resetStats();
    ext_Queue<int> eq(0,90,&sd);
    for(int i=0; i<30; i++){
    
	eq.push(ia, 20);
    }
    eq.poll(io,20);
    for(int i=0; ia[i]!=0; i++){
	std::cout << io[i]<<" ";

    }
    std::cout << std::endl;
    sd.printStats();

}

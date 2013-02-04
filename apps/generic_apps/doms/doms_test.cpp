#include <external_interface/external_interface.h>
#include <external_interface/arduino/arduino_sdcard.h>
#include <external_interface/arduino/arduino_debug.h>
#include <external_interface/arduino/arduino_clock.h>
#include "external_stack.hpp"
#include "external_queue.hpp"
#include <stdlib.h>
//#define RELOAD
//15011034
typedef wiselib::OSMODEL Os;

class App {
    public:

	// NOTE: this uses up a *lot* of RAM, way too much for uno!

	void init(Os::AppMainParameter& value) {
	    debug_ = &wiselib::FacetProvider<Os, Os::Debug>::get_facet( value );
	    clock_ = &wiselib::FacetProvider<Os, Os::Clock>::get_facet(value);

	    debug_->debug( "SD Card test application running" );
	    sd_ = &wiselib::FacetProvider<Os, Os::BlockMemory>::get_facet(value);
	    sd_->init();
	    srand(8);
	//	test_queue<2>();
	   /* 
	    for(uint64_t i=1;i<=100000; i*=10){
		debug_->debug(">>>> %d", i);
		debug_->debug("B: 1");
	    test_stack<1>(i);
		debug_->debug("B: 2");
	    test_stack<2>(i);
		debug_->debug("B: 3");
	    test_stack<3>(i);
		debug_->debug("B: 4");
	    test_stack<4>(i);
	    }*/
	    debug_->debug("startTest");
	    test_queue<2>(100000);
	    for(uint64_t j=1; j<=100000; j*=10){
		debug_->debug(">>>> %d",j);
		debug_->debug("B: 2");
		test_queue<2>(j);
		debug_->debug("B: 3");
		test_queue<3>(j);
		debug_->debug("B: 4");
		test_queue<4>(j);
	    }
	    
	    /*
	       test_stack<2>(80);
	       debug_->debug("Start Buffersize %d, Ratio %d/100",2,90);
	       test_queue<2>(90);
	       debug_->debug("Start Buffersize %d, Ratio %d/100",3,60);
	       test_queue<3>(60);
	       debug_->debug("End Testing");*/
	    //test_queue<4>(60);
	    //test_queue<3>(90);

	    debug_->debug("Start TimeTest Stack");
	    time_stack<2>();
	    time_stack<3>();
	    time_stack<4>();

	    debug_->debug("Start TimeTest queue");
	    time_queue<2>();

	}
	template<uint32_t buffersize>
	    void time_stack(){
		ExternalStack<uint64_t, buffersize> es(sd_,1,100000,true);
		uint64_t cz=0;
		uint64_t tmp=0;
		int d=0;
		uint64_t j=0;

		uint64_t countPushed=0, countPoped=0;
		for(uint64_t i=0;i<100;i++){
		    Os::Clock::time_t start = clock_->time();
		    for(j=0; j<1000; j++){
			d = rand()%100;
			if(d<100-i){
			    if(es.push(rand()%1000)) countPushed+=1;

			} else {
			    if(es.pop(&tmp)) countPoped+=1;
			}
		    }
		    Os::Clock::time_t stop = clock_->time() - start;
		    cz+=stop;
		  //  debug_->debug("10000 IOs in %d",stop);
		}

		    sd_->stat();
		debug_->debug("TIME: %d",cz);
		debug_->debug("PUSHED: %u",countPushed);
		debug_->debug("POPED: %u",countPoped);
	    }

	template<uint32_t buffersize>
	    void time_queue(){
		ExternalQueue<uint64_t, buffersize> eq(sd_,1,100000,true);
		uint64_t cz=0;
		uint64_t tmp=0;
		int d=0;
		uint64_t j =0;

		uint64_t countPushed=0, countPoped=0;
		for(uint64_t i=0; i<100;i++){
		    Os::Clock::time_t start = clock_->time();
		    for(j=0;j<1000;j++){
			d = rand()%100;
			if(d<100-i){
			    if(eq.offer(rand()%1000)) countPushed+=1;
			}else {
			    if(eq.poll(&tmp)) countPoped+=1;
			}
		    }
		    Os::Clock::time_t stop = clock_->time() - start;
		    cz+=stop;
		    //debug_->debug("10000 IOs in %d",stop);

		}

		    sd_->stat();
		debug_->debug("TIME: %d",cz);
		debug_->debug("PUSHED: %u",countPushed);
		debug_->debug("POPED: %u",countPoped);

	    }

	template<int buffersize>
	    void test_stack(uint64_t perrun){
		debug_->debug("Testing Stack");
		ExternalStack<uint64_t, buffersize> testA(sd_,10001,20000,true);
		ExternalStack<uint64_t, buffersize> testB(sd_,20001,30000,true);
		{
		    ExternalStack<uint64_t, buffersize> es(sd_,1,3,true);
		    if(es.size()!=0){
			debug_->debug("wrong size");
			exit(1);
		    }
		    if(!es.isEmpty()){
			debug_->debug("empty wrong");
			exit(1);
		    }
		    es.~ExternalStack();
		}
		bool toA=true;


		uint64_t inStack=0;
		uint64_t poped=0;
		uint64_t tmp=0;
		uint64_t toPush=0;

		for(uint64_t i=0; i<=101; i++){
		    ExternalStack<uint64_t, buffersize> es(sd_,1,10000);
		    for(uint64_t j=0; j<perrun; j++){
			int d = rand() %100;
			toPush = (rand()%1000);
			if(d<100-i){
			    if(es.push(toPush)){
				inStack+=1;
				if(toA){
				    testA.push(toPush);
				    toA=false;
				} else {
				    testB.push(toPush);
				    toA=true;
				}
			    } 
			} else {
			    bool succ = es.pop(&poped);
			    if((!succ && inStack>0) || (succ && inStack<=0)){
				debug_->debug("Wrong BOOL %d",succ);
				debug_->debug("bool %d",succ);
				debug_->debug("inStack %d",inStack);
				exit(1);
			    }
			    if(succ){
				if(toA){
				    toA=false;
				    testB.pop(&tmp);
				} else {
				    toA=true;
				    testA.pop(&tmp);
				}

				if(tmp!=poped){
				    debug_->debug("Wrong POP %u popped",poped);
				    debug_->debug(">> %u should be",tmp);
				    debug_->debug(">>> %u",tmp!=poped);
				    debug_->debug(">> size: %u",es.size());
				    debug_->debug(">> should be: %u",inStack);
				    exit(1);
				}
				inStack-=1;
				//debug_->debug("Poped: %d",poped);
			    } 

			}
			if((es.isEmpty() && inStack>0) || (!es.isEmpty()&& inStack<=0)){
			    debug_->debug("isEmpty()");
			    exit(1);
			}
			if(es.size()!=inStack){
			    debug_->debug("false size %d,%d",inStack,es.size());
			    debug_->debug("%d",es.size());
			    debug_->debug("%d",inStack);
			    exit(1);
			}
		    }
		    //  es.~ExternalStack();

		    debug_->debug(">>%d ",i);

		}
		debug_->debug("End testing Stack");
	    }
	template<int BUFFERSIZE>
	    void test_queue(uint64_t perrun){

		debug_->debug("Testing Queue");
		{
		    ExternalQueue<uint64_t, 2> eq(sd_,1,10);
		    if(eq.size()!=0){
			debug_->debug("wrong size");
			exit(1);
		    }
		    if(!eq.isEmpty()){
			debug_->debug("empty wrong");
			exit(1);
		    }

		    eq.~ExternalQueue();
		}
		sd_->stat();

		//ExternalQueue<uint64_t, BUFFERSIZE> eq(sd_,1,10000);
#ifndef RELOAD
		ExternalQueue<uint64_t, BUFFERSIZE> eq(sd_,1,1000000);
#endif
		uint64_t push=0;
		uint64_t pop=0;
		uint64_t inQ=0;
		uint64_t tmp=0;
		uint64_t xpop=0;
		uint64_t xpush=0;
		for(uint64_t i=0; i<=101; i++){
		    xpop = 0;
		    xpush = 0;
#ifdef RELOAD
		    ExternalQueue<uint64_t, BUFFERSIZE> eq(sd_,1,10000);
#endif
		    for(uint64_t j=0; j<perrun; j++){
			int d = rand()%100;
			if(d<100-i){
			    bool succ =eq.offer(push);
			    if(succ){
				//debug_->debug("PUSHED %d.",push);
				push++;
				inQ++;
				xpush+=1;
			    }
			} else {
			    bool succ =eq.poll(&tmp);
			    if((!succ && inQ>0) || (succ && inQ<=0)){
				debug_->debug("BOO ERROR");
				exit(1);
			    }
			    if(succ){
				//	debug_->debug("POPED %d!",tmp);
				inQ--;
				if(tmp!=pop){
				    debug_->debug("POP ERROR read: %u Failed",tmp);
				    debug_->debug("read %u",pop);
				    debug_->debug(">> %u",pop!=tmp);
				    debug_->debug(">> %u",i);
				    debug_->debug(">> %u",j);
				    debug_->debug(">> %u",inQ);
				    debug_->debug(">> %u", eq.size());

				    exit(1);
				}
				//debug_->debug("POPED %d. In QUEUE %d.",tmp,inQ);
				pop++;
				xpop++;
			    }
			}
			if((eq.isEmpty() && inQ>0) || (!eq.isEmpty()&& inQ<=0)){
			    debug_->debug("isEmpty()");
			    exit(1);
			}
			if(inQ!= eq.size()){
			    debug_->debug("wrong size %d,%d",inQ,eq.size());
			    exit(1);
			}
		    }

		    // eq.~ExternalQueue();
		    /*
		    debug_->debug("%d0000 IOs: ",i);
		    debug_->debug("pushed %d Elements", xpush);
		    debug_->debug("poped %d Elements", xpop);
		    debug_->debug("Remaining %u Elements in Queue",inQ);
		    sd_->stat();
		    */
		}


	    }

    private:
	//static Os::Debug dbg;
	Os::Debug::self_pointer_t debug_;
	Os::Clock::self_pointer_t clock_;


	Os::BlockMemory::self_pointer_t sd_;
};

//Os::Debug App::dbg = Os::Debug();

wiselib::WiselibApplication<Os, App> app;
void application_main( Os::AppMainParameter& value ) {
    app.init(value);
}

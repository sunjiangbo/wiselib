#define USE_RAM_BLOCK_MEMORY 1
#include <external_interface/external_interface.h>
#define INFO
#include "external_stack.hpp"
#include "external_queue.hpp"
#define RELOAD
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
	    srand(9);/*
	    for(uint32_t i=1; i<100000; i*=10){
		test_stack<1>(i);
		test_stack<2>(i);
		test_stack<3>(i);
		test_stack<4>(i);
		test_stack<5>(i);
		test_stack<6>(i);
		test_stack<10>(i);
		test_queue<2>(i);
		test_queue<3>(i);
		test_queue<4>(i);
		test_queue<5>(i);
		test_queue<6>(i);
		test_queue<7>(i);
		test_queue<8>(i);
		test_queue<12>(i);

	    }*/
	    test_stack<1>(100);
	    test_queue<2>(100);
	    test_stack<2>(1000);
	    test_stack<2>(10000);
	    test_stack<2>(100000);
	    exit(0);
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
	    test_queue<4>(100000);
	    exit(0);
	    test_stack<1>(100000);
	    test_stack<2>(100000);
	    return;
	    test_queue<2>(100000);
	    //test_queue<2>(100000);
	    for(uint64_t j=1; j<=100000; j*=10){
		debug_->debug(">>>> %d",j);
		debug_->debug("B: 2");
		test_queue<2>(j);
		debug_->debug("B: 3");
		test_queue<3>(j);
		debug_->debug("B: 4");
		test_queue<4>(j);
	    }



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
			    if(es.push(toPush)==Os::SUCCESS){
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
			    int succ = es.pop(&poped);
			    if((succ!=Os::SUCCESS && inStack>0) || (succ==Os::SUCCESS && inStack<=0)){
				debug_->debug("Wrong BOOL %d",succ);
				debug_->debug("bool %d",succ);
				debug_->debug("inStack %d",inStack);
				exit(1);
			    }
			    if(succ==Os::SUCCESS){
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
				    debug_->debug(">>>> j = %u0000",j/10000);
				    debug_->debug("         +u",j%10000);
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
			    debug_->debug("%u",es.size());
			    debug_->debug("%u",inStack);
			    exit(1);
			}


		    }
		    //  es.~ExternalStack();

		    debug_->debug(">>%d ",i);

		}
		    ExternalStack<uint64_t, buffersize> es(sd_,1,10000);
		while(!es.isEmpty()){
		    int succ = es.pop(&poped);
		    if((succ!=Os::SUCCESS && inStack>0) || (succ==Os::SUCCESS && inStack<=0)){
			debug_->debug("Wrong BOOL %d",succ);
			debug_->debug("bool %d",succ);
			debug_->debug("inStack %d",inStack);
			exit(1);
		    }
		    if(succ==Os::SUCCESS){
			if(toA){
			    toA=false;
			    testB.pop(&tmp);
			} else {
			    toA=true;
			    testA.pop(&tmp);
			}

			if(tmp!=poped){
			    debug_->debug("wrong pop stack");
			    exit(1);
			}
			inStack-=1;
			//debug_->debug("Poped: %d",poped);
		    }
		    if((es.isEmpty() && inStack>0) || (!es.isEmpty()&& inStack<=0)){
			debug_->debug("isEmpty()");
			exit(1);
		    }
		    if(es.size()!=inStack){
			debug_->debug("false size %d,%d",inStack,es.size());
			debug_->debug("%u",es.size());
			debug_->debug("%u",inStack);
			exit(1);
		    }

		}
		if(inStack>0){debug_->debug("Should be empty stack"); exit(1);}
		debug_->debug("End testing Stack");
	    }
	template<int BUFFERSIZE>
	    void test_queue(uint64_t perrun){

		debug_->debug("Testing Queue");
		{
		    ExternalQueue<uint64_t, 2> eq(sd_,0,10);
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

		//ExternalQueue<uint64_t, BUFFERSIZE> eq(sd_,1,10000);
#ifndef RELOAD
		ExternalQueue<uint64_t, BUFFERSIZE> eq(sd_,0,2000,true);
#endif
		uint64_t push=0;
		uint64_t pop=0;
		uint64_t inQ=0;
		uint64_t tmp=0;
		uint64_t xpop=0;
		uint64_t xpush=0;
		uint64_t tmpp=0;
		for(uint64_t i=0; i<=101; i++){
		    debug_->debug(">>> RUN >>>>> %u",i);
		    xpop = 0;
		    xpush = 0;
#ifdef RELOAD
		    ExternalQueue<uint64_t, BUFFERSIZE> eq(sd_,0,2000);
#endif
		    for(uint64_t j=0; j<perrun; j++){
			int d = rand()%100;
			if(d<100-i){
			    int succ =eq.offer(push);
			    if(succ==Os::SUCCESS){
				//debug_->debug("PUSHED %d.",push);
				push++;
				inQ++;
				xpush+=1;
			    }
			} else {
			    eq.peek(&tmpp);
			    int succ =eq.poll(&tmp);
			    if((succ!=Os::SUCCESS && inQ>0) || (succ==Os::SUCCESS && inQ<=0)){
				debug_->debug("BOO ERROR");
				exit(1);
			    }
			    if(succ==Os::SUCCESS){
				//	debug_->debug("POPED %d!",tmp);
				inQ--;
				if(tmp!=pop || tmpp!=pop){
				    debug_->debug("POP ERROR read: %u Failed",tmp);
				    debug_->debug("read %u",pop);
				    debug_->debug(">> %u",pop!=tmp);
				    debug_->debug(">> %u",i);
				    debug_->debug(">> %u00",j/100);
				    debug_->debug("   +%u",j%100);
				    debug_->debug(">> inQ= %u0000",inQ/10000);
				    debug_->debug("   +%u",inQ%10000);
				    debug_->debug(">> modSize %u", eq.size()%1000);
				    debug_->debug(">>>> tmp %u",tmp/10000);
				    debug_->debug(">>>>     +%u", tmp%10000);
				    debug_->debug(">>>>>>pop %u", pop/10000);
				    debug_->debug(">>>>>>    +%u", pop%10000);

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
			    debug_->debug("wrong size");
			    debug_->debug("is %u",eq.size());
			    debug_->debug("should be %u", inQ);
			    debug_->debug(">> %u", push);
			    debug_->debug(">> %u", pop);
			    exit(1);
			}
		    }
		    eq.flush();
		    debug_->debug(">>>> RUNTHROUGH %u",perrun/10000);

		    // eq.~ExternalQueue();
		    /*
		       debug_->debug("%d0000 IOs: ",i);
		       debug_->debug("pushed %d Elements", xpush);
		       debug_->debug("poped %d Elements", xpop);
		       debug_->debug("Remaining %u Elements in Queue",inQ);
		       sd_->stat();
		     */
		}
#ifndef RELOAD
		while(!eq.isEmpty()){
		    int succ =eq.poll(&tmp);
		    if((succ!=Os::SUCCESS && inQ>0) || (succ==Os::SUCCESS && inQ<=0)){
			debug_->debug("BOO ERROR");
			exit(1);
		    }
		    if(succ==Os::SUCCESS){
			//	debug_->debug("POPED %d!",tmp);
			inQ--;
			if(tmp!=pop){
			    debug_->debug("POP ERROR read: %u Failed",tmp);
			    debug_->debug("read %u",pop);
			    debug_->debug(">> %u",pop!=tmp);
			    debug_->debug(">> inQ= %u0000",inQ/10000);
			    debug_->debug("   +%u",inQ%10000);
			    debug_->debug(">> modSize %u", eq.size()%1000);
			    debug_->debug(">>>> tmp %u",tmp/10000);
			    debug_->debug(">>>>     +%u", tmp%10000);
			    debug_->debug(">>>>>>pop %u", pop/10000);
			    debug_->debug(">>>>>>    +%u", pop%10000);

			    exit(1);
			}
			//debug_->debug("POPED %d. In QUEUE %d.",tmp,inQ);
			pop++;
			xpop++;
		    }

		}
		if(inQ>0) { debug_->debug("Should be empty queue"); exit(1);}
#endif


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

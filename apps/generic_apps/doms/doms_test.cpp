#include <external_interface/external_interface.h>
#include <external_interface/arduino/arduino_sdcard.h>
#include <external_interface/arduino/arduino_debug.h>
#include <external_interface/arduino/arduino_clock.h>
#include "external_stack.hpp"
#include "external_queue.hpp"
#include <stdlib.h>
//14011050
typedef wiselib::OSMODEL Os;

class App {
    public:

	// NOTE: this uses up a *lot* of RAM, way too much for uno!

	void init(Os::AppMainParameter& value) {
	    debug_ = &wiselib::FacetProvider<Os, Os::Debug>::get_facet( value );
	    clock_ = &wiselib::FacetProvider<Os, Os::Clock>::get_facet(value);

	    debug_->debug( "SD Card test application running" );
	    sd_.init();
	    srand(6);

	    //test_stack<3>(90);
	    test_stack<2>(80);
	    debug_->debug("Start Buffersize %d, Ratio %d/100",2,90);
	    test_queue<2>(90);
	    debug_->debug("Start Buffersize %d, Ratio %d/100",3,60);
	    test_queue<3>(60);
	    debug_->debug("End Testing");	
	}
	template<int buffersize>
	    void test_stack(int writeRat){
		debug_->debug("Testing Stack");
		ExternalStack<uint64_t, buffersize,false> testA(&sd_,10001,20000,true);
		ExternalStack<uint64_t, buffersize,false> testB(&sd_,20001,30000,true);
		{
		    ExternalStack<uint64_t, buffersize> es(&sd_,1,3,true);
		    es.~ExternalStack();
		}
		bool toA=true;


		uint64_t inStack=0;
		uint64_t poped=0;
		uint64_t tmp=0;
		uint64_t toPush=0;

		for(uint64_t i=0; i<100; i++){
		    ExternalStack<uint64_t, buffersize, true,true> es(&sd_,1,10000);
		    for(uint64_t j=0; j<10000; j++){
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
			    } else {
				debug_->debug("unsucc push");
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
				    debug_->debug("Wrong POP %d %d ",poped,tmp);
				    exit(1);
				}
				inStack-=1;
				//debug_->debug("Poped: %d",poped);
			    } else {
				debug_->debug("unsucc POP");
			    }
			}
		    }
		  //  es.~ExternalStack();

		    debug_->debug(">>%d0000 ",i);

		}
		debug_->debug("End testing Stack");
	    }
	template<int BUFFERSIZE>
	    void test_queue(int writeRat){

		debug_->debug("Testing Queue");
		{
		    ExternalQueue<uint64_t, 2> eq(&sd_,1,10);
		    eq.~ExternalQueue();
		}

		//ExternalQueue<uint64_t, BUFFERSIZE> eq(&sd_,1,10000);
		uint64_t push=0;
		uint64_t pop=0;
		uint64_t inQ=0;
		uint64_t tmp=0;
		uint64_t xpop=0;
		uint64_t xpush=0;
		for(uint64_t i=0; i<100; i++){
		    xpop = 0;
		    xpush = 0;

		    ExternalQueue<uint64_t, BUFFERSIZE> eq(&sd_,1,10000);
		    for(uint64_t j=0; j<10000; j++){
			int d = rand()%100;
			if(d<writeRat){
			    bool succ =eq.push(push);
			    if(succ){
				//debug_->debug("PUSHED %d.",push);
				push++;
				inQ++;
				xpush+=1;
			    }
			} else {
			    bool succ =eq.pop(&tmp);
			    if((!succ && inQ>0) || (succ && inQ<=0)){
				debug_->debug("BOO ERROR");
				exit(1);
			    }
			    if(succ){
				//	debug_->debug("POPED %d!",tmp);
				inQ--;
				if(tmp!=pop){
				    debug_->debug("POP ERROR read: %d, Excpected: %d Failed",tmp,pop);
				    exit(1);
				}
				//debug_->debug("POPED %d. In QUEUE %d.",tmp,inQ);
				pop++;
				xpop++;
			    }

			}
		    }

		    // eq.~ExternalQueue();
		    debug_->debug("%d0000 IOs: pushed %d , poped %d , in Queue %d ",i,xpush,xpop, inQ);
		}


	    }

    private:
	//static Os::Debug dbg;
	Os::Debug::self_pointer_t debug_;
	Os::Clock::self_pointer_t clock_;


	wiselib::ArduinoSdCard<Os> sd_;
};

//Os::Debug App::dbg = Os::Debug();

wiselib::WiselibApplication<Os, App> app;
void application_main( Os::AppMainParameter& value ) {
    app.init(value);
}

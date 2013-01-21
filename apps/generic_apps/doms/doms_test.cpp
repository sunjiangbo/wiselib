#include <external_interface/external_interface.h>
#include <external_interface/arduino/arduino_sdcard.h>
#include <external_interface/arduino/arduino_debug.h>
#include <external_interface/arduino/arduino_clock.h>
#include "external_stack.hpp"
#include "external_queue.hpp"
#include <stdlib.h>
//15011034
typedef wiselib::OSMODEL Os;

class App {
    public:

	// NOTE: this uses up a *lot* of RAM, way too much for uno!

	void init(Os::AppMainParameter& value) {
	    debug_ = &wiselib::FacetProvider<Os, Os::Debug>::get_facet( value );
	    clock_ = &wiselib::FacetProvider<Os, Os::Clock>::get_facet(value);

	    debug_->debug( "SD Card test application running" );
	    sd_.init();
	    srand(7);

	    //test_stack<3>(90);
	    /*
	       test_stack<2>(80);
	       debug_->debug("Start Buffersize %d, Ratio %d/100",2,90);
	       test_queue<2>(90);
	       debug_->debug("Start Buffersize %d, Ratio %d/100",3,60);
	       test_queue<3>(60);
	       debug_->debug("End Testing");*/
	    test_queue<2>(60);
	    test_stack<2>(60);
	    
	    test_stack<1>(60);

	    debug_->debug("Start TimeTest Stack");
	    time_stack<1>();
	    time_stack<2>();
	    time_stack<3>();
	    time_stack<4>();

	    debug_->debug("Start TimeTest queue");
	    time_queue<2>();
	    time_queue<3>();
	    time_queue<4>();

	}
	template<uint32_t buffersize>
	    void time_stack(){
		ExternalStack<uint64_t, buffersize> es(&sd_,1,100000,true);
		uint64_t cz=0;
		uint64_t tmp=0;
		int d=0;
		uint64_t j=0;
		es.isEmpty();
		for(uint64_t i=0;i<100;i++){
		    Os::Clock::time_t start = clock_->time();
		    for(j=0; j<1000; j++){
			d = rand()%100;
			if(d<100-i){
			    es.push(rand()%1000);
			} else {
			    es.pop(&tmp);
			}
			es.size();
			es.isEmpty();
		    }
		    Os::Clock::time_t stop = clock_->time() - start;
		    cz+=stop;
		  //  debug_->debug("10000 IOs in %d",stop);
		   // sd_.stat();
		}

		debug_->debug("TIME: %d",cz);
	    }

	template<uint32_t buffersize>
	    void time_queue(){
		ExternalQueue<int, buffersize> eq(&sd_,1,100000,true);
		uint64_t cz=0;
		int tmp=0;
		int d=0;
		uint64_t j =0;
		for(uint64_t i=0; i<100;i++){
		    Os::Clock::time_t start = clock_->time();
		    for(j=0;j<1000;j++){
			d = rand()%100;
			if(d<100-i){
			    eq.offer(rand()%1000);
			}else {
			    eq.poll(&tmp);
			}
		    }
		    Os::Clock::time_t stop = clock_->time() - start;
		    cz+=stop;
		    //debug_->debug("10000 IOs in %d",stop);
		    //sd_.stat();

		}

		debug_->debug("TIME: %d",cz);
	    }

	template<int buffersize>
	    void test_stack(int writeRat){
		debug_->debug("Testing Stack");
		ExternalStack<uint64_t, buffersize> testA(&sd_,10001,20000,true);
		ExternalStack<uint64_t, buffersize> testB(&sd_,20001,30000,true);
		{
		    ExternalStack<uint64_t, buffersize> es(&sd_,1,3,true);
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

		for(uint64_t i=0; i<=100; i++){
		    ExternalStack<uint64_t, buffersize> es(&sd_,1,10000);
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

		    debug_->debug(">>%d0000 ",i);

		}
		debug_->debug("End testing Stack");
	    }
	template<int BUFFERSIZE>
	    void test_queue(int writeRat){

		debug_->debug("Testing Queue");
		{
		    ExternalQueue<uint64_t, 2> eq(&sd_,1,10);
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

		//ExternalQueue<uint64_t, BUFFERSIZE> eq(&sd_,1,10000);
		uint64_t push=0;
		uint64_t pop=0;
		uint64_t inQ=0;
		uint64_t tmp=0;
		uint64_t xpop=0;
		uint64_t xpush=0;
		for(uint64_t i=0; i<=100; i++){
		    xpop = 0;
		    xpush = 0;

		    ExternalQueue<uint64_t, BUFFERSIZE> eq(&sd_,1,10000);
		    for(uint64_t j=0; j<10000; j++){
			int d = rand()%100;
			if(d<writeRat){
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
				    debug_->debug("POP ERROR read: %d, Excpected: %d Failed",tmp,pop);
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

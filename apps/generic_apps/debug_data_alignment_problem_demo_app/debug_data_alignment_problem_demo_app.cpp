#include <external_interface/external_interface.h>

typedef wiselib::OSMODEL OsModel;

class DebugAlignmentProblemApp {
	
	public:
		typedef typename OsModel::size_t size_t;
		
	private: 
		typedef typename OsModel::Debug::self_pointer_t Debug_ptr;
		
	public:	
		DebugAlignmentProblemApp() {}
		
		void setDebug(Debug_ptr debug_) { this->debug_ = debug_; }
		
		void test() {
		
			debug_->debug("sizeof(size_t) = %d", sizeof(size_t));
			debug_->debug("sizeof(int) = %d", sizeof(int));
			debug_->debug("sizeof(bool) = %d", sizeof(bool));
		
			debug_->debug("On some platforms the following output lines match up, and on some they do not!");
		
			/* This is due to the fact that the placeholders á la printf() used in debug(),
			e.g. %d, expects an int value, whatever "int" may mean on the platform. If you
			for example pass a uint32_t into the debug function, then on a platform where
			int is 4 bytes large you will not have any issues, but a platform where in is
			only 2 Bytes large, and it thus expects a 16-bit value on the stack, but a 
			32-bit value was written instead, you may get allignment problems depending
			on padding, which can result in invalid output.*/
	
			int foo_0 = 42;
			bool bar_0 = 1;
			int blah_0 = 42;
			debug_->debug("foo_0 = %d, bar_0 = %d, blah_0 = %d", foo_0, bar_0, blah_0);
			
			/*Specifically this case causes trouble when compiling for Arduino for example 
			where int is a 16-bit Datatype.*/
			size_t foo_1 = 42;
			bool bar_1 = 1;
			int blah_1 = 42;
			debug_->debug("foo_1 = %d, bar_1 = %d, blah_1 = %d", foo_1, bar_1, blah_1);
			
			/*Casting as done here doesn't really present a viable solution though because 
			(quite obviously) a larger datatype (e.g. 4-Byte) can't always be casted to a
			smaller datatype (e.g. 2-Byte) without changing the actual value. */
			size_t foo_2 = 42;
			bool bar_2 = 1;
			int blah_2 = 42;
			debug_->debug("foo_2 = %d, bar_2 = %d, blah_2 = %d", (int) foo_2, (int) bar_2, (int) blah_2);
			
		}
		
		void init( OsModel::AppMainParameter& value ) {
			debug_ = &wiselib::FacetProvider<OsModel, OsModel::Debug>::get_facet( value );
			this->setDebug(debug_);
			this->test();
			exit(0);
		}
	
	private:
		Debug_ptr debug_;
		
		
};

// --------------------------------------------------------------------------
wiselib::WiselibApplication<OsModel, DebugAlignmentProblemApp> alignment_test;
// --------------------------------------------------------------------------
void application_main( OsModel::AppMainParameter& value )
{
	alignment_test.init( value );
}

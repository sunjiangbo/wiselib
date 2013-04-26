#define PC_USE_RAM_BLOCK_MEMORY 1

#ifdef ISENSE
	#define DEBUG_ISENSE 1
	extern "C" { void assert(int) { } }
	#define DEBUG_GRAPHVIZ 0
	#define DEBUG_OSTREAM 0
	#define BITMAP_CHUNK_ALLOCATOR_CHECK 0
	#define B_PLUS_TREE_CHECK 0
	
	#include <isense/util/get_os.h>
	
	//#define DBG(format, ...) GET_OS.debug(format, __VA_ARGS__)
#else
	#define DEBUG_GRAPHVIZ 0
	#define DEBUG_OSTREAM 1
	#define BITMAP_CHUNK_ALLOCATOR_CHECK 1
	#define B_PLUS_TREE_CHECK 1
#endif
	

// Tell iSense to use an external sd card for block memory
#define USE_SDCARD 1

#include "external_interface/external_interface.h"
#ifdef ISENSE
	void* malloc(size_t n) { return isense::malloc(n); }
	void free(void* p) { isense::free(p); }
#endif
	
#include <algorithms/block_memory/bitmap_chunk_allocator.h>
#include <algorithms/block_memory/cached_block_memory.h>
#include <algorithms/block_memory/b_plus_tree.h>
#include <algorithms/block_memory/b_plus_dictionary.h>
#include <algorithms/block_memory/b_plus_hash_set.h>
#include <util/tuple_store/tuplestore.h>
#include <algorithms/hash/fnv.h>
#include <util/pstl/vector_static.h>
//#include <util/pstl/set_map.h>

typedef wiselib::OSMODEL Os;
typedef Os::size_t size_type;
typedef Os::block_data_t block_data_t;

//typedef wiselib::SimpleDefragBlockAllocator<Os, Os::BlockMemory> Mem;
typedef wiselib::BitmapChunkAllocator<Os, Os::BlockMemory, 4> AllocatedMem;
typedef wiselib::CachedBlockMemory<Os, AllocatedMem, 10> CachedMem;
typedef CachedMem Mem;

typedef wiselib::BPlusTree<Os, Mem, ::uint32_t, ::uint32_t> Tree;
//typedef wiselib::BitmapChunkAllocator<Os, Os::BlockMemory, 4> AMem;

unsigned long high;
size_type stacksize() {
	unsigned long low;
	low = (unsigned long)&low;
	return high - low;
}


class Tuple {
	// {{{
	public:
		enum {
			SIZE = 3
		};
		typedef Tuple self_type;
		
		Tuple() {
			for(size_type i = 0; i < SIZE; i++) {
				spo_[i] = 0;
			}
		}
		
		/*
		Tuple(int x) {
			block_data_t *bdt = reinterpret_cast<block_data_t*>(x);
			for(size_type i = 0; i < SIZE; i++) {
				spo_[i] = bdt;
			}
		}
		*/
		
		void free_deep(size_type i) {
			//delete[] spo_[i];
			free(spo_[i]);
			spo_[i] = 0;
		}
		void destruct_deep() {
			for(size_type i = 0; i < SIZE; i++) { free_deep(i); }
		}
		
		// operator= as default
		
		block_data_t* get(size_type i) {
			return spo_[i];
		}
		size_type length(size_type i) { return spo_[i] ? strlen((char*)spo_[i]) : 0; }
		void set(size_type i, block_data_t* data) {
			spo_[i] = data;
		}
		void set_deep(size_type i, block_data_t* data) {
			size_type l = strlen((char*)data) + 1;
			//spo_[i] = new block_data_t[l];
			spo_[i] = (block_data_t*)malloc(l * sizeof(block_data_t));
			memcpy(spo_[i], data, l);
		}
		
		static int compare(int col, ::uint8_t *a, int alen, ::uint8_t *b, int blen) {
			if(alen != blen) { return (int)blen - (int)alen; }
			for(int i = 0; i < alen; i++) {
				if(a[i] != b[i]) { return (int)b[i] - (int)a[i]; }
			}
			return 0;
		}
		
		// comparison ops are only necessary for some tuple container types.
	
		// note that this comparasion function
		// -- in contrast to compare() -- has to assume that spo_[i] might
		// contain a dictionary key instead of a pointer to an actual value
		// and thus compares differently.
		int cmp(const self_type& other) const {
			for(size_type i = 0; i < SIZE; i++) {
				if(spo_[i] != other.spo_[i]) {
					return spo_[i] < other.spo_[i] ? -1 : (spo_[i] > other.spo_[i]);
				}
			}
			return 0;
		}
		
		bool operator==(const self_type& other) const { return cmp(other) == 0; }
		bool operator>(const self_type& other) const { return cmp(other) > 0; }
		bool operator<(const self_type& other) const { return cmp(other) < 0; }
		bool operator>=(const self_type& other) const { return cmp(other) >= 0; }
		bool operator<=(const self_type& other) const { return cmp(other) <= 0; }
		
#if (DEBUG_GRAPHVIZ || DEBUG_OSTREAM)
		friend std::ostream& operator<<(std::ostream& os, const Tuple& t) {
			os << "(" << (void*)t.spo_[0] << " " << (void*)t.spo_[1] << " " << (void*)t.spo_[2] << ")";
			return os;
		}
#endif
		
	private:
		block_data_t *spo_[SIZE];
		//char spo_[100][3];
	// }}}
};

#define COL(X) (1 << (X))
//typedef wiselib::vector_static<Os, Tuple, 1000> TupleContainer;
typedef wiselib::BPlusTree<Os, Mem, Tuple, bool> TupleBPT;
//typedef wiselib::SetMap<Os, TupleBPT> TupleContainer;
typedef wiselib::BPlusHashSet<Os, Mem, wiselib::Fnv32<Os>, Tuple> TupleContainer;

typedef wiselib::BPlusDictionary<Os, Mem, wiselib::Fnv32<Os> > TupleDictionary;
typedef wiselib::TupleStore<Os, TupleContainer, TupleDictionary,
		Os::Debug, COL(0) | COL(1) | COL(2), &Tuple::compare> TS;

class ExampleApplication
{
	public:
		void init( Os::AppMainParameter& value )
		{
			debug_ = &wiselib::FacetProvider<Os, Os::Debug>::get_facet( value );
			block_memory_ = &wiselib::FacetProvider<Os, Os::BlockMemory>::get_facet( value );
			rand_ = &wiselib::FacetProvider<Os, Os::Rand>::get_facet( value );
			timer_ = &wiselib::FacetProvider<Os, Os::Timer>::get_facet( value );
			
		#ifdef ISENSE
			value.clock().watchdog_stop();
		#endif
			
			debug_->debug("block ts test. stack=%d", stacksize());
			
			// Give the SD card some time to be detected
			debug_->debug("waiting 5s to enable sd card detection...");
			timer_->set_timer<ExampleApplication, &ExampleApplication::go>(5000, this, 0);
		}
		
		void go(void*) {
		#ifdef ISENSE
			GET_OS.clock().watchdog_stop();
		#endif
			
			debug_->debug("memory init... stack=%d", stacksize());
			block_memory_->init();
			//debug_->debug("memory wipe...");
			//block_memory_->wipe();
			debug_->debug("allocator init...");
			mem_.init(block_memory_, debug_);
			
			debug_->debug("memory initialized");
			
			//test_bca();
			//test_btree(1000, 500);
			test_ts();
			//mem_.debug_graphviz();
		}
		
		block_data_t* generate_uri() {
			enum { MAXLEN = 100 };
			static char buf[MAXLEN];
			char rnd1[21];
			
			size_type l = 1; //rand_->operator()() % 10 + 10;
			for(size_type i = 0; i < l; i++) {
				rnd1[i] = 'a' + rand_->operator()() % 26;
			}
			rnd1[l] = '\0';
			
			snprintf(buf, MAXLEN, "<http://www.spitfire-project.eu/%s>", rnd1);
			return reinterpret_cast<block_data_t*>(buf);
		}
		
		
		// Note: some of those contain sume block buffers and would not fit on
		// the stack so dont make them local vars!
		TS ts;
		TupleDictionary dict;
		TupleContainer container;
			
		void test_ts() {
			debug_->debug("test_ts() stack=%d", stacksize());
			
			TS::Tuple tuple;
			
			enum { INS = 100, DEL = 99 };
			
			debug_->debug("init data structures");
			
			container.init(&mem_, debug_);
			dict.init(&mem_, debug_);
			ts.init(&dict, &container, debug_);
			
			debug_->debug("check()");
			
			container.check();
			dict.check();
			ts.check();
			
			for(size_type i = 0; i < INS; i++) {
				//debug_->debug("creating tuple");
				//debug_->debug("generate_uri test: %s", generate_uri());
				tuple.set_deep(0, generate_uri());
				tuple.set_deep(1, generate_uri());
				tuple.set_deep(2, generate_uri());
				
				//debug_->debug("inserting st=%d", stacksize());
				debug_->debug("ins: (%s %s %s) i=%d", (char*)tuple.get(0), (char*)tuple.get(1), (char*)tuple.get(2), i);
				ts.insert(tuple);
			}
			
			debug_->debug("---- done inserting ----");
			
			size_type i = 0;
			for(TS::iterator iter = ts.begin(); iter != ts.end(); ++iter) {
				//debug_->debug("i=%d", i++);
				//debug_->debug("(%x %x %x)", (void*)iter->get(0), (void*)iter->get(1), (void*)iter->get(2));
				debug_->debug("(%s %s %s) i=%d", iter->get(0), iter->get(1), iter->get(2), i++);
			}
			dict.tree().debug_graphviz();
			debug_->debug("ts elems: %d", ts.size());
			debug_->debug("dict elems: %d", dict.size());
			
			/*
			debug_->debug("---- NOW DELETING ----");
			erase_random(ts, DEL);
			
			
			for(TS::iterator iter = ts.begin(); iter != ts.end(); ++iter) {
				debug_->debug("(%s %s %s)", iter->get(0), iter->get(1), iter->get(2));
			}
			dict.tree().debug_graphviz("b_plus_tree_postdel.dot");
			debug_->debug("dict elems: %d", dict.size());
			
			assert(ts.size() == (INS - DEL));
			*/
			
		}
		
		/*
		void test_bca() {
			
			debug_->debug("entry size: %d", AMem::SUMMARY_SIZE);
			debug_->debug("height: %d", AMem::SUMMARY_HEIGHT);
			debug_->debug("chunk map blocks: %d", AMem::CHUNK_MAP_BLOCKS);
			debug_->debug("root entries: %d", AMem::ENTRIES_PER_ROOT_BLOCK);
			debug_->debug("summary entries: %d", AMem::ENTRIES_PER_SUMMARY_BLOCK);
			debug_->debug("%d", AMem::ENTRIES_PER_ROOT_BLOCK * AMem::ENTRIES_PER_SUMMARY_BLOCK);
			debug_->debug("%d", AMem::ENTRIES_PER_ROOT_BLOCK * AMem::ENTRIES_PER_SUMMARY_BLOCK * AMem::ENTRIES_PER_SUMMARY_BLOCK);
			
			debug_->debug("total: %d", AMem::TOTAL_MAP_BLOCKS);
			
			
			Os::block_data_t buf[AMem::BUFFER_SIZE];
			
			block_memory_->init();
			AMem amem;
			amem.init(block_memory_, debug_);
			
			AMem::address_t a0 = amem.create(buf);
			debug_->debug("a0=%d", a0);
			
			a0 = amem.create(buf);
			debug_->debug("a0=%d", a0);
			a0 = amem.create(buf);
			debug_->debug("a0=%d", a0);
			a0 = amem.create(buf);
			debug_->debug("a0=%d", a0);
			
			AMem::ChunkAddress addr = amem.allocate_chunks(20);
			
			a0 = amem.create(buf);
			debug_->debug("++++ a0=%d", a0);
			
			addr = amem.allocate_chunks(20);
			addr = amem.allocate_chunks(20);
			addr = amem.allocate_chunks(20);
			addr = amem.allocate_chunks(20);
			addr = amem.allocate_chunks(20);
			addr = amem.allocate_chunks(20);
			addr = amem.allocate_chunks(20);
			addr = amem.allocate_chunks(20);
			addr = amem.allocate_chunks(20);
			addr = amem.allocate_chunks(20);
			addr = amem.allocate_chunks(20);
			addr = amem.allocate_chunks(20);
			addr = amem.allocate_chunks(20);
			addr = amem.allocate_chunks(20);
			addr = amem.allocate_chunks(20);
			addr = amem.allocate_chunks(20);
			addr = amem.allocate_chunks(20);
			addr = amem.allocate_chunks(20);
			addr = amem.allocate_chunks(20);
			addr = amem.allocate_chunks(20);
			addr = amem.allocate_chunks(20);
			addr = amem.allocate_chunks(20);
			addr = amem.allocate_chunks(20);
			addr = amem.allocate_chunks(20);
			addr = amem.allocate_chunks(20);
			addr = amem.allocate_chunks(20);
			addr = amem.allocate_chunks(20);
			addr = amem.allocate_chunks(20);
			addr = amem.allocate_chunks(20);
			addr = amem.allocate_chunks(20);
			addr = amem.allocate_chunks(20);
			
			addr = amem.allocate_chunks(13);
			//addr = amem.allocate_chunks(3);
			//addr = amem.allocate_chunks(3);
			AMem::address_t a = amem.create(buf);
			addr = amem.allocate_chunks(20);
			amem.free(a);
			
			//addr = amem.allocate_chunks(70);
			amem.debug_graphviz();
		}
		*/
			
		void test_btree(size_t ins, size_t erase) {
			tree_.init(&mem_, debug_);
			
			insert(ins);
			//tree_.debug_graphviz();
			//erase_from_beginning(10);
			erase_random(tree_, erase);
			assert(tree_.size() == (ins - erase));
		}
		
		void insert(size_type n) {
			for(size_type i = 0; i < n; i++) {
				Tree::key_type k = i; // rand_->operator()() % 1000;
				tree_.insert(k, k*k);
			}
		}
		
		void erase_from_beginning(size_type n) {
			Tree::iterator it = tree_.begin();
			for(size_type i = 0; i < n; i++) {
				debug_->debug("it @ %d:%d", it.block_address(), it.index());
				it = tree_.erase(it);
			}
		}
		
		
		template<typename Container>
		void erase_random(Container& c, size_type n) {
			for(size_type i = 0; i < n; i++) {
				size_type pos = 0;
				for(typename Container::iterator it = c.begin(); it != c.end(); ++it, pos++) {
					double p = 1.0 / (c.size() - pos);
					double r = (double)rand_->operator()() / (double)Os::Rand::RANDOM_MAX;
					if(r <= p) {
						c.erase(it);
						break;
					}
				} // for it
			} // for i
		}
		
	private:
		Os::Debug::self_pointer_t debug_;
		Os::BlockMemory::self_pointer_t block_memory_;
		Os::Rand::self_pointer_t rand_;
		Os::Timer::self_pointer_t timer_;
		Mem mem_;
		Tree tree_;
};

// --------------------------------------------------------------------------
wiselib::WiselibApplication<Os, ExampleApplication> example_app;
// --------------------------------------------------------------------------
void application_main( Os::AppMainParameter& value )
{
	int dummy;
	high = (unsigned long)&dummy;
	
	example_app.init( value );
}

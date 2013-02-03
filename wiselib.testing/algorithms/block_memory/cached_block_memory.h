/***************************************************************************
 ** This file is part of the generic algorithm library Wiselib.           **
 ** Copyright (C) 2008,2009 by the Wisebed (www.wisebed.eu) project.      **
 **                                                                       **
 ** The Wiselib is free software: you can redistribute it and/or modify   **
 ** it under the terms of the GNU Lesser General Public License as        **
 ** published by the Free Software Foundation, either version 3 of the    **
 ** License, or (at your option) any later version.                       **
 **                                                                       **
 ** The Wiselib is distributed in the hope that it will be useful,        **
 ** but WITHOUT ANY WARRANTY; without even the implied warranty of        **
 ** MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the         **
 ** GNU Lesser General Public License for more details.                   **
 **                                                                       **
 ** You should have received a copy of the GNU Lesser General Public      **
 ** License along with the Wiselib.                                       **
 ** If not, see <http://www.gnu.org/licenses/>.                           **
 ***************************************************************************/

#ifndef CACHED_BLOCK_MEMORY_H
#define CACHED_BLOCK_MEMORY_H

namespace wiselib {
	
	/**
	 * @brief
	 * 
	 * @ingroup
	 * 
	 * @tparam 
	 */
	template<
		typename OsModel_P,
		typename BlockMemory_P,
		int CACHE_SIZE_P
	>
	class CachedBlockMemory : public BlockMemory_P {
		
		public:
			typedef OsModel_P OsModel;
			typedef typename OsModel::block_data_t block_data_t;
			typedef typename OsModel::size_t size_type;
			
			typedef BlockMemory_P BlockMemory;
			typedef typename BlockMemory::address_t address_t;
			typedef typename BlockMemory::ChunkAddress ChunkAddress;
			
			typedef CachedBlockMemory<OsModel_P, BlockMemory_P, CACHE_SIZE_P> self_type;
			typedef self_type* self_pointer_t;
			
			enum {
				CACHE_SIZE = CACHE_SIZE_P,
				BLOCK_SIZE = BlockMemory::BLOCK_SIZE
			};
			
			class CacheEntry {
				public:
					void mark_used() {
						// skip date 0 as it has as special meaning
						if(current_date_ == 0) { current_date_++; }
						date_ = current_date_++;
					}
					void mark_unused() { date_ = 0; }
					bool used() { return date_ != 0; }
					
					size_type date() { return date_; }
					block_data_t* data() { return data_; }
					address_t& address() { return address_; }
					
				private:
					static size_type current_date_;
					
					block_data_t data_[BlockMemory::BUFFER_SIZE];
					address_t address_;
					size_type date_;
			};
			
			
			/*
			typedef CachedBlockMemory<OsModel_P, BlockMemory_P> self_type;
			typedef self_type* self_pointer_t;
			
			enum {
				BLOCK_SIZE = BlockMemory::BLOCK_SIZE,
				BUFFER_SIZE = BlockMemory::BUFFER_SIZE,
				SIZE = BlockMemory::SIZE
			};
			
			enum {
				SUCCESS = BlockMemory::SUCCESS,
				ERR_UNSPEC = BlockMemory::ERR_UNSPEC
			};
			
			enum {
				NO_ADDRESS = BlockMemory::NO_ADDRESS
			*/
			
			/*
			 * Dirty hack:
			 * this init() method is tailored to specifically copy
			 * the one of bitmap chunk allocator.
			 * TODO: find a more generic way to handle initialization of the
			 * praent class. Maybe revert to "has-a" instead of "is-a"
			 * (then we need to care about whether the 'parent' class is
			 * allocating or not!)
			 */
			template<typename BM>
			int init(BM* block_memory, typename OsModel::Debug *debug) {
				memset(cache_, 0, sizeof(cache_));
				BlockMemory::init(block_memory, debug);
				return OsModel::SUCCESS;
			}
			
			ChunkAddress create_chunks(block_data_t* buffer, size_type bytes) {
				ChunkAddress r = BlockMemory::create_chunks(buffer, bytes);
				invalidate(r.address());
				return r;
			}
			
			void write_chunks(block_data_t* buffer, ChunkAddress addr, size_type bytes) {
				BlockMemory::write_chunks(buffer, addr, bytes);
				invalidate(addr.address());
			}
			
			int write(block_data_t* buffer, address_t a) {
				update(buffer, a);
				BlockMemory::write(buffer, a);
				return OsModel::SUCCESS;
			}
			
			block_data_t* get(address_t a) {
				size_type i = find(a);
				if(cache_[i].used() && cache_[i].address() == a) {
					// <DEBUG>
					block_data_t buf[BlockMemory::BUFFER_SIZE];
					BlockMemory::read(buf, a);
					for(size_type j = 0; j < BlockMemory::BUFFER_SIZE; j++) {
						assert(buf[j] == cache_[i].data()[j]);
					}
					// </DEBUG>
				
					return cache_[i].data();
				}
				
				cache_[i].mark_used();
				cache_[i].address() = a;
				BlockMemory::read(cache_[i].data(), a);
				
				
				
				
				return cache_[i].data();
			}
			
			void update(block_data_t* new_data, address_t a) {
				size_type i = find(a);
				CacheEntry &e = cache_[i];
				
				// only update if a already in the cache or
				// free slot available!
				if(e.used() && (e.address() != a)) { return; }
				
				memcpy(e.data(), new_data, BLOCK_SIZE);
				e.mark_used();
				e.address() = a;
			}
			
			void invalidate(address_t a) {
				size_type i = find(a);
				CacheEntry &e = cache_[i];
				
				if(e.used() && e.address() == a) {
					e.mark_unused();
				}
				
				assert(
					!cache_[find(a)].used() ||
					(cache_[find(a)].address() != a)
				);
			}
		
		private:
				
			/**
			 * Returns slot containing block for given address a.
			 * If no such slot is available, will return a free slot.
			 * If that is also not available, will return
			 * a least expendable entry (the earliest inserted one).
			 */
			size_type find(address_t a) {
				size_type lowest_idx = 0, lowest_date = -1;
				size_type unused_idx = -1;
				
				for(size_type i = 0; i < CACHE_SIZE; i++) {
					CacheEntry &e = cache_[i];
					
					if(e.used()) {
						if(e.address() == a) { return i; }
						else if(e.date() <= lowest_date) {
							lowest_idx = i;
							lowest_date = e.date();
						}
					}
					else if(unused_idx == (size_type)-1) { unused_idx = i; }
				}
				
				if(unused_idx != (size_type)-1) { return unused_idx; }
				return lowest_idx;
			}
			
			CacheEntry cache_[CACHE_SIZE];
			
	}; // CachedBlockMemory
	
	template<
		typename OsModel_P,
		typename BlockMemory_P,
		int CACHE_SIZE_P
	>
	typename CachedBlockMemory<OsModel_P, BlockMemory_P, CACHE_SIZE_P>::size_type
	CachedBlockMemory<OsModel_P, BlockMemory_P, CACHE_SIZE_P>::CacheEntry::current_date_ = 0;
}

#endif // CACHED_BLOCK_MEMORY_H


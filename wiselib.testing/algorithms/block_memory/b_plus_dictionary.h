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

#ifndef B_PLUS_DICTIONARY_H
#define B_PLUS_DICTIONARY_H

#include "b_plus_tree.h"
#include "b_plus_hash_set.h"
#include <util/traits.h>

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
		typename Hash_P,
		typename Mapped_P = char*
	>
	class BPlusDictionary {
		public:
			typedef OsModel_P OsModel;
			typedef typename OsModel::block_data_t block_data_t;
			typedef typename OsModel::size_t size_type;
			typedef BlockMemory_P BlockMemory;
			typedef typename BlockMemory::ChunkAddress ChunkAddress;
			typedef Hash_P Hash;
			typedef typename Hash::hash_t hash_t;
			typedef BPlusTree<OsModel, BlockMemory, hash_t, ChunkAddress> Tree;
			typedef ChunkAddress key_type;
			typedef Mapped_P mapped_type;
			typedef size_type refcount_t;
			typedef BPlusHashSet<OsModel, BlockMemory, Hash, mapped_type> HashSet;
			typedef typename HashSet::Entry Entry;
			typedef Payload<mapped_type> PL;
			
			enum { ABSTRACT_KEYS = true };
			static const key_type NULL_KEY;
			
			enum ErrorCodes {
				SUCCESS = OsModel::SUCCESS, ERR_UNSPEC = OsModel::ERR_UNSPEC
			};
			
		private:
		
		public:
			
			BPlusDictionary() {
			}
			
			int init(typename BlockMemory::self_pointer_t block_memory, typename OsModel::Debug::self_pointer_t debug) {
				debug_ = debug;
				block_memory_ = block_memory;
				//tree_.init(block_memory_, debug_);
				hash_set_.init(block_memory_, debug_);
				return SUCCESS;
			}
			
			key_type insert(block_data_t* ptr) {
				//debug_->debug("dict ins a0 sz=%d st=%d", sizeof(mapped_type), stacksize());
				//debug_->debug("sz(hashset it)=%d sz(tree it)=%d sz(keytype)=%d", sizeof(typename HashSet::iterator),
						//sizeof(typename HashSet::Tree::iterator), sizeof(key_type));
				return insert(PL::from_data(ptr));
			}
			
			key_type insert(mapped_type value) {
				typename HashSet::iterator it = hash_set_.insert(value);
				assert(it != hash_set_.end());
				return it.chunk_address();
			}
			
			key_type find(block_data_t* ptr) {
				return find(PL::from_data(ptr));
			}
			
			key_type find(mapped_type value) {
				typename HashSet::iterator it = hash_set_.find(value);
				return it.chunk_address();
			}
			
			void erase(key_type k) {
				Entry& e = *reinterpret_cast<Entry*>(entry_buffer_);
				read_entry(e, k);
				typename HashSet::iterator it = hash_set_.find(e.payload());
				hash_set_.erase(it);
			} // erase(k)
			
			mapped_type operator[](key_type k) {
				Entry& e = *reinterpret_cast<Entry*>(entry_buffer_);
				read_entry(e, k);
				e.check();
				return e.payload();
			}
			
			block_data_t* get_value(key_type k) {
				return PL::data((*this)[k]);
			}
			
			void free_value(block_data_t* ptr) {
			}
			
			void free_value(mapped_type v) {
			}
			
			Tree& tree() { return hash_set_.tree(); }
			
			size_type size() { return hash_set_.size(); }
			
			void check() {
				assert(block_memory_ != 0);
			}
		
		private:
			void read_entry(Entry& e, ChunkAddress addr) {
				assert(addr != ChunkAddress::invalid());
				block_memory_->read_chunks(reinterpret_cast<block_data_t*>(&e), addr, sizeof(Entry));
				block_memory_->read_chunks(reinterpret_cast<block_data_t*>(&e), addr, e.total_length());
				
			}
			
			/// for returning values e.g. by get_value()
			block_data_t entry_buffer_[BlockMemory::BUFFER_SIZE];
			typename OsModel::Debug::self_pointer_t debug_;
			typename BlockMemory::self_pointer_t block_memory_;
			HashSet hash_set_;
	}; // BPlusDictionary
	
	template<
		typename OsModel_P, typename BlockMemory_P, typename Hash_P, typename Mapped_P
	>
	const typename BPlusDictionary<OsModel_P, BlockMemory_P, Hash_P, Mapped_P>::ChunkAddress BPlusDictionary<OsModel_P, BlockMemory_P, Hash_P, Mapped_P>::NULL_KEY = ChunkAddress::invalid();
}

#endif // B_PLUS_DICTIONARY_H


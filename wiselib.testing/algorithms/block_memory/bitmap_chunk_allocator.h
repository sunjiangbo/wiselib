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

#ifndef BITMAP_CHUNK_ALLOCATOR_H
#define BITMAP_CHUNK_ALLOCATOR_H

#if (DEBUG_OSTREAM || DEBUG_GRAPHVIZ)
	#include <fstream>
	#include <iostream>
	#include <iomanip>
#endif

#include <util/meta.h>
#include <util/standalone_math.h>
#include "chunk_map_block.h"
#include "summary_block.h"

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
		size_t CHUNK_SIZE_P,
		typename Debug = typename OsModel_P::Debug
	>
	class BitmapChunkAllocator {
		public:
			class ChunkAddress;
			
			typedef OsModel_P OsModel;
			typedef typename OsModel::block_data_t block_data_t;
			typedef typename OsModel::size_t size_type;
			typedef BlockMemory_P BlockMemory;
			typedef typename BlockMemory::address_t address_t;
			typedef BitmapChunkAllocator<OsModel, BlockMemory, CHUNK_SIZE_P> self_type;
			typedef self_type* self_pointer_t;
			typedef StandaloneMath<OsModel> Math;
			
		//private:
			
			typedef typename SmallUint< BlockMemory::BLOCK_SIZE / CHUNK_SIZE_P >::t summary_t;
			enum { NO_SUMMARY = (summary_t)(-1) };
			
			enum {
				SUMMARY_SIZE = sizeof(summary_t),
				CHUNK_SIZE = CHUNK_SIZE_P,
				CHUNKS_PER_BLOCK = BlockMemory::BLOCK_SIZE / CHUNK_SIZE,
				CHUNK_MAP_BITS = BlockMemory::SIZE * CHUNKS_PER_BLOCK,
				//CHUNK_MAP_BITS = (4UL * 1024UL * 1024UL * 1024UL / 512UL) * CHUNKS_PER_BLOCK,
				CHUNK_MAP_BYTES = DivCeil< CHUNK_MAP_BITS, 8 >::value,
				
				// # of blocks being used for chunk maps
				CHUNK_MAP_BLOCKS = DivCeil< CHUNK_MAP_BYTES, BlockMemory::BLOCK_SIZE >::value,
				ENTRIES_PER_SUMMARY_BLOCK = BlockMemory::BLOCK_SIZE / SUMMARY_SIZE,
				BLOCKS_PER_BLOCK = 8 * BlockMemory::BLOCK_SIZE / CHUNKS_PER_BLOCK
			};
			
			template<
				size_t entries,
				bool fits_in_one_block = (entries <= ENTRIES_PER_SUMMARY_BLOCK)
			>
			struct RootBlockEntries {
				static const size_t value = entries;
			};
			
			template< size_t entries >
			struct RootBlockEntries<entries, false> {
				static const size_t value = RootBlockEntries< entries / ENTRIES_PER_SUMMARY_BLOCK >::value;
			};
			
			enum {
				ENTRIES_PER_ROOT_BLOCK = RootBlockEntries<CHUNK_MAP_BLOCKS>::value,
				SUMMARY_HEIGHT = Log< CHUNK_MAP_BLOCKS, ENTRIES_PER_SUMMARY_BLOCK >::value + 1,
				TOTAL_MAP_BLOCKS = TreeNodes< CHUNK_MAP_BLOCKS, ENTRIES_PER_SUMMARY_BLOCK >::value,
			};
			
		public:
			class ChunkAddress {
					enum { CHUNK_BITS = Log<CHUNKS_PER_BLOCK, 2>::value };
				
				public:
					ChunkAddress() : addr_(0) {
					}
					
					ChunkAddress(const ChunkAddress& other) : addr_(other.addr_) {
					}
					
					/*ChunkAddress(address_t a) : addr_(a) {
					}*/
					
					static ChunkAddress invalid() {
						ChunkAddress r;
						r.addr_ = -1;
						return r;
					}
					
					address_t address() { return addr_ >> CHUNK_BITS; }
					void set_address(address_t address) { addr_ = (address << CHUNK_BITS) | offset(); }
					size_type offset() { return addr_ & ((1 << CHUNK_BITS) - 1); }
					
					ChunkAddress& operator+=(size_type i) {
						address_t chunk = offset();
						address_t a = address();
						chunk += i;
						if(chunk >= CHUNKS_PER_BLOCK) {
							a += chunk / CHUNKS_PER_BLOCK;
							chunk %= CHUNKS_PER_BLOCK;
						}
						addr_ = (a << CHUNK_BITS) | chunk;
						return *this;
					}
					
					size_type absolute_chunk() {
						return address() * CHUNKS_PER_BLOCK + offset();
					}
					
					bool operator==(const ChunkAddress& other) { return other.addr_ == addr_; }
					bool operator!=(const ChunkAddress& other) { return other.addr_ != addr_; }
					
#if (DEBUG_OSTREAM || DEBUG_GRAPHVIZ)
					friend std::ostream& operator<<(std::ostream& os, ChunkAddress a) {
						os << "@" << a.address() << "." << a.offset();
						return os;
					}
#endif // DEBUG_OSTREAM
					
				private:
					address_t addr_;
			};
			
			typedef ChunkMapBlock<OsModel, BlockMemory, summary_t, CHUNK_SIZE> CMBlock;
			typedef SummaryBlock<OsModel, BlockMemory, summary_t> SBlock;
			
			enum {
				BLOCK_SIZE = BlockMemory::BLOCK_SIZE,
				BUFFER_SIZE = BlockMemory::BUFFER_SIZE,
				SIZE = BlockMemory::SIZE - TOTAL_MAP_BLOCKS,
				NO_ADDRESS = BlockMemory::NO_ADDRESS
			};
			
			void init(BlockMemory* block_memory, typename Debug::self_pointer_t debug) {
				block_memory_ = block_memory;
				debug_ = debug;
				
				debug_->debug("bitmap chunk allocator init");
				debug_->debug("chunk size          : %lu", (size_type)CHUNK_SIZE);
				debug_->debug("chunks per block    : %lu", (size_type)CHUNKS_PER_BLOCK);
				debug_->debug("mem size (#bloks)   : %lu", (size_type)BlockMemory::SIZE);
				debug_->debug("chunk map bits      : %lu", (size_type)CHUNK_MAP_BITS);
				debug_->debug("chunk map bytes     : %lu", (size_type)CHUNK_MAP_BYTES);
				debug_->debug("summary height      : log%lu(cbm=%lu) = %lu",
						(size_type)ENTRIES_PER_SUMMARY_BLOCK,
						(size_type)CHUNK_MAP_BLOCKS,
						(size_type)SUMMARY_HEIGHT);
				debug_->debug("root entries        : %lu", (size_type)ENTRIES_PER_ROOT_BLOCK);
				debug_->debug("summary entries     : %lu", (size_type)ENTRIES_PER_SUMMARY_BLOCK);
				debug_->debug("chunk map block bits: %lu", (size_type)(BLOCK_SIZE * 8));
				debug_->debug("");
				for(size_type i = 0; i <= SUMMARY_HEIGHT; i++) {
					debug_->debug("[layer %lu  %8lx - %8lx]",
							i, layer_start(i), layer_start(i) + layer_size(i)
					);
				}
				debug_->debug("end                 : %lu", (size_type)((layer_start(SUMMARY_HEIGHT) + layer_size(SUMMARY_HEIGHT))));
				
				
			#ifdef PC
				for(size_type i = 0; i < layer_start(SUMMARY_HEIGHT) + layer_size(SUMMARY_HEIGHT); i++) {
					allocate_block();
				}
			#endif
			}
			
			void wipe() {
				block_memory_->wipe();
			}
			
			address_t create(block_data_t* buffer) {
				debug_graphviz();
				return allocate_block();
			}
			
			void free(address_t a) {
				free_block(a);
			}
			
			void write(block_data_t* buffer, address_t a) {
				block_memory_->write(buffer, a);
			}
			
			void read(block_data_t* buffer, address_t a) {
				block_memory_->read(buffer, a);
			}
			
			
			
			ChunkAddress create_chunks(block_data_t* buffer, size_type bytes) {
				size_type chunks = (bytes + CHUNK_SIZE - 1) / CHUNK_SIZE;
				assert(chunks * 4 >= bytes);
				
				ChunkAddress r = allocate_chunks(chunks);
				block_data_t buf[BlockMemory::BUFFER_SIZE];
				block_memory_->read(buf, r.address());
				memcpy(buf + r.offset() * CHUNK_SIZE, buffer, bytes);
				block_memory_->write(buf, r.address());
				return r;
			}
			
			ChunkAddress allocate_chunks(size_type n) {
				SBlock summary_block;
				
				Finder finder(summary_block, n, this);
				finder.allocate(0, 0);
				return finder.chunk_address();
			}
			
			void free_chunks(ChunkAddress a, size_type bytes) {
				size_type chunks = (bytes + CHUNK_SIZE - 1) / CHUNK_SIZE;
				
				CMBlock cmblock;
				address_t a_cmblock = layer_start(SUMMARY_HEIGHT) + a.absolute_chunk() / (BlockMemory::BLOCK_SIZE * 8);
				read_block(cmblock, a_cmblock);
				cmblock.unmark(a.absolute_chunk() % (BlockMemory::BLOCK_SIZE * 8), chunks);
				write_block(cmblock, a_cmblock);
			}
			
			void read_chunks(block_data_t* buffer, ChunkAddress addr, size_type bytes) {
				block_data_t buf[BlockMemory::BUFFER_SIZE];
				block_memory_->read(buf, addr.address());
				memcpy(buffer, buf + addr.offset() * CHUNK_SIZE, bytes);
			}
			
			void write_chunks(block_data_t* buffer, ChunkAddress addr, size_type bytes) {
				block_data_t buf[BlockMemory::BUFFER_SIZE];
				block_memory_->read(buf, addr.address());
				memcpy(buf + addr.offset() * CHUNK_SIZE, buffer, bytes);
				block_memory_->write(buf, addr.address());
			}
			
			// Debugging -- Graphviz
			// {{{
			
#if DEBUG_GRAPHVIZ
			void debug_graphviz(const char *fn = "bitmap_chunk_allocator.dot") {
				enum { MAX_PER_LAYER = 100U };
				std::ofstream out(fn);
				SBlock block;
				
				out << "digraph G {" << std::endl;
				
				for(size_type layer = 0; layer < SUMMARY_HEIGHT; layer++) {
					for(size_type i = 0; i < Math::template min<size_type>(MAX_PER_LAYER, layer_size(layer)); i++) {
						address_t address = layer_start(layer) + i;
						read_block(block, address);
						out << "b" << address << " [shape=plaintext, label=";
						block.debug_graphviz_label(out, address);
						out << "] ;" << std::endl;
						
						size_type n = (layer == 0) ? (size_type)ENTRIES_PER_ROOT_BLOCK : (size_type)ENTRIES_PER_SUMMARY_BLOCK;
						for(size_type j = 0; j < Math::template min<size_type>(MAX_PER_LAYER, n); j++) {
							out << "b" << address << " -> b" <<
								(layer_start(layer + 1) + i * n + j) << ";" << std::endl;
						}
					}
				}
				
				
				CMBlock &cmblock = *reinterpret_cast<CMBlock*>(&block);
				
				for(size_type i = 0; i < Math::template min<size_type>(MAX_PER_LAYER, layer_size(SUMMARY_HEIGHT)); i++) {
					address_t address = layer_start(SUMMARY_HEIGHT) + i;
					read_block(cmblock, address);
					out << "  b" << address << " [shape=plaintext, label=";
					cmblock.debug_graphviz_label(out, address);
					out << "  ] ;" << std::endl;
				}
				
				out << "}" << std::endl;
				out.close();
			} // debug_graphviz()
#else
			void debug_graphviz(const char *_ = 0) { }
#endif
			// }}}
			
		private:
			
			template<typename Block>
			void read_block(Block& block, address_t a) {
				block_memory_->read(reinterpret_cast<block_data_t*>(&block), a);
			}
			
			template<typename Block>
			void write_block(Block& block, address_t a) {
				block_memory_->write(reinterpret_cast<block_data_t*>(&block), a);
			}
			
			address_t allocate_block() {
				return allocate_chunks(CHUNKS_PER_BLOCK).address();
			}
			
			void free_block(address_t a) {
				CMBlock cmblock;
				address_t a_cmblock = layer_start(SUMMARY_HEIGHT) + a / BLOCKS_PER_BLOCK;
				read_block(cmblock, a_cmblock);
				cmblock.unmark(CHUNKS_PER_BLOCK * (a % BLOCKS_PER_BLOCK), CHUNKS_PER_BLOCK);
				write_block(cmblock, a_cmblock);
			}
			
			class Finder {
				public:
					Finder(SBlock& summary_block, size_type n, self_type* allocator)
						: summary_block_(summary_block), n_(n), allocator_(allocator) {
					}
					
					/*
					 * @return NO_SUMMARY if summary did not change, else the new
					 * summary
					 */
					summary_t allocate(size_type layer, address_t address) {
						summary_t r = NO_SUMMARY;
						allocator_->read_block(summary_block_, layer_start(layer) + address);
						
						if(layer == SUMMARY_HEIGHT) {
							CMBlock &chunk_map_block = *reinterpret_cast<CMBlock*>(&summary_block_);
							chunk_address_.set_address(address * BlockMemory::BLOCK_SIZE * 8 / CHUNKS_PER_BLOCK);
							size_type offset = chunk_map_block.find(n_);
							
							chunk_address_ += offset;
							summary_t before = chunk_map_block.summary();
							chunk_map_block.mark(offset, n_);
							allocator_->write_block(chunk_map_block, layer_start(layer) + address);
							summary_t after = chunk_map_block.summary();
							if(after != before) { r = after; }
						}
						else {
							address_t next_address = address * ENTRIES_PER_SUMMARY_BLOCK + summary_block_.find(n_, ENTRIES_PER_SUMMARY_BLOCK);
							summary_t s = allocate(layer + 1, next_address);
							if(s != NO_SUMMARY) {
								allocator_->read_block(summary_block_, layer_start(layer) + address);
								summary_t before = summary_block_.summary(ENTRIES_PER_SUMMARY_BLOCK);
								
								summary_block_[next_address] = s;
								allocator_->write_block(summary_block_, layer_start(layer) + address);
								summary_t after = summary_block_.summary(ENTRIES_PER_SUMMARY_BLOCK);
								if(after != before) { r = after; }
							} // if s
						}
						
						return r;
					}
					
					ChunkAddress& chunk_address() { return chunk_address_; }
					
				private:
					SBlock &summary_block_;
					size_type n_;
					ChunkAddress chunk_address_;
					self_type *allocator_;
			};
			
			static address_t layer_start(size_type layer) {
				switch(layer) {
					case 0:
						return 0;
						break;
					case 1:
						return 1;
						break;
					default: {
							address_t r = 1;
							size_type l = ENTRIES_PER_ROOT_BLOCK;
							for(size_type i = 1; i < layer; i++) {
								r += l;
								l *= ENTRIES_PER_SUMMARY_BLOCK;
							}
							return r;
						}
						break;
				};
			}
			
			static address_t layer_size(size_type layer) {
				switch(layer) {
					case 0:
						return 1;
						break;
					default: {
							size_type l = ENTRIES_PER_ROOT_BLOCK;
							for(size_type i = 1; i < layer; i++) {
								l *= ENTRIES_PER_SUMMARY_BLOCK;
							}
							return l;
						}
						break;
				};
			}
			
			address_t root_;
			BlockMemory *block_memory_;
			typename Debug::self_pointer_t debug_;
		
	}; // BitmapChunkAllocator
	
}

#endif // BITMAP_CHUNK_ALLOCATOR_H


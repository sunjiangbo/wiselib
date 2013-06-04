#ifndef PC_VIRTUAL_BLOCKMEMORY_H
#define PC_VIRTUAL_BLOCKMEMORY_H

namespace wiselib {

//TODO: doxygen

template<typename OsModel_P, int nrOfBlocks = 1000, int BLOCKSIZE = 512>
class VirtualSD {

public:
	typedef OsModel_P OsModel;
	typedef VirtualSD self_type;
	typedef self_type* self_pointer_t;
	typedef typename OsModel::block_data_t block_data_t;
	typedef typename OsModel::size_t address_t;
	
	enum {
		BLOCK_SIZE = blocksize, ///< size of block in byte (usable payload)
	};
	
	enum {
		NO_ADDRESS = (address_t)(-1), ///< address_t value denoting an invalid address
	};
	
	enum {
		SUCCESS = OsModel::SUCCESS,
		ERR_IO = OsModel::ERR_IO,
		ERR_NOMEM = OsModel::ERR_NOMEM,
		ERR_UNSPEC = OsModel::ERR_UNSPEC,
	}; 

	int init() {

	}

};

} //namespace

#endif //PC_VIRTUAL_BLOCKMEMORY_H

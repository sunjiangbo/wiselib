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


#ifndef BYTE_H
#define BYTE_H

namespace wiselib {
   namespace protobuf {

template<
   typename OsModel_P,
   typename Buffer_P
>
class Byte {
   public:
      typedef OsModel_P Os;
      typedef Buffer_P buffer_t;
      typedef typename Os::block_data_t block_data_t;
      
      static bool write(buffer_t& buffer, buffer_t& buffer_end, block_data_t v) {
         if(buffer == buffer_end) { return false; }
         *buffer = v;
         ++buffer;
         return true;
      }
      
      template<typename T>
      static bool read(buffer_t& buffer, buffer_t& buffer_end, T& out) {
         if(buffer == buffer_end) { return false; }
         out = *buffer;
         ++buffer;
         return true;
      }
};

   }
}

#endif // VARINT_H
// vim: set ts=3 sw=3 expandtab:


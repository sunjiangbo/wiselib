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
#ifndef __ALGORITHMS_LOCALIZATION_DISTANCE_BASED_LOCALIZATION_ITER_LATERATION_MESSAGES_H
#define __ALGORITHMS_LOCALIZATION_DISTANCE_BASED_LOCALIZATION_ITER_LATERATION_MESSAGES_H

#include "util/serialization/simple_types.h"
#include "util/serialization/math_vec.h"
#include "algorithms/localization/distance_based/util/localization_defutils.h"

namespace wiselib
{

   template<typename OsModel_P,
            typename Radio_P>
   class LocalizationIterLaterationMessage
   {
   public:
      typedef OsModel_P OsModel;
      typedef Radio_P Radio;
      typedef typename Radio::node_id_t node_id_t;
      typedef typename Radio::block_data_t block_data_t;
      typedef typename Radio::size_t size_t;
      typedef typename Radio::message_id_t message_id_t;
      // --------------------------------------------------------------------
      inline LocalizationIterLaterationMessage();
      // --------------------------------------------------------------------
      inline message_id_t msg_id()
      { return read<OsModel, block_data_t, message_id_t>( buffer ); };
      // --------------------------------------------------------------------
      inline void set_msg_id( message_id_t id )
      { write<OsModel, block_data_t, message_id_t>( buffer, id ); }
      // --------------------------------------------------------------------
      inline double confidence()
      { return read<OsModel, block_data_t, double>(buffer + CONFIDENCE_POS); }
      // --------------------------------------------------------------------
      inline void set_confidence( double confidence )
      { write<OsModel, block_data_t, double>(buffer + CONFIDENCE_POS, confidence); }
      // --------------------------------------------------------------------
      inline Vec position()
      { return read<OsModel, block_data_t, Vec>(buffer + POSITION_POS); }
      // --------------------------------------------------------------------
      inline void set_position( Vec pos )
      { write<OsModel, block_data_t, Vec>(buffer + POSITION_POS, pos); }
      // --------------------------------------------------------------------
      inline size_t buffer_size()
      { return MSGEND_POS; }

   private:
      enum data_positions
      {
         CONFIDENCE_POS = sizeof(message_id_t),
         POSITION_POS = CONFIDENCE_POS + sizeof(double),
         MSGEND_POS = POSITION_POS + 3 * sizeof(double)
      };

      block_data_t buffer[MSGEND_POS];
   };
   // -----------------------------------------------------------------------
   template<typename OsModel_P,
            typename Radio_P>
   LocalizationIterLaterationMessage<OsModel_P, Radio_P>::
   LocalizationIterLaterationMessage()
   {
      set_msg_id( 0 );
      set_confidence( 0.0 );
      set_position( UNKNOWN_POSITION );
   }
   // -----------------------------------------------------------------------
   // -----------------------------------------------------------------------
   // -----------------------------------------------------------------------
   template<typename OsModel_P,
            typename Radio_P>
   class LocalizationIterLaterationSoundMessage
   {
   public:
      typedef OsModel_P OsModel;
      typedef Radio_P Radio;
      typedef typename Radio::node_id_t node_id_t;
      typedef typename Radio::block_data_t block_data_t;
      typedef typename Radio::size_t size_t;
      typedef typename Radio::message_id_t message_id_t;
      // --------------------------------------------------------------------
      inline LocalizationIterLaterationSoundMessage()
      {}
      // --------------------------------------------------------------------
      inline message_id_t msg_id()
      { return read<OsModel, block_data_t, message_id_t>( buffer ); };
      // --------------------------------------------------------------------
      inline void set_msg_id( message_id_t id )
      { write<OsModel, block_data_t, message_id_t>( buffer, id ); }
      // --------------------------------------------------------------------
      inline size_t buffer_size()
      { return MSGEND_POS; }

   private:
      enum data_positions
      {
         MSGEND_POS = sizeof(message_id_t)
      };

      block_data_t buffer[MSGEND_POS];
   };

}// namespace wiselib
#endif

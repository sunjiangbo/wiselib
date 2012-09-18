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

#ifndef MESSAGE_TYPES_H
#define MESSAGE_TYPES_H

namespace wiselib {
	
	enum { MSG_CONSTRUCT = 80, MSG_BRIDGE_REQUEST, MSG_DESTRUCT_DETOUR };
	
	template<typename OsModel_P, typename Radio_P>
	class BridgeRequestMessage {
		public:
			typedef Radio_P Radio;
			typedef typename Radio::node_id_t node_id_t;
			
			BridgeRequestMessage() : message_type_(MSG_BRIDGE_REQUEST) {
			}
			
			node_id_t low_origin() { return low_origin_; }
			node_id_t high_origin() { return high_origin_; }
			void set_low_origin(node_id_t l) { low_origin_ = l; }
			void set_high_origin(node_id_t l) { high_origin_ = l; }
			void set_is_low(bool i) { from_low_ = i; }
			bool from_low() { return from_low_; }
			
		private:
			node_id_t low_origin_, high_origin_;
			bool from_low_;
			::uint8_t message_type_;
	};
	
	template<typename OsModel_P, typename Radio_P>
	class DestructDetourMessage {
		public:
			typedef OsModel_P OsModel;
			typedef Radio_P Radio;
			typedef typename Radio::node_id_t node_id_t;
			
			DestructDetourMessage() : message_type_(MSG_DESTRUCT_DETOUR) {
			}
			
			DestructDetourMessage(BridgeRequestMessage<OsModel, Radio>& bridge_request_message) { 
				// TODO
			}
			
			DestructDetourMessage(node_id_t low_origin, node_id_t high_origin) {
			}
			
			node_id_t low_origin() { return 0; } // TODO
			node_id_t high_origin() { return 0; } // TODO
			void set_low_origin(node_id_t l) { low_origin_ = l; }
			void set_high_origin(node_id_t l) { high_origin_ = l; }
		
		private:
			node_id_t low_origin_, high_origin_;
			::uint8_t message_type_;
	};
	
	template<typename OsModel_P, typename Radio_P>
	class RenumberMessage {
		public:
			typedef Radio_P Radio;
			typedef typename Radio::node_id_t node_id_t;
			
		private:
			::uint8_t message_type_;
	};
}

#endif // MESSAGE_TYPES_H


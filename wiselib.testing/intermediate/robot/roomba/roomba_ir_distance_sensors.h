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

// vim: set noexpandtab ts=4 sw=4:

#ifndef ROOMBA_IR_DISTANCE_SENSORS_H
#define ROOMBA_IR_DISTANCE_SENSORS_H

template<typename OsModel_P, typename Roomba_P>
class RoombaIRDistanceSensors {
	public:
		RoombaIRDistanceSensor_not_implemented_yet
		
		typedef OsModel_P OsModel;
		typedef Roomba_P Roomba;
		typedef typename Roomba::Math_P Math;
		typedef typename OsModel::size_t size_t;
		typedef uint16_t value_t;
		
		typedef double angle_t;
		typedef double length_t;
		
		init(Roomba& roomba) {
			roomba_ = &roomba;
		}
		
		size_t number() { return 6; }
		
		inline angle_t angle(size_t n) {
			// TODO: Measure actual values
			static const double a[6] = {
				Math::PI_2, Math::PI_3, Math::PI_6,
				-Math::PI_6, -Math::PI_3, -Math::PI_2
			};
			return a[n];
		} // angle()
		
		size_t nearest(angle_t radians) {
			// TODO: Measure actual values
			double angle = Math::radians_to_degrees(radians);
			if(angle <= -75.0) { return 5; }
			else if(angle <= -45.0) { return 4; }
			else if(angle <= 0.0) { return 3; }
			else if(angle <= 45.0) { return 2; }
			else if(angle <= 75.0) { return 1; }
			else { return 0; }
		}
		
		inline length_t distance(size_t n) {
			return 50.0; // TODO: Replace with actual value
		}
		
		value_t operator()(size_t n) {
			typename Roomba::value_t data = (*roomba_)();
			switch(n) {
				case 0: return data.light_bump_left_signal;
				case 1: return data.light_bump_front_left_signal;
				case 2: return data.light_bump_center_left_signal;
				case 3: return data.light_bump_center_right_signal;
				case 4: return data.light_bump_front_right_signal;
				case 5: return data.light_bump_right_signal;
				default:
					assert(false);
			}
		} // operator()
		
		value_t max_value(size_t n) { return 4095; } // according to ROI spec
		
	private:
		Roomba* roomba_;
};

#endif // ROOMBA_IR_DISTANCE_SENSORS_H


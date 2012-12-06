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
#ifndef __CONTIKI_SKY_BATTERY_SENSOR__
#define __CONTIKI_SKY_BATTERY_SENSOR__

#include "external_interface/contiki/contiki_types.h"
#include "external_interface/contiki/contiki_os.h"

extern "C"
{
	#include "contiki.h" 
	#include "dev/battery-sensor.h"
	
}
#include <stdio.h>

namespace wiselib
{
	/** \brief Contiki Implementation of \ref request_sensor_concept "Request 
	 *  Sensor Concept"
	 *
	 * Contiki implementation of the \ref request_sensor_concept "Request
	 * Sensor Concept" ...
	 */
	template<typename OsModel_P>
	class ContikiSkyBatterySensor
	{
	public:
		typedef OsModel_P OsModel;
		
		typedef ContikiSkyBatterySensor<OsModel> self_type;
		typedef self_type* self_pointer_t;
		
		typedef int value_t;
		
		//------------------------------------------------------------------------
		
		enum ErrorCodes
		{
			SUCCESS = OsModel::SUCCESS,
			ERR_UNSPEC = OsModel::ERR_UNSPEC
		};
		
		//------------------------------------------------------------------------
		
		enum StateData
		{
			READY = OsModel::READY,
			NO_VALUE = OsModel::NO_VALUE,
			INACTIVE = OsModel::INACTIVE
		};
		
		//------------------------------------------------------------------------
		
		///@name Constructor/Destructor
		///
		/** Default constructor
		 *
		 */
		ContikiSkyBatterySensor( )
		{
			state_ = READY;
			SENSORS_ACTIVATE( battery_sensor );
		}
		
		~ContikiSkyBatterySensor()
		{
			state_ = INACTIVE;
		}
		///
		
		//------------------------------------------------------------------------
		
		///@name Getters and Setters
		///
		/** Returns the current state of the sensor
		 *
		 *  \return The current state
		 */
		int state()
		{
			return state_;
		}
		
		//------------------------------------------------------------------------
		
		/** Returns actual battery voltage in mV
		 *
		 *  \returns Battery voltage in mV
		 */
		value_t operator()( void )
		{	
			int battery_value = battery_sensor.value( 0 );
			battery_value = ( uint32_t ) ( ( ( ( double ) battery_value ) / 4096.0 ) 
				* 2.5 );
			return battery_value;
		}
		
		/** Disables the Sensor
		 * 
		 */
		void disable()
		{
				SENSORS_DEACTIVATE( battery_sensor );
		}
		///
		
	private:
		/// The current state
		StateData state_;
   };
};

#endif // __CONTIKI_SKY_BATTERY_SENSOR__
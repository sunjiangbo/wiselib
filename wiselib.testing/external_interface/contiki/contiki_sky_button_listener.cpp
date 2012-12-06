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

extern "C"
{
	#include "contiki.h"
	#include "process.h" 
	#include "dev/button-sensor.h"
	#include <stdio.h>
}

#include "contiki_sky_button_listener.h"

namespace wiselib
{
	static contiki_sky_button_delegate_t receiver;
	
	PROCESS( button_event_process, "Button Event Listener" );

	PROCESS_THREAD(button_event_process, ev, data)
	{	
		PROCESS_EXITHANDLER( return stopContikiSkyButtonListening() );
		PROCESS_BEGIN();
		
		SENSORS_ACTIVATE(button_sensor);
		
		while (1)
		{
			PROCESS_WAIT_EVENT();
			
			if( ev == sensors_event )
			{
				if(data == &button_sensor)
				{
					receiver();
				}
			}
		}
		
		PROCESS_END();
	}
	
	void initContikiSkyButtonListening()
	{
		receiver = contiki_sky_button_delegate_t();
		process_start( &button_event_process, 0);
	}
	
	int stopContikiSkyButtonListening()
	{
		SENSORS_DEACTIVATE(button_sensor);
		contiki_sky_button_delete_receiver();
		return 0;
	}
	
	void contiki_sky_button_set_receiver( contiki_sky_button_delegate_t& d )
	{
		receiver = d;
	}
	
	void contiki_sky_button_delete_receiver()
	{
		receiver = contiki_sky_button_delegate_t();
	}
}
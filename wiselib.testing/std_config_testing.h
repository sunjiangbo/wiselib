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
#ifndef __CONFIG_TESTING_H
#define __CONFIG_TESTING_H

// --------------------------------------------------------------------------
// --------------------- External Interface ---------------------------------
// --------------------------------------------------------------------------

// ---------------- iSense --------------------------------------------------
// #define DEBUG_ISENSE_LIGHT_SENSOR

// Add debug information when sending messages - if failed, a debug message
// is printed (with the state passed in confirm())
// #define DEBUG_ISENSE_EXTENDED_TX_RADIO
// Add random delay before sending a message. The delay is a value
// 0..ISENSE_EXTENDED_TX_RADIO_RANDOM_DELAY_MAX in milliseconds.
// #define ISENSE_EXTENDED_TX_RADIO_RANDOM_DELAY
// #define ISENSE_EXTENDED_TX_RADIO_RANDOM_DELAY_MAX 30

// --------------------------------------------------------------------------
// --------------------- Algorithms -----------------------------------------
// --------------------------------------------------------------------------

// ---------------- Routing -------------------------------------------------
#define ROUTING_FLOODING_DEBUG
#define ROUTING_STATIC_DEBUG

// ---------------- Localization --------------------------------------------
#define LOCALIZATION_DISTANCEBASED_DVHOP_DEBUG
#define LOCALIZATION_DISTANCEBASED_SUMDIST_DEBUG
#define LOCALIZATION_DISTANCEBASED_EUCLIDEAN_DEBUG

#define LOCALIZATION_DISTANCEBASED_MINMAX_DEBUG
#define LOCALIZATION_DISTANCEBASED_LATERATION_DEBUG

// #define LOCALIZATION_DISTANCEBASED_ITER_LATERATION_DEBUG

// --------------------------------------------------------------------------
// --------------------- Util -----------------------------------------------
// --------------------------------------------------------------------------

// ---------------- Wisebed Node API ----------------------------------------

#define UTIL_REMOTE_UART_DEBUG
// #define VIRTUAL_RADIO_DEBUG

#endif

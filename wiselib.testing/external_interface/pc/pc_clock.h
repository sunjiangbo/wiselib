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

#ifndef PC_CLOCK_H
#define PC_CLOCK_H

#define time_t posix_time_t
#include <time.h>
#undef time_t

namespace wiselib
{
   template<typename OsModel_P>
   class PCClockModel
   {
   public:
      typedef OsModel_P OsModel;

      typedef PCClockModel<OsModel> self_type;
      typedef self_type* self_pointer_t;

      //typedef struct timespec time_t;
      typedef uint16_t micros_t;
      typedef uint16_t millis_t;
      typedef uint32_t seconds_t;
      
      class time_t {
         public:
            time_t() { }
            
            time_t(timespec& t) : timespec_(t) {
            }
            
            time_t operator+(time_t& other) {
               time_t r(*this);
               r.timespec_.tv_sec += other.timespec_.tv_sec;
               r.timespec_.tv_nsec += other.timespec_.tv_nsec;
               if(r.timespec_.tv_nsec >= 1000000000L) {
                  r.timespec_.tv_sec++; r.timespec_.tv_nsec -= 1000000000L;
               }
               return r;
            }
            
            time_t operator-(time_t& other) {
               time_t r(*this);
               if(*this < other) {
                  r.timespec_.tv_sec = 0;
                  r.timespec_.tv_nsec = 0;
               }
               else {
                  r.timespec_.tv_sec -= other.timespec_.tv_sec;
                  r.timespec_.tv_nsec -= other.timespec_.tv_nsec;
                  if(timespec_.tv_nsec < other.timespec_.tv_nsec) {
                     r.timespec_.tv_sec--;
                     r.timespec_.tv_nsec += 1000000000L;
                  }
               }
               return r;
            }
            
            int cmp(time_t& other) {
               if(timespec_.tv_sec == other.timespec_.tv_sec) {
                  return (timespec_.tv_nsec < other.timespec_.tv_nsec) ? -1 : (timespec_.tv_nsec > other.timespec_.tv_nsec);
               }
               return timespec_.tv_sec > other.timespec_.tv_sec ? 1 : -1;
            }
            
            bool operator<(time_t& other) { return cmp(other) < 0; }
            bool operator>(time_t& other) { return cmp(other) > 0; }
            bool operator<=(time_t& other) { return cmp(other) <= 0; }
            bool operator>=(time_t& other) { return cmp(other) >= 0; }
            bool operator==(time_t& other) { return cmp(other) == 0; }
            bool operator!=(time_t& other) { return cmp(other) != 0; }
            
            struct timespec timespec_;
      };
      
      typedef time_t value_t;

      enum States
      {
         READY = OsModel::READY,
         NO_VALUE = OsModel::NO_VALUE,
         INACTIVE = OsModel::INACTIVE
      };

      PCClockModel();

      int state();
      time_t time();
      //void set_time(time_t); // not supported
      micros_t microseconds( time_t );
      millis_t milliseconds( time_t );
      seconds_t seconds( time_t );
   };

   template<typename OsModel_P>
   PCClockModel<OsModel_P>::
   PCClockModel()
   {
   }

   template<typename OsModel_P>
   int PCClockModel<OsModel_P>::
   state()
   {
      return READY;
   }

   template<typename OsModel_P>
   typename PCClockModel<OsModel_P>::time_t PCClockModel<OsModel_P>::
   time()
   {
      time_t time;

      clock_gettime( CLOCK_REALTIME, &time.timespec_ );
      return time;
   }

   template<typename OsModel_P>
   typename PCClockModel<OsModel_P>::micros_t PCClockModel<OsModel_P>::
   microseconds( time_t t )
   {
      return ( t.timespec_.tv_nsec / 1000 ) % 1000;
   }

   template<typename OsModel_P>
   typename PCClockModel<OsModel_P>::millis_t PCClockModel<OsModel_P>::
   milliseconds( time_t t )
   {
      return ( t.timespec_.tv_nsec / 1000000 ) % 1000;
   }

   template<typename OsModel_P>
   typename PCClockModel<OsModel_P>::seconds_t PCClockModel<OsModel_P>::
   seconds( time_t t )
   {
      return t.timespec_.tv_sec;
   }

} // namespace wiselib

#endif // PC_CLOCK_H


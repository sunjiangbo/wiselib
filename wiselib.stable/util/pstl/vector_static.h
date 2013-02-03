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
#ifndef __WISELIB_INTERNAL_INTERFACE_STL_VECTOR_STATIC_H
#define __WISELIB_INTERNAL_INTERFACE_STL_VECTOR_STATIC_H

#include "util/pstl/iterator.h"
#include <string.h>

namespace wiselib
{

   template<typename OsModel_P,
            typename Value_P,
            int VECTOR_SIZE>
   class vector_static
   {
   public:
      typedef Value_P value_type;
      typedef value_type* pointer;
      typedef value_type& reference;

      typedef vector_static<OsModel_P, value_type, VECTOR_SIZE> vector_type;

      typedef normal_iterator<OsModel_P, pointer, vector_type> iterator;

      typedef typename OsModel_P::size_t size_type;
      // --------------------------------------------------------------------
      vector_static()
      {
         start_ = &vec_[0];
         finish_ = start_;
         end_of_storage_ = start_ + VECTOR_SIZE;
      }
      // --------------------------------------------------------------------
      vector_static( const vector_static& vec )
      { *this = vec; }
      // --------------------------------------------------------------------
      ~vector_static() {}
      // --------------------------------------------------------------------
      vector_static& operator=( const vector_static& vec )
      {
         memcpy( vec_, vec.vec_, sizeof(vec_) );
         start_ = &vec_[0];
         finish_ = start_ + (vec.finish_ - vec.start_);
         end_of_storage_ = start_ + VECTOR_SIZE;
         return *this;
      }
      // --------------------------------------------------------------------
      ///@name Iterators
      ///@{
      iterator begin()
      { return iterator( start_ ); }
      // --------------------------------------------------------------------
      iterator end()
      { return iterator( finish_ ); }
      ///@}
      // --------------------------------------------------------------------
      ///@name Capacity
      ///@{
      size_type size() const
      { return size_type(finish_ - start_); }
      // --------------------------------------------------------------------
      size_type max_size() const
      { return VECTOR_SIZE; }
      // --------------------------------------------------------------------
      size_type capacity() const
      { return VECTOR_SIZE; }
      // --------------------------------------------------------------------
      bool empty() const
      { return size() == 0; }
      ///@}
      // --------------------------------------------------------------------
      ///@name Element Access
      ///@{
      reference operator[](size_type n)
      {
         return *(this->start_ + n);
      }
      // --------------------------------------------------------------------
      reference at(size_type n)
      {
         return (*this)[n];
      }
      // --------------------------------------------------------------------
      reference front()
      {
         return *begin();
      }
      // --------------------------------------------------------------------
      reference back()
      {
         return *(end() - 1);
      }
      // --------------------------------------------------------------------
      pointer data()
      { return pointer(this->start_); }
      ///@}
      // --------------------------------------------------------------------
      ///@name Modifiers
      ///@{
      template <class InputIterator>
      void assign ( InputIterator first, InputIterator last )
      {
         clear();
         for ( InputIterator it = first; it != last; ++it )
            push_back( *it );
      }
      // --------------------------------------------------------------------
      void assign( size_type n, const value_type& u )
      {
         clear();
         for ( unsigned int i = 0; i < n; ++i )
            push_back( u );
      }
      // --------------------------------------------------------------------
      void push_back( const value_type& x )
      {
         if ( finish_ != end_of_storage_ )
         {
            *finish_ = x;
            ++finish_;
         }
      }
      // --------------------------------------------------------------------
      void pop_back()
      {
         if ( finish_ != start_ )
            --finish_;
      }
      // --------------------------------------------------------------------
      iterator insert( const value_type& x )
      {
         return insert( end(), x );
      }
      // --------------------------------------------------------------------
      iterator insert( iterator position, const value_type& x )
      {
         // no more elements can be inserted because vector is full
         if ( size() == max_size() )
            return iterator(finish_);

         value_type cur = x, temp;
         for ( iterator it = position; it != end(); ++it )
         {
            temp = *it;
            *it = cur;
            cur = temp;
         }
         push_back( cur );

         return position;
      }
      // --------------------------------------------------------------------
      void insert( iterator position, size_type n, const value_type& x )
      {
         for ( int i = 0; i < n; ++i )
            insert( position, x );
      }
      // --------------------------------------------------------------------
      iterator erase( iterator position )
      {
         if ( position == end() )
            return end();

         for ( iterator cur = position; cur != end(); cur++ )
            *cur = *(cur + 1);

         pop_back();

         return position;
      }
      // --------------------------------------------------------------------
      iterator erase( iterator first, iterator last )
      {
         if ( first == end() || first == last )
            return first;

         iterator ret = first;

         while ( last != end() )
         {
            *first = *last;
            first++;
            last++;
         }

         finish_ = &(*first);
         return ret;
      }
      // --------------------------------------------------------------------
      void swap( vector_type& vec )
      {
         vector_type tmp = *this;
         *this = vec;
         vec = tmp;
      }
      // --------------------------------------------------------------------
      void clear()
      {
         finish_ = start_;
      }
      ///@}

   protected:
      value_type vec_[VECTOR_SIZE];

      pointer start_, finish_, end_of_storage_;
   };

}

#endif
/* vim: set ts=3 sw=3 tw=78 noexpandtab :*/
/* vim: set ts=3 sw=3 tw=78 expandtab :*/

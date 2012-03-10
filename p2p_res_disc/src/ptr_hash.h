/*
This program is part of Netmodeler, a library for graph and network
modeling and simulation.
Copyright (C) 2007  P. Oscar Boykin <boykin@pobox.com>, University of Florida

This program is free software; you can redistribute it and/or
modify it under the terms of the GNU General Public License
as published by the Free Software Foundation; either version 2
of the License, or (at your option) any later version.

This program is distributed in the hope that it will be useful,
but WITHOUT ANY WARRANTY; without even the implied warranty of
MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
GNU General Public License for more details.

You should have received a copy of the GNU General Public License
along with this program; if not, write to the Free Software
Foundation, Inc., 59 Temple Place - Suite 330, Boston, MA  02111-1307, USA.
*/

#ifndef starsky__ptr_hash_h
#define starsky__ptr_hash_h

namespace Starsky {

template<typename T>
struct ptr_hash {
  
  size_t operator()(const T* val) const {
    return (size_t)((size_t)val >> 2);
  }

};

}
#endif

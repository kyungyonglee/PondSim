/*
This program is part of Netmodeler, a library for graph and network
modeling and simulation.
Copyright (C) 2005  University of California
Copyright (C) 2005  P. Oscar Boykin <boykin@pobox.com>, University of Florida

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

#ifndef starsky__degreelawnetfac_h
#define starsky__degreelawnetfac_h

#include <networkfactory.h>
#include <random.h>
#include <discreterandvar.h>

namespace Starsky {

/**
 * A NetworkFactory that produces random networks with given
 * degree distributions
 */
	
class DegreeLawNetFac : public NetworkFactory {

  public:
    DegreeLawNetFac(int nodes, DiscreteRandVar& dpf, Random& ran, bool indep=true);
    DegreeLawNetFac(int nodes, DiscreteRandVar& dpf, Random& ran,
                    NodeFactory* nf, EdgeFactory* ef,
                    bool indep=true);

    virtual Network* create();
  protected:

    int _nodes;
    DiscreteRandVar& _dpf;
    Random& _rand;
    bool _indep;
	
};
	
}


#endif

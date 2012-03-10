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

#include "newmancom.h"

using namespace std;
using namespace Starsky;

double NewmanCom::getNextJoin(const Network& net,
                               const std::map<Node*, int>& node_community,
		               const std::map<int, std::map<int, double> >& e_ij,
			       const vector< double >& a_i,
			       int& join1, int& join2)
{
    double delta_q, tmp_delta;
    bool got_first = false;
    //It is important to initialize the communities with
    //sane values.  If (for some reason) there are no edges
    //between these communities,
    if( node_community.size() > 1 ) {
      map<Node*, int>::const_iterator tmpit;
      tmpit = node_community.begin();
      join1 = tmpit->second;
      tmpit++;
      join2 = tmpit->second;
      //Make sure join1 <= join2:
      if( join2 < join1 ) { int tmp = join1; join1 = join2; join2 = tmp; }
    }
    else {
      join1 = node_community.begin()->second;
      join2 = join1;
    }
    delta_q = 0.0;
    int com1, com2;
    map<Node*, int>::const_iterator com1_it, com2_it;
    auto_ptr<EdgeIterator> ei( net.getEdgeIterator() );
    while( ei->moveNext() ) {
      Edge* this_edge = ei->current();
      com1_it = node_community.find( this_edge->first );
      com2_it = node_community.find( this_edge->second );
      com1 = com1_it->second;
      com2 = com2_it->second;
      if( com1 != com2 ) {
	std::map<int, std::map<int, double> >::const_iterator mit = e_ij.find(com1);
	std::map<int, double>::const_iterator mit2 = mit->second.find(com2);
	double eij = mit2->second;
        tmp_delta = 2 * (eij - a_i[com1] * a_i[com2] );
        if( (!got_first) || ( tmp_delta > delta_q ) ) {
          //Make sure join1 <= join2:
          if( com1 < com2 ) { join1 = com1; join2 = com2; }
          else { join1 = com2; join2 = com1; }
          delta_q = tmp_delta;
	  got_first = true;
        }
      }
    }
    
    return delta_q;
}

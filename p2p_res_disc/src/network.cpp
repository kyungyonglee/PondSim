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

#include "network.h"

//#define DEBUG_MSG(msg) std::cerr << __FILE__ << " (" << __LINE__ << ") " << msg << std::endl;
#define DEBUG_MSG(msg)
//#define DEBUG

using namespace std;
using namespace Starsky;

//Initialization of the static empty node set
const Network::NodePSet Network::_empty_nodeset;
const Network::EdgeSet Network::_empty_edgeset;
const Network Network::_empty_net;

Network::CountMap Network::_ref_count;


Network::Network()  {
  _edge_count = 0;
}

Network::Network(const Network& net) {
  operator=(net);
}

Network::~Network() {
    DEBUG_MSG("~Network: " << this)
#ifdef DEBUG
    cout << "In Network::~Network() about to clear()" << endl;
#endif
    clear();
    DEBUG_MSG("In Network::~Network() cleared()")
}

Network::Network(istream& input) {
  _edge_count = 0;
  readFrom(input);
}

bool Network::add(const Edge& e) {
   Edge* n_e = getEdgePtr(e);
   if( e.first == e.second ) {
	   cerr << "adding a loop: (" << e.first << ", " << e.second
		<< ")"  << endl;
   }
   if( n_e == 0 ) {
     n_e = e.clone();
   }
   return add(n_e);
}

bool Network::add(Edge* edge) {
    
    bool success = false;
    if( !has(edge) ) {
      //This is a new edge:
      list<INetworkMonitor*>::iterator nm_it;
      for(nm_it = _net_mon.begin(); nm_it != _net_mon.end(); nm_it++) {
        (*nm_it)->preEdgeAdd(edge);
      }
     
      //Account for the edges and nodes

      //This is a new edge, so bump the ref count:
      incrementRefCount(edge);
      //make sure these nodes are in the network
      add(edge->first);
      add(edge->second);
         
      _node_to_edges[edge->first].insert(edge);
      _node_to_edges[edge->second].insert(edge);
      //Increment the edge count:
      _edge_count++;
      
      //Tell the network monitors
      for(nm_it = _net_mon.begin(); nm_it != _net_mon.end(); nm_it++) {
        (*nm_it)->postEdgeAdd(edge);
      }
      success = true;
    }
    else {
      //We already have this edge, don't add it again
      success = false;
    }
    return success;
}

void Network::add(const Network* net) {
  auto_ptr<NodeIterator> ni( net->getNodeIterator() );
  while( ni->moveNext() ) { add( ni->current() ); }
  auto_ptr<EdgeIterator> ei( net->getEdgeIterator() );
  while( ei->moveNext() ) { add( ei->current() ); }
}

bool Network::add(Node* node) {
#ifdef DEBUG
  cout << "Network(" << this << ") adding Node: " << node << endl;
#endif
  if( ! has(node) ) {
    list<INetworkMonitor*>::iterator nm_it;
    for(nm_it = _net_mon.begin(); nm_it != _net_mon.end(); nm_it++) {
      (*nm_it)->preNodeAdd(node);
    }
    _node_to_edges[ node ] = _empty_edgeset;
    incrementRefCount(node);
    for(nm_it = _net_mon.begin(); nm_it != _net_mon.end(); nm_it++) {
      (*nm_it)->postNodeAdd(node);
    }
    return true;
  }
  else {
    //The node was already there:
  }
  return false;
}

void Network::add(INetworkMonitor* nm)
{
  _net_mon.push_back(nm);
  nm->postAdd(this);
}

void Network::clear() {
    //Delete all the memory
#ifdef DEBUG
    cout << "About to decrement all edges" << endl;
#endif
    clearEdges();
#ifdef DEBUG 
    cout << "About to decrement all nodes" << endl;
#endif
    auto_ptr<NodeIterator> ni( getNodeIterator() );
    while( ni->moveNext() ) {
#ifdef DEBUG
      cout << "Decrementing: " << ni->current() << endl;
#endif
      Node* node = ni->current();
      decrementRefCount(node);
    }
    _node_to_edges.clear();
}

void Network::addJoiningEdgesFrom(const Network* net)
{
  auto_ptr<NodeIterator> ni( getNodeIterator() );
  while(ni->moveNext()) {
    Node* this_node = ni->current();
    //Iterate over all the edges this node has in
    //the other network and and them if both ends
    //are in this network:
    EdgeIterator* eip = net->getEdgeIterator(this_node); 
    if( eip != 0 ) {
      //This node is in the network
      auto_ptr<EdgeIterator> ei( eip );
      while( ei->moveNext() ) {
        Edge* this_edge = ei->current();
        Node* other = this_edge->getOtherNode(this_node);
        if( has(other) ) {
          add( this_edge );
        }
        else {
          //The other end is not in this network.
        }
      }
    }
    else {
      //This node is not in net.
    }
  }
}

void Network::clearEdges() {
    GraphMap::iterator neit = _node_to_edges.begin();
    EdgeSet edges;
    for(neit = _node_to_edges.begin();
        neit != _node_to_edges.end();
        neit++) {
      edges.insert( neit->second.begin(), neit->second.end() );
      //Clear this set of edges:
      neit->second.clear();
    }
    //Now actually remove the edges:
    EdgeSet::iterator eit;
    for( eit = edges.begin();
         eit != edges.end();
         eit++ ) {
      decrementRefCount(*eit);
    }
}

Network* Network::clone() const {
  Network* net = newNetwork();
  *net = *this;
  return net;
}

int Network::decrementvRefCount(void* p) {
    CountMap::iterator ref_it = _ref_count.find(p);
    
    ref_it->second = ref_it->second - 1;
    int ret = ref_it->second;
    if( ret == 0 ) { _ref_count.erase( ref_it ); }
    return ret;
}

int Network::getAssociatedNumber(Node* aNode) const {

    /**
     * \todo getDistance never returns -1, maybe we want to do so
     * in networks with more than one component
     */
    return getDistance(aNode);
}

double Network::getAverageDegree() const {
    double dn = getNodeSize();
    double de = getEdgeSize();
    return 2.0 * de / dn;
}

vector<int> Network::getCCHist(int bins) const {

    vector<int> ret_val;
    double this_cc;
    int this_bin;
    
    ret_val.resize(bins);
    for(int j = 0; j < bins; j++) {
        ret_val[j] = 0;
    }
    
    auto_ptr<NodeIterator> ni( getNodeIterator() );
    while( ni->moveNext() ) {
      Node* n = ni->current();
      if( getDegree( n ) != 0 ) {
        this_cc = getClusterCoefficient( n );
	this_bin = (int)(this_cc * (double)bins);
	if( this_cc < 1.0) {
	  ret_val[this_bin] = ret_val[this_bin] + 1;
	}
	else {
          ret_val[bins - 1] = ret_val[bins - 1] + 1;
	}
      }
    }
    return ret_val;

}

map<int, double> Network::getCCvsDegree() const {

    map<int, set<double> > all_cc;
    map<int, set<double> >::const_iterator all_it;
    set<double>::const_iterator d_it;
    map<int, double> ret_val;
    int degree;
    double cc;
    
    auto_ptr<NodeIterator> ni( getNodeIterator() );
    while( ni->moveNext() ) {
      Node* n = ni->current();
        cc = getClusterCoefficient( n );
	degree = getDegree( n );
	all_cc[degree].insert(cc);
    }
    double average;
    for(all_it = all_cc.begin(); all_it != all_cc.end(); all_it++) {
	average = 0.0;
        for(d_it = all_it->second.begin();
	    d_it != all_it->second.end();
	    d_it++) {

            average += *d_it;
	}
	average /= (double)(all_it->second.size());
	ret_val[all_it->first] = average;
    }
    return ret_val;
	
}

double Network::getCCStdErr() const {
  double cc = getClusterCoefficient();
  if( cc == 0.0 ) {
    /*
     * If the whole network has cc = 0, removing nodes won't change it
     */
    return 0.0;
  }
  double sigma2 = 0.0;
  auto_ptr<NodeIterator> ni( getNodeIterator() );
  while( ni->moveNext() ) {
    Network tmp = *this;
    tmp.remove( ni->current() );
    double tmp_cc = tmp.getClusterCoefficient();
    sigma2 += (cc - tmp_cc) * (cc - tmp_cc);
  }
  double n_count = (double)getNodeSize();
  return sqrt( (n_count - 1.0) * sigma2/n_count );
}

NodeIterator* Network::getNeighborIterator(Node* n) const {
  GraphMap::const_iterator neit = _node_to_edges.find(n);
  if( neit == _node_to_edges.end() ) {
    //No such edge:
    return 0;
  }
  
  return new NeighborIterator(n, neit->second );
}

Network* Network::getNeighbors(Node* node) const {
  GraphMap::const_iterator it;
  it = _node_to_edges.find(node);
  Network* net = newNetwork();
  if( it != _node_to_edges.end() ) {
    EdgeSet::const_iterator eit;
    //Add all the Nodes:
    for( eit = it->second.begin();
         eit != it->second.end();
         eit++) {
      //Add them in:
      net->add( (*eit)->getOtherNode( node ) ); 
    }
  }
  else {
    //Return an empty network
  }
  return net;
}

double Network::getClusterCoefficient(Node* node) const {
  int degree = getDegree(node);
  if( degree > 1 ) {
    int triangles = getTriangles(node);
    double dd = (double)degree;
    double d2 = dd * (dd - 1.0)/2.0;
    return (double)triangles/d2;
  }
  else {
    //This is not well defined;
    return 0.0;
  }
}

double Network::getClusterCoefficient(NodeIterator* nodes) const {
    double out = 0.0;
    double this_cc = 0.0;
    int nodes_count = 0;
    while( nodes->moveNext() ) {
        Node* this_node = nodes->current();
	//Skip nodes that have 1 degrees or less
        if( getDegree( this_node ) > 1 ) {
          this_cc = getClusterCoefficient(this_node);
          out += this_cc;
	}
	++nodes_count;
    }
    if ( nodes_count == 0 ) { return 0.0; }
    return out / (double)( nodes_count );
}

double Network::getClusterCoefficient() const {
    auto_ptr<NodeIterator> ni( getNodeIterator() );
    return getClusterCoefficient( ni.get() );
}

int Network::getDegree(Node* node) const {
    GraphMap::const_iterator i = _node_to_edges.find(node);
    
    if( i != _node_to_edges.end() ) {
      return i->second.size();
    }
    else {
      return 0;
    }
}

int Network::getDistancesFrom(Node* start,
		              std::map<Node*, int>& distances,
			      std::map<Node*, int>& weights,
			      std::set<Node*>& leaf_nodes,
			      int max_depth) const {
  
  //make sure the output is intialized
  distances.clear();
  weights.clear();
  leaf_nodes.clear();
  //This is a queue to keep track of the nodes doing a breadth first search
  queue<Node*> to_visit;
  distances[start] = 0;
  weights[start] = 1;
  int max_dist = 0,
      distance = 0,
      neighbor_max_dist = 0,
      weight = 1;
  to_visit.push(start);
  Node* tmp_node = 0;
  //Only go a certain depth:
  bool is_max_depth = (max_depth != -1);
  map<Node*, int>::iterator dit;
  
  while( to_visit.size() > 0 ) {
    tmp_node = to_visit.front();
    int tmp_node_distance = distances[tmp_node];
    distance = tmp_node_distance + 1;
    weight = weights[tmp_node];
    //Just stop if the distance is more than we should go
    if( is_max_depth && (distance > max_depth) ) { break; }
    neighbor_max_dist = 0;
    NeighborIterator neighit(tmp_node, _node_to_edges.find(tmp_node)->second );
    while( neighit.moveNext() ) {
      Node* n = neighit.current();
      dit = distances.find( n );
      if( dit == distances.end() ) {
	//We have not yet visited this node
	pair< map<Node*, int>::iterator, bool> result;
        result = distances.insert( pair<Node*, int>(n, distance) );
        //Update dit:
        dit = result.first;
	weights.insert( pair<Node*, int>(n, weight) );
	to_visit.push( n );
	//update the maximum
	max_dist = (max_dist < distance ) ? distance : max_dist;
      }
      else if( dit->second == distance ) {
        //There is more than one way to reach this node:
	weights[n] += weight;
      }
      //We know dit:
      if( neighbor_max_dist < dit->second ) {
        neighbor_max_dist = dit->second;
      }
    }
    /**
     * We check all the nodes at distance d before d+1 (breadth first)
     * so, once a distance is assigned it is never decreased
     * Hence, we know now the distance of all tmp_node neighbors.
     * If they are all less than or equal to distance from tmp_node,
     * then tmp_node must be a leaf node
     */
    if( neighbor_max_dist <= tmp_node_distance ) {
      leaf_nodes.insert(tmp_node);
    }
    //Remove the first element in the list
    to_visit.pop();
  }
  return max_dist; 
}

int Network::getDistance(Node* start, Node* end) const {

    //A slight optimization, we could also get all the neighbors and check:
    if (start == end) return 0;
    map<Node*, int> distances, weights;
    map<Node*, int>::iterator dit;
    set<Node*> leafs;
    //We just find all the distances:
    int max = getDistancesFrom(start, distances, weights, leafs);
    if( end != 0) {
      dit = distances.find(end);
      if( dit != distances.end() ) { return dit->second; }
    }
    if( end == 0 ) { return max; }
    //We only get here if there is no path from start to end
    return -1;
}

int Network::getDistanceDist(Node* start,
		             map<int, int>& distribution) const {
  map<Node*, int> distances, weights;
  map<Node*, int>::iterator dit;
  set<Node*> leaf_nodes;
  
  int max = getDistancesFrom(start, distances, weights, leaf_nodes);
  auto_ptr<NodeIterator> ni( getNodeIterator() );
  while( ni->moveNext() ) {
    dit = distances.find( ni->current() );
    if( dit == distances.end() ) {
      //This node is unreachable from start
      distribution[-1]++;
    }
    else {
      distribution[ dit->second ]++;
    }
  }
  return max;
}


int Network::getDistanceDist(map<int, int>& distribution) const {
  map<Node*, int> distances, weights;
  map<Node*, int>::iterator dit;
  set<Node*> leaf_nodes;
  
  int max = -1,
      tmp_max,
      radius = 0;
  auto_ptr<NodeIterator> ni( getNodeIterator() );
  while( ni->moveNext() ) {
    tmp_max = getDistancesFrom( ni->current(), distances, weights, leaf_nodes);
    max = (max < tmp_max) ? tmp_max : max;
    radius = (tmp_max < radius) ? tmp_max : radius;
    auto_ptr<NodeIterator> mit( getNodeIterator() );
    while( mit->moveNext() ) {
      dit = distances.find( mit->current() );
      if( dit == distances.end() ) {
        //This node is unreachable from start
        distribution[-1]++;
      }
      else {
        distribution[ dit->second ]++;
      }
    }
  }
  //It is easy to find the center if we want to do that...
  return radius;
}

Edge* Network::getEdgeBetweennessFor(Node* target,
		                     map<Edge*, double>& my_between) const
{
  //Start a new betweeness calculation
  my_between.clear();
  map<Node*, int> distances, weights;
  set<Node*> leafs;
  //Get the weights of all the nodes and find the leaf nodes
  int mdist = getDistancesFrom(target, distances, weights, leafs);
  //Initialize the edges from the leafs:
  set<Node*>::iterator leaf_it;
  Node* tmp_node, *other;
  double max_between = 0.0, tmp_between = 0.0;
  Edge* max_edge = 0;
  /**
   * Here is map so we can go from greatest to shortest distance.
   * This ensures that we have always calculated the "below score"
   * before trying to compute score for an edge
   */
  map<int, NodePSet> distance_to_nodes;
  for( leaf_it = leafs.begin();
       leaf_it != leafs.end();
       leaf_it++) {
    auto_ptr<EdgeIterator> es_it( getEdgeIterator( *leaf_it ) );
    while( es_it->moveNext() ) {
      Edge* this_edge = es_it->current();
      other = this_edge->getOtherNode( *leaf_it );
      if( leafs.find( other ) == leafs.end() ) {
        //This edge does not go to another leaf:
	tmp_between = (double)weights[other] /
		                    (double)weights[*leaf_it];
	my_between[ this_edge ] = tmp_between;
        if( tmp_between > max_between ) {
          max_between = tmp_between;
	  max_edge = this_edge;
	}
	//Climb it in an order.  When we insert we see if
	//it is new or not:
	distance_to_nodes[ distances[other] ].insert(other);
      }
    }
  }
  map<Edge*, double>::iterator betit;
  bool have_all_below = false;
  EdgeSet above_edges;
  //We have initialized the first edges
  map<int, NodePSet>::reverse_iterator dit;
  NodePSet::iterator nit;
  for( dit = distance_to_nodes.rbegin();
       dit != distance_to_nodes.rend();
       dit++) {
   while( dit->second.size() > 0 ) {
    nit = dit->second.begin();
    //Take care of the next node
    tmp_node = *nit;
    //Initialize variables for the below score
    double below_score = 0.0;
    above_edges.clear();
    have_all_below = true;
    //Look at all the edges for this node to get the below score
    auto_ptr<EdgeIterator> es_it( getEdgeIterator( tmp_node ) );
    while( es_it->moveNext() ) {
      Edge* this_edge = es_it->current();
      other = this_edge->getOtherNode( tmp_node );
      if( distances[other] > distances[tmp_node] ) {
        //The neighbor is further from target
	betit = my_between.find(this_edge);
	if( betit != my_between.end() ) {
	  //We already have the score for this edge
	  below_score += betit->second;
	}
	else if( have_all_below ) {
	  //We have not scored this edge
	    have_all_below = false;
	    cerr << "Don't have all below!! Node: "
		 << tmp_node << " = " << tmp_node->toString() << endl;
	}
      }
      else if( distances[other] < distances[tmp_node] ) {
        //This is an above edge:
	above_edges.insert( this_edge );
      }
    }
    if( have_all_below ) {
      //We can compute the betweenness for all above_edges
      double weight_ratio = 0.0;
      for(EdgeSet::iterator e_it = above_edges.begin();
	  e_it != above_edges.end();
	  e_it++) {
        //Make sure we don't try to score edges twice
        if( my_between.find( *e_it ) == my_between.end() ) {
          other = (*e_it)->getOtherNode( tmp_node );
          weight_ratio = (double)weights[other]/(double)weights[tmp_node];
	  tmp_between = (below_score + 1.0) * weight_ratio;
	  my_between[ *e_it ] = tmp_between;
          if( tmp_between > max_between ) {
            max_between = tmp_between;
	    max_edge = *e_it;
	  }
	  distance_to_nodes[ distances[other] ].insert( other );
        }
	else {
          //This should not happen:
	  cerr << "Tried to score an edge twice: "
	       << "Edge: " << *e_it << ", " << (*e_it)->toString() << endl;
	}
      }
      /**
       * We have computed the betweenness for all above edges
       * and since we go from large distance to small distance
       * we can never see this node again (all future nodes will
       * be as close or closer than this one, never further).
       */
      dit->second.erase(nit);
    }
   }
  }
  return max_edge;
}

Edge* Network::getEdgeBetweenness(map<Edge*, double>& betweenness) const {
  
  Edge* max_edge = 0;
  double max_between = 0.0, tmp_between = 0.0;
  map<Edge*, double> my_between;
  auto_ptr<NodeIterator> ni( getNodeIterator() );
  while( ni->moveNext() ) {
    max_edge = getEdgeBetweennessFor( ni->current(), my_between);
    /*
    cout << "source: " << (*n_it)->toString() << endl
	 << "max edge: " << max_edge->toString() << endl
	 << "betweenness: " << my_between[max_edge] << endl;
    */
    //We have score each of the appropriate edges:
    map<Edge*, double>::iterator betit;
    for(betit = my_between.begin();
        betit != my_between.end();
        betit++) {
      betweenness[ betit->first ] += betit->second;
      tmp_between = betweenness[ betit->first ];
      if( tmp_between > max_between ) {
        max_between = tmp_between;
        max_edge = betit->first;
      }
    }
  }
  return max_edge;
}

double Network::getEdgeCC(Edge* e) const {

  /*
   * First get the number of wedges:
   */
  int w = getWedges(e);
  if( w > 0 ) {
    double wedges = (double)w;
  /*
   * Each edge can be in two wedges in a particular triangle,
   * and each wedge can be in at most one triangle, so
   * we define the edgeCC as the ratio of the number of 
   * of triangles to wedges/2:
   */
    double triangles = (double)getTriangles(e);
    double cc = 2.0 * triangles/wedges;
    return cc;
  }
  else {
    return 0.0;
  }
}

Network* Network::getEdges(Node* node) const {
  Network* net = newNetwork();
  GraphMap::const_iterator neit = _node_to_edges.find(node);
  if( neit != _node_to_edges.end() ) {
    EdgeSet::const_iterator eit;
    for(eit = neit->second.begin();
        eit != neit->second.end();
        eit++) {
      //Add all the edges:
      net->add( *eit );
    }
  }
  return net;
}

Edge* Network::getEdge(Node* from, Node* to) const {
  if( (0 == from) || (0 == to) ) {
    return 0;
  }
  int df = getDegree(from);
  int dt = getDegree(to);
  if( (df == 0) || (dt == 0) ) {
    //One of them has no edges:
    return 0;
  }
  //Get the edges of the node with the smaller degree
  Node* check = from;
  if( df > dt ) {
    check = to;
  }
  GraphMap::const_iterator it;
  it = _node_to_edges.find(check);
  const EdgeSet& eds = it->second;
  EdgeSet::const_iterator eit = eds.begin();
  for(eit = eds.begin(); eit != eds.end(); eit++)
  {
    if( (*eit)->connects( from,  to ) ) {
      return *eit;
    }
  }
  return 0; 
}

EdgeIterator* Network::getEdgeIterator() const
{
  return new Network::NetEdgeIterator(_node_to_edges);
}

EdgeIterator* Network::getEdgeIterator(Node* n) const
{
  GraphMap::const_iterator neit = _node_to_edges.find(n);
  if( neit == _node_to_edges.end() ) {
    //No such edge:
    return 0;
  }
  return new ContainerIterator<EdgeSet>(neit->second);
}

Edge* Network::getEdgePtr(const Edge& edge) const {
    
    //Find the ptr of the matching edge:
    GraphMap::const_iterator it1, it2;
    it1 = _node_to_edges.find(edge.first);
    it2 = _node_to_edges.find(edge.second);
    //Make sure these nodes have some edges:
    if( (it1 == _node_to_edges.end()) || (it2 == _node_to_edges.end() ) ) {
      return 0;
    }
    
    EdgeSet::const_iterator eit1;
    //Look through the smaller set:
    if( it1->second.size() < it2->second.size() ) {
       for( eit1 = it1->second.begin(); eit1 != it1->second.end(); eit1++) {
         if( (*(*eit1))==edge ) {
           return *eit1;
	 }
       }
    }
    else {
        for( eit1 = it2->second.begin(); eit1 != it2->second.end(); eit1++) {
          if( (*(*eit1))== edge ) {
            return *eit1;
	  }
	}
    }
    return 0;
}

int Network::getEdgeSize() const {
  return _edge_count;
}

EdgeIntMember Network::getEIMember(const std::string& name) const {

  if (name == "getTriangles") {
      return &Network::getTriangles;
  }
  else if( name == "getWedges" ) {
    return &Network::getWedges;
  }
  return 0;
}
Iterator<std::string>* Network::getEIMembers() const {
  std::set<std::string>* mems = new std::set<std::string>();
  mems->insert("getTriangles");
  mems->insert("getWedges");
  return new ContainerPIterator<std::set<std::string> >(mems);
}

EdgeDoubMember Network::getEDMember(const std::string& name) const {
  if (name == "getEdgeCC") {
    return &Network::getEdgeCC;
  }
  return 0;
}

Iterator<std::string>* Network::getEDMembers() const {
  std::set<std::string>* mems = new std::set<std::string>();
  mems->insert("getEdgeCC");
  return new ContainerPIterator<std::set<std::string> >(mems);
}

double Network::getExpectedTransitivity() const {
  
  double exp_cc;
  int n = getNodeSize();
  double edges = (double)getEdgeSize();
  /*
   * First we need to compute the degree distribution
   */
  double k = 0.0;
  map<int, int> degdist;
  auto_ptr<NodeIterator> ni( getNodeIterator() );
  while( ni->moveNext() ) {
    int deg = getDegree( ni->current() );
    degdist[deg] = degdist[deg] + 1;
    k += (double)deg; 
  }
  k = k/(double)getNodeSize();
  /*
   * Now we have the degree distribution
   */
  map<int, int>::const_iterator degit,degit2;
  exp_cc = 0.0;
  double coeff;
  double deg,deg2,dcount,dcount2;
  for(degit2=degdist.begin(); degit2!=degdist.end(); degit2++) {
    for(degit=degdist.begin(); degit!=degdist.end(); degit++) {
      deg = (double)degit->first - 1.0; //Excess degree
      dcount = (double)degit->second/((double)n) * (deg + 1.0)/k;
      deg2 = (double)degit2->first - 1.0; //Excess degree
      dcount2 = (double)degit2->second/((double)n) * (deg2 + 1.0)/k;
      coeff = (1.0 - deg*deg2/(2*edges*edges));
      coeff = pow(coeff,edges);
      exp_cc += (dcount * dcount2 * ( 1.0 - coeff));
    }
  }
  return exp_cc;
}

NodeIntMember Network::getNIMember(const std::string& name) const {
  if (name == "getAssociatedNumber") {
      return &Network::getAssociatedNumber;
  }
  else if (name == "getDegree" ) {
      return &Network::getDegree;
  }
  else if (name == "getTriangles" ) {
      return &Network::getTriangles;
  }
  else if (name == "getWedges") {
      return &Network::getWedges;
  }
  return 0;
}

Iterator<std::string>* Network::getNIMembers() const {
  std::set<std::string>* mems = new std::set<std::string>();
  mems->insert("getAssociatedNumber");
  mems->insert("getDegree");
  mems->insert("getTriangles");
  mems->insert("getWedges");
  return new ContainerPIterator<std::set<std::string> >(mems);
}

NodeDoubMember Network::getNDMember(const std::string& name) const {
  if (name == "getClusterCoefficient" ) {
      return &Network::getClusterCoefficient;
  }
  return 0;
}
Iterator<std::string>* Network::getNDMembers() const {
  std::set<std::string>* mems = new std::set<std::string>();
  mems->insert("getClusterCoefficient");
  return new ContainerPIterator<std::set<std::string> >(mems);
}

#ifndef HIDE_STL

Network::NodePSet Network::getNeighborhood(Node* node, int distance) const {

 map<Node*, int> distances, weights;
 set<Node*> leaves;
 /* Use the general breadth first code */
 getDistancesFrom(node, distances, weights, leaves, distance);
 /* Now the nodes we reached are all the ones with a distance: */
 NodePSet result;
 map<Node*, int>::iterator n_it;
 for(n_it = distances.begin(); n_it != distances.end(); n_it++) {
   result.insert( n_it->first ); 
 }
 return result;
}

#endif

NodeIterator* Network::getNodeIterator() const {
  Network::NetNodeIterator* ni = new Network::NetNodeIterator();
  ni->_begin = _node_to_edges.begin();
  ni->_end = _node_to_edges.end();
  ni->reset();
  return ni;
}

int Network::getNodeSize() const {
  return _node_to_edges.size();
}

double Network::getTransitivity() const {
  int triangles, wedges;
  getTrianglesWedges(triangles,wedges);
  return 3.0 * (double)triangles/(double)wedges;
}

int Network::getTriangles(Node* n) const {
  /*
   * The number of triangles is the number of edges
   * between the first neighbors of n.
   * The neighborhood includes all nodes and edges that
   * are distance 1 or less from n, so get the neighborhood
   * and remove n
   */
  auto_ptr<Network> hood( getEdges(n) );
  hood->addJoiningEdgesFrom( this );
  //Remove the node in question, what remains
  //is the edges between the neighbors:
  hood->remove(n);
  return hood->getEdgeSize();
}

int Network::getTriangles(Edge* this_edge) const {

  int triangles = 0;
  int wedges = 0;
  //Look at the wedges from one end:
  Node* this_node = this_edge->first;
  Node* other1 = this_edge->second;
  auto_ptr<EdgeIterator> ei1( getEdgeIterator(this_node) );
  while( ei1->moveNext() ) {
    Edge* edgeone = ei1->current();
    if( edgeone != this_edge ) {
      //There is one more wedge:
      wedges++;
      //See if it is a triangle:
	Node* other = edgeone->getOtherNode(this_node);
      if( getEdge(other, other1) != 0 ) {
        //There is an edge between other - other1
        triangles++;
      }
    }
  }
  //Look at the wedges from the other end:
  this_node = this_edge->second;
  other1 = this_edge->first;
  auto_ptr<EdgeIterator> ei2( getEdgeIterator(this_node) );
  while( ei2->moveNext() ) {
    Edge* edgetwo = ei2->current();
    if( edgetwo != this_edge ) {
      //There is one more wedge:
      wedges++;
      //See if it is a triangle:
	Node* other = edgetwo->getOtherNode(this_node);
      if( getEdge(other, other1) != 0 ) {
        //There is an edge between other - other1
        triangles++;
      }
    }
  }
  //We saw the triangles 2 times, once
  //from one "end" once from the other
  //reduce the number:
  triangles /= 2;
  return triangles;
}

void Network::getTrianglesWedges(int& triangles, int& wedges) const {

  /*
   * Iterate over all edges and count the number
   * of triangles and wedges seen.
   *
   * Each edge will be seen six times in a triangle
   * and twice in a wedge.
   * 
   */
  triangles = 0;
  wedges = 0;
  auto_ptr<EdgeIterator> ei( getEdgeIterator() );
  while( ei->moveNext() ) {
    Edge* this_edge = ei->current();
    //Look at the wedges from one end:
    Node* this_node = this_edge->first;
    Node* other1 = this_edge->second;
    auto_ptr<EdgeIterator> ei2( getEdgeIterator( this_node ) );
    while( ei2->moveNext() ) {
      Edge* edge2 = ei2->current(); 
      if( edge2 != this_edge ) {
        //There is one more wedge:
        wedges++;
        //See if it is a triangle:
	Node* other = edge2->getOtherNode(this_node);
        if( getEdge(other, other1) != 0 ) {
          //There is an edge between other - other1
          triangles++;
        }
      }
    }
    //Look at the wedges from the other end:
    this_node = this_edge->second;
    other1 = this_edge->first;
    auto_ptr<EdgeIterator> ei3( getEdgeIterator(this_node) );
    while( ei3->moveNext() ) {
      Edge* edge3 = ei3->current();
      if( edge3 != this_edge ) {
        //There is one more wedge:
        wedges++;
        //See if it is a triangle:
	Node* other = edge3->getOtherNode(this_node);
        if( getEdge(other, other1) != 0 ) {
          //There is an edge between other - other1
          triangles++;
        }
      }
    }
  }
  //We saw the triangles 6 times, reduce the number:
  triangles /= 6;
  //We saw edge wedge 2 times:
  wedges /= 2;
}

int Network::getWedges(Node* n) const
{
  int w = getDegree(n);
  w = w * (w - 1)/2;
  return w;
}

int Network::getWedges(Edge* e) const
{
  int wedges = getDegree(e->first) + getDegree(e->second) - 2;
  return wedges;
}
bool Network::has(const Edge& edge) const {
    return ( 0 != getEdgePtr(edge));
}

bool Network::has(Edge* edge) const {
  GraphMap::const_iterator neit = _node_to_edges.find( edge->first );
  if( neit != _node_to_edges.end() ) {
    //We have the first node, but do we have the edge:
    return (neit->second.count( edge ) != 0 );
  }
  else {
    //This network does not have the node, it can't have the edge:
    return false;
  }
}

bool Network::has(Node* node) const {
    return (_node_to_edges.find(node) != _node_to_edges.end());
}

int Network::incrementvRefCount(void* p) {
    CountMap::iterator ref_it = _ref_count.find(p);
    if( ref_it != _ref_count.end() ) {
      //This was here for debugging, but valgrind thinks everything is fine,
      //and it was never triggered, so don't bother:
      //if( ref_it->second == 0 ) { cerr << "going from 0 -> 1 "<< p << endl; } 
      ref_it->second = ref_it->second + 1;
      return ref_it->second;
    }
    else {
      _ref_count.insert( pair<void*, int>(p, 1) );
      return 1;
    }
    return -1;
}

Network* Network::newNetwork() const {
  return new Network();
}

void Network::printTo(ostream& out) const {

#if 1
   /*
    * This a really simple format:
    * node : neighbor1 neighbor2 ... neighborN <newline>
    */
  auto_ptr<NodeIterator> ni1( getNodeIterator() );
  while( ni1->moveNext() ) {
    Node* n1 = ni1->current();
    //Print all its neighbors:
    auto_ptr<NodeIterator> ni2( getNeighborIterator(n1) );
    out << n1->toString() << " :";
    while( ni2->moveNext() ) {
      out << " " << ni2->current()->toString();
    }
    //That's all the neighbors
    out << endl;
  }
#endif
	
#if 0
    /*
     * This is for use with graphviz, an AT&T graph visualization package
     * I had to cast the add some text ("n") so as not to confuse graphviz.
     */
	
    NodePSet::const_iterator i;
    ConnectedNodePSet::const_iterator j;
    map< Node*, ConnectedNodePSet >::const_iterator k;
    string label;
    int node_count = 0;
    
    out << "graph g {" << endl;
    for(i = node_set.begin(); i != node_set.end(); i++) {
	label = (*i)->toString();
	if ( label != "" ) {
	  out << "n" << *i << " [label=\"" << label 
		  << "\",width=0.25,height=0.25,fontsize=9];" << endl;
	}
	else {
          out << "n" << *i << " [label=\"" << node_count
		  << "\",width=0.25,height=0.25,fontsize=9];" << endl;
	}
        k = connection_map.find( *i );
	if( k != connection_map.end() ) {
	    for(j = k->second.begin(); j != k->second.end(); j++) {
                out << "n" << *i << " -- " << "n" << *j << ";" << endl;
	    }
	}
	else {
          //This node has no connections:
	  out << "n" << *i << ";" << endl;
	}
	node_count++;
    }
    out << "}" << endl;
#endif
    
#if 0
    //This is for Tulip, a GPL graph viewer.
    //It MUST use integers to refer to nodes and edges.

    NodePSet::const_iterator i;
    for(i = node_set.begin(); i != node_set.end(); i++) {
	out << "(node " << (int)*i << ")" << endl;
    }
    EdgeSet::const_iterator j;
    int k = 1;
    for(j = edge_set.begin(); j != edge_set.end(); j++) {
        out << "(edge " << k++ << " " << (int)j->first << " " << (int)j->second << ")" << endl;
    }
#endif
    
}

void Network::readFrom(istream& in) {

  SuperString line;
  map<string, Node*> name_map;
  while( getline(in, line, '\n') ) {
    if( line[0] == '#' ) { //This is a comment
      continue;
    }
    vector<SuperString> result = line.split(" : ");
    Node* first = 0;
    Node* second = 0;
    if( name_map.find(result[0]) == name_map.end() ) {
      //This is a new node:
      if( result[0] != "" ) {
        first = new Node(result[0]);
        name_map[ result[0] ] = first;
        add( first );
      }
    }
    else {
      first = name_map[ result[0] ];
    }
    if( result.size() == 1 ) { //There was no neighbors
      continue;
    }
    result = result[1].split(" ");
    vector<SuperString>::iterator sit;
    for(sit = result.begin();
	sit != result.end();
	sit++) {
      second = 0;
      if( name_map.find( *sit ) == name_map.end() ) {
        //This is a new node:
	if( *sit != "" ) {
          second = new Node( *sit );
          name_map[ *sit ] = second;
	  add( second );
	}
      }
      else {
        second = name_map[ *sit ];
      }
      //Add the edge:
      if( first && second ) {
        add( Edge(first, second) );
      }
    }
  }
	
}

int Network::remove(const Edge& edge) {
  Edge* e_p = getEdgePtr(edge);
  int ret = 0;
  if(e_p) {
    ret += remove(e_p);
  }
  return ret;
}

int Network::remove(Edge* e_p) {
  if( has( e_p ) ) {
    //This edge is in the network	  
    list<INetworkMonitor*>::iterator nm_it;
    for(nm_it = _net_mon.begin(); nm_it != _net_mon.end(); nm_it++) {
      (*nm_it)->preEdgeRemove(e_p);
    }
    _edge_count--;
    _node_to_edges[e_p->first].erase(e_p);
    _node_to_edges[e_p->second].erase(e_p);
       
    for(nm_it = _net_mon.begin(); nm_it != _net_mon.end(); nm_it++) {
      (*nm_it)->postEdgeRemove(e_p);
    }
    //remove a reference to the edge:
    //This should be done last, other wise the postEdgeRemove may get
    //a deleted pointer
    decrementRefCount(e_p);
    return 1;
  }
  else {
    //This node is not in the network:
    return 0;
  }
}

int Network::remove(Node* node) {
  int removed_edges = 0;
  GraphMap::iterator ne_it = _node_to_edges.find(node);
  if( ne_it != _node_to_edges.end() ) {
    list<INetworkMonitor*>::iterator nm_it;
    //Let everybody know we are removing this node
    for(nm_it = _net_mon.begin(); nm_it != _net_mon.end(); nm_it++) {
      (*nm_it)->preNodeRemove(node);
    }
    //Remove all the edges associated with the node
    EdgeSet::iterator e_it;
    for(e_it = ne_it->second.begin(); e_it != ne_it->second.end(); e_it++) {
      removed_edges += remove( *e_it );
    }
    //Remove the entry from the big table
    _node_to_edges.erase(node);
    //Let everyone know the edge is gone:
    for(nm_it = _net_mon.begin(); nm_it != _net_mon.end(); nm_it++) {
      (*nm_it)->postNodeRemove(node);
    }
    //Get rid of one of the references to this node, possibly deleting it.
    decrementRefCount(node);
  }
  return removed_edges;
}

int Network::remove(NodeIterator* nodes) {
    int ret = 0;
    /**
     * We make a list of the nodes to remove first, so the
     * iterator is not exposed to a changing network
     * then we remove the edges.
     */
    std::list<Node*> _to_remove;
    nodes->pushInto( _to_remove );
    std::list<Node*>::iterator rit;
    for(rit = _to_remove.begin(); rit != _to_remove.end(); rit++)
    {
      ret += remove( *rit ); 
    }
    return ret;
}

int Network::remove(EdgeIterator* edges) {
    int ret = 0;
    /**
     * We make a list of the edges to remove first, so the
     * iterator is not exposed to a changing network
     * then we remove the edges.
     */
    std::list<Edge*> _to_remove;
    edges->pushInto(_to_remove);
    std::list<Edge*>::iterator rit;
    for(rit = _to_remove.begin(); rit != _to_remove.end(); rit++)
    {
      ret += remove( *rit ); 
    }
    return ret;
}

void Network::remove(INetworkMonitor* nm) {
  list<INetworkMonitor*>::iterator nmit;
  for(nmit = _net_mon.begin(); nmit != _net_mon.end(); nmit++) {
    if( *nmit == nm ) {
      break;
    }
  }
  if( nmit != _net_mon.end() ) {
    //Just remove this guy:
    _net_mon.erase(nmit);
  }
}

int Network::removeAndCluster(Node* n) {
    auto_ptr<NodeIterator> ni( getNeighborIterator(n) );
    while( ni->moveNext() ) {
      auto_ptr<NodeIterator> nj( ni->clone() );
      while( nj->moveNext() ) {
        add( Edge( ni->current(), nj->current() ) );
      }
    }
    //Now return the node
    return remove(n);
}

bool Network::operator<(const Network& aNet) const {
    if( this == &aNet ) { return false; }
    //Make sure networks with smaller nodes are <
    if( getNodeSize() != aNet.getNodeSize() ) {
        return (getNodeSize() < aNet.getNodeSize());
    }
    //okay, these two networks have the same number of nodes, but how about edges:
    if( getEdgeSize() != aNet.getEdgeSize() ) {
        return (getEdgeSize() < aNet.getEdgeSize() );
    }
    
    //If two networks are the same size,
    //but have different set of nodes, let the STL sort it out:
    if( _node_to_edges != aNet._node_to_edges ) {
      auto_ptr<NodeIterator> ni1( getNodeIterator() );
      auto_ptr<NodeIterator> ni2( aNet.getNodeIterator() );
      while( ni1->moveNext() && ni2->moveNext() ) {
        Node* n1 = ni1->current();
        Node* n2 = ni2->current();
        if( n1 != n2 ) {
          return n1 < n2;
        }
      }
      auto_ptr<EdgeIterator> ei1( getEdgeIterator() );
      auto_ptr<EdgeIterator> ei2( aNet.getEdgeIterator() );
      while( ei1->moveNext() && ei2->moveNext() ) {
        Edge* e1 = ei1->current();
        Edge* e2 = ei2->current();
        if( e1 != e2 ) {
          return e1 < e2;
        }
      }

    }
    //I guess these are equal
    return false;
}

Network& Network::operator=(const Network& aNet) {
  if( this != &aNet) {
      clear();
      _edge_count = aNet.getEdgeSize();
      _node_to_edges = aNet._node_to_edges;
      
      //Bump the reference count on each of the nodes:
      auto_ptr<NodeIterator> ni( aNet.getNodeIterator() );
      while(ni->moveNext()) {
        incrementRefCount( ni->current() );
      }
      //Bump the reference count on each of the edges:
      auto_ptr<EdgeIterator> ei( aNet.getEdgeIterator() );
      while(ei->moveNext()) {
        incrementRefCount( ei->current() );
      }
  }
  return (*this);
}

#if 0
bool Network::Merge(const Network& aNet, int aStart){
 //check if the ranges are correct
	if (this->getNodes().size() < ((aStart-1) + aNet.getNodes().size())){
		cerr<<" the sizes do not match in merge";
		return(false);
	}
	set<Node*>::iterator start_iterator,i,j;
	start_iterator = node_set.begin();
	for( int icounter = 0; icounter<(aStart-1); icounter++)
		start_iterator ++;
	i = start_iterator;
	j = aNet.node_set.begin();
	for(;j != aNet.node_set.end(); j++){
	// get neihbors of j find them and add proper edges
		ConnectedNodePSet j_neighbors = aNet.getNeighbors(*j);
		ConnectedNodePSet::const_iterator n_it;
	 	for(n_it = j_neighbors.begin(); n_it!=j_neighbors.end(); n_it++){
			NodePSet::const_iterator m_it = start_iterator;
			NodePSet::const_iterator p_it ;
			for( p_it = aNet.node_set.begin();((*p_it) != (*n_it)); p_it++){
				m_it++;
			}
			add(Edge(*m_it,*i));
		}
		i++;
	}
	return(true);
}

#endif



void Network::readFromGDL(std::istream& in){
  SuperString line;
  map<string, Node*> name_map;
  list<string> edge_src;
  list<string> edge_dst;
  int node=0, edge=0;
  Node* temp1 = 0;
  Node* temp2 = 0;

  while( getline(in, line, '\n') ) 
  {
    if( line[0] == '/' && line[1] == '/' ) continue;

    vector<SuperString> result = line.split(" ");
    vector<SuperString>::iterator sit;
    string nodename="", source="", target="";

    for (sit = result.begin(); sit != result.end(); sit++)
    {
      // note: should be able to handle node/edge definitions that span multiple lines now
      // note: doesn't handle multiword titles/sources/destinations, but they shouldn't exist anyway

      if (*sit == "node:")
        node = 1;
      else if (*sit == "edge:" || *sit == "nearedge:")
        edge = 1;
      else if (node == 1 && *sit == "title:")
      {
        sit++;
        nodename = *sit;

        if (name_map.find(nodename) == name_map.end()) // if new node...
        {
          temp1 = new Node(nodename);
          name_map[nodename] = temp1;
          add( temp1 );
          //cout<<"Created node "<<nodename<<endl;
        }

        nodename = "";
        node = 0;
      }
      else if (edge == 1 && (*sit == "targetname:" || *sit == "target:"))
      {
        sit++;
        target = *sit;
      }
      else if (edge == 1 && (*sit == "sourcename:" || *sit == "source:"))
      {
        sit++;
        source = *sit;
      }

      if (edge == 1 && source != "" && target != "")
      {
        edge_src.push_front(source);
        edge_dst.push_front(target);
        source = "";
        target = "";
        edge = 0;
      }

    }
  }

  list<string>::iterator i = edge_src.begin(), j = edge_dst.begin();

  while (i != edge_src.end())
  {
    //cout<<"Created edge from "<<*i<<" to "<<*j<<endl;
    temp1 = name_map[*i];
    temp2 = name_map[*j];
    add( Edge(temp1, temp2) );
    i++;
    j++;
  }

	
}

void Network::printToGDL(ostream& out) const {

  /*
   * This GDL output is for use with aiSee/VCG graph tools
   */
	
  NodePSet::const_iterator i;
  string label;
  int node_count = 0;
    
  out<<"graph: {\n";
  out<<"  title: \"untitled\"\n";
  out<<"  colorentry 16: 161 215 244\n";
  out<<"  color: lightblue\n";
  out<<"  layoutalgorithm: forcedir\n";
  out<<"  repulsion: 150\n";
  out<<"  attraction: 150\n";
  out<<"\n";


  auto_ptr<NodeIterator> ni( getNodeIterator() );
  while( ni->moveNext() ) {
    Node* n = ni->current();
    label = n->toString();
    out<<"  node: { title: \""<<label<<"\" label: \""<<label<<"\" }\n";
	  //out << 'n' << *i << " [label=\"" << label << "\",width=0.25,height=0.25,fontsize=9];" << endl;
    auto_ptr<NodeIterator> cni( getNeighborIterator(n) );
    while( cni->moveNext() ) {
      Node* n2 = cni->current();
      out<<"  edge: { sourcename: \""<<label<<"\" targetname: \""<<(n2)->toString()<<"\" }\n";
        //out << 'n' << *i << " -> " << 'n' << *j << ";" << endl;
    }
    node_count++;
  }
  out << "}\n" << endl;    
}

NodeIterator* Network::NetNodeIterator::clone()
{
  NetNodeIterator* ni = new NetNodeIterator();
  ni->_nit = _nit;
  ni->_begin = _begin;
  ni->_end = _end;
  ni->_called_movenext = _called_movenext;
  return ni;
}

Node* const & Network::NetNodeIterator::current()
{
  
  if( !_called_movenext ) {
    throw std::exception();
  }
  return _nit->first;
}

bool Network::NetNodeIterator::moveNext()
{
  if( !_called_movenext ) {
    _called_movenext = true;
  }
  else {
    _nit++;
  } 
  return _nit != _end;
}

void Network::NetNodeIterator::reset()
{
  _nit = _begin;
  _called_movenext = false;
}


Network::NetEdgeIterator::NetEdgeIterator(const GraphMap& m) {
  _begin = m.begin();
  _end = m.end();
  reset();
}


Network::NetEdgeIterator::NetEdgeIterator() {
}

EdgeIterator* Network::NetEdgeIterator::clone()
{
  NetEdgeIterator* ne = new NetEdgeIterator();
  ne->_nit = _nit;
  ne->_begin = _begin;
  ne->_end = _end;
  ne->_eit = _eit;
  ne->_called_movenext = _called_movenext;
  return ne;
}

Edge* const & Network::NetEdgeIterator::current()
{
  
  if( !_called_movenext ) {
    throw std::exception();
  }
  if( _eit == _nit->second.end() ) {
    throw exception();
  }
  DEBUG_MSG("End of Current");
  return (*_eit);
}

bool Network::NetEdgeIterator::moveNext()
{
  if( _begin == _end ) {
    //This is empty
    return false;
  }
  DEBUG_MSG("Start of MoveNext");
  if( false == _called_movenext ) {
    DEBUG_MSG("not called movenext");
    _called_movenext = true;
  }
  else {
    DEBUG_MSG("Already called movenext");
    _eit++;
  }
  //Advance to the next real edge
  DEBUG_MSG("Looking for next edge");
  bool found_next = false;
  while( ( false == found_next ) && ( _nit != _end ) ) {
    /*
     * check to see if we need to advance nit
     */
    if( _eit == _nit->second.end() ) {
      /*
       * We are at the end of one EdgeSet, go to the next:
       */
      if( _nit != _end ) {
        _nit++;
	if( _nit != _end ) {
          /*
           * There are more EdgeSets to look at
           */
          _eit = _nit->second.begin();
          //We think we have found next, we double check in the next if block
          found_next = ( _eit != _nit->second.end() );
        }
      }
      else {
        //There are no more nodes.
        found_next = false;
      }
    }
    else {
      //Looks like we have a candidate edge,
      found_next = true;
    }
    /*
     * Here we check that we don't hit the same edge twice
     * be making sure that edge->first == node
     */
    if( found_next && ( (*_eit)->first != _nit->first ) ) {
      /*
       * Skip edges unless _nit is the first Node.  This is so we don't
       * count edges twice
       */
      _eit++;
      //We have to loop through again to check this new edge
      found_next = false;
    }
    else {
      //found_next AND edge->first == node.  Great!
      //We don't need to say this, but it is true: found_next = true;
    }
  }
  DEBUG_MSG("End of EdgeIterator::MoveNext"); 
  return found_next;
}

void Network::NetEdgeIterator::reset()
{
  _nit = _begin;
  if( _nit != _end ) {
    _eit = _nit->second.begin();
  }
  else {
    //we don't need to worry, moveNext won't ever move next 
  }
  _called_movenext = false;
}

Network::NeighborIterator::NeighborIterator(Node* n, const EdgeSet& edges) {
  _beg = edges.begin();
  _end = edges.end();
  _neighbors_of = n;
  reset();
}

Network::NeighborIterator::NeighborIterator() {

}

NodeIterator* Network::NeighborIterator::clone()
{
  NeighborIterator* ne = new NeighborIterator();
  ne->_neighbors_of = _neighbors_of;
  ne->_eit = _eit;
  ne->_beg = _beg;
  ne->_end = _end;
  ne->_moved_to_first = _moved_to_first;
  return ne;
}

Node* const & Network::NeighborIterator::current()
{
  return (*_eit)->getOtherNode( _neighbors_of );
}

bool Network::NeighborIterator::moveNext()
{
  if( _moved_to_first ) {
    _eit++;
  }
  else {
    _moved_to_first = true;
  }
  return ( _eit != _end );
}

void Network::NeighborIterator::reset()
{
  _eit = _beg;
  _moved_to_first = false;
}

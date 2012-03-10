/*
This program is part of Netmodeler, a library for graph and network
modeling and simulation.
Copyright (C) 2005  University of California
Copyright (C) 2005  P. Oscar Boykin <boykin@pobox.com>, University of Florida
Copyright (C) 2006  Tae Woong Choi  <twchoi@ufl.edu>, University of Florida

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
#include <string.h>
#include <deetoomessage.h>
#include <assert.h>
#include <tr1/tuple>
#include "MapReduceTask.h"
#include <algorithm>

using namespace Starsky;
using namespace std;
using namespace std::tr1;
//#define INT64
#ifdef INT64
  typedef unsigned long long my_int;
#else
  typedef unsigned long my_int;
#endif
#define BIDIRECTION
//typedef unsigned long long my_int;

DeetooMessage::DeetooMessage(my_int r0, my_int r1, double n_fail, Ran1Random& r_num, double p_fail, double ff_p_fail, bool wait_factor, int allowed_time) : _r0(r0), _r1(r1), _cache(true), _r_num(r_num), _p_fail(p_fail), _n_fail(n_fail), _wait_factor(wait_factor), _allowed_time(allowed_time), _ff_p_fail(ff_p_fail)
{
  if (r0 > r1) {
	cerr << "starting point should be less than ending point" << endl
		<< "(" << _r0 << ", " << _r1 << ")" << endl;
    cerr <<"manipulating starting point " << (_r0-1) <<endl;
  }
  _mid_range = (my_int)( ( (double)(_r0)+(double)(_r1) )/(double)(2) );
  out_edge_count = 1;
  init_node = NULL;
  #ifdef INT64
    _dist_to_lower = 18446744073709551615LL;
  #else  
    _dist_to_lower = 4294967295L;
  #endif
  insert_fail = false;
}

bool DeetooMessage::inRange(AddressedNode* inode)
{
    my_int nd_addr = inode->getAddress(_cache);
    return ( ( nd_addr >= _r0) && ( nd_addr <= _r1) );
}	

DeetooNetwork* DeetooMessage::visit(Node* n, Network& net)
{
  DeetooNetwork* d2n = dynamic_cast<DeetooNetwork*>( net.newNetwork() );
  DeetooNetwork* org_net = dynamic_cast<DeetooNetwork*>(&net);
  AddressedNode* start = dynamic_cast<AddressedNode*> (n);      //start node for broadcasting 
  //d2n->add(start);
  _network_size = org_net->node_map.size() ;
  
  
//  cout << "node size = " << org_net->node_map.size() << endl;
  srand(_r_num.getDouble01()*100000);
  int fail_node_add = 0, index=0, prev_index=-1;
  bool only_layer_1_drop = false;
  int drop_node_num =  only_layer_1_drop == true ? (int)_n_fail: _network_size  * _n_fail;
  char remove_list[_network_size];
  memset(remove_list, 0, sizeof(remove_list));
  
  if(only_layer_1_drop){
    auto_ptr<NodeIterator> ni(net.getNeighborIterator(start) );
    auto_ptr<NodeIterator> cni(net.getNeighborIterator(start) );
    std::map<my_int, AddressedNode*> neighbors;  
    while(ni->moveNext()){
      AddressedNode* current_node = dynamic_cast<AddressedNode*> (ni->current() );
      my_int my_int_node_addr = current_node->getAddress(true);
      neighbors[my_int_node_addr]=current_node; 
    }

    int neighbor_num = neighbors.size();
    cout << neighbor_num << endl;
//    drop_node_num = 2;
    vector<int> added_index;
    bool already_added = false;
    for(;drop_node_num>0;){
      index = rand()%neighbor_num;
      std::vector<int>::iterator itrt;
//      cout << "fail node index = " << index << endl;
      for(itrt=added_index.begin();itrt != added_index.end();itrt++){
        if(index == (*itrt)){
          already_added = true;
          break;
        }
      }
      if(already_added == true){
        already_added = false;
        continue;
      }else{
        added_index.push_back(index);
        already_added = false;
      }

      std::map<my_int, AddressedNode*>::iterator map_itrt;
//      cout << "index is " << index << endl;
      if(prev_index != -1 && (abs(prev_index-index)==1) ){
  //      cout << "adjacent node failure" << endl;
      }
      prev_index = index;
      my_int node_index = -1;
      for(map_itrt=neighbors.begin();map_itrt != neighbors.end();map_itrt++){
        if(index ==  0){
          node_index = map_itrt->second->index_network;
//          cout << "node_index " << node_index << endl;
          break;
        }
        index--;
      }
      
      if(node_index == -1){
        continue;
      }else{
//        cout << "failed node address is " << failnode->getAddress(true) << endl;
        drop_node_num--;
        remove_list[node_index] = 1;
      }
    }
  }else{
    my_int run_index = 0;
    while(fail_node_add != drop_node_num){
      run_index++;
      index = rand()%_network_size;
      if(remove_list[index] == 0){
        remove_list[index] = 1;
        fail_node_add++;
      }
    }
  }
  bool duplication_mode = false;
  for(int run_index=0;run_index < 1; run_index++){
//    duplication_mode = !duplication_mode;
//    cout << "number of removed nodes is " << remove_list.size() << endl;
    std::vector<my_int> result = visit(start, net, *d2n, remove_list, duplication_mode, _allowed_time);
    my_int missing_region_sum = 0;
    missing_region_sum = 0;
    for(int i=0;i<result.size();i+=2){
      if(i==0){
//          cout << "returned nodes = " << result[i] << " latency = " << result[i+1] << endl;
 //         cout << result[i] << " " << result[i+1] << endl;
          d2n->Latency = result[i+1] ;
//        cout << result[i+1] << " ";
      }else{
   //     cout << "missing region sum get " << result[i+1] << " to " <<result[i] << endl;
        missing_region_sum += (result[i+1] - result[i]);
      }
    }
//    cout << duplication_mode << "\t" << ((double)missing_region_sum/(double)(_r1-_r0)) << " missing region sum "<<missing_region_sum << " total space " << (_r1-_r0) << endl;
//    cout << ((double)missing_region_sum/(double)(_r1-_r0)) << endl;
      my_int combined_node_num = _network_size*(1-((double)missing_region_sum/(double)(_r1-_r0)));
//      cout << combined_node_num << endl;
  }
  return d2n;
}

//void DeetooMessage::visit(AddressedNode* start, Network& net, DeetooNetwork& visited_net)
std::vector<my_int> DeetooMessage::visit(AddressedNode* start, Network& net, DeetooNetwork& visited_net, char failed_nodes[], bool duplicate, int allowed_time)
{
  //For the network reliability test, with failure prob. p, returns nothing.
//  cout << "index = " << start->index_network << endl;
  MapReduceTask mrt = MapReduceTask(start->getAddress(true), _r_num, _p_fail, _ff_p_fail, failed_nodes);
  std::vector<my_int> cum_result, dup_cum_result;
  map<int, std::tr1::tuple<double, my_int, my_int, my_int, vector<my_int> > > result_collection;

  my_int fail_result;
  int edge_latency = 0, new_allowed_latency=0;
  my_int elapsed_time=0;
  int mean_latency = 600;
  edge_latency = 200 + _r_num.getDouble01() * 400;   
//  edge_latency = mean_latency;
/*
  if (p_edgefail < _p_fail) { 
      insert_fail = true;
      return (std::vector<int>)NULL; 
  }
*/
  DeetooNetwork* org_net = dynamic_cast<DeetooNetwork*>(&net);
  _network_size = org_net->node_map.size();
  if(allowed_time == -1){
    new_allowed_latency = allowed_time;
  }else if((allowed_time-edge_latency*2) < 0){
     new_allowed_latency = 0;
  }else{
     new_allowed_latency = allowed_time-edge_latency*2;
  }
  // If start node is not in the range(_r0, _r1), find the closest neighbor to lower bound in range.
  if ( (!inRange(start) ) )  //node is not in this range
  {
      AddressedNode* next_node=NULL;
      my_int current_dist_to_lower = _dist_to_lower;
      auto_ptr<NodeIterator> ni(net.getNeighborIterator(start) );
//      cout << "self addr = " << start->getAddress(true) << " begin addr = " <<_r0 << " end addr = " << _r1 <<endl;
      while (ni->moveNext()  )
      {
          AddressedNode* c_node = dynamic_cast<AddressedNode*> (ni->current() );
      	  if ( inRange(c_node) ) {
    	      init_node = c_node;
    //          cout << "111 not in range" << endl;
    	      return visit(c_node, net, visited_net, failed_nodes, false, allowed_time);
    	  }else {
    	      my_int dist = c_node->getDistanceTo(_mid_range, _cache) ;
//            cout << "dist = " <<dist << " c_node = " << c_node->getAddress(true) << " mid range = "  << _mid_range << endl;
                  if ( dist < _dist_to_lower )
    	      {
                      next_node = c_node;
    	          _dist_to_lower = dist;
    	      }
    	    }
          }	
          if (current_dist_to_lower == _dist_to_lower) {
            std::vector<my_int> no_result(0, 0);
  //          cout << "222 not in range" << endl;
            return no_result;
 //           return (std::vector<my_int>)NULL;
      }
      //We have the closest neighbor to lower, start over there
      out_edge_count++;
//      cout << "333 not in range" << endl;
      return visit(next_node, net, visited_net, failed_nodes, false, allowed_time);
  }
  if ( init_node == NULL ) { init_node = start;}

  visited_net.add(start);
  //We are in the range, get the neighbors.
  //divide range to upper and lower.
  //get upper neighbors and lower neighbors.
  //std::map will sort them according to their address, lowest first.
  //will divide neighbors to upeer and lower groups wrt its address
  std::map<my_int, AddressedNode*> lower_neighbors;  
  std::map<my_int, AddressedNode*> upper_neighbors;
  auto_ptr<NodeIterator> ni(net.getNeighborIterator(start) );
  while(ni->moveNext() )
  {
    AddressedNode* current_node = dynamic_cast<AddressedNode*> (ni->current() );
    //check if current node is within the range
    if (inRange(current_node)) {
      my_int c_node_addr = current_node->getAddress(_cache);
      if (c_node_addr < start->getAddress(_cache) )
      {
         lower_neighbors[c_node_addr]=current_node; 
//         cout << "lower address node = " << c_node_addr << endl;
      }
      else if (c_node_addr > start->getAddress(_cache) )
      {
         upper_neighbors[c_node_addr]=current_node;
//         cout << "upper  address node = " << c_node_addr << endl;
      }
    }
  }

  mrt.Map(&cum_result);
  mrt.Map(&dup_cum_result);

    //Start with lower neighbors first.
    my_int last_lower = _r0, two_hops_lower = _r0;
    my_int last_upper = _r1, two_hops_upper = _r1;
    my_int dup_last_upper, dup_last_lower;
    std::map<my_int, AddressedNode*>::const_iterator it_low;
    for (it_low=lower_neighbors.begin(); it_low!=lower_neighbors.end(); it_low++)
    {
  	   if ( _r0 != _r1) {
            //We don't need to add the node, it is also done when we add an edge
            //visited_net.add(it_low->second);
  	     visited_net.add(Edge(start, it_low->second) );
         my_int begin_addr;
#ifdef BIDIRECTION
         begin_addr = last_lower;
#else
        begin_addr = duplicate?two_hops_lower:last_lower;
#endif
  	     DeetooMessage m_low = DeetooMessage(begin_addr, it_low->first, _n_fail, _r_num, _p_fail, _ff_p_fail, _wait_factor);
  	     //Here is the recursion.  Note we don't make a new network
  	     std::vector<my_int> result = m_low.visit(it_low->second, net, visited_net, failed_nodes, false, new_allowed_latency);
         elapsed_time = mrt.GetElapsedTime(edge_latency, it_low->second->index_network);
         if(_wait_factor){
           tuple<double, my_int, my_int, my_int, vector<my_int> > tpl_result((double)(it_low->first - begin_addr)/(double)(_r1-_r0), it_low->second->index_network, begin_addr, it_low->first, result);
//           pair <double, vector<my_int> > frac_rslt_pair ((double)(it_low->first - begin_addr)/(double)(_r1-_r0), result);
           while(result_collection.count(elapsed_time+result[1]) > 0){
             elapsed_time += 1;
           }
           result_collection[elapsed_time+result[1]] = tpl_result;
         }else{
           mrt.Reduce(&cum_result, result, it_low->second->index_network, elapsed_time, allowed_time);
           fail_result = cum_result.back();
           cum_result.pop_back();
           if(fail_result == 0){
   //          cout << "org node failure detected by " << it_low->first << "  from " << begin_addr << " to "<< it_low->first << endl;
             updateVector(cum_result, begin_addr, it_low->first);
           }
         }         
         two_hops_lower = last_lower;
  	      last_lower = it_low->first +1;
  	   }
    } 
    if(duplicate==true && lower_neighbors.size() != 0){
#ifdef BIDIRECTION
      dup_last_upper = start->getAddress(true)-1;
      std::map<my_int, AddressedNode*>::reverse_iterator rev_it_low;
      for (rev_it_low = lower_neighbors.rbegin(); rev_it_low != lower_neighbors.rend(); rev_it_low++)
      {
         if ( _r0 != _r1) {
              //We don't need to add the node, it is also done when we add an edge
              //visited_net.add(it_low->second);
//           visited_net.add(Edge(start, it_low->second) );
           DeetooMessage m_low = DeetooMessage(rev_it_low->first, dup_last_upper, _n_fail, _r_num, _p_fail, _ff_p_fail, _wait_factor);
           //Here is the recursion.  Note we don't make a new network
           std::vector<my_int> result = m_low.visit(rev_it_low->second, net, visited_net, failed_nodes, false, new_allowed_latency);
           elapsed_time = mrt.GetElapsedTime(edge_latency, rev_it_low->second->index_network);
          mrt.Reduce(&dup_cum_result, result, rev_it_low->second->index_network, elapsed_time, allowed_time);
          fail_result = dup_cum_result.back();
          dup_cum_result.pop_back();
          if(fail_result == 0){
//            cout << "backup node failure detected by " << rev_it_low->first <<" from " << rev_it_low->second<< " to "<< dup_last_upper << endl;
            updateVector(dup_cum_result, rev_it_low->first, dup_last_upper);
          }
          dup_last_upper = rev_it_low->first -1;
          if(dup_last_upper > _r1){
 //           cout <<"dup_last_upper : weird situation: "<<dup_last_upper << "lower  neighbor size is " << lower_neighbors.size() << endl;
          }
         }
      } 
#else
      if(lower_neighbors.size() == 1){
        DeetooMessage m_low = DeetooMessage(two_hops_lower, last_lower, _n_fail, _r_num, _p_fail, _ff_p_fail, _wait_factor);
         //Here is the recursion.  Note we don't make a new network
         AddressedNode* an = lower_neighbors[last_lower-1];
        std::vector<my_int> result = m_low.visit(an, net, visited_net, failed_nodes, false, new_allowed_latency);
         elapsed_time = mrt.GetElapsedTime(edge_latency, an->index_network);
         mrt.Reduce(&cum_result, result, an->index_network, elapsed_time, allowed_time);
      }else{
        AddressedNode* an = lower_neighbors[two_hops_lower-1];
        DeetooMessage m_low = DeetooMessage(two_hops_lower-1, last_lower, _n_fail, _r_num, _p_fail, _ff_p_fail, _wait_factor);
         //Here is the recursion.  Note we don't make a new network
        std::vector<my_int> result = m_low.visit(an, net, visited_net, failed_nodes, false, new_allowed_latency);
         elapsed_time = mrt.GetElapsedTime(edge_latency, an->index_network);
         mrt.Reduce(&cum_result, result, an->index_network, elapsed_time, allowed_time);
      }
  
      fail_result = cum_result.back();
      cum_result.pop_back();
      if(fail_result == 0){
//        cout << "backup (non-bidrection) node failure detected by " << two_hops_lower-1 << " from " << two_hops_lower-1<< " to "<< last_lower << endl;
        updateVector(cum_result, two_hops_lower-1, last_lower);
      }
#endif
    }

    //go to the uppper side.
    
    std::map<my_int, AddressedNode*>::reverse_iterator it_up;
    for (it_up=upper_neighbors.rbegin(); it_up!=upper_neighbors.rend(); it_up++)
    {
      if (_r0 != _r1) {
          //We don't need to add the node, it is also done when we add an edge
          //visited_net.add(it_up->second);
        visited_net.add(Edge(start, it_up->second) );
        my_int end_addr;
#ifdef BIDIRECTION
        end_addr = last_upper;
#else
        end_addr = duplicate?two_hops_upper:last_upper;
#endif
        DeetooMessage m_up = DeetooMessage(it_up->first, end_addr, _n_fail, _r_num, _p_fail, _ff_p_fail, _wait_factor);
        std::vector<my_int> result = m_up.visit(it_up->second, net, visited_net, failed_nodes, false, new_allowed_latency);
        elapsed_time = mrt.GetElapsedTime(edge_latency, it_up->second->index_network);
        if(_wait_factor){
     //     pair <double, vector<my_int> > frac_rslt_pair ((double)(end_addr - it_up->first)/(double)(_r1-_r0), result);
          tuple<double, my_int, my_int, my_int, vector<my_int> > tpl_result((double)(end_addr - it_up->first)/(double)(_r1-_r0), it_up->second->index_network, it_up->first, end_addr, result);
          while(result_collection.count(elapsed_time+result[1]) > 0){
            elapsed_time += 1;
          }
          result_collection[elapsed_time+result[1]] = tpl_result;
        }else{
          mrt.Reduce(&cum_result, result, it_up->second->index_network, elapsed_time, allowed_time);
          fail_result = cum_result.back();
          cum_result.pop_back();
          if(fail_result == 0){
    //        cout << "org node failure detected by " << it_up->first << " from " << it_up->first << " to "<< end_addr << endl;
            updateVector(cum_result, it_up->first, end_addr);
          }
        }        
        two_hops_upper = last_upper;
        last_upper = it_up->first -1;
      }
    }
    
    if(duplicate==true && upper_neighbors.size() != 0){
#ifdef BIDIRECTION
    dup_last_lower = start->getAddress(true) + 1;
    std::map<my_int, AddressedNode*>::const_iterator con_it_up;
    for (con_it_up=upper_neighbors.begin(); con_it_up!=upper_neighbors.end(); con_it_up++)
    {
      if (_r0 != _r1) {
          //We don't need to add the node, it is also done when we add an edge
          //visited_net.add(it_up->second);
        DeetooMessage m_up = DeetooMessage(dup_last_lower, con_it_up->first, _n_fail, _r_num, _p_fail, _ff_p_fail, _wait_factor);
        //Here is the recursion.  Note we don't make a new network
        std::vector<my_int> result = m_up.visit(con_it_up->second, net, visited_net, failed_nodes, false, new_allowed_latency);
        elapsed_time = mrt.GetElapsedTime(edge_latency, con_it_up->second->index_network);
        mrt.Reduce(&dup_cum_result, result, con_it_up->second->index_network, elapsed_time, allowed_time);
        fail_result = dup_cum_result.back();
        dup_cum_result.pop_back();
        if(fail_result == 0){
//          cout << "backup node failure detected by " << con_it_up->first << " from " << dup_last_lower<< " to "<< con_it_up->first << endl;
          updateVector(dup_cum_result, dup_last_lower, con_it_up->first);
        }
        dup_last_lower = con_it_up->first + 1;
        if(dup_last_lower > _r1){
          cout <<"weird situation: "<<dup_last_lower << " upper neighbor size is " << upper_neighbors.size() << endl;
        }
      }
    }

#else
      if(upper_neighbors.size() == 1){
        AddressedNode* an = upper_neighbors[last_upper+1];
        DeetooMessage m_up = DeetooMessage(last_upper, two_hops_upper, _n_fail, _r_num, _p_fail, _ff_p_fail, _wait_factor);
        std::vector<my_int> result = m_up.visit(upper_neighbors[last_upper+1], net, visited_net, failed_nodes, false, new_allowed_latency);
        elapsed_time = mrt.GetElapsedTime(edge_latency, last_upper+1);
        mrt.Reduce(&cum_result, result, last_upper+1, elapsed_time, allowed_time);
      }else{
        AddressedNode* an = upper_neighbors[two_hops_upper+1];
        DeetooMessage m_up = DeetooMessage(last_upper, two_hops_upper+1, _n_fail, _r_num, _p_fail, _ff_p_fail, _wait_factor);
        std::vector<my_int> result = m_up.visit(an, net, visited_net, failed_nodes, false, new_allowed_latency);
        elapsed_time = mrt.GetElapsedTime(edge_latency, an->index_network);
        mrt.Reduce(&cum_result, result, an->index_network, elapsed_time, allowed_time);
      }

      fail_result = cum_result.back();
      cum_result.pop_back();
      if(fail_result == 0){
 //       cout << "backup (non-bidirection node failure detected by " << two_hops_lower-1 << " from " << last_lower << " to "<< it_low->first << endl;
        updateVector(cum_result, last_lower, it_low->first);
      }
#endif      
    }
    
#ifdef BIDIRECTION
  if(duplicate == true){
    std::vector<my_int> result;
//    cout << "result address = " << &result << endl;
     DeetooMessage m_low = DeetooMessage(dup_last_lower, _r1, _n_fail, _r_num, _p_fail, _ff_p_fail, _wait_factor);
     my_int node_id;
     //Here is the recursion.  Note we don't make a new network
     if(lower_neighbors.size() != 0){
       std::map<my_int, AddressedNode*>::iterator itr = lower_neighbors.begin();
       result = m_low.visit((itr->second), net, visited_net, failed_nodes, false, new_allowed_latency);
       elapsed_time = mrt.GetElapsedTime(edge_latency, itr->second->index_network);
//       cout << "before print" << endl;
 //      cout << "before mapreduce result [0] " << &result << endl;
//       cout << "end print" << endl;
       mrt.Reduce(&dup_cum_result, result, itr->second->index_network, elapsed_time, allowed_time);
       node_id = itr->first; 
     }else{
        std::map<my_int, AddressedNode*>::reverse_iterator ritr = upper_neighbors.rbegin();
        ritr++;
        result = m_low.visit(ritr->second, net, visited_net, failed_nodes, false, new_allowed_latency);
        elapsed_time = mrt.GetElapsedTime(edge_latency, ritr->second->index_network);
        mrt.Reduce(&dup_cum_result, result, ritr->second->index_network, elapsed_time, allowed_time);
        node_id = ritr->first; 
     }
     fail_result = dup_cum_result.back();
     dup_cum_result.pop_back();
    if(fail_result == 0){
//      cout << "backup node failure detected by " << node_id << " from " << dup_last_lower << " to "<< _r1 << endl;
      updateVector(dup_cum_result, dup_last_lower, _r1);
    }

    DeetooMessage m_up = DeetooMessage(_r0, dup_last_upper, _n_fail, _r_num, _p_fail, _ff_p_fail, _wait_factor);
    //Here is the recursion.  Note we don't make a new network
    if(upper_neighbors.size() != 0){
      std::map<my_int, AddressedNode*>::reverse_iterator ritr = upper_neighbors.rbegin();
      result = m_up.visit(ritr->second, net, visited_net, failed_nodes, false, new_allowed_latency);
      elapsed_time = mrt.GetElapsedTime(edge_latency, ritr->second->index_network);
      mrt.Reduce(&dup_cum_result, result, ritr->second->index_network, elapsed_time, allowed_time);
      node_id = ritr->first;
    }else{
      std::map<my_int, AddressedNode*>::iterator itr = lower_neighbors.begin();
      itr++;
      result = m_up.visit(itr->second, net, visited_net, failed_nodes, false, new_allowed_latency);
      elapsed_time = mrt.GetElapsedTime(edge_latency, itr->second->index_network);
      mrt.Reduce(&dup_cum_result, result, itr->second->index_network, elapsed_time, allowed_time);
      node_id = itr->first;
    }

    fail_result = dup_cum_result.back();
    dup_cum_result.pop_back();
    if(fail_result == 0){
//      cout << "backup node failure detected by " << node_id << " from " << _r0 << " to "<< dup_last_upper << endl;
      updateVector(dup_cum_result, _r0, dup_last_upper);
    }
  }
#endif
  if(_wait_factor == true){
    map<my_int, AddressedNode*>::reverse_iterator lower_ritrt = lower_neighbors.rbegin();
    map<my_int, AddressedNode*>::iterator upper_itrt = upper_neighbors.begin();
    double complete_region = 0.0;
    if(lower_neighbors.size() != 0){
      complete_region += (double)(start->getAddress(true) - lower_ritrt->first)/(double)(_r1-_r0);
    }
    if(upper_neighbors.size() != 0){
      complete_region += (double)(upper_itrt->first - start->getAddress(true) )/(double)(_r1-_r0);
    }
//    complete_region = 0.0;
    map<int, std::tr1::tuple<double, my_int, my_int, my_int, vector<my_int> > >::iterator itrt;
//    cout << "begin" << endl;
    int prev_cum_time = 1000000;
    for(itrt = result_collection.begin();itrt != result_collection.end();itrt++){
      int cum_elapsed_time = itrt->first;
      tuple<double, my_int, my_int, my_int, vector<my_int> > tuple_result = (*itrt).second;
      double fraction = std::tr1::get<0>(tuple_result);
      my_int child_address = std::tr1::get<1>(tuple_result);
      my_int c_begin_addr = std::tr1::get<2>(tuple_result);
      my_int c_end_addr = std::tr1::get<3>(tuple_result);
      vector<my_int> child_result = std::tr1::get<4>(tuple_result);
      
      int estimated_nodes = (_network_size)*((1-complete_region)*(_r1-_r0)/(double)4294967295);
      int additional_time = estimated_nodes>1?prev_cum_time+mean_latency*5*(log(estimated_nodes)+1):prev_cum_time+mean_latency*3;
      if((c_end_addr-c_begin_addr) > ((_r1-_r0 - (4294967295/_network_size)))){
        additional_time += mean_latency*3;
      }
      mrt.Reduce(&cum_result, child_result, child_address, cum_elapsed_time-child_result[1], additional_time);
      fail_result = cum_result.back();
      cum_result.pop_back();
      if(fail_result == 0){
//          cout << "org node failure detected by " << it_low->first << "  from " << begin_addr << " to "<< it_low->first << endl;
        updateVector(cum_result, c_begin_addr, c_end_addr);
//        prev_cum_time = cum_elapsed_time;

      }else{
        prev_cum_time = cum_elapsed_time;
//        cout << "previous cumulative time "<< prev_cum_time <<" additional time " << mean_latency*2*(log(estimated_nodes)+1) << " cumulative elapsed time " << cum_elapsed_time << " estimated nodes " << estimated_nodes << endl;
      }
      complete_region += fraction;
//      cout << "complete region = " << complete_region << " latency = " << cum_elapsed_time << " estimated remaining nodes " << estimated_nodes << " network size = " <<_network_size << endl;
    }
//    if(complete_region == 0.0){
//      cout << "lower neighbor number " << lower_neighbors.size() << " upper neighbor number " << upper_neighbors.size() << endl;
//    }
//    cout << "end " << complete_region <<endl;
  }
  if(duplicate == true){
    my_int missing_region_sum=0;
    for(int i=0;i<dup_cum_result.size();i+=2){
      if(i==0){
 //         cout << "returned nodes = " << dup_cum_result[i] << " latency = " << dup_cum_result[i+1] << endl;
      }else{
  //      cout << "backup query missing region from " << dup_cum_result[i] << " to " << dup_cum_result[i+1] <<endl;
        missing_region_sum += (dup_cum_result[i+1] - dup_cum_result[i]);
      }
    }
    cout << ((double)missing_region_sum/(double)(_r1-_r0)) << endl;

    missing_region_sum = 0;
    for(int i=0;i<cum_result.size();i+=2){
      if(i==0){
//          cout << "returned nodes = " << cum_result[i] << " latency = " << cum_result[i+1] << endl;
      }else{
        missing_region_sum += (cum_result[i+1] - cum_result[i]);
  //      cout << "original query missing region from " << cum_result[i] << " to " << cum_result[i+1] <<endl;
      }
    }
    cout << ((double)missing_region_sum/(double)(_r1-_r0)) << endl;
    return combineResults(cum_result, dup_cum_result);
  }
//  cout << "no dup resutl return" << endl;
//  cout << "cum_result[0] = " << cum_result.size()<< endl;
//  cout << "done" << endl;
  return cum_result;
}

void DeetooMessage::updateVector(std::vector<my_int>& org_vect, my_int begin_addr, my_int end_addr){
  vector<int> remove_cand;
  for(int i=2;i<org_vect.size();i+=2){
//    cout << "in update vector " << org_vect[i] << " " <<org_vect[i+1] << " " << begin_addr << " " <<end_addr << endl;
    if(org_vect[i] >= begin_addr && org_vect[i+1] <= end_addr){
      remove_cand.push_back(i);
      remove_cand.push_back(i+1);
    }
  }
  
  vector<int>::reverse_iterator itrt;
  for(itrt=remove_cand.rbegin();itrt!=remove_cand.rend();itrt++){
 //   cout << "remove index = " << *itrt  <<endl;
    org_vect.erase(org_vect.begin()+(*itrt));
  }
  if(end_addr < begin_addr){
    cout << "begin addr " << begin_addr << " is bigger than end address " << end_addr << endl;
  }
  org_vect.push_back(begin_addr);
  org_vect.push_back(end_addr);
}

std::vector<my_int> DeetooMessage::combineResults(std::vector<my_int>& cum_result, std::vector<my_int>& dup_cum_result){
  std::vector<my_int> new_result;
  my_int comb_count = cum_result[0] > dup_cum_result[0] ? cum_result[0] : dup_cum_result[0];
  my_int comb_lat = cum_result[1] > dup_cum_result[1] ? cum_result[1] : dup_cum_result[1];
  new_result.push_back(comb_count);
  new_result.push_back(comb_lat);
  for(int i=2;i<cum_result.size();i+=2){
    for(int j=2;j<dup_cum_result.size();j+=2){
      if(cum_result[i]==dup_cum_result[j] && cum_result[i+1]==dup_cum_result[j+1]){
        new_result.push_back(cum_result[i]);
        new_result.push_back(cum_result[i+1]);
//        if(cum_result[i+1] < cum_result[i]){
//          cout << cum_result[i] << " " << cum_result[i+1] << " " << dup_cum_result[j] << " " << dup_cum_result[j+1] << endl;
  //      }
//        cout << "missing region from " << cum_result[i] << " to "<< cum_result[i+1] << endl;
        continue;
      }else if(cum_result[i]>dup_cum_result[j] && cum_result[i]<dup_cum_result[j+1] && cum_result[i+1] > dup_cum_result[j+1]){
        new_result.push_back(cum_result[i]);
        new_result.push_back(dup_cum_result[j+1]);
  //      if(dup_cum_result[j+1] < cum_result[i]){
 //         cout << cum_result[i] << " " << cum_result[i+1] << " " << dup_cum_result[j] << " " << dup_cum_result[j+1] << endl;
//        }        
 //       cout << "missing region from " << cum_result[i] << " to "<< dup_cum_result[j+1] << endl;
        continue;
      }else if(cum_result[i]<dup_cum_result[j] && cum_result[i+1]>dup_cum_result[j] && cum_result[i+1]<dup_cum_result[j+1]){
        new_result.push_back(dup_cum_result[j]);
        new_result.push_back(cum_result[i+1]);
//        if(cum_result[i+1] < dup_cum_result[j]){
 //         cout << cum_result[i] << " " << cum_result[i+1] << " " << dup_cum_result[j] << " " << dup_cum_result[j+1] << endl;
//        }        
  //      cout << "missing region from " << dup_cum_result[j] << " to "<< cum_result[i+1] << endl;
        continue;
      }else if(cum_result[i]>dup_cum_result[j] && cum_result[i+1]<dup_cum_result[j+1]){
        new_result.push_back(cum_result[i]);
        new_result.push_back(cum_result[i+1]);
//        if(cum_result[i+1] < cum_result[i]){
 //         cout << cum_result[i] << " " << cum_result[i+1] << " " << dup_cum_result[j] << " " << dup_cum_result[j+1] << endl;
//        }        
 //       cout << "missing region from " << cum_result[i] << " to "<< cum_result[i+1] << endl;
        continue;
      }else if(cum_result[i]<dup_cum_result[j] && cum_result[i+1]>dup_cum_result[j+1]){
        new_result.push_back(dup_cum_result[j]);
        new_result.push_back(dup_cum_result[j+1]);
//        if(dup_cum_result[j+1] < dup_cum_result[j]){
 //         cout << cum_result[i] << " " << cum_result[i+1] << " " << dup_cum_result[j] << " " << dup_cum_result[j+1] << endl;
//        }        
 //       cout << "missing region from " << dup_cum_result[j] << " to "<< dup_cum_result[j+1] << endl;
        continue;
      }else if(cum_result[i]==dup_cum_result[j] && cum_result[i+1] != dup_cum_result[j+1]){
        new_result.push_back(cum_result[i]);
        new_result.push_back(min(dup_cum_result[j+1], cum_result[i+1]));
//        if((min(dup_cum_result[j+1], cum_result[i+1]))< cum_result[i]){
 //         cout << cum_result[i] << " " << cum_result[i+1] << " " << dup_cum_result[j] << " " << dup_cum_result[j+1] << endl;
//        }        
  //      cout << "missing region from " << dup_cum_result[j] << " to "<< min(dup_cum_result[j+1], cum_result[i+1]) << endl;   
        continue;
      }else if(cum_result[i]!=dup_cum_result[j] && cum_result[i+1] == dup_cum_result[j+1]){
        new_result.push_back(max(cum_result[i], dup_cum_result[j]));
        new_result.push_back(cum_result[i+1]);
 //       if(cum_result[i+1] < (max(cum_result[i], dup_cum_result[j]))){
   //       cout << cum_result[i] << " " << cum_result[i+1] << " " << dup_cum_result[j] << " " << dup_cum_result[j+1] << endl;
 //       }        
      // cout << "missing region from " << max(cum_result[i], dup_cum_result[j]) << " to "<< cum_result[i+1]<< endl;       
        continue;
      }
    }
  }
  return new_result;
}

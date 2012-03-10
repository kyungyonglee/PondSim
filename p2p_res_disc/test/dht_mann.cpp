#include <netmodeler.h>
#include <iostream>
#include <sstream>
#include <fstream>
#include <cmath>
#include <time.h>

/*
 * add functionality of edge failure to d2_test.cpp
 */
using namespace Starsky;
using namespace std;
#define ADDR_MAX 65536
//#define INT64
#ifdef INT64
  typedef unsigned long long my_int;
  #define ADDR_MAX 4294967296LL
#else
  typedef unsigned long my_int;
  #define ADDR_MAX 65536L
#endif

void printInfo(map<my_int, pair<double, double> > result) {
	ofstream myfile("output1");
	myfile << "#nodes: " << "\t" << "hit rate" << "\t" << "ave msgs" << "\t" << "hops/hit_rate" << endl;
	map<my_int,pair<double, double> >::iterator it;
	for (it=result.begin(); it!=result.end(); it++) {
		double hops_ps = it->second.second / it->second.first;
		myfile << it->first << "\t" << it->second.first << "\t" << it->second.second << "\t" << hops_ps << endl;
	}
}
//random string generator
std::set<std::string> rstringGenerator ( int howmany, int length, Ran1Random& r )
{
    std::set<std::string> items;
    for (int no=0; no < howmany; no++)
    {
	std::string item;
	for (int i = 0; i < length; i++)
	{
            int rand_no = (int) (r.getDouble01() * 122);
	    if ( rand_no < 65 ) { rand_no = 65 + rand_no % 56;}
	    if ( (rand_no > 90) && (rand_no < 97) ) { rand_no += 6; }
	    item += (char)rand_no;		  
	}
	items.insert(item);
    }
    return items;
}

int main(int argc, char *argv[]) 
{
  //int max_node = 100000;
  if (argc < 7) {
    cerr << "Usage: " << argv[0] << ", number of nodes, node_fail_prob, p_drop_prob, fail_fail_drop_prob" << endl;
  }
  int run_index = 0;
  int nodes = atoi(argv[1]);
  double n_fail = atof(argv[2]);
  double p = atof(argv[3]);
  double ff_p = atof(argv[4]);

  //    Ran1Random ran_no = Ran1Random(time(NULL));
  Ran1Random ran_no;
  my_int rg_start, rg_end; 
  map<my_int, pair<double, double> > result;
  auto_ptr<DeetooNetwork> cacheNet_ptr( new DeetooNetwork(nodes, ran_no) );
  cacheNet_ptr->createEvenNet(nodes);
  my_int sum_c_nodes=0, sum_c_edges=0, sum_c_in_depth=0, sum_c_depth=0;
  auto_ptr<NodeIterator> ni (cacheNet_ptr->getNodeIterator() );
  while ( ni->moveNext() ) {
    AddressedNode* item_source = dynamic_cast<AddressedNode*> ( ni->current() );
    double cqsize = (double) ADDR_MAX; 		
    std::pair<my_int, my_int> c_ranges = cacheNet_ptr->getRange(cqsize);
    my_int rg_start = c_ranges.first, rg_end = c_ranges.second;
    DeetooNetwork* cacheNet = cacheNet_ptr.get();
    auto_ptr<DeetooMessage> cache_m ( new DeetooMessage(rg_start, rg_end, n_fail, ran_no, p, ff_p, is_wait_factor, allowed_time) );
    auto_ptr<DeetooNetwork> cached_net( cache_m->visit(item_source, *cacheNet) );
    my_int c_in_depth = cached_net->getDistance(cache_m->init_node); //in range depth
//    cout << "depth = =" << c_in_depth << endl;
    sum_c_in_depth += c_in_depth;
    my_int c_depth = c_in_depth + cache_m->out_edge_count;   // in depth + out of range depth
    sum_c_depth += c_depth;
    my_int c_nodes = cached_net->getNodeSize();
    sum_c_nodes += c_nodes;
    my_int c_edges = cached_net->getEdgeSize();
    sum_c_edges += c_edges;
    run_index++;
  }
}    

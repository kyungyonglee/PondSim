#ifndef starsky__mapreduce_task_H
#define starsky__mapreduce_task_H
#include <map>
#include <vector>
#include "netmodeler.h"
#include <ran1random.h>


//#define INT64
#ifdef INT64
  typedef unsigned long long my_int;
#else
  typedef unsigned long my_int;
#endif
using namespace Starsky;


namespace Starsky {
class MapReduceTask {
  public:
     MapReduceTask(my_int self_addr, Ran1Random&  ran, double p_drop_prob, double ff_p_drop_prob, char* failed_nodes);
     ~MapReduceTask();
    void Map(std::vector<my_int> *args);
    my_int GetElapsedTime(int edge_latency, my_int my_address);

    /**
     * Performs the givn Reduce function with cumulated result and new result from child nodes
     * @param cum_result cumulated result so far
     * @param cur_result ruturned result from the child node
     * @return new cumulated result (cum_result U cur_result
     */
    std::vector<my_int> Reduce(std::vector<my_int> *cum_result, std::vector<my_int> cur_result, my_int child_addr, my_int elapsed_time, int allowed_time);
  protected:
    double _packet_drop_prob;
    Ran1Random& _random;
    double _ff_p_drop_prob;
    char* _failed_nodes;
    my_int _self_addr;
    Ran1Random* _packet_drop_rand;
    int _max_resends;
  };
}

#endif


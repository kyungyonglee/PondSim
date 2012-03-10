#include "MapReduceTask.h"
#include <cstdlib>
#include <iostream>
#include <time.h>
#include <tr1/random>
#include <math.h>
//#include <tr1/unordered_map>

using namespace Starsky;
using namespace std;
using namespace std::tr1;

MapReduceTask::MapReduceTask(my_int self_addr, Ran1Random&  ran, double p_drop_prob, double ff_p_drop_prob, char* failed_nodes):_self_addr(self_addr), _random(ran), _packet_drop_prob(p_drop_prob), _ff_p_drop_prob(ff_p_drop_prob){
//  cout << "MapReduce Task creation" << endl;
//  _base_latency = 0;
  _packet_drop_rand = new Ran1Random(ceil(_random.getDouble01()*1000000));
  _max_resends = 5;
  _failed_nodes = failed_nodes;
}

MapReduceTask::~MapReduceTask(){
  delete(_packet_drop_rand);
}

void MapReduceTask::Map(std::vector<my_int> *input){
//  std::tr1::ranlux64_base_01 engine;
//  srand(time(NULL));
//  engine.seed(rand());
//  std::tr1::exponential_distribution<double> exponential(0.001);
//  cout << "exponential value is " << exponential(engine) <<endl;
  
  input->push_back(1);
  input->push_back(0);
}

std::vector<my_int> MapReduceTask::Reduce(std::vector<my_int> *cum_results, std::vector<my_int> new_results, my_int child_index, my_int elapsed_time, int allowed_time){
  if(new_results.size() == 0){
    new_results.push_back(0);
    new_results.push_back(0);
  }
  
  if(_failed_nodes[child_index] != 0){
    if(allowed_time > 0){
      if(new_results[1]+elapsed_time > allowed_time){
        if(cum_results->at(1) < allowed_time){
          cum_results->at(1) = allowed_time;
        }
      }else{
        if(cum_results->at(1) < new_results[1]+elapsed_time){
          cum_results->at(1) = new_results[1]+elapsed_time;
        }
      }
    }else{
//      if(new_results == NULL){
  //      cout << "new result is null " << endl;
    //  }
/*
      cout << "begin" <<endl;
      cout << "cum_results->at(1)  " << cum_results->at(1) << endl;;
      cout << "next" <<endl;
      cout << " elapsed_time " << elapsed_time << endl;
      cout << "last" <<endl;
      cout << "new_results[0] " << new_results[0]<<endl;
      cout << "new_results[1] " << new_results[1]<< endl;
      cout << "done" << endl;
      */
  //    my_int temp1 = cum_results->at(1), temp2 = elapsed_time, temp3 = new_results[1];
      if(cum_results->at(1) < elapsed_time+ new_results[1]){
        cum_results->at(1) = elapsed_time+ new_results[1];
      }
    }
//    cout << "fail detection at " << child_addr <<endl;
    cum_results->push_back(0);    // 0 means failure
    return *cum_results;
  }

  if(allowed_time != -1 && (allowed_time < new_results[1]+elapsed_time)){
    cum_results->at(1) = allowed_time;
    cum_results->push_back(0);    // 0 means failure
//    cout << "0 inserted " <<endl;
    return *cum_results;    
  }
  
  for(int i=0;i<new_results.size();i++){
    if(i == 0){
      cum_results->at(i) = (cum_results->at(i) + new_results[i]);
    }else if(i==1){
      if(cum_results->at(i) < elapsed_time+ new_results[i]){
        cum_results->at(i) = elapsed_time + new_results[i];
      }
    }else{
      cum_results->push_back(new_results[i]);
    }
  }
  cum_results->push_back(1); // 1 means no failure
  return *cum_results;
}

my_int MapReduceTask::GetElapsedTime(int edge_latency, my_int child_index){  
  my_int total_time = 2*edge_latency, reschedule_time = 0, drop_count = 0;
  double drop_prob = _packet_drop_prob;
  if((_failed_nodes[child_index] != 0) ){
    for(int i=0;i<_max_resends;i++){
//    for(int i=0;;i++){      
       drop_count++;
       reschedule_time = edge_latency*ceil(_packet_drop_rand->getDouble01() * (std::tr1::pow((my_int)2,drop_count)));
       reschedule_time = reschedule_time>10000?10000:reschedule_time;
    //   reschedule_time = 5000;
       total_time += (2*edge_latency + reschedule_time);          
    }
    return total_time;
  }
  
  double drop_decison = _packet_drop_rand->getDouble01();
  while(drop_decison < drop_prob ){
 //   cout << "drop prob = " << drop_prob << " ff_p_drop_prob = " << _ff_p_drop_prob << endl;
    drop_count++;
    reschedule_time = edge_latency*ceil(_packet_drop_rand->getDouble01() * (std::tr1::pow((my_int)2,drop_count)));
    reschedule_time = reschedule_time>10000?10000:reschedule_time;
//   cout<< "drop count = " << drop_count << " reschedule time = " << reschedule_time << endl;
 //   reschedule_time = 5000;
    total_time += (2*edge_latency + reschedule_time);    
//    cout << "latency exponential backoff value = " << drop_count <<" edge latency " << edge_latency << " total added time = " << total_time <<endl;
    drop_decison = _packet_drop_rand->getDouble01();
    if(_ff_p_drop_prob > 0.0){
      drop_prob = _ff_p_drop_prob;
    }
  }
  return total_time;
}

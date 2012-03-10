#include <iostream>
#include <string.h>
#include <cmath>
#include "event_scheduler.h"
#include "event_action.h"

using namespace std;
using namespace klee_simulation;

int main(int argc, char *argv[]){
  int job_ia_time, run_time_secs, num_flock_server;
  string submit_filename;
  srand(time(NULL));
  if (argc == 4){
    job_ia_time = atoi(argv[2]);
    run_time_secs = atoi(argv[1]);
    num_flock_server = atoi(argv[3]);
  }else if(argc == 5){
    job_ia_time = atoi(argv[2]);
    run_time_secs = atoi(argv[1]);
    num_flock_server = atoi(argv[3]);
    submit_filename = argv[4];
  }else{
    cout << "event_test total_run_time_secs job_inter_arrival_time_msec num_flock_server" << endl;
    return 1;
  }
  EventScheduler* event_scheduler = new EventScheduler();
  vector<MatchmakingServer*> mm_servers;
  int mm_latency = num_flock_server > 1000 ? 1:1000/num_flock_server;
  mm_latency = job_ia_time;
  for(int i=0;i<num_flock_server;i++){
    mm_servers.push_back(new MatchmakingServer(event_scheduler, mm_latency, 60000, 20000));
  }
  JobSubmitter* job_submitter;
  if(submit_filename.size() == 0){
    job_submitter = new JobSubmitter(event_scheduler, mm_servers, job_ia_time);
  }else{
    job_submitter = new JobSubmitterFromLog(event_scheduler, mm_servers, submit_filename);
  }
  job_submitter->Execute();
  event_scheduler->Run(run_time_secs*1000);
  job_submitter->GetMatchmakingTimeStat();
}

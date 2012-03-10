

#include <iostream>
#include <string.h>
#include <cmath>
#include <sstream>
#include <fstream>
#include "event_action.h"

using namespace std;
using namespace klee_simulation;

EventAction::EventAction(EventScheduler* event_scheduler) : _event_scheduler(event_scheduler){
}

MatchmakingServer::MatchmakingServer(EventScheduler* esched, int mm_time_msec, int mm_period, int mm_min_period) : EventAction(esched), _matchmaking_time(mm_time_msec), _mm_min_period(mm_min_period), _mm_period(mm_period){
//  _last_mm_exec_time = ~(rand()%mm_period);
  _last_mm_exec_time = 0;
//  cout <<this <<  " : _last_mm_exec_time = " << _last_mm_exec_time << endl;
  _run_matchmaking = false;
  _mm_next_event_time = 0;
  _job_submitter = NULL;
  _random_mm_period = true;
}

void MatchmakingServer::SetJobSubmitter(JobSubmitter* js){
  _job_submitter = js;
}

void MatchmakingServer::Execute(){
  int cur_time = _event_scheduler->GetCurrentTime();
  if(_mm_next_event_time > 0 && _mm_next_event_time == cur_time){
  //  cout <<"matchmaking " << cur_time<<endl;
    if(_queued_job_list.size() != 0){
      int pop_id = _queued_job_list.front();
      int submit_time = _mm_throughtput[pop_id];
      _mm_throughtput[pop_id] = (cur_time - submit_time)+_matchmaking_time;
      _queued_job_list.pop();
//      cout <<this << " job complete id = " << pop_id << " cur_time = " << cur_time <<endl;
      _job_submitter->JobMmComplete(pop_id,  (cur_time - submit_time)+_matchmaking_time, _matchmaking_time);
      _mm_next_event_time = cur_time + _matchmaking_time;
      _event_scheduler->InsertEventTemp(_matchmaking_time, this, false);
    }else{
//      cout <<this << " : periodic : " << cur_time << endl;
      int next_scheduling = _random_mm_period == true ? (rand()%((_mm_period-20)*2)) + 21 : _mm_period; //in order to variate scheduling time of many flock servers
      _event_scheduler->InsertEventTemp(next_scheduling, this, false);
      _mm_next_event_time = cur_time + next_scheduling;
      _last_mm_exec_time = cur_time;
      _run_matchmaking = false;
    }
  }
}

void MatchmakingServer::JobSubmit(int job_id){
  int cur_time = _event_scheduler->GetCurrentTime();
  int elapsed_time = cur_time - _last_mm_exec_time;
  _mm_throughtput[job_id] = cur_time;
  _queued_job_list.push(job_id);
  if(_run_matchmaking == false){
    if(elapsed_time >= _mm_min_period){
      _event_scheduler->InsertEventTemp(1, this, false);
      _mm_next_event_time = cur_time+1;
//      cout << this <<" instantaneous start cur_time "<< cur_time << " : next event time = "<< _mm_next_event_time << endl;
    }else{
       _event_scheduler->InsertEventTemp(_mm_min_period - elapsed_time,this, false);
       _mm_next_event_time = cur_time + _mm_min_period - elapsed_time;
//       cout << this <<" delayed start cur_time " << cur_time << " : next event time = "<< _mm_next_event_time << endl;       
    }    
    _run_matchmaking = true;
  }
}

void MatchmakingServer::GetMatchmakingTimeStat(){
  int total_size = _mm_throughtput.size(), min_time=0x7fffffff, max_time = 0;
  long taken_time=0;
  map<int,int>::iterator mmt_it;
  int non_complete_num=0;
  while(!_queued_job_list.empty()){
    non_complete_num++;
    _mm_throughtput.erase(_queued_job_list.front());
    _queued_job_list.pop();
  }
  
  for(mmt_it=_mm_throughtput.begin();mmt_it!=_mm_throughtput.end();mmt_it++){
    taken_time += mmt_it->second;
    min_time = min_time < mmt_it->second ? min_time : mmt_it->second;
    max_time = max_time < mmt_it->second ? mmt_it->second : max_time;
  }
  cout << total_size << "\t" << (taken_time/total_size) << "\t" << min_time << "\t" << max_time << "\t" << non_complete_num<< endl;
}

int MatchmakingServer::GetMatchmakingTime(int job_id){
  return _mm_throughtput.count(job_id) != 0 ? _mm_throughtput[job_id] : -1;
}

void MatchmakingServer::DeleteQueuedJobs(){
  while(!_queued_job_list.empty()){
    _mm_throughtput.erase(_queued_job_list.front());
    _queued_job_list.pop();
  }
}

void MatchmakingServer::SetMmPeriodMode(bool random_mode){
  _random_mm_period = random_mode;
}

JobSubmitter::JobSubmitter(EventScheduler* esched, MatchmakingServer* mm_server, int submit_rate): EventAction(esched), _mm_server(mm_server), _avg_submit_rate(submit_rate){
  InitParam();
  _flock_enabled = false;
}

JobSubmitter::JobSubmitter(EventScheduler* esched, vector<MatchmakingServer*>& flock_mm_servers, int submit_rate): EventAction(esched), _flock_mm_servers(flock_mm_servers), _avg_submit_rate(submit_rate){
  InitParam();
  _flock_enabled = true;
  _mm_server = flock_mm_servers[0];
}

void JobSubmitter::InitParam(){
  _rand_eng.seed(time(NULL));
  _poisson_dist = std::tr1::poisson_distribution<double>(100.0);
  bool mm_random_period = _flock_mm_servers.size() > 1 ? true : false;
  for(int i=0;i<_flock_mm_servers.size();i++){
    MatchmakingServer* mm_server = _flock_mm_servers[i];
    mm_server->SetJobSubmitter(this);
    mm_server->SetMmPeriodMode(mm_random_period);
  }
  _pending_jobs[0] = -1;
}

void JobSubmitter::Execute(){
  int cur_time = _event_scheduler->GetCurrentTime();
  int id = _pending_jobs[cur_time];
  if(id == -1){
    SubmitJob();
    int next_submit = (_poisson_dist(_rand_eng)* _avg_submit_rate)/100;
//    cout << next_submit << endl;
    _event_scheduler->InsertEventTemp(next_submit, this, false);  
    _pending_jobs[cur_time+next_submit] = -1;
  }else{
    SubmitJob(id);
  }
  _pending_jobs.erase(cur_time);
}

void JobSubmitter::SubmitJob(){
  int id = rand();
  _job_ids.push_back(id);
  _job_mm_time[id] = new vector<int>();
  _mm_server->JobSubmit(id);
}

void JobSubmitter::SubmitToFlock(){
  int id = rand();
  _job_ids.push_back(id);
  _job_mm_time[id] = new vector<int>();  
  for(int i=0;i<_flock_mm_servers.size();i++){
    MatchmakingServer* mm_server = _flock_mm_servers[i];
    mm_server->JobSubmit(id);
  }
}

void JobSubmitter::SubmitJob(int id){
  vector<int>* cur_time = _job_mm_time[id];
  if(cur_time == NULL || cur_time->size() == _flock_mm_servers.size()){
    return;
  }
  MatchmakingServer* mm_server = _flock_mm_servers[cur_time->size()];
  mm_server->JobSubmit(id);
}

void JobSubmitter::GetMatchmakingTimeStat(){
  int taken_time = 0, each_time=0, min_time=0x7fffffff, max_time=0;
  bool not_complete = false;
  for(int j=0;j<_flock_mm_servers.size();j++){
    MatchmakingServer* mm_server = _flock_mm_servers[j];
    mm_server->DeleteQueuedJobs();
  }
  long total_time = 0, num_target_jobs=0;
  for(int i=1;i<_job_ids.size();i++){
    taken_time = 0;
    not_complete = false;
    for(int j=0;j<_flock_mm_servers.size();j++){
      MatchmakingServer* mm_server = _flock_mm_servers[j];
      each_time = mm_server->GetMatchmakingTime(_job_ids[i]);
      if(each_time < 0){
        not_complete = true;
        break;
      }
      taken_time += each_time;
    }
    if(not_complete == false){
      total_time += taken_time;
      num_target_jobs++;
      min_time = taken_time <min_time ? taken_time : min_time;
      max_time = taken_time < max_time ? max_time : taken_time;
    }
  }
  cout << _flock_mm_servers.size() <<"\t" << _avg_submit_rate <<"\t" << (total_time/num_target_jobs) << "\t" << min_time << "\t" << max_time << endl;
}

void JobSubmitter::JobMmComplete(int id, int time, int due){
  int cur_time = _event_scheduler->GetCurrentTime();
  _pending_jobs[due+cur_time] = id;
  vector<int>* times = _job_mm_time[id];
  times->push_back(time);
  _event_scheduler->InsertEventTemp(due, this, false);
}

JobSubmitterFromLog::JobSubmitterFromLog(EventScheduler* esched, vector<MatchmakingServer*>& flock_mm_servers, string scenario_filename) : JobSubmitter(esched, flock_mm_servers, 0){
  BuildSubmissionScenario(scenario_filename);
}
JobSubmitterFromLog::~JobSubmitterFromLog(){
  map<int, vector<int>* >::iterator lat_it;
  for(lat_it=_latency_map.begin();lat_it!=_latency_map.end();lat_it++){
    delete lat_it->second;
  }
}

void JobSubmitterFromLog::BuildSubmissionScenario(string input_file){
  string line;
  ifstream file(input_file.c_str());
  if(file.is_open()){
    int prev_time = 0, cur_time = 0;
    while(file.good()){
      getline(file, line);
      char* pch;
      pch = strtok((char*)line.c_str(), "\t");
      while(pch != NULL){        
        cur_time = (int)atoi(pch);
        break;
      }
//      cout << prev_time << " : " << cur_time << endl;
      if(cur_time == prev_time){
        _job_submission_time.push(1);
      }else{
        _job_submission_time.push((cur_time-prev_time)*1000);
      }
      prev_time = cur_time;
    }
    file.close();
  }
}

void JobSubmitterFromLog::Execute(){
  int cur_time = _event_scheduler->GetCurrentTime();
  SubmitJob();
  if(_job_submission_time.size() == 0){
    return;
  }
  int next_submit = _job_submission_time.front();
  _job_submission_time.pop();
  if(next_submit == 0){
    next_submit++;
  }        
//  cout << next_submit << endl;
  _event_scheduler->InsertEventTemp(next_submit, this, false);  
}

void JobSubmitterFromLog::JobMmComplete(int id, int total_taken_time, int due){
  int cur_time = _event_scheduler->GetCurrentTime();
  int submit_time = cur_time - total_taken_time + due;
  int key = submit_time/3600000;
  vector<int>* latency_vector;
  if(_latency_map.count(key) == 0){
    latency_vector = new vector<int>();
    _latency_map[key] = latency_vector;
  }else{
    latency_vector = _latency_map[key];
  }
  latency_vector->push_back(total_taken_time);
}

void JobSubmitterFromLog::GetMatchmakingTimeStat(){
  map<int, vector<int>* >::iterator lat_it;
  for(lat_it=_latency_map.begin();lat_it!=_latency_map.end();lat_it++){
    vector<int>* lat_vec = lat_it->second;
//    cout << lat_it->first << " : " << lat_vec->size() << endl;
    long sum = 0;
    for(int i=0;i<lat_vec->size();i++){
      sum += (*lat_vec)[i];
    }
    cout << (sum/lat_vec->size()) << endl;
  }
}





#ifndef klee_simulation_event_action_h
#define klee_simulation_event_action_h
#include <iostream>
#include <string.h>
#include <cmath>
#include <queue>
#include <tr1/random>
#include "event_scheduler.h"

using namespace std;
namespace klee_simulation{
  class JobSubmitter;
  class EventAction{
    public:
      EventAction(EventScheduler* event_scheduler);
      virtual void Execute() = 0;
    protected:
      EventScheduler* _event_scheduler;
  };

  class MatchmakingServer : public EventAction{
    public:
      MatchmakingServer(EventScheduler* esched, int mm_time_msec, int mm_period, int mm_min_period);
      void Execute();
      void JobSubmit(int id);
      void GetMatchmakingTimeStat();
      int GetMatchmakingTime(int job_id);
      void DeleteQueuedJobs();
      void SetJobSubmitter(JobSubmitter* js);
      void SetMmPeriodMode(bool random_mode);
    protected:
      queue<int> _queued_job_list;
      map<int,int> _mm_throughtput;
      JobSubmitter* _job_submitter;
      int _mm_next_event_time;
      int _matchmaking_time;
      int _mm_period;
      int _mm_min_period;
      int _last_mm_exec_time;
      bool _run_matchmaking;
      bool _random_mm_period;
  };

  class JobSubmitter : public EventAction{
    public:
      JobSubmitter(EventScheduler* esched, MatchmakingServer* mm_server, int submit_rate);
      JobSubmitter(EventScheduler* esched, vector<MatchmakingServer*>& mm_server, int submit_rate);
      virtual void Execute();      
      virtual void GetMatchmakingTimeStat();      
      virtual void JobMmComplete(int id, int time, int due);
    protected:
      void SubmitJob();      
      void SubmitJob(int id);
      void InitParam();
      void SubmitToFlock();
      MatchmakingServer* _mm_server;
      vector<MatchmakingServer*> _flock_mm_servers;
      map<int, vector<int>* > _job_mm_time;
      map<int,int> _pending_jobs;
      vector<int> _job_ids;
      int _avg_submit_rate;
      std::tr1::ranlux64_base_01 _rand_eng;
      std::tr1::poisson_distribution<double> _poisson_dist;     
      bool _flock_enabled;
  };

  class JobSubmitterFromLog : public JobSubmitter{
    public:
      JobSubmitterFromLog(EventScheduler* esched, vector<MatchmakingServer*>& flock_mm_servers, string scenario_filename);
      ~JobSubmitterFromLog();
      void Execute();
      void JobMmComplete(int id, int time, int due);
      void GetMatchmakingTimeStat();
    protected:      
      void BuildSubmissionScenario(string input_file);
      queue<int> _job_submission_time;
      map<int, vector<int>* > _latency_map;
  };
}
#endif

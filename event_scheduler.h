#ifndef klee_simulation_event_scheduler_h
#define klee_simulation_event_scheduler_h
#include <iostream>
#include <string.h>
#include <cmath>
#include <map>

using namespace std;
namespace klee_simulation{
  class EventAction;
  class EventScheduler{
    public:
      EventScheduler();
      ~EventScheduler();
      bool InsertEventTemp(int time, EventAction* action, bool absolute_time);      
      void CommitNewEvent();
      void Run(int run_time);
      int GetCurrentTime();

    protected:
      multimap<int, EventAction*>* _event_queue;
      multimap<int, EventAction*>* _temp_event;
      int _cur_time;
  };
}
#endif

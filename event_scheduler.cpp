

#include <iostream>
#include <string.h>
#include <cmath>
#include "event_scheduler.h"
#include "event_action.h"

using namespace std;
using namespace klee_simulation;

EventScheduler::EventScheduler(){
  _cur_time = 0;
  _event_queue = new multimap<int, EventAction*>();
  _temp_event = new multimap<int, EventAction*>();
}

EventScheduler::~EventScheduler(){
  delete _event_queue;
  delete _temp_event;
}

bool EventScheduler::InsertEventTemp(int due_time,EventAction * action, bool absolute_time){
  int new_time = absolute_time == true ? due_time : _cur_time +due_time;
  if(new_time <= _cur_time){
    return false;
  }
  _temp_event->insert(pair<int, EventAction*>(new_time, action));
}

void EventScheduler::CommitNewEvent(){
  multimap<int, EventAction*>::iterator temp_it;
  for(temp_it=_temp_event->begin();temp_it!=_temp_event->end();temp_it++){
    _event_queue->insert(pair<int, EventAction*>(temp_it->first, temp_it->second));
  }
  _temp_event->clear();
}

void EventScheduler::Run(int run_time){
  pair<multimap<int, EventAction*>::iterator, multimap<int,EventAction*>::iterator> range;
  multimap<int,EventAction*>::iterator entry_it;
  CommitNewEvent();
  for(_cur_time = 0; _cur_time < run_time;_cur_time++){
    if(_event_queue->count(_cur_time) != 0){
      range = _event_queue->equal_range(_cur_time);
      for(entry_it=range.first;entry_it!=range.second;++entry_it){
        entry_it->second->Execute();
      }
      _event_queue->erase(_cur_time);
      CommitNewEvent();
    }
  }
}

int EventScheduler::GetCurrentTime(){
  return _cur_time;
}



#ifndef EVENT_HANDLER_HPP
#define EVENT_HANDLER_HPP

#include <sys/event.h>

#include <map>
#include <set>

#include "core.hpp"

class EventHandler {
 public:
  EventHandler();

  virtual ~EventHandler();

  int Init(int event_size);
  int Polling(int& nev, long timeout_sec);
  int Regist(int ident, int16_t filter, uint64_t flags, uint32_t fflags,
             int64_t data, void* udata);
  int RegistTimeout(int ident, int time_sec, void* udata);
  int Delete(int ident, int16_t filter);
  int DeleteAll(int ident);

  struct kevent* events() const;

 private:
  EventHandler(const EventHandler& ref);

  EventHandler& operator=(const EventHandler& ref);

  int kq_;
  struct kevent* events_;
  int event_size_;
  std::map<int, std::set<int> > registed_event_;
};

#endif

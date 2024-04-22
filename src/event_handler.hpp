#ifndef EVENT_HANDLER_HPP
#define EVENT_HANDLER_HPP

#include <sys/event.h>

#include <map>
#include <set>

#include "core.hpp"

typedef struct kevent Event;

class EventHandler {
 public:
  typedef std::map<int, std::set<Event> > AddedEvent;

  EventHandler();

  virtual ~EventHandler();

  int Init(int event_size);
  int Polling(int& nev, long timeout_sec);
  int Add(int ident, int16_t filter, uint64_t flags, uint32_t fflags,
          int64_t data, void* udata);
  int Delete(int ident, int16_t filter);
  int AddWithTimer(int ident, int16_t filter, uint64_t flags, uint32_t fflags,
                   int64_t data, void* udata, int time_sec);
  int Delete(int ident);

  const struct kevent* events() const;
  struct kevent* events();

 protected:
  // EventHandler();

 private:
  EventHandler(const EventHandler& ref);

  // EventHandler& operator=(const EventHandler& ref);

  int kq_;
  struct kevent* events_;
  AddedEvent added_event_;
};

bool operator<(const Event& lhs, const Event& rhs);
bool operator==(const Event& lhs, const Event& rhs);

#endif

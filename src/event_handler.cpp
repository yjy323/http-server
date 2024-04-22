#include "event_handler.hpp"

#include <unistd.h>

#include <iostream>

#include "stdlib.h"

EventHandler::EventHandler()
    : kq_(-1), events_(0), added_event_(AddedEvent()) {}

EventHandler::~EventHandler() {
  if (kq_ != -1) close(kq_);
  if (events_ != 0) delete[] events_;
}

int EventHandler::Init(int event_size) {
  if (kq_ != -1) {
    close(kq_);
    kq_ = -1;
  }
  if (events_ != 0) {
    delete[] events_;
    events_ = 0;
  }

  if ((kq_ = kqueue()) == -1) return ERROR;
  if ((events_ = new Event[event_size]) == 0) return ERROR;

  return OK;
}

int EventHandler::Polling(int& nev, long timeout_sec) {
  if (events_ == 0) return ERROR;

  std::size_t event_size = sizeof(events_) / sizeof(events_);

  struct timespec timeout;
  timeout.tv_sec = timeout_sec;
  timeout.tv_nsec = 0;

  nev = kevent(kq_, NULL, 0, events_, event_size, &timeout);

  if (nev == -1) return ERROR;
  return OK;
}

int EventHandler::Add(int ident, int16_t filter, uint64_t flags,
                      uint32_t fflags, int64_t data, void* udata) {
  Event event;

  EV_SET(&event, ident, filter, EV_ADD | flags, fflags, data, udata);
  if (kevent(kq_, &event, 1, NULL, 0, NULL) == -1) {
    return ERROR;
  }

  if (added_event_.find(ident) == added_event_.end())
    added_event_[ident] = std::set<Event>();
  added_event_[ident].insert(event);

  return OK;
}

int EventHandler::Delete(int ident, int16_t filter) {
  Event event;

  EV_SET(&event, ident, filter | EV_DELETE, 0, 0, 0, 0);
  if (kevent(kq_, &event, 1, 0, 0, 0) == -1) {
    return ERROR;
  }

  if (added_event_.find(ident) != added_event_.end()) {
    std::set<Event>& event_set = added_event_[ident];
    if (event_set.find(event) != event_set.end()) {
      event_set.erase(event);
    }
  }

  return OK;
}

int EventHandler::AddWithTimer(int ident, int16_t filter, uint64_t flags,
                               uint32_t fflags, int64_t data, void* udata,
                               int time_sec) {
  if (this->Add(ident, filter, flags, fflags, data, udata) == -1 ||
      this->Add(ident, EVFILT_TIMER, EV_ONESHOT, 0, time_sec * 1000, udata) ==
          -1) {
    return ERROR;
  }

  return OK;
}

int EventHandler::Delete(int ident) {
  Add(ident, EV_DELETE, 0, 0, 0, 0);
  added_event_.erase(ident);

  return OK;
}

const struct kevent* EventHandler::events() const { return events_; }
struct kevent* EventHandler::events() { return events_; }

bool operator<(const Event& lhs, const Event& rhs) {
  if (lhs.ident != rhs.ident) {
    return lhs.ident < rhs.ident;
  }
  return lhs.filter < rhs.filter;
}

bool operator==(const Event& lhs, const Event& rhs) {
  return (lhs.ident == rhs.ident && lhs.filter == rhs.filter);
}

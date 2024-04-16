#include "event_handler.hpp"

#include <unistd.h>

#include <iostream>

#include "stdlib.h"

#define INIT_ERROR_MESSAGE "EventHandler 초기화 중 error 발생"
#define POLLING_ERROR_MESSAGE "event Polling 중 error 발생"

#define POLLING_TIMEOUT_MESSAGE "event Polling timeout"

EventHandler::EventHandler()
    : kq_(-1), events_(0), event_size_(-1), registed_event_() {}

EventHandler::~EventHandler() {
  if (this->kq_ != -1) close(this->kq_);
  if (this->events_ != 0) delete[] this->events_;
}

int EventHandler::Init(int event_size) {
  if (this->kq_ == -1) this->kq_ = kqueue();
  if (this->events_ == 0) this->events_ = new struct kevent[event_size];
  this->event_size_ = event_size;
  this->registed_event_.clear();

  if (this->kq_ == -1 || this->events_ == 0) {
    std::cerr << INIT_ERROR_MESSAGE << std::endl;
    return ERROR;
  }

  return OK;
}

int EventHandler::Polling(int& nev, long timeout_sec) {
  struct timespec timeout;

  timeout.tv_sec = timeout_sec;
  timeout.tv_nsec = 0;

  nev = kevent(this->kq_, NULL, 0, this->events_, this->event_size_, &timeout);
  if (nev == -1) {
    std::cerr << POLLING_ERROR_MESSAGE << std::endl;

    return ERROR;
  } else if (nev == 0) {
    std::clog << POLLING_TIMEOUT_MESSAGE << std::endl;

    return ERROR;
  }

  std::clog << nev << "개의 event catch!" << std::endl;

  return OK;
}

int EventHandler::Regist(int ident, int16_t filter, uint64_t flags,
                         uint32_t fflags, int64_t data, void* udata) {
  struct kevent event;

  EV_SET(&event, ident, filter, EV_ADD | flags, fflags, data, udata);
  if (kevent(this->kq_, &event, 1, NULL, 0, NULL) == -1) {
    std::cerr << "event 등록 실패 { ";
    std::cerr << "ident: " << ident << ", ";
    std::cerr << "filter: " << filter << ", ";
    std::cerr << " }" << std::endl;

    return ERROR;
  }

  if (this->registed_event_.find(ident) == this->registed_event_.end())
    this->registed_event_[ident] = std::set<int>();
  this->registed_event_[ident].insert(filter);

  return OK;
}

int EventHandler::Delete(int ident, int16_t filter) {
  struct kevent event;

  std::map<int, std::set<int> >::iterator it_registed_event =
      this->registed_event_.find(ident);
  if (it_registed_event == this->registed_event_.end()) return ERROR;

  std::set<int>& registed_filter = it_registed_event->second;
  if (registed_filter.find(filter) == registed_filter.end()) return ERROR;

  EV_SET(&event, ident, filter, EV_DELETE, 0, 0, 0);
  kevent(this->kq_, &event, 1, NULL, 0, NULL);

  registed_filter.erase(filter);
  if (registed_filter.size() == 0) {
    this->registed_event_.erase(ident);
  }

  return OK;
}

int EventHandler::DeleteAll(int ident) {
  struct kevent event;

  std::map<int, std::set<int> >::iterator it_registed_event =
      this->registed_event_.find(ident);
  if (it_registed_event == this->registed_event_.end()) return ERROR;

  std::set<int>& registed_filter = it_registed_event->second;
  for (std::set<int>::iterator it_registed_filter = registed_filter.begin();
       it_registed_filter != registed_filter.end(); it_registed_filter++) {
    EV_SET(&event, ident, *it_registed_filter, EV_DELETE, 0, 0, 0);
    kevent(this->kq_, &event, 1, NULL, 0, NULL);
  }

  this->registed_event_.erase(ident);

  return OK;
}

struct kevent* EventHandler::events() const { return this->events_; }

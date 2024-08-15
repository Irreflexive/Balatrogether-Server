#ifndef BALATROGETHER_LISTENER_H
#define BALATROGETHER_LISTENER_H

#include "types.hpp"
#include "util/misc.hpp"
#include "util/logs.hpp"

namespace balatrogether {
  template <typename T>
  class Event {
    public:
      Event(string command);
      string getCommand();
      virtual void execute(T object, client_t client, json req) {};
    private:
      string command;
  };

  template <typename T, typename E>
  class EventListener {
    public:
      EventListener(T object);
      virtual ~EventListener();
      void add(E event);
      bool process(client_t client, json req);
      virtual void client_error(T object, client_t client, json req, client_exception& e) {};
    protected:
      std::vector<E> events;
      T object;
  };

  template <typename T>
  inline Event<T>::Event(string command)
  {
    this->command = command;
  }

  template <typename T>
  inline string Event<T>::getCommand()
  {
    return this->command;
  }

  template <typename T, typename E>
  inline EventListener<T, E>::EventListener(T object)
  {
    this->object = object;
  }

  template <typename T, typename E>
  inline EventListener<T, E>::~EventListener()
  {
    for (E event : this->events) delete event;
  }

  template <typename T, typename E>
  inline void EventListener<T, E>::add(E event)
  {
    this->events.push_back(event);
  }

  template <typename T, typename E>
  inline bool EventListener<T, E>::process(client_t client, json req)
  {
    if (!req["cmd"].is_string()) return false;
    for (E event : this->events) {
      if (event->getCommand() == req["cmd"].get<string>()) {
        try {
          event->execute(this->object, client, req);
          return true;
        } catch (client_exception& e) {
          logger::error << e.what() << std::endl;
          this->client_error(this->object, client, req, e);
          return e.keep();
        } catch (std::exception& e) {
          logger::error << e.what() << std::endl;
          return false;
        }
      }
    }
    return false;
  }
}

#endif
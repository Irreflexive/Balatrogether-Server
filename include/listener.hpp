#ifndef BALATROGETHER_LISTENER_H
#define BALATROGETHER_LISTENER_H

#include "types.hpp"

using namespace balatrogether;

#include "player.hpp"

template <class T>
class balatrogether::Event {
  public:
    Event(string command);
    string getCommand();
    virtual void execute(T object, client_t client, json req) {};
  private:
    string command;
};

template <class T>
class balatrogether::EventListener {
  public:
    EventListener(T object);
    virtual ~EventListener();
    void add(Event<T> *event);
    bool process(client_t client, json req);
    virtual void client_error(T object, client_t client, json req, client_exception& e) {};
  private:
    std::vector<Event<T>*> events;
    T object;
};

template <class T>
inline Event<T>::Event(string command)
{
  this->command = command;
}

template <class T>
inline string Event<T>::getCommand()
{
  return this->command;
}

template <class T>
inline EventListener<T>::EventListener(T object)
{
  this->object = object;
}

template <class T>
inline balatrogether::EventListener<T>::~EventListener()
{
  for (Event<T> *event : this->events) delete event;
}

template <class T>
inline void EventListener<T>::add(Event<T> *event)
{
  this->events.push_back(event);
}

template <class T>
inline bool EventListener<T>::process(client_t client, json req)
{
  if (!req["cmd"].is_string()) return false;
  for (Event<T>* event : this->events) {
    if (event->getCommand() == req["cmd"].get<string>()) {
      try {
        event->execute(this->object, client, req);
        return true;
      } catch (client_exception& e) {
        logger::debug << e.what() << std::endl;
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

#endif
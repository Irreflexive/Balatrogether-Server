#ifndef BALATROGETHER_NETWORK_H
#define BALATROGETHER_NETWORK_H

#include "types.hpp"
#include "util.hpp"

#define BUFFER_SIZE 65536

using namespace balatrogether;

#include "player.hpp"

class balatrogether::NetworkManager {
  public:
    NetworkManager(bool ssl, bool outputKey);
    ~NetworkManager();
    bool handshake(client_t c);
    void send(client_list_t receivers, json payload);
    json receive(client_t sender);
  private:
    size_t peek(client_t client, char* buffer, size_t bytes);
    size_t read(client_t client, char* buffer, size_t bytes);
    size_t write(client_t client, char* buffer, size_t bytes);
    SSL_CTX* ssl_ctx = nullptr;
};

template <class T>
class balatrogether::NetworkEvent {
  public:
    NetworkEvent(string command);
    string getCommand();
    virtual void execute(T object, client_t client, json req) {};
  private:
    string command;
};

template <class T>
class balatrogether::EventListener {
  public:
    EventListener(T object);
    void add(NetworkEvent<T> *event);
    bool process(client_t client, json req);
  private:
    std::vector<NetworkEvent<T>*> events;
    T object;
};

template <class T>
inline NetworkEvent<T>::NetworkEvent(string command)
{
  this->command = command;
}

template <class T>
inline string NetworkEvent<T>::getCommand()
{
  return this->command;
}

template <class T>
inline EventListener<T>::EventListener(T object)
{
  this->object = object;
}

template <class T>
inline void EventListener<T>::add(NetworkEvent<T> *event)
{
  this->events.push_back(event);
}

template <class T>
inline bool EventListener<T>::process(client_t client, json req)
{
  if (!req["cmd"].is_string()) return false;
  for (NetworkEvent<T>* event : this->events) {
    if (event->getCommand() == req["cmd"].get<string>()) {
      try {
        event->execute(this->object, client, req);
        return true;
      } catch (client_exception& e) {
        logger::debug("%s", e.what());
        // TODO: add client error functionality
        return e.keep();
      } catch (std::exception& e) {
        logger::error("%s", e.what());
        return false;
      }
    }
  }
  return false;
}

#endif
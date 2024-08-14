#include "preq.hpp"

typedef std::unordered_map<string, preq_t>::iterator preq_iter_t;

PersistentRequest::PersistentRequest(player_t creator)
{
  this->id = std::to_string(rand());
  this->original = creator;
  this->data = json();
  this->created = clock();
}

string PersistentRequest::getId()
{
  return this->id;
}

player_t PersistentRequest::getCreator()
{
  return this->original;
}

json PersistentRequest::getData()
{
  return this->data;
}

void PersistentRequest::setData(json data)
{
  this->data = data;
}

PersistentRequestManager::PersistentRequestManager(int requestLifetime, int collectionInterval)
{
  this->requestLifetime = requestLifetime;
  this->collectionInterval = collectionInterval;
  std::thread(&PersistentRequestManager::clearUnresolved, this).detach();
  logger::info("Persistent request manager initialized");
}

PersistentRequestManager::~PersistentRequestManager()
{
  for (std::pair<string, preq_t> it : this->requests) {
    delete it.second;
  }
}

preq_t PersistentRequestManager::create(player_t creator)
{
  preq_t preq = new PersistentRequest(creator);
  this->requests.insert(std::make_pair(preq->id, preq));
  return preq;
}

preq_t PersistentRequestManager::getById(string requestId)
{
  preq_iter_t result = this->requests.find(requestId);
  if (result == this->requests.end()) return NULL;
  return result->second;
}

void PersistentRequestManager::complete(string requestId)
{
  preq_iter_t result = this->requests.find(requestId);
  if (result == this->requests.end()) return;
  this->requests.erase(requestId);
  delete result->second;
}

void PersistentRequestManager::clearUnresolved()
{
  while (true) {
    std::this_thread::sleep_for(std::chrono::seconds(this->collectionInterval));
    for (preq_iter_t it = this->requests.begin(); it != this->requests.end(); it++) {
      if ((clock() - it->second->created) / CLOCKS_PER_SEC > this->requestLifetime) {
        this->complete(it->second->getId());
      }
    }
  }
}

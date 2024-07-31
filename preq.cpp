#include "preq.hpp"

typedef std::unordered_map<std::string, PersistentRequest*>::iterator preq_iter_t;

PersistentRequest::PersistentRequest(player_t creator)
{
  this->id = std::to_string(rand());
  this->original = creator;
  this->data = json();
  this->created = clock();
}

std::string PersistentRequest::getId()
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

PersistentRequestManager::~PersistentRequestManager()
{
  for (std::pair<std::string, PersistentRequest*> it : this->requests) {
    delete it.second;
  }
}

PersistentRequest *PersistentRequestManager::create(player_t creator)
{
  PersistentRequest* preq = new PersistentRequest(creator);
  this->requests.insert(std::make_pair(preq->id, preq));
  return preq;
}

PersistentRequest *PersistentRequestManager::getById(std::string requestId)
{
  preq_iter_t result = this->requests.find(requestId);
  if (result == this->requests.end()) return NULL;
  return result->second;
}

void PersistentRequestManager::complete(std::string requestId)
{
  preq_iter_t result = this->requests.find(requestId);
  if (result == this->requests.end()) return;
  this->requests.erase(requestId);
  delete result->second;
}
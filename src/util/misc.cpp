#include <limits.h>
#include "util/misc.hpp"
#include "httplib.h"

using namespace balatrogether;
using cache_value = std::pair<clock_t, string>;

std::map<steamid_t, cache_value> displayNameCache;

// Returns the directory that the program being executed is in
string balatrogether::getpath()
{
  char result[PATH_MAX];
  realpath("/proc/self/exe", result);
  string path = string(result);
  return path.substr(0, path.find_last_of('/'));
}

// Given a player authentication struct, return the display name associated with the Steam ID
string balatrogether::getplayername(player_auth &auth)
{
  if (auth.fetchKey.size() == 0) throw std::runtime_error("No Steam API key");
  std::map<steamid_t, cache_value> newCache;
  string name;
  for (auto p : displayNameCache) {
    if ((clock() - p.second.first) / CLOCKS_PER_SEC < 60) {
      newCache.insert(p);
    } else {
      continue;
    }
    if (p.first == auth.steamId) {
      name = p.second.second;
    }
  }
  displayNameCache = newCache;

  if (name.size() == 0) {
    httplib::Client http("http://api.steampowered.com");
    httplib::Result res = http.Get("/ISteamUser/GetPlayerSummaries/v0002/?key=" + auth.fetchKey + "&steamids=" + auth.steamId);
    if (res && res->status >= 200 && res->status < 300) {
      json body = json::parse(res->body);
      name = body["response"]["players"][0]["personaname"].get<string>();
      displayNameCache.insert(std::make_pair(auth.steamId, cache_value(clock(), name)));
    }
  }
  
  return name;
}

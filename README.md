# Balatrogether Server

This is the server software for the [Balatrogether](https://github.com/Irreflexive/Balatrogether) mod. It requires OpenSSL to run. The server runs over port 7063, and Balatrogether-enabled clients will use this port to connect internally. 

The server is equipped with a console to handle some configuration options and manage the players in the server. Type "help" to view a list of commands.

## Installation

Go to the [releases](https://github.com/Irreflexive/Balatrogether-Server/releases) and download the version corresponding to your operating system. Place the executable in a folder. On launch, Balatrogether will create a default configuration file in the same folder. You may edit and save these settings while the server is *not running*, then start it again for them to take effect.

```js
{
  // A list of Steam IDs that will be unable to connect to the server in any capacity
  "banned_users": [],
  // The number of lobbies that the server supports
  "max_lobbies": 1,
  // The maximum number of players allowed in each lobby
  "max_players": 8,
  // An optional Steam Web API key, used to determine display names from numeric Steam IDs. If set to null, players will only be able to view the names of those on their friends list
  "steam_api_key": "api_key or null",
  // Determines whether encryption is enabled. For now, this must be set to true, or else players will be unable to connect to the server
  "tls_enabled": true,
  // Enables or disables the whitelist
  "whitelist_enabled": false,
  // If enabled, only those on the whitelist can join the server
  "whitelist": []
}
```
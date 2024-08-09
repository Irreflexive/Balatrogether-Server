#ifndef BALATROGETHER_SERVER_EVENTS_H
#define BALATROGETHER_SERVER_EVENTS_H

#include "../network.hpp"
#include "../server.hpp"

// Triggered when a player wants to join the server, exchanging their steam ID and unlock hash
class JoinEvent : public NetworkEvent<server_t> {
  public:
    JoinEvent() : NetworkEvent("JOIN") {};
    virtual void execute(server_t server, client_t client, json req);
};

// Triggered when the lobby host wants to start a run
class StartRunEvent : public NetworkEvent<server_t> {
  public:
    StartRunEvent() : NetworkEvent("START") {};
    virtual void execute(server_t server, client_t client, json req);
};

// Triggered when a co-op player is highlighting a card
class HighlightCardEvent : public NetworkEvent<server_t> {
  public:
    HighlightCardEvent() : NetworkEvent("HIGHLIGHT") {};
    virtual void execute(server_t server, client_t client, json req);
};

// Triggered when a co-op player is unhighlighting a card
class UnhighlightCardEvent : public NetworkEvent<server_t> {
  public:
    UnhighlightCardEvent() : NetworkEvent("UNHIGHLIGHT") {};
    virtual void execute(server_t server, client_t client, json req);
};

// Triggered when a co-op player unhighlights the entire hand
class UnhighlightAllEvent : public NetworkEvent<server_t> {
  public:
    UnhighlightAllEvent() : NetworkEvent("UNHIGHLIGHT_ALL") {};
    virtual void execute(server_t server, client_t client, json req);
};

// Triggered when a co-op player plays selected cards
class PlayHandEvent : public NetworkEvent<server_t> {
  public:
    PlayHandEvent() : NetworkEvent("PLAY_HAND") {};
    virtual void execute(server_t server, client_t client, json req);
};

// Triggered when a co-op player discards a hand
class DiscardHandEvent : public NetworkEvent<server_t> {
  public:
    DiscardHandEvent() : NetworkEvent("DISCARD_HAND") {};
    virtual void execute(server_t server, client_t client, json req);
};

// Triggered when a co-op player clicks the sort by suit/rank buttons
class SortHandEvent : public NetworkEvent<server_t> {
  public:
    SortHandEvent() : NetworkEvent("SORT_HAND") {};
    virtual void execute(server_t server, client_t client, json req);
};

// Triggered when a co-op player moves a card from one position to another
class ReorderCardsEvent : public NetworkEvent<server_t> {
  public:
    ReorderCardsEvent() : NetworkEvent("REORDER") {};
    virtual void execute(server_t server, client_t client, json req);
};

// Triggered when a co-op player selects a blind
class SelectBlindEvent : public NetworkEvent<server_t> {
  public:
    SelectBlindEvent() : NetworkEvent("SELECT_BLIND") {};
    virtual void execute(server_t server, client_t client, json req);
};

// Triggered when a co-op player skips a blind
class SkipBlindEvent : public NetworkEvent<server_t> {
  public:
    SkipBlindEvent() : NetworkEvent("SKIP_BLIND") {};
    virtual void execute(server_t server, client_t client, json req);
};

// Triggered when a co-op player sells a card
class SellCardEvent : public NetworkEvent<server_t> {
  public:
    SellCardEvent() : NetworkEvent("SELL") {};
    virtual void execute(server_t server, client_t client, json req);
};

// Triggered when a co-op player buys a card
class BuyCardEvent : public NetworkEvent<server_t> {
  public:
    BuyCardEvent() : NetworkEvent("BUY") {};
    virtual void execute(server_t server, client_t client, json req);
};

// Triggered when a co-op player uses a card
class UseCardEvent : public NetworkEvent<server_t> {
  public:
    UseCardEvent() : NetworkEvent("USE") {};
    virtual void execute(server_t server, client_t client, json req);
};

// Triggered when a co-op player clicks the buy and use button on a card
class BuyAndUseCardEvent : public NetworkEvent<server_t> {
  public:
    BuyAndUseCardEvent() : NetworkEvent("BUY_AND_USE") {};
    virtual void execute(server_t server, client_t client, json req);
};

// Triggered when a co-op player skips a booster pack
class SkipBoosterEvent : public NetworkEvent<server_t> {
  public:
    SkipBoosterEvent() : NetworkEvent("SKIP_BOOSTER") {};
    virtual void execute(server_t server, client_t client, json req);
};

// Triggered when a co-op player rerolls in the shop
class RerollEvent : public NetworkEvent<server_t> {
  public:
    RerollEvent() : NetworkEvent("REROLL") {};
    virtual void execute(server_t server, client_t client, json req);
};

// Triggered when a co-op player exits the shop
class NextRoundEvent : public NetworkEvent<server_t> {
  public:
    NextRoundEvent() : NetworkEvent("NEXT_ROUND") {};
    virtual void execute(server_t server, client_t client, json req);
};

// Triggered when a co-op player ends the round and goes to the shop
class GoToShopEvent : public NetworkEvent<server_t> {
  public:
    GoToShopEvent() : NetworkEvent("GO_TO_SHOP") {};
    virtual void execute(server_t server, client_t client, json req);
};

// Triggered when a co-op player clicks the endless mode button
class EndlessEvent : public NetworkEvent<server_t> {
  public:
    EndlessEvent() : NetworkEvent("ENDLESS") {};
    virtual void execute(server_t server, client_t client, json req);
};

#endif
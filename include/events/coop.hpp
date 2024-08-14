#ifndef BALATROGETHER_COOP_EVENTS_H
#define BALATROGETHER_COOP_EVENTS_H

#include "../network.hpp"
#include "../lobby.hpp"
#include "../listener.hpp"

namespace balatrogether {
  // Triggered when a co-op player is highlighting a card
  class HighlightCardEvent : public Event<lobby_t> {
    public:
      HighlightCardEvent() : Event("HIGHLIGHT") {};
      virtual void execute(lobby_t lobby, client_t client, json req);
  };

  // Triggered when a co-op player is unhighlighting a card
  class UnhighlightCardEvent : public Event<lobby_t> {
    public:
      UnhighlightCardEvent() : Event("UNHIGHLIGHT") {};
      virtual void execute(lobby_t lobby, client_t client, json req);
  };

  // Triggered when a co-op player unhighlights the entire hand
  class UnhighlightAllEvent : public Event<lobby_t> {
    public:
      UnhighlightAllEvent() : Event("UNHIGHLIGHT_ALL") {};
      virtual void execute(lobby_t lobby, client_t client, json req);
  };

  // Triggered when a co-op player plays selected cards
  class PlayHandEvent : public Event<lobby_t> {
    public:
      PlayHandEvent() : Event("PLAY_HAND") {};
      virtual void execute(lobby_t lobby, client_t client, json req);
  };

  // Triggered when a co-op player discards a hand
  class DiscardHandEvent : public Event<lobby_t> {
    public:
      DiscardHandEvent() : Event("DISCARD_HAND") {};
      virtual void execute(lobby_t lobby, client_t client, json req);
  };

  // Triggered when a co-op player clicks the sort by suit/rank buttons
  class SortHandEvent : public Event<lobby_t> {
    public:
      SortHandEvent() : Event("SORT_HAND") {};
      virtual void execute(lobby_t lobby, client_t client, json req);
  };

  // Triggered when a co-op player moves a card from one position to another
  class ReorderCardsEvent : public Event<lobby_t> {
    public:
      ReorderCardsEvent() : Event("REORDER") {};
      virtual void execute(lobby_t lobby, client_t client, json req);
  };

  // Triggered when a co-op player selects a blind
  class SelectBlindEvent : public Event<lobby_t> {
    public:
      SelectBlindEvent() : Event("SELECT_BLIND") {};
      virtual void execute(lobby_t lobby, client_t client, json req);
  };

  // Triggered when a co-op player skips a blind
  class SkipBlindEvent : public Event<lobby_t> {
    public:
      SkipBlindEvent() : Event("SKIP_BLIND") {};
      virtual void execute(lobby_t lobby, client_t client, json req);
  };

  // Triggered when a co-op player sells a card
  class SellCardEvent : public Event<lobby_t> {
    public:
      SellCardEvent() : Event("SELL") {};
      virtual void execute(lobby_t lobby, client_t client, json req);
  };

  // Triggered when a co-op player buys a card
  class BuyCardEvent : public Event<lobby_t> {
    public:
      BuyCardEvent() : Event("BUY") {};
      virtual void execute(lobby_t lobby, client_t client, json req);
  };

  // Triggered when a co-op player uses a card
  class UseCardEvent : public Event<lobby_t> {
    public:
      UseCardEvent() : Event("USE") {};
      virtual void execute(lobby_t lobby, client_t client, json req);
  };

  // Triggered when a co-op player clicks the buy and use button on a card
  class BuyAndUseCardEvent : public Event<lobby_t> {
    public:
      BuyAndUseCardEvent() : Event("BUY_AND_USE") {};
      virtual void execute(lobby_t lobby, client_t client, json req);
  };

  // Triggered when a co-op player skips a booster pack
  class SkipBoosterEvent : public Event<lobby_t> {
    public:
      SkipBoosterEvent() : Event("SKIP_BOOSTER") {};
      virtual void execute(lobby_t lobby, client_t client, json req);
  };

  // Triggered when a co-op player rerolls in the shop
  class RerollEvent : public Event<lobby_t> {
    public:
      RerollEvent() : Event("REROLL") {};
      virtual void execute(lobby_t lobby, client_t client, json req);
  };

  // Triggered when a co-op player exits the shop
  class NextRoundEvent : public Event<lobby_t> {
    public:
      NextRoundEvent() : Event("NEXT_ROUND") {};
      virtual void execute(lobby_t lobby, client_t client, json req);
  };

  // Triggered when a co-op player ends the round and goes to the shop
  class GoToShopEvent : public Event<lobby_t> {
    public:
      GoToShopEvent() : Event("GO_TO_SHOP") {};
      virtual void execute(lobby_t lobby, client_t client, json req);
  };

  // Triggered when a co-op player clicks the endless mode button
  class EndlessEvent : public Event<lobby_t> {
    public:
      EndlessEvent() : Event("ENDLESS") {};
      virtual void execute(lobby_t lobby, client_t client, json req);
  };
}

#endif
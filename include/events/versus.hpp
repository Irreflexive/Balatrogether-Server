#ifndef BALATROGETHER_VERSUS_EVENTS_H
#define BALATROGETHER_VERSUS_EVENTS_H

#include "../network.hpp"
#include "../lobby.hpp"
#include "../listener.hpp"

namespace balatrogether {
  // Triggered when a player sells Annie and Hallie
  class SwapJokersEvent : public Event<lobby_t> {
    public:
      SwapJokersEvent() : Event("SWAP_JOKERS") {};
      virtual void execute(lobby_t lobby, client_t client, json req);
  };

  // Triggered when a green seal triggers
  class GreenSealEvent : public Event<lobby_t> {
    public:
      GreenSealEvent() : Event("GREEN_SEAL") {};
      virtual void execute(lobby_t lobby, client_t client, json req);
  };

  // Triggered when the Eraser voucher is redeemed
  class EraserEvent : public Event<lobby_t> {
    public:
      EraserEvent() : Event("ERASER") {};
      virtual void execute(lobby_t lobby, client_t client, json req);
  };

  // Triggered when the Paint Bucket voucher is redeemed
  class PaintBucketEvent : public Event<lobby_t> {
    public:
      PaintBucketEvent() : Event("PAINT_BUCKET") {};
      virtual void execute(lobby_t lobby, client_t client, json req);
  };

  // Triggered to retrieve opponents' cards and jokers for network packs
  class GetCardsAndJokersEvent : public Event<lobby_t> {
    public:
      GetCardsAndJokersEvent() : Event("GET_CARDS_AND_JOKERS") {};
      virtual void execute(lobby_t lobby, client_t client, json req);
  };

  // Triggered when a player is ready for a versus boss blind
  class ReadyForBossEvent : public Event<lobby_t> {
    public:
      ReadyForBossEvent() : Event("READY_FOR_BOSS") {};
      virtual void execute(lobby_t lobby, client_t client, json req);
  };

  // Triggered when a player is eliminated
  class EliminatedEvent : public Event<lobby_t> {
    public:
      EliminatedEvent() : Event("ELIMINATED") {};
      virtual void execute(lobby_t lobby, client_t client, json req);
  };

  // Triggered when a player is eliminated
  class DefeatedBossEvent : public Event<lobby_t> {
    public:
      DefeatedBossEvent() : Event("DEFEATED_BOSS") {};
      virtual void execute(lobby_t lobby, client_t client, json req);
  };
}

#endif
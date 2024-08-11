#ifndef BALATROGETHER_VERSUS_EVENTS_H
#define BALATROGETHER_VERSUS_EVENTS_H

#include "../network.hpp"
#include "../lobby.hpp"

using namespace balatrogether;

// Triggered when a player sells Annie and Hallie
class SwapJokersEvent : public NetworkEvent<lobby_t> {
  public:
    SwapJokersEvent() : NetworkEvent("SWAP_JOKERS") {};
    virtual void execute(lobby_t lobby, client_t client, json req);
};

// Triggered when a green seal triggers
class GreenSealEvent : public NetworkEvent<lobby_t> {
  public:
    GreenSealEvent() : NetworkEvent("GREEN_SEAL") {};
    virtual void execute(lobby_t lobby, client_t client, json req);
};

// Triggered when the Eraser voucher is redeemed
class EraserEvent : public NetworkEvent<lobby_t> {
  public:
    EraserEvent() : NetworkEvent("ERASER") {};
    virtual void execute(lobby_t lobby, client_t client, json req);
};

// Triggered when the Paint Bucket voucher is redeemed
class PaintBucketEvent : public NetworkEvent<lobby_t> {
  public:
    PaintBucketEvent() : NetworkEvent("PAINT_BUCKET") {};
    virtual void execute(lobby_t lobby, client_t client, json req);
};

// Triggered to retrieve opponents' cards and jokers for network packs
class GetCardsAndJokersEvent : public NetworkEvent<lobby_t> {
  public:
    GetCardsAndJokersEvent() : NetworkEvent("GET_CARDS_AND_JOKERS") {};
    virtual void execute(lobby_t lobby, client_t client, json req);
};

// Triggered when a player is ready for a versus boss blind
class ReadyForBossEvent : public NetworkEvent<lobby_t> {
  public:
    ReadyForBossEvent() : NetworkEvent("READY_FOR_BOSS") {};
    virtual void execute(lobby_t lobby, client_t client, json req);
};

// Triggered when a player is eliminated
class EliminatedEvent : public NetworkEvent<lobby_t> {
  public:
    EliminatedEvent() : NetworkEvent("ELIMINATED") {};
    virtual void execute(lobby_t lobby, client_t client, json req);
};

// Triggered when a player is eliminated
class DefeatedBossEvent : public NetworkEvent<lobby_t> {
  public:
    DefeatedBossEvent() : NetworkEvent("DEFEATED_BOSS") {};
    virtual void execute(lobby_t lobby, client_t client, json req);
};

#endif
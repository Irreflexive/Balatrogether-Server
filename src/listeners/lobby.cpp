#include "listeners/lobby.hpp"
#include "util/response.hpp"
#include "events/setup.hpp"
#include "events/coop.hpp"
#include "events/versus.hpp"

LobbyEventListener::LobbyEventListener(lobby_t lobby) : EventListener(lobby)
{
  this->add(new StartRunEvent);

  this->add(new HighlightCardEvent);
  this->add(new UnhighlightCardEvent);
  this->add(new UnhighlightAllEvent);
  this->add(new PlayHandEvent);
  this->add(new DiscardHandEvent);
  this->add(new SortHandEvent);
  this->add(new ReorderCardsEvent);
  this->add(new SelectBlindEvent);
  this->add(new SkipBlindEvent);
  this->add(new SellCardEvent);
  this->add(new BuyCardEvent);
  this->add(new UseCardEvent);
  this->add(new BuyAndUseCardEvent);
  this->add(new SkipBoosterEvent);
  this->add(new RerollEvent);
  this->add(new NextRoundEvent);
  this->add(new GoToShopEvent);
  this->add(new EndlessEvent);

  this->add(new SwapJokersEvent);
  this->add(new GreenSealEvent);
  this->add(new EraserEvent);
  this->add(new PaintBucketEvent);
  this->add(new GetCardsAndJokersEvent);
  this->add(new ReadyForBossEvent);
  this->add(new EliminatedEvent);
  this->add(new DefeatedBossEvent);

  lobby->getLogger() << "Listening for lobby events" << std::endl;
}

void LobbyEventListener::client_error(lobby_t lobby, client_t client, json req, client_exception &e)
{
  lobby->getServer()->getNetworkManager()->send({client}, response::error(e.what()));
}
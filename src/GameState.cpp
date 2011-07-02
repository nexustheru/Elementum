#include "GameState.h"
#include "GameManager.h"
#include "EventManager.h"
#include "InputManager.h"
#if OGRE_PLATFORM == OGRE_PLATFORM_APPLE
#include "CEGUIBase/CEGUI.h"
#else
#include "CEGUI/CEGUI.h"
#endif

namespace Pixy
{

  GAME_STATE GameState::getId() const {
    return mId;
  }

	void GameState::changeState( GameState *state ) {
		GameManager::getSingletonPtr()->changeState( state );
	}

	void GameState::pushState( GameState *state ) {
		GameManager::getSingletonPtr()->pushState( state );
	}

	void GameState::popState( void ) {
		GameManager::getSingletonPtr()->popState();
	}

	void GameState::requestShutdown( void ) {

		// notify server we're shutting down
		/*Event* lEvt = EventManager::getSingleton().createEvt("Logout");
		EventManager::getSingleton().hook(lEvt);
		lEvt = NULL;*/

		GameManager::getSingletonPtr()->requestShutdown();
	}

} // end of namespace Pixy

/*
 * Copyright (C) Shroom Studios, Inc - All Rights Reserved
 *
 * Unauthorized copying of this file, via any medium is strictly prohibited
 * Proprietary and confidential
 *
 * Written by Ahmad Amireh <ahmad@shroom-studios.com>, September 2011
 */

#include "Intro.h"
#include "GameManager.h"
#include "EventManager.h"
#include "NetworkManager.h"
#include "UIEngine.h"
#include "GfxEngine.h"
#include "ScriptEngine.h"
#include "ResourceParser.h"
#include "EntityManager.h"
#include "Puppet.h"
#include "Deck.h"
#include "Spell.h"
//#include "Lobby.h"
#include "FxEngine.h"
#include "Combat.h"

using namespace Ogre;
namespace Pixy
{

	Intro* Intro::mIntro = 0;

	Intro::Intro()
  : //mIOService(),
    mStrand(GameManager::getSingleton().getIOService()),
    //~ mWork(GameManager::getSingleton().getIOService()),
    //~ mWorker(0),
    fSetup(false) {
    mLog = 0;
    mPuppet = 0;
    mPuppetName = "";
    mId = GameState::State::Intro;
	}

	Intro::~Intro() {
    for (Intro::puppets_t::iterator puppet = mPuppets.begin();
      puppet != mPuppets.end();
      ++puppet)
    {
      delete *puppet;
    }
    mPuppets.clear();
    if (mPuppet)
      delete mPuppet;
    mPuppet = 0;

    if (mLog) {
		  delete mLog;
		  mLog = 0;
		}
	}

	void Intro::enter( void ) {

		if (!mLog)
		  mLog = new log4cpp::FixedContextCategory(PIXY_LOG_CATEGORY, "Intro");

    //~ mWorker = new boost::thread(boost::bind(&boost::asio::io_service::run, &GameManager::getSingleton().getIOService()));

		mLog->infoStream() << "---- Entering ----";

		mEvtMgr = EventManager::getSingletonPtr();
    mEvtMgr->clear();
		mNetMgr = NetworkManager::getSingletonPtr();

		// init engines
		mGfxEngine = GfxEngine::getSingletonPtr();
    mGfxEngine->setupIntro();

    mFxEngine = FxEngine::getSingletonPtr();
    mFxEngine->setup();

		mUIEngine = UIEngine::getSingletonPtr();
		mUIEngine->setup();

		mScriptEngine = ScriptEngine::getSingletonPtr();
		mScriptEngine->setup(mId);

    //~ bind(EventUID::JoinLobby, boost::bind(&Intro::onJoinLobby, this, _1));
    bind(EventUID::SyncPuppets, boost::bind(&Intro::onSyncPuppets, this, _1));
    bind(EventUID::SyncPuppetData, boost::bind(&Intro::onSyncPuppetData, this, _1));
    bind(EventUID::MatchFound, boost::bind(&Intro::onMatchFound, this, _1));

		// start the interface chain
		mScriptEngine->runScript("intro/entry_point.lua");
    mScriptEngine->passToLua("Intro.onEnter", 0);

		/*
		mLog->infoStream() << "creating puppet and a deck";
		Puppet* lPuppet = new Puppet();
		Deck* lDeck = new Deck(*lPuppet);
		lDeck->generate();
		*/
		/*
		for (int i=0; i<50; ++i) {

			UIEvent_Register* _evt = mEvtMgr->createEvt<UIEvent_Register>();
			_evt->setType(EVT_REQ);
			_evt->setUsername("vxvxvx");
			_evt->setPassword("fubarxxzz");

			mEvtMgr->hook(_evt);
		}
		*/

		//~ if (!mNetMgr->connect()) {
      //~ mLog->errorStream() << "Could not connect to server, aborting";
      //~ return GameManager::getSingleton().requestShutdown();
		//~ }

		//mLog->debugStream() << "evt id " << _evt->getId();

		//mLog->infoStream() << "Initialized successfully.";
    fSetup = true;
	}


	void Intro::exit( void ) {

    if (fSetup)
      mScriptEngine->passToLua("Intro.onExit", 0);

		//mNetMgr->disconnect();

		/*delete mScriptEngine;
		delete mUIEngine;
		delete mGfxEngine;
		delete mEvtMgr;*/
    //~ mScriptEngine->cleanup();
    //~ mUIEngine->cleanup();



		mLog->infoStream() << "---- Exited ----";

		delete mLog;
		mLog = 0;

    //~ mPuppetName = mPuppet->getName();



    unbindAll();

    //~ mIOService.stop();
    //~ mWorker->join();
    //~ delete mWorker;

	}

	void Intro::update( unsigned long lTimeElapsed ) {

		mEvtMgr->update();

		processEvents();

		mGfxEngine->update(lTimeElapsed);
		mUIEngine->update(lTimeElapsed);
		mScriptEngine->update(lTimeElapsed);
    mFxEngine->update(lTimeElapsed);

	}

	void Intro::keyPressed( const OIS::KeyEvent &e )
	{
    mUIEngine->keyPressed(e);
    mScriptEngine->keyPressed(e);
	}

	void Intro::keyReleased( const OIS::KeyEvent &e ) {
    mUIEngine->keyReleased(e);
    mScriptEngine->keyReleased(e);
		/*switch (e.key) {
			case OIS::KC_ESCAPE:
				this->requestShutdown();
				break;
			case OIS::KC_SPACE:
				//fireLoginEvt();
				GameManager::getSingleton().changeState(Combat::getSingletonPtr());
				//~ GameManager::getSingleton().changeState(Lobby::getSingletonPtr());
				break;
		}*/

	}

	bool Intro::mouseMoved( const OIS::MouseEvent &e )
	{
		// Update CEGUI with the mouse motion
    mUIEngine->mouseMoved(e);
    mGfxEngine->mouseMoved(e);
    return true;
	}

	bool Intro::mousePressed( const OIS::MouseEvent &e, OIS::MouseButtonID id ) {
    mUIEngine->mousePressed(e, id);
    mGfxEngine->mousePressed(e, id);
    return true;
	}

	bool Intro::mouseReleased( const OIS::MouseEvent &e, OIS::MouseButtonID id ) {
    mUIEngine->mouseReleased(e, id);
    mGfxEngine->mouseReleased(e, id);
    return true;
	}

	void Intro::pause( void ) {
	}

	void Intro::resume( void ) {
	}

	Intro* Intro::getSingletonPtr( void ) {
		if( !mIntro )
		    mIntro = new Intro();

		return mIntro;
	}

	Intro& Intro::getSingleton( void ) {
		return *getSingletonPtr();
	}

  bool Intro::onSyncPuppets(const Event& e)
  {
    if (e.Feedback != EventFeedback::Ok) {
      mScriptEngine->_passPuppetListing();
      return true;
    }

    mLog->debugStream() << "Syncing puppet listing";

    for (Intro::puppets_t::iterator puppet = mPuppets.begin();
      puppet != mPuppets.end();
      ++puppet)
    {
      delete *puppet;
    }
    mPuppets.clear();

    typedef std::list<BasePuppet*> l_puppets_t;

    std::istringstream in(e.getProperty("Data"));
    ResourceParser *lParser = new ResourceParser(&GameManager::getSingleton().getEntityMgr(), in);
    l_puppets_t lPuppets = lParser->syncGamePuppetsFromDump();
    delete lParser;
    in.clear();

    // we have to cast the objects in order to populate our own container because
    // of return type covariance incompatibility with STL containers
    for (l_puppets_t::iterator lPuppet = lPuppets.begin(); lPuppet != lPuppets.end(); ++lPuppet)
    {
      mPuppets.push_back(static_cast<Puppet*>(*lPuppet));
    }
    lPuppets.clear();

    mScriptEngine->_passPuppetListing();
    return true;
  }

  bool Intro::onSyncPuppetData(const Event& e)
  {
    if (e.Feedback != EventFeedback::Ok)
      return true;

    if (mPuppet)
      delete mPuppet;

    std::istringstream in(e.getProperty("Data"));
    ResourceParser *lParser = new ResourceParser(&GameManager::getSingleton().getEntityMgr(), in);
    mPuppet = static_cast<Puppet*>(lParser->syncGamePuppetsFromDump().front());
    delete lParser;
    in.clear();

    mScriptEngine->_passPuppet();
    return true;
  }



  bool Intro::onJoinLobby(const Event& e)
  {
    // to do: validation
    mPuppetName = e.getProperty("Puppet");

    //GameManager::getSingleton().changeState(Lobby::getSingletonPtr());
    return true;
  }

  bool Intro::onMatchFound(const Event& e)
  {
    GameManager::getSingleton().changeState(Combat::getSingletonPtr());

    return true;
  }

  std::string const& Intro::getPuppetName() const {
    return mPuppetName;
  }

  void Intro::setPuppetName(std::string inName)
  {
    mPuppetName = inName;
  }

  Intro::puppets_t const& Intro::getPuppets()
  {
    return mPuppets;
  }
  Puppet* Intro::getPuppet()
  {
    return mPuppet;
  }

} // end of namespace

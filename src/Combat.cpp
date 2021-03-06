/*
 * Copyright (C) Shroom Studios, Inc - All Rights Reserved
 *
 * Unauthorized copying of this file, via any medium is strictly prohibited
 * Proprietary and confidential
 *
 * Written by Ahmad Amireh <ahmad@shroom-studios.com>, September 2011
 */

#include "Combat.h"
#include "GameManager.h"
#include "Intro.h"
#include "Puppet.h"
#include "Unit.h"
#include "Spell.h"
#include "EventManager.h"
#include "NetworkManager.h"
#include "UIEngine.h"
#include "GfxEngine.h"
#include "FxEngine.h"
#include "ScriptEngine.h"
#include "EntityManager.h"
#include "Renderable.h"
#include "ResourceParser.h"

namespace Pixy
{

  Combat::Combat()
  : //mIOService(GameManager::getSingleton().getIOService()),
    mStrand(GameManager::getSingleton().getIOService()),
    //~ mWork(GameManager::getSingleton().getIOService()),
    //~ mWorker(0),
    fSetup(false),
    fIsDebugging(false) {

  }
	Combat* Combat::mCombat = 0;

	Combat* Combat::getSingletonPtr( void ) {
		if( !mCombat )
		    mCombat = new Combat();

		return mCombat;
	}

	Combat& Combat::getSingleton( void ) {
		return *getSingletonPtr();
	}

	/* +-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+ *
	 *	Bootstrap
	 * +-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+ */

	void Combat::enter( void ) {

    //~ mWorker = new boost::thread(boost::bind(&boost::asio::io_service::run, &mIOService));

    mId = GameState::State::Combat;

		mLog = new log4cpp::FixedContextCategory(PIXY_LOG_CATEGORY, "Combat");

		mLog->infoStream() << "---- Entering ----";
    mLog->infoStream() << "My Listener UID: " << mUID;

		mPuppets.clear();
    mDeathlist.clear();
    mChargers.clear();
    mAttackers.clear();
    mBlockers.clear();

		mEvtMgr = EventManager::getSingletonPtr();
    mEvtMgr->clear();

		mNetMgr = NetworkManager::getSingletonPtr();
		if (!mNetMgr->connect()) {
      mLog->errorStream() << "Could not connect to server, aborting";
      return GameManager::getSingleton().requestShutdown();
		}

    mEntityMgr = &GameManager::getSingleton().getEntityMgr();

		mGfxEngine = GfxEngine::getSingletonPtr();
		mGfxEngine->setup();

    mFxEngine = FxEngine::getSingletonPtr();
    mFxEngine->setup();

		mUIEngine = UIEngine::getSingletonPtr();
		mUIEngine->setup();

		mScriptEngine = ScriptEngine::getSingletonPtr();
		mScriptEngine->setup(mId);

		// start the interface chain
		mScriptEngine->runScript("combat/entry_point.lua");
    if (!mScriptEngine->passToLua("Combat.onEnter", 0))
    {
      mLog->errorStream() << "Lua state entrance routine has failed, bailing out";
      return;
    }

		mLog->infoStream() << "i'm up!";
    mPuppet = 0;

    if (__isDebugging())
    {
      mLog->infoStream() << "running in debug mode";
      mPuppetName = "Cranberry"; // __DEBUG__
      bind(EventUID::GameDataSynced, boost::bind(&Combat::onGameDataSynced, this, _1)); // __DEBUG__
      bind(EventUID::Login, boost::bind(&Combat::onLogin, this, _1)); // __DEBUG__
      bind(EventUID::SyncPuppets, boost::bind(&Combat::onSyncPuppets, this, _1)); // __DEBUG__
      bind(EventUID::JoinLobby, boost::bind(&Combat::onJoinLobby, this, _1)); // __DEBUG__
    }

    bind(EventUID::ChangingState, boost::bind(&Combat::onChangingState, this, _1));
    bind(EventUID::MatchFound, boost::bind(&Combat::onMatchFound, this, _1));
    bind(EventUID::SyncMatchPuppets, boost::bind(&Combat::onSyncMatchPuppets, this, _1));
    bind(EventUID::StartTurn, boost::bind(&Combat::onStartTurn, this, _1));
    bind(EventUID::TurnStarted, boost::bind(&Combat::onTurnStarted, this, _1));
    bind(EventUID::DrawSpells, boost::bind(&Combat::onDrawSpells, this, _1));
    bind(EventUID::CastSpell, boost::bind(&Combat::onCastSpell, this, _1));
    bind(EventUID::CreateUnit, boost::bind(&Combat::onCreateUnit, this, _1));
    bind(EventUID::UpdatePuppet, boost::bind(&Combat::onUpdatePuppet, this, _1));
    bind(EventUID::UpdateUnit, boost::bind(&Combat::onUpdateUnit, this, _1));
    bind(EventUID::RemoveUnit, boost::bind(&Combat::onRemoveUnit, this, _1));
    //bind(EventUID::EntityDied, boost::bind(&Combat::onEntityDied, this, _1));
    bind(EventUID::StartBlockPhase, boost::bind(&Combat::onStartBlockPhase, this, _1));
    bind(EventUID::Charge, boost::bind(&Combat::onCharge, this, _1));
    bind(EventUID::CancelCharge, boost::bind(&Combat::onCancelCharge, this, _1));
    bind(EventUID::Block, boost::bind(&Combat::onBlock, this, _1));
    bind(EventUID::CancelBlock, boost::bind(&Combat::onCancelBlock, this, _1));
    bind(EventUID::EndBlockPhase, boost::bind(&Combat::onEndBlockPhase, this, _1));


    if (!__isDebugging())
    {
      mPuppetName = Intro::getSingleton().getPuppetName();
      Event e(EventUID::SyncMatchPuppets);
      mNetMgr->send(e);
    }

    inBlockPhase = false;
    fSetup = true;
	}


	void Combat::exit( void ) {
    mLog->infoStream() << "Exiting";

    if (fSetup)
      mScriptEngine->passToLua("Combat.onExit", 0);

    mLog->debugStream() << "\tdestroying " << mPuppets.size() << " puppets";
		while (!mPuppets.empty())
    {
      delete mPuppets.back();
      mPuppets.pop_back();
    }
		mPuppets.clear();
		mDeathlist.clear();
    mChargers.clear();
    mAttackers.clear();
    mBlockers.clear();

    mPuppet = 0;
    mActivePuppet = 0;
    mWaitingPuppet = 0;

    inBlockPhase = false;

		/*delete mScriptEngine;
		delete mUIEngine;
		delete mGfxEngine;
		delete mEvtMgr;*/

		mLog->infoStream() << "---- Exited ----";

		delete mLog;
		mLog = 0;

    unbindAll();

    //~ mIOService.stop();
    //~ mWorker->join();
    //~ delete mWorker;
	}

	/* +-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+ *
	 *	Misc
	 * +-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+ */

  void Combat::__setIsDebugging(bool setting)
  {
    fIsDebugging = setting;
  }

  bool Combat::__isDebugging() const
  {
    return fIsDebugging;
  }

	void Combat::keyPressed( const OIS::KeyEvent &e )	{
		mUIEngine->keyPressed(e);
		mGfxEngine->keyPressed(e);
		mScriptEngine->keyPressed(e);
	}

	void Combat::keyReleased( const OIS::KeyEvent &e ) {
	  mUIEngine->keyReleased(e);
		mGfxEngine->keyReleased(e);
		mScriptEngine->keyReleased(e);
	}

	bool Combat::mouseMoved( const OIS::MouseEvent &e )	{
		mUIEngine->mouseMoved(e);
		mGfxEngine->mouseMoved(e);

    return true;
	}

	bool Combat::mousePressed( const OIS::MouseEvent &e, OIS::MouseButtonID id ) {
    if (!mPuppet) // game hasn't started
      return false;

		//~ if (mUIEngine->mousePressed(e, id))
      //~ return true;
    mUIEngine->mousePressed(e, id);
		mGfxEngine->mousePressed(e, id);

    return true;
	}

	bool Combat::mouseReleased( const OIS::MouseEvent &e, OIS::MouseButtonID id ) {
    if (!mPuppet) // game hasn't started
      return false;

		//~ if (mUIEngine->mouseReleased(e, id))
      //~ return true;

    mUIEngine->mouseReleased(e, id);
		mGfxEngine->mouseReleased(e, id);

    return true;
	}

	void Combat::pause( void ) {
	}

	void Combat::resume( void ) {
	}

  boost::asio::strand& Combat::getStrand() {
    return mStrand;
  }

	/* +-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+ *
	 *	Main Routines
	 * +-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+ */

	void Combat::update( unsigned long lTimeElapsed ) {

		mEvtMgr->update();

    processEvents();

		mGfxEngine->update(lTimeElapsed);
    mFxEngine->update(lTimeElapsed);
		mScriptEngine->update(lTimeElapsed);
		mUIEngine->update(lTimeElapsed);

    if (!mDeathlist.empty()) {
      deathlist_t::iterator unit;
      for (unit = mDeathlist.begin();
           unit != mDeathlist.end();
           ++unit)
      {
        bool found = false;
        // verify the mark in the remote deathlist
        for (rdeathlist_t::iterator uid = mRDeathlist.begin();
          uid != mRDeathlist.end();
          ++uid)
        {
          if ((*uid) == (*unit)->getUID())
          {
            found = true;
            break;
          }
        }
        assert(found);
        mLog->debugStream() << "destroying unit " << (*unit) << " permanently";
        // remove the unit from the attackers list if it was charging
        for (attackers_t::iterator _attacker = mAttackers.begin();
          _attacker != mAttackers.end();
          ++_attacker)
        {
          if ((*_attacker)->getUID() == (*unit)->getUID())
          {
            mAttackers.erase(_attacker);
            break;
          }
        }
        static_cast<Puppet*>((Entity*)(*unit)->getOwner())->detachUnit((*unit)->getUID());
      }

      mDeathlist.clear();
    }
	}

	void Combat::registerPuppet(Puppet* inPuppet) {
    mLog->infoStream() << "a new Puppet has joined the battle: " << inPuppet;

		mPuppets.push_back(inPuppet);
    if (!mScriptEngine->passToLua("Puppets.onAddPuppet", 1, "Pixy::Puppet", (void*)inPuppet))
    {
      mLog->errorStream() << "unable to register puppet in Lua, aborting";
      throw lua_runtime_error("unable to register puppet in Lua, aborting");
    }

    if (inPuppet->getName() == mPuppetName)
      assignPuppet(inPuppet);
    else
      mScriptEngine->passToLua("Puppets.onAssignEnemyPuppet", 1, "Pixy::Puppet", (void*)inPuppet);
	}

  void Combat::assignPuppet(Puppet* inPuppet) {
    assert(inPuppet);

    mLog->infoStream() << "I'm playing with the puppet: " << inPuppet;
    mPuppet = inPuppet;
    mScriptEngine->passToLua("Puppets.onAssignSelfPuppet", 1, "Pixy::Puppet", (void*)mPuppet);
  }

	/* +-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+ *
	 *	Helpers
	 * +-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+ */

  Puppet* Combat::getPuppet() {
    return mPuppet;
  }
  Puppet* Combat::getActivePuppet() {
    return mActivePuppet;
  }

  Combat::puppets_t const& Combat::getPuppets() {
    return mPuppets;
  }

  Puppet* Combat::getPuppet(int inUID) {
    puppets_t::const_iterator itr;
    for (itr = mPuppets.begin(); itr != mPuppets.end(); ++itr)
      if ((*itr)->getUID() == inUID)
        return *itr;

    //assert(false);
    throw invalid_uid("in Combat::getPuppet() : " + stringify(inUID));
  }

  Puppet* Combat::getEnemy(int inUID) {
    puppets_t::const_iterator itr;
    for (itr = mPuppets.begin(); itr != mPuppets.end(); ++itr)
      if ((*itr)->getUID() != inUID)
        return *itr;

    //assert(false);
    throw invalid_uid("in Combat::getEnemy() : " + stringify(inUID));
  }

  Unit* Combat::getUnit(int inUID) {
    puppets_t::const_iterator puppet;
    for (puppet = mPuppets.begin();
         puppet != mPuppets.end();
         ++puppet)
    {
      Puppet::units_t::const_iterator unit;
      for (unit = (*puppet)->getUnits().begin();
           unit != (*puppet)->getUnits().end();
           ++unit)
        if ((*unit)->getUID() == inUID)
          return *unit;
    }

    //assert(false);
    throw invalid_uid("in Combat::getUnit() : " + stringify(inUID));
  }

	/* +-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+ *
	 *	Event Handlers
	 * +-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+ */

  /*bool Combat::onSyncGameData(const Event& evt) {
    using std::string;
    using std::vector;

    mLog->infoStream() << "received game data, populating Resource Manager...";

    std::string senc = evt.getProperty("Data");

    vector<unsigned char> encoded(senc.begin(), senc.end());
    vector<unsigned char> raw;

    if (Archiver::decodeLzma(raw, encoded, evt.Rawsize) != 1) {
      std::cerr << "decoding failed!! \n";
    }

    string raw2str(raw.begin(), raw.end());

    std::istringstream datastream(raw2str);

    GameManager::getSingleton().getResMgr().populate(datastream);


    return true;
  }*/



  bool Combat::onGameDataSynced(const Event& evt) {
    //~ mNetMgr->send(Event(EventUID::SyncGameData));
    Event _evt(EventUID::Login);
    _evt.setProperty("Username", "Kandie");
    _evt.setProperty("Password", "tuonela");
    mNetMgr->send(_evt);

    return true;
  }

  bool Combat::onLogin(const Event& evt) {
    Event _evt(EventUID::SyncPuppets);
    mNetMgr->send(_evt);

    return true;
  }

  bool Combat::onSyncPuppets(const Event& inEvt) {
    Event _evt(EventUID::JoinLobby);
    _evt.setProperty("Puppet", mPuppetName);
    mNetMgr->send(_evt);
    return true;
  }
  bool Combat::onJoinLobby(const Event& inEvt) {
    Event _evt(EventUID::JoinQueue);
    if (mPuppetName == "Candy")
      _evt.setProperty("D", "Fire Template 1");
    else
      _evt.setProperty("D", "Earth Template 1");
    mNetMgr->send(_evt);
    return true;
  }

  bool Combat::onMatchFound(const Event& inEvt) {
    Event e(EventUID::SyncMatchPuppets);
    mNetMgr->send(e);

    return true;
  }

  bool Combat::onChangingState(const Event& e) {
    //if (e.hasProperty("State") && e.getProperty("State") == "Intro")
    changeState(Intro::getSingletonPtr());

    return true;
  }

  bool Combat::onSyncMatchPuppets(const Event& inEvt) {

    using std::string;
    using std::vector;

    //string senc(inEvt.getProperty("Data"));
    /*vector<unsigned char> encoded(senc.begin(), senc.end());
    vector<unsigned char> raw;

    if (Archiver::decodeLzma(raw, encoded, inEvt.Rawsize) != 1) {
      std::cerr << "decoding failed!! \n";
    }

    string raw2str(raw.begin(), raw.end());

    std::cout << "Uncompressed puppet data: " << raw2str << "\n";*/
    mLog->infoStream() << "match puppet data received:";

    mLog->infoStream() << "\tparsing puppet data";
    std::istringstream data(inEvt.getProperty("Data"));
    //~ list<Puppet*> lPuppets = GameManager::getSingleton().getResMgr().puppetsFromStream(datastream);

    ResourceParser *lParser = new ResourceParser(mEntityMgr, data);
    std::list<BasePuppet*> lPuppets = lParser->syncGamePuppetsFromDump();
    data.clear();

    assert(lPuppets.size() > 1);

    mLog->infoStream() << "\tpuppets parsed, registering them";
    for (std::list<BasePuppet*>::iterator puppet = lPuppets.begin();
         puppet != lPuppets.end();
         ++puppet)
      this->registerPuppet(static_cast<Puppet*>(*puppet));

    // set up the scene
    mGfxEngine->setupCombat();
    //~ mScriptEngine->passToLua("Gfx.PrepareScene", 0);

    // render all the puppets
    for (puppets_t::iterator puppet_itr = mPuppets.begin();
         puppet_itr != mPuppets.end();
         ++puppet_itr) {
      Puppet* puppet = *puppet_itr;
      puppet->live();
      mScriptEngine->passToLua("Puppets.onCreatePuppet", 1, "Pixy::Puppet", (void*)puppet);
      //~ mGfxEngine->attachToScene(puppet->getRenderable());
    }
    mLog->infoStream() << "\tpuppet parsing was successful";

    mLog->infoStream() << "battle has begun, informing server we are ready";
    mScriptEngine->passToLua("Combat.onGameStarted", 0);

    // arbitrarily assign active and waiting puppets
    mActivePuppet = mPuppets.front();
    mWaitingPuppet = mPuppets.back();

    Event resp(EventUID::Ready);
    mNetMgr->send(resp);

    return true;
  }

  void Combat::handleNewTurn() {

    mLog->infoStream() << "handling a new turn:";
    mLog->debugStream()
      << "\tactive puppet is: " << mActivePuppet
      << ", inactive puppet is: " << mWaitingPuppet;

    mScriptEngine->passToLua("Puppets.onAssignActivePuppet", 1, "Pixy::Puppet", (void*)mActivePuppet);
    mScriptEngine->passToLua("Turns.onNewTurn", 0);

    // apply active buffs
    mLog->debugStream() << "\tapplying buffs on puppet";
    for (Puppet::spells_t::const_iterator buff = mActivePuppet->getBuffs().begin();
         buff != mActivePuppet->getBuffs().end();
         ++buff)
    {
      mScriptEngine->passToLua(
        "Spells.onProcessBuff", 3,
        "Pixy::Entity", (*buff)->getCaster(),
        "Pixy::Entity", (*buff)->getTarget(),
        "Pixy::Spell", *buff);
    }

    // remove all expired puppet buffs
    {
      mLog->debugStream() << "\tdetermining expired buffs on puppet for removal";
      std::vector<Spell*> expired;
      for (Puppet::spells_t::const_iterator buff = mActivePuppet->getBuffs().begin();
           buff != mActivePuppet->getBuffs().end();
           ++buff)
        if ((*buff)->hasExpired())
          expired.push_back(*buff);

      mLog->debugStream() << "\tremoving " << expired.size() << " expired buffs on puppet";
      for (std::vector<Spell*>::iterator expired_spell = expired.begin();
           expired_spell != expired.end();
           ++expired_spell)
        mActivePuppet->detachBuff((*expired_spell)->getUID());
    }

    mLog->debugStream() << "\tprocessing buffs on puppet units";
    for (Puppet::units_t::const_iterator unit_itr = mActivePuppet->getUnits().begin();
         unit_itr != mActivePuppet->getUnits().end();
         ++unit_itr)
    {
      Unit* unit = *unit_itr;
      unit->reset();
      unit->getUp();

      // apply active buffs
      mLog->debugStream() << "\t\tunit " << unit << " has " << unit->getBuffs().size() << " buffs";
      for (Unit::spells_t::const_iterator buff = unit->getBuffs().begin();
           buff != unit->getBuffs().end();
           ++buff)
      {
        mLog->debugStream() << "\t\tapplying active buff " << (*buff) << " on unit " << unit;
        mScriptEngine->passToLua(
        "Spells.onProcessBuff", 3,
        "Pixy::Entity", (*buff)->getCaster(),
        "Pixy::Entity", (*buff)->getTarget(),
        "Pixy::Spell", *buff);
      }

      // remove all expired puppet buffs
      {
        std::vector<Spell*> expired;
        for (Unit::spells_t::const_iterator buff = unit->getBuffs().begin();
             buff != unit->getBuffs().end();
             ++buff)
        {
          if ((*buff)->hasExpired())
            expired.push_back(*buff);
        }

        for (std::vector<Spell*>::iterator expired_spell = expired.begin();
             expired_spell != expired.end();
             ++expired_spell)
        {
          mLog->debugStream() << "\t\tremoving expired buff " << (*expired_spell) << " from unit " << unit;
          unit->detachBuff((*expired_spell)->getUID());
        }
      }

    }

  }

  bool Combat::onStartTurn(const Event& inEvt) {
    mLog->debugStream() << "my turn is starting";

    mActivePuppet = mPuppet;
    if (mPuppet == mPuppets.front())
      mWaitingPuppet = mPuppets.back();
    else
      mWaitingPuppet = mPuppets.front();

    // send the event back, acknowledging the order
    mNetMgr->send(inEvt);

    handleNewTurn();

    return true;
  }

  bool Combat::onTurnStarted(const Event& inEvt) {
    assert(inEvt.hasProperty("P")); // _DEBUG_

    mLog->infoStream() << "opponent's turn has started";

    mWaitingPuppet = mActivePuppet;
    mActivePuppet = getPuppet(convertTo<int>(inEvt.getProperty("P")));

    assert(mActivePuppet); // _DEBUG_

    handleNewTurn();

    return true;
  }


  bool Combat::onDrawSpells(const Event& inEvt) {
    using std::vector;
    using std::string;
    using std::istringstream;

    mLog->infoStream() << "parsing drawn spells";

    istringstream lData(inEvt.getProperty("Data"));
    string lLine;

    // parse the drawn spells and add them to our hand
    {
      // how many spells to create
      getline(lData, lLine);
      int nrDrawSpells = atoi(Utility::split(lLine, ';').back().c_str());

      // the next line contains the actual string UIDs/names and the owner uid
      getline(lData, lLine);
      vector<string> elements = Utility::split(lLine, ';');

      int lPuppetUID = convertTo<int>(elements[0].erase(0,1).c_str()); // strip out the leading $
      Puppet* lPuppet = getPuppet(lPuppetUID);
      assert(lPuppet); // _DEBUG_

      //int nrSpells = convertTo<int>(elements[1].c_str());
      assert((elements.size()-1)/2 == nrDrawSpells);
      mLog->debugStream() << "\tpuppet " << lPuppet << " has received " << nrDrawSpells << " spells";
      int spellsParsed = 0;
      int index = 0;
      while (++spellsParsed <= nrDrawSpells) {
        Spell* lSpell = 0;
        try {
          lSpell = mEntityMgr->getSpell(elements[++index]);
        } catch (UnidentifiedResource& e)
        {
          // report bug
          return true;
        }

        lSpell->_setUID(convertTo<int>(elements[++index]));
        lPuppet->attachSpell(lSpell);

        mLog->debugStream() << "attaching spell: " << lSpell << " to puppet " << lPuppet;

        // draw the spell button in the UI
        if (lPuppet == mPuppet) {
          mScriptEngine->passToLua("Spells.onDrawSpell", 1, "Pixy::Spell", (void*)lSpell);
        }

        lSpell = 0;
      }

      lPuppet = 0;
    }

    // parse the spells to be discarded
    {
      getline(lData, lLine);
      int nrDropSpells = atoi(Utility::split(lLine, ';').back().c_str());

      if (nrDropSpells > 0) {

        // the next line contains the actual string UIDs/names and the owner uid
        getline(lData, lLine);
        vector<string> elements = Utility::split(lLine, ';');

        int lPuppetUID = convertTo<int>(elements[0].erase(0,1).c_str()); // strip out the leading $
        Puppet* lPuppet = getPuppet(lPuppetUID);
        assert(lPuppet); // _DEBUG_

        //int nrSpells = convertTo<int>(elements[1].c_str());
        assert((elements.size()-1) == nrDropSpells);

        mLog->debugStream() << "dropping " << nrDropSpells << " spells from puppet " << lPuppet;

        int spellsParsed = 0;
        int index = 0;
        while (++spellsParsed <= nrDropSpells)
        {
          Spell* lSpell = 0;
          try {
            lSpell = lPuppet->getSpell(convertTo<int>(elements[++index]));
          } catch (invalid_uid& e)
          {
            // report bug
            return true;
          }
          mLog->debugStream() << "removing spell " << lSpell << " from puppet " << lPuppet;

          assert(lSpell); // _DEBUG_

          if (lPuppet == mPuppet)
          {
            mScriptEngine->passToLua("Spells.onDropSpell", 1, "Pixy::Spell", (void*)lSpell);
          }

          lPuppet->detachSpell(lSpell->getUID());
          lSpell = 0;
        }

        lPuppet = 0;
      }
    }

    return true;
  }

  bool Combat::onCastSpell(const Event& inEvt) {
    mLog->debugStream() << "casting a spell";
    if (inEvt.Feedback == EventFeedback::InvalidRequest) {
      // the UID was invalid
      mLog->errorStream() << "my request to cast a spell was rejected!\n";
      mScriptEngine->passToLua("Spells.onCastSpellRejected", 1, "Pixy::Event", &inEvt);
      return true;
    }

    assert(inEvt.hasProperty("Spell") && inEvt.hasProperty("C"));

    // it's ok, let's find the spell, the caster, and the target
    Puppet* lCaster = getPuppet(convertTo<int>(inEvt.getProperty("C")));
    Spell* lSpell = lCaster->getSpell(convertTo<int>(inEvt.getProperty("Spell")));

    /*for (puppets_t::iterator puppet = mPuppets.begin();
         puppet != mPuppets.end();
         ++puppet)
      try {
        lSpell = (*puppet)->getSpell(convertTo<int>(inEvt.getProperty("Spell")));
        lCaster = static_cast<Puppet*>(lSpell->getCaster()->getEntity());
        break;
      } catch (...) { lSpell = 0; }*/

    assert(lSpell && lCaster && lSpell->getCaster()->getUID() == lCaster->getUID());

    mLog->debugStream() << "\tspell is: " << lSpell << " and its caster is: " << lCaster;

    Entity* lTarget = 0;
    if (inEvt.hasProperty("T")) {
      try {
        // is the target a puppet?
        lTarget = getPuppet(convertTo<int>(inEvt.getProperty("T")));
        mLog->debugStream() << "target: " << lTarget;
      } catch (invalid_uid& e) {
        try {
          // a unit?
          lTarget = getUnit(convertTo<int>(inEvt.getProperty("T")));
        } catch (invalid_uid& e) {
          // invalid UID
          mLog->errorStream() << "couldn't find spell target with id " << inEvt.getProperty("T");
          // report bug
          return true;
        }
      }
      lSpell->setTarget(lTarget);
    } else
      lSpell->setTarget(lCaster);

    if (lSpell->getTarget() != lCaster)
      mLog->debugStream()<<"\t\tspell's target is " << lSpell->getTarget();

    mScriptEngine->passToLua(
      "Spells.onCastSpell", 3,
      "Pixy::Entity", lCaster,
      "Pixy::Entity", lTarget,
      "Pixy::Spell", lSpell);
    // ...
    //std::cout << "casted a spell! " << lSpell->getName() << "#" << lSpell->getUID() << "\n";
    // remove it from the UI
    if (lCaster == mPuppet) {
      mLog->debugStream() << "\tcasting of " << lSpell << " was successful, now removing the spell";
      mScriptEngine->passToLua("Spells.onDropSpell", 1, "Pixy::Spell", (void*)lSpell);
    }
    //mUIEngine->dropSpell(_spell);
    // remove it from the puppet's hand if it's not a buff
    lCaster->detachSpell(lSpell->getUID(), lSpell->getDuration() == 0);

    mLog->debugStream() << "\tthe spell casting process is complete";
    return true;
  }

  bool Combat::onCreateUnit(const Event& evt) {
    mLog->debugStream() << "creating a unit:";
    assert(evt.hasProperty("Name") && evt.hasProperty("OUID") && evt.hasProperty("UID"));

    Puppet* lOwner = 0;
    Unit* lUnit = 0;

    try {
      lOwner = getPuppet(convertTo<int>(evt.getProperty("OUID")));
    } catch(invalid_uid& e)
    {
      // report bug
      assert(false);
      return true;
    }

    //std::cout << "CreateUnit name: " << evt.getProperty("Name") << "\n";
    try {
      lUnit = mEntityMgr->getUnit(evt.getProperty("Name"), lOwner->getRace());
    } catch (invalid_uid& e)
    {
      // report bug
      assert(false);
      return true;
    }

    lUnit->deserialize(evt);
    lOwner->attachUnit(lUnit);

    assert(lUnit->getOwner());

    mLog->debugStream() << "\tthe created unit is: " << lUnit;

    lUnit->live();
    //~ mGfxEngine->attachToScene(_unit->getRenderable());

    mScriptEngine->passToLua("Units.onCreateUnit", 1, "Pixy::Unit", (void*)lUnit);

    lUnit = 0;
    lOwner = 0;

    mLog->debugStream() << "\tunit creation successful";

    return true;
  }

  bool Combat::onUpdatePuppet(const Event& evt) {
    assert(evt.hasProperty("UID"));

    Puppet* lPuppet = getPuppet(convertTo<int>(evt.getProperty("UID")));
    assert(lPuppet); // todo: report bug if not found

    //std::cout << "Updating puppet named: " << _puppet->getName() << "\n";

    lPuppet->deserialize(evt);
    mScriptEngine->passToLua("Puppets.onUpdatePuppet", 1, "Pixy::Puppet", (void*)lPuppet);

    return true;
  }

  bool Combat::onUpdateUnit(const Event& evt) {
    assert(evt.hasProperty("UID"));

    Unit* lUnit = getUnit(convertTo<int>(evt.getProperty("UID")));
    assert(lUnit);

    //std::cout << "Updating unit named: " << _unit->getName() << "#" << _unit->getUID() << "\n";

    lUnit->deserialize(evt);
    if (lUnit->isDead())
      markForDeath(lUnit);

    return true;
  }

  bool Combat::onRemoveUnit(const Event& evt) {
    mLog->debugStream() << "Unit#" << evt.getProperty("UID") << " is marked for removal by server.";
    mRDeathlist.push_back(convertTo<int>(evt.getProperty("UID")));

    // todo: assert that the unit exists

    return true;
  }

  bool Combat::onEntityDied(const Event& evt) {
    markForDeath(static_cast<Unit*>(evt.Any));
    return true;
  }

  bool Combat::onStartBlockPhase(const Event& evt) {
    if (mActivePuppet != mPuppet)
      inBlockPhase = true;

    // get up all the opponent units with summoning sickness
    for (Puppet::units_t::iterator unit = mWaitingPuppet->getUnits().begin();
         unit != mWaitingPuppet->getUnits().end();
         ++unit)
      if ((*unit)->hasSummoningSickness())
        (*unit)->getUp();

    return true;
  }

  bool Combat::onCharge(const Event& evt) {
    Unit *attacker = 0;

    assert(evt.hasProperty("UID"));
    try {
      attacker = mActivePuppet->getUnit(convertTo<int>(evt.getProperty("UID")));
    } catch (invalid_uid& e) {
      mLog->errorStream()  << "invalid charge event parameters : " << e.what();
      // report bug
      assert(false);
      return true;
    }

    // move the unit
    //std::cout << evt.getProperty("UID") << " is charging for an attack\n";

    mAttackers.push_back(attacker);
    attacker->setAttackOrder(mAttackers.size());
    attacker->updateTextOverlay();
    attacker->_setState(Unit::State::Charging);

    return true;
  }

  bool Combat::onCancelCharge(const Event& evt) {
    Unit *attacker = 0;

    assert(evt.hasProperty("UID"));
    try {
      attacker = mActivePuppet->getUnit(convertTo<int>(evt.getProperty("UID")));
    } catch (invalid_uid& e) {
      mLog->errorStream() << "invalid charge event parameters : " << e.what();
      // report bug
      assert(false);
      return true;
    }

    //std::cout << evt.getProperty("UID") << " is no longer charging\n";
    mAttackers.remove(attacker);

    // recalculate attack orders and display them
    attacker->setAttackOrder(0);
    attacker->updateTextOverlay();
    attacker->_setState(Unit::State::Ready);

    int i=0;
    for (attackers_t::iterator unit = mAttackers.begin();
         unit != mAttackers.end();
         ++unit) {
      (*unit)->setAttackOrder(++i);
      (*unit)->updateTextOverlay();
    }

    return true;
  }

  bool Combat::onBlock(const Event& evt) {
    Unit *attacker, *blocker = 0;

    assert(evt.hasProperty("B") && evt.hasProperty("A"));
    try {
      attacker = mActivePuppet->getUnit(convertTo<int>(evt.getProperty("A")));
      blocker = mWaitingPuppet->getUnit(convertTo<int>(evt.getProperty("B")));
    } catch (invalid_uid& e) {
      mLog->errorStream()  << "invalid block event parameters : " << e.what();
      // report bug
      assert(false);
      return true;
    }

    // does the attacker have any other units blocking?
    blockers_t::iterator blockers = mBlockers.find(attacker);
    if (blockers == mBlockers.end()) {
      // this is the first blocker
      blockers = mBlockers.insert( std::make_pair(attacker, std::list<Unit*>()) ).first;
    }
    blockers->second.push_back(blocker);
    blocker->setAttackOrder(blockers->second.size());
    blocker->setBlockTarget(attacker);
    blocker->_setState(Unit::State::Blocking);

    blocker->updateTextOverlay();

    return true;
  }

  bool Combat::onCancelBlock(const Event& evt) {
    Unit *blocker, *attacker = 0;

    assert(evt.hasProperty("B") && evt.hasProperty("A"));
    try {
      blocker = mWaitingPuppet->getUnit(convertTo<int>(evt.getProperty("B")));
      attacker = mActivePuppet->getUnit(convertTo<int>(evt.getProperty("A")));
    } catch (invalid_uid& e) {
      mLog->errorStream() << "invalid charge event parameters : " << e.what();
      // report bug
      assert(false);
      return true;
    }

    assert(blocker && attacker);

    /*
     * 1) remove blocker from blockers list
     * 2) if no blockers r left, remove the entry, otherwise
     *    recalculate attack orders
     */
    blockers_t::iterator entry = mBlockers.find(attacker);

    assert(entry != mBlockers.end());

    entry->second.remove(blocker);

    if (entry->second.empty()) {
      mBlockers.erase(entry);
    } else {
      int i=0;
      for (std::list<Unit*>::iterator blocker = entry->second.begin();
           blocker != entry->second.end();
           ++blocker) {
        (*blocker)->setAttackOrder(++i);
        (*blocker)->updateTextOverlay();
      }
    }

    // reset the blocker
    blocker->setAttackOrder(0);
    blocker->setBlockTarget(0);
    blocker->_setState(Unit::State::Ready);
    blocker->updateTextOverlay();

    return true;
  }

  bool Combat::onEndBlockPhase(const Event& evt) {
    if (mActivePuppet != mPuppet)
      inBlockPhase = false;

    doBattle();

    return true;
  }

  void Combat::doBattle() {

    // first call
    if (mChargers.empty()) {
      // simulate the battle
      mLog->debugStream() << "simulating battle with : "
      << mAttackers.size() << " attacking units and "
      << mBlockers.size() << " units being blocked";

      // assign attack orders and display them
      int i=0;
      for (attackers_t::iterator unit = mAttackers.begin();
           unit != mAttackers.end();
           ++unit) {
        (*unit)->setAttackOrder(++i);
        (*unit)->updateTextOverlay();
      }

      for (blockers_t::iterator unit_pair = mBlockers.begin();
           unit_pair != mBlockers.end();
           ++unit_pair) {
        int i = 0;
        for (attackers_t::iterator unit = unit_pair->second.begin();
             unit != unit_pair->second.end();
             ++unit) {
          (*unit)->setAttackOrder(++i);
          (*unit)->updateTextOverlay();
        }
      }
    }
    // battle is over, move all remaining units back
    if (mAttackers.empty()) {
      mLog->debugStream() << "\tall attackers are done, moving them back";

      for (puppets_t::iterator puppet = mPuppets.begin();
      puppet != mPuppets.end();
      ++puppet)
      {
        for (Puppet::units_t::const_iterator unit = (*puppet)->getUnits().begin();
        unit != (*puppet)->getUnits().end();
        ++unit)
        {
          bool isAttacker = (*unit)->getOwner()->getUID() == mActivePuppet->getUID();
          if (!(*unit)->isReady() && !(*unit)->isDying())
            (*unit)->move(Unit::Position::Ready, boost::bind(
              (isAttacker ? &Combat::onMoveBackAndRest : &Combat::onMoveBack),
              this, *unit));
        }
      }

      // move the blockers first
      /*for (blockers_t::iterator unit_pair = mBlockers.begin();
           unit_pair != mBlockers.end();
           ++unit_pair)
        for (attackers_t::iterator unit = unit_pair->second.begin();
             unit != unit_pair->second.end();
             ++unit) {
          if (!(*unit)->isDead()) {
            (*unit)->move(POS_READY, boost::bind(&Combat::onMoveBack, this, *unit));
          } else {
            //static_cast<Puppet*>((Entity*)unit->getOwner())->detachUnit(unit->getUID());
            //mDeathlist.push_back(*unit);
            //~ markForDeath(*unit);
          }
        }

      for (attackers_t::iterator unit = mChargers.begin();
           unit != mChargers.end();
           ++unit) {
        if (!(*unit)->isDead())
          (*unit)->move(POS_READY, boost::bind(&Combat::onMoveBackAndRest, this, *unit));
        else {
          //static_cast<Puppet*>((Entity*)unit->getOwner())->detachUnit(unit->getUID());
          //mDeathlist.push_back(*unit);
          //~ markForDeath(*unit);
        }
      }*/

      mChargers.clear();
      mBlockers.clear();
      mAttackers.clear();

      // tell the server we're done simulating the battle
      mNetMgr->send(Event(EventUID::BattleOver));

      mLog->infoStream() << "\tbattle is over";
      return;
    }



    // go through every attacking unit:
    Unit* unit = mAttackers.front();
    unit->_setState(Unit::State::Attacking);
    //for (auto unit : mAttackers) {
      // if it's being blocked, go through every blocker
      // else, let it attack the puppet
      if (mBlockers.find(unit) == mBlockers.end()) {
        unit->moveAndAttack(mWaitingPuppet);
        //~ break;
      } else {
        unit->moveAndAttack(mBlockers.find(unit)->second);
      }
    //}

  }

  void Combat::onMoveBack(Unit* inUnit) {
    inUnit->setAttackOrder(0);
    inUnit->reset();
  }
  void Combat::onMoveBackAndRest(Unit* inUnit) {
    onMoveBack(inUnit);
    inUnit->rest();
  }

  void Combat::unitAttacked(Unit* inUnit) {
    mAttackers.remove(inUnit);
    mChargers.push_back(inUnit);
    mLog->infoStream() << "a unit " << inUnit << " is done attacking, " << mAttackers.size() << " left";
  }

  void Combat::markForDeath(Unit* inUnit) {
    // add the unit to the deathlist only if it's not there
    for (deathlist_t::iterator unit = mDeathlist.begin();
         unit != mDeathlist.end();
         ++unit)
      if ((*unit)->getUID() == inUnit->getUID())
        return;

    mDeathlist.push_back(inUnit);
  }
} // end of namespace

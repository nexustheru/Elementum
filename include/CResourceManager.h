/*
 * Copyright (C) Shroom Studios, Inc - All Rights Reserved
 *
 * Unauthorized copying of this file, via any medium is strictly prohibited
 * Proprietary and confidential
 *
 * Written by Ahmad Amireh <ahmad@shroom-studios.com>, September 2011
 */

#ifndef H_CResourceManager_H
#define H_CResourceManager_H

#include "Pixy.h"
#include "ResourceManager.h"
#include "CUnit.h"
#include "CPuppet.h"
#include "CSpell.h"
//#include "CDeck.h"

using std::string;
using std::vector;
using std::list;
using std::istringstream;
namespace Pixy
{
  //class CSpell;
  //class CUnit;
  class CDeck;
	class CResourceManager : public ResourceManager {
    public:
      typedef std::list<CSpell*> spells_t;
      typedef std::list<CUnit*> units_t;

      CResourceManager();
      virtual ~CResourceManager();

      virtual CSpell* const getSpell(std::string inName);
      virtual CSpell* const getSpell(std::string inName, Pixy::RACE inRace);
      virtual CUnit* const getUnit(std::string inName);
      virtual CUnit* const getUnit(std::string inName, Pixy::RACE inRace);
      list<CPuppet*> puppetsFromStream(istringstream &stream);
      list<CPuppet*>& getPuppets();

      void clearDatabase();

      spells_t const& _getSpells(Pixy::RACE);
    protected:

      virtual CSpell* getModelSpell(std::string inName);
      virtual CUnit* getModelUnit(std::string inName);

      void parsePuppetsSection(istringstream &stream, int section, int nrEntries);
      void parsePuppetsStats(vector<string>& entries);
      void parsePuppetsDecks(vector<string>& entries);
      virtual void parseSpells(std::vector<std::string>& entries);
      virtual void parseMinions(std::vector<std::string>& entries);
      virtual void parseMinionSpells(std::vector<std::string>& entries);

      void assignTalents(Puppet& inPuppet, string inTalents);
      void assignDeck(Puppet& inPuppet, string inName, string inSpells, int inUseCount);

      CPuppet* getPuppet(string inName, list<CPuppet*>& inPuppets);

      spells_t mSpells[4];
      units_t mUnits[4];

      list<CPuppet*> mPuppets;
  };
}

#endif

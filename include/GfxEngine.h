/*
 * Copyright (C) Shroom Studios, Inc - All Rights Reserved
 *
 * Unauthorized copying of this file, via any medium is strictly prohibited
 * Proprietary and confidential
 *
 * Written by Ahmad Amireh <ahmad@shroom-studios.com>, September 2011
 */

#ifndef H_GfxEngine_H
#define H_GfxEngine_H

#include "Engine.h"
#include "EventListener.h"
#include "InputListener.h"

#include <Ogre.h>

// forward declarations
/*namespace Ogre {
  class Root;
  class Camera;
  class SceneManager;
  class Viewport;
  class RenderWindow;
  class OverlayManager;
  class Overlay;
  class Vector3;
  class Real;
  class SceneNode;
  class String;
}*/

/*
namespace Hydrax {
  class Hydrax;
}
namespace Caelum {
  class CaelumSystem;
}
*/
class DotSceneLoader;


namespace OgreMax {
  class OgreMaxScene;
}
namespace OgreBites {
  class SdkCameraMan;
  class SdkTrayManager;
}

class HDRCompositor;

class MovableTextOverlay;
class MovableTextOverlayAttributes;

using Ogre::Vector3;
using Ogre::Real;
using Ogre::SceneNode;
using Ogre::String;
using OgreBites::SdkCameraMan;
using OgreBites::SdkTrayManager;
namespace Pixy {

  class Mobile;
  class Animable;
  class Entity;
  class Unit;
  class Puppet;
  class EventManager;
  class MousePicker;
  class GenericMousePicker;
  class PolyMousePicker;
  class OgreRTT;
	/*! \class GfxEngine
	 *	\brief
	 *	Handles all graphics related features of the game, acts as the immediate
	 *	wrapper over OGRE3D. The game scene is entirely managed by and through
	 *	this Engine.
	 */
	class GfxEngine : public Engine, public EventListener, public InputListener, public Ogre::RenderTargetListener {

	public:

    enum QueryFlags {
      TERRAIN_MASK = 1<<0,
      ENTITY_MASK = 1<<1
    };

    enum MousePickingMode {
      GENERIC,
      POLY
    };

		virtual ~GfxEngine();
		static GfxEngine* getSingletonPtr();

    Ogre::ColourValue mMTOFontColor;
    Ogre::String mMTOFontName;
    int mMTOFontSize;
    Ogre::String mMTOMaterialName;
		//! used for setting Puppetes' starting positions in Scene
		Vector3 mPuppetPos[2];
		Vector3 mPuppetScale, mUnitScale;

    Vector3 mUnitMargin;
    Vector3 mPuppetMargin;
    int mPackSpacing;

    Vector3 mCameraYawPitchDist;

		virtual bool setup();
		virtual void update(unsigned long lTimeElapsed);
		virtual bool cleanup();

    void disableMouseCaptureOverUIElement(std::string const& inElementName);
    void enableMouseCaptureOverUIElement(std::string const& inElementName);

		void setCamera(const Ogre::String& inCameraName);

    void createSphere(const std::string& strName, const float r, const int nRings = 16, const int nSegments = 16);

    void switchMousePickingMode();
    int getMousePickingMode() const;

		//! Attaches a Pixy::Entity to an SceneNode and renders it
		/*!
		 * Called upon by CombatManager::createUnit();
		 * Determines the appropriate empty SceneNode in which to render
		 * the given inEntity. Moreover, "attaches" Pixy::Entity
		 * to the given Ogre::Entity for later retrieval, thus,
		 * linking the GameObject with SceneObject.
		 */
		bool attachToScene(Entity* inEntity);

		//! Detaches a Pixy::Entity from an SceneNode and removes it from Scene
		/*!
		 * Called upon by CombatManager::destroyUnit();
		 * Detaches the Ogre::Entity from SceneNode;
		 * stops rendering it. Also detaches Pixy::Entity
		 * from the Ogre::Entity.
		 */
		void detachFromScene(Entity* inEntity);

    void setupMovableTextOverlays();

		//! Moves a SceneNode to a destination using a Waypoint
		/*!
		 * Moves a matching SceneNode to inDestination.
		 * @return true the node is still moving
		 * @return false the node is done moving
		 */
		//bool moveUnit(Unit* inUnit, int inDestination);

		SdkCameraMan* getCameraMan();
		Ogre::Camera* getCamera();
		Ogre::Root* getRoot();
		Ogre::SceneManager* getSceneMgr();
    Ogre::RenderWindow *getWindow();
		Ogre::Viewport* getViewport();

		//! Sets up OGRE SceneManager
		void setupSceneManager();

		//! Sets up OGRE Viewport to which our Camera will be attached
		void setupViewports();

		//! Sets up OGRE Camera, attaches it to Viewport, and fixes it position
		void setupCamera();

		//! Sets up OGRE Terrain, using terrain.cfg file for config
		void setupTerrain();

		//! Sets up OGRE Lights, attaches 2 spotlights, each for a Puppet respectively
		void setupLights();

		void setupWater();

		void setupSky();

		/*!
		 * \brief Sets up OGRE SceneNodes in which all of our GameObjects
		 * (Pixy::Entity, Unit, and Puppet) will be rendered.
		 *
		 * \note The position of these nodes are fixed throughout the Combat.
		 * \remarks
		 *  Each Puppet can own no more than 10 Units, thus we have
		 *  22 SceneNodes in total for Puppetes and Units in-game.
		 *  However, there are 3 more SceneNodes to account for,
		 *  which describe the position in which Units will be
		 *  taking action;
		 *  \li a) Shared_offence : is used by all Units in-game when
		 *      they are attacking a Unit that is blocking them.
		 *  \li b) Host_defence : is used by Host's Units when they
		 *      are blocking an Attacking unit.
		 *  \li c) Client_defence : like b), but for Client's Units
		 *      respectively.
		 */
		void setupNodes();

		//! Creates movement Waypoints for all SceneNodes in scene
		/*!
		 * Each SceneNode is attached to 5 "Waypoints" which
		 * describe it's position in a given state;
		 * Since Units have the ability to move to 5 different
		 * spots, this method defines the positions of these
		 * spots. Spots are:
		 * \li POS_PASSIVE
		 * \li POS_CHARGING
		 * \li POS_OFFENCE
		 * \li POS_ATTACK
		 * \li POS_BLOCK
		 */
		void setupWaypoints();

		bool setupIntro();
		bool setupCombat();

		void keyPressed( const OIS::KeyEvent &e );
		void keyReleased( const OIS::KeyEvent &e );

		bool mouseMoved( const OIS::MouseEvent &e );
		bool mousePressed( const OIS::MouseEvent &e, OIS::MouseButtonID id );
		bool mouseReleased( const OIS::MouseEvent &e, OIS::MouseButtonID id );

		Entity* getSelected();

    void updateMe(Mobile*);
    void stopUpdatingMe(Mobile*);

    void changeOwnership(Pixy::Unit*);

    OgreMax::OgreMaxScene* loadScene(std::string inOgreMaxScene);
    void unloadScene(OgreMax::OgreMaxScene* inScene);

    void enableCompositorEffect(std::string inEffect);

    void loadDotScene(std::string inScene, std::string inName);

    Ogre::Vector2 getScreenCoords(Ogre::MovableObject* inObject);

    void setYawPitchDist(Ogre::Vector3 inVec);
    void trackNode(Ogre::SceneNode* inNode);

    void _setUserAny(Ogre::MovableObject*, void*);

    void attachRTT(OgreRTT*);
    void detachRTT(OgreRTT*);

	protected:
	  void (GfxEngine::*mUpdate)(unsigned long);

	  void updateNothing(unsigned long lTimeElapsed);
	  void updateIntro(unsigned long lTimeElapsed);
	  void updateCombat(unsigned long lTimeElapsed);

    virtual void preRenderTargetUpdate(const Ogre::RenderTargetEvent& evt);
    virtual void postRenderTargetUpdate(const Ogre::RenderTargetEvent& evt);

		Ogre::Root           *mRoot;
		Ogre::Camera         *mCamera, *mCamera2, *mCamera3, *mCamera4;
		Ogre::SceneManager   *mSceneMgr;
		Ogre::Viewport       *mViewport;
		Ogre::RenderWindow	 *mRenderWindow;
		Ogre::OverlayManager *mOverlayMgr;
		Ogre::Overlay        *mOverlay;
		EventManager		 *mEvtMgr;
		SdkCameraMan		 *mCameraMan;
		DotSceneLoader		 *mSceneLoader;
		//Hydrax::Hydrax *mHydrax;
		//Caelum::CaelumSystem *mCaelumSystem;
		HDRCompositor *mHDRComp;

    SdkTrayManager *mTrayMgr;

    Ogre::FrameEvent evt;

    Puppet *mPlayer, *mEnemy;

		Entity* mSelected; // selected entity

    //! used for setting Objects' direction in Scene
		Vector3 mDirection[2];

    std::list<std::string> mUIInputCapturingElements;

    typedef std::map<Mobile*, bool> mobiles_t;
    mobiles_t mMobiles;

    typedef std::list<Animable*> animables_t;
    animables_t mAnimables;

    typedef std::list<MovableTextOverlay*> mtos_t;
    mtos_t mMTOs;

		/*!
		 * \brief Creates an Ogre::Entity and renders it in an
		 * appropriate SceneNode. Must not be called directly;
		 * only reachable via attachToScene()
		 */
		SceneNode* renderEntity(Entity* inEntity);

		/*!
		 * \brief Creates a SceneNode with the given attributes
		 *
		 * \note used by setupNodes()
		 * @return the created SceneNode
		 */
		SceneNode* createNode(String& inName, Vector3& inPosition, Vector3& inScale, Vector3& inDirection, SceneNode* inParent=NULL);

    /*!
     * \brief Creates a list of 5 Waypoints for a matching SceneNode
     *
     * The waypoints are determined in relation to
     * the owner's Hero position, this way, if a Hero's
     * position changes, all the waypoints for Units are
     * changed accordingly.
     * \note used by setupWaypoints()
     */
    void createWaypoint(int inOwner, int inNode, std::string, std::string);

    std::vector<Ogre::Vector3> mWaypoints[2][10];

    MovableTextOverlayAttributes* attrs;

	  bool onEntitySelected(const Event&);
    bool onEntitySelectedAttack(const Event&);
    bool onEntitySelectedBlock(const Event&);

    bool onCharge(const Event&);
    bool onCancelCharge(const Event&);
    bool onBlock(const Event&);
    bool onCancelBlock(const Event&);
    bool onStartBlockPhase(const Event&);
    bool onEndBlockPhase(const Event&);
    bool onMatchFinished(const Event&);

	  void highlight(Entity* inEntity);
	  void dehighlight();

    bool inBlockPhase;

    OgreMax::OgreMaxScene* mScene;

    std::string getNodeIdPrefix(Puppet*);
    std::string getNodeIdPrefix(Unit*);

    GenericMousePicker *mGenericPicker;
    PolyMousePicker *mPolyPicker;
    MousePicker* mPicker;

    std::list<OgreRTT*> mRTTs;

	private:
		static GfxEngine* _myGfxEngine;
		GfxEngine();
		GfxEngine(const GfxEngine& src);
		GfxEngine& operator=(const GfxEngine& rhs);
	};
}
#endif

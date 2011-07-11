/*
 *  GfxEngine.cpp
 *  Elementum
 *
 *  Created by Ahmad Amireh on 2/11/10.
 *  Copyright 2010 JU. All rights reserved.
 *
 */

#include "GfxEngine.h"
#include "GameManager.h"
#include "EventManager.h"
#include "CPuppet.h"
#include "CUnit.h"
#include "Renderable.h"
#include "Combat.h"
#include "UIEngine.h"
#include "NetworkManager.h"
#include "InputManager.h"

#if PIXY_PLATFORM == PIXY_PLATFORM_APPLE
#include <CEGUIBase/CEGUI.h>
#include <CEGUIBase/CEGUISystem.h>
#include <CEGUIBase/CEGUISchemeManager.h>
#else
#include <CEGUI/CEGUI.h>
#include <CEGUI/CEGUISystem.h>
#include <CEGUI/CEGUISchemeManager.h>
#include <CEGUI/CEGUIInputEvent.h>
#endif

#include <Ogre.h>
//#include <OGRE/Terrain/OgreTerrain.h>
//#include <OGRE/Terrain/OgreTerrainGroup.h>
//#include <OGRE/Plugins/OctreeSceneManager/OgreOctreeSceneManager.h>
//#include <OGRE/Plugins/Hydrax/Hydrax.h>
//#include <OGRE/Plugins/Caelum/Caelum.h>
//#include "Plugins/Hydrax/Hydrax.h"
//#include "Plugins/Hydrax/Noise/FFT/FFT.h"
//#include "Plugins/Hydrax/Noise/Perlin/Perlin.h"
//#include "Plugins/Hydrax/Noise/Real/Real.h"
//#include "Plugins/Hydrax/Modules/ProjectedGrid/ProjectedGrid.h"
//#include "Plugins/Hydrax/Modules/RadialGrid/RadialGrid.h"
//#include "Plugins/Hydrax/Modules/SimpleGrid/SimpleGrid.h"
//#include "Plugins/Caelum/CaelumSystem.h"

//#include "dotscene/DotSceneLoader.h"
#include "ogre/HelperLogics.h"
#include "ogre/SdkCameraMan.h"
#include "ogre/HDRCompositor.h"
#include "ogre/MovableTextOverlay.h"
#include "ogre/RectLayoutManager.h"
#include "ogre/OgreSdkTrays.h"


namespace Pixy {

	GfxEngine* GfxEngine::_myGfxEngine = NULL;

	GfxEngine* GfxEngine::getSingletonPtr() {
		if( !_myGfxEngine ) {
		    _myGfxEngine = new GfxEngine();
		}

		return _myGfxEngine;
	}

	GfxEngine::GfxEngine() {
		mLog = new log4cpp::FixedContextCategory(PIXY_LOG_CATEGORY, "GfxEngine");
		mLog->infoStream() << "firing up";
		fSetup = false;
		//mPlayers.clear();
		mCameraMan = 0;
		//mSceneLoader = 0;
    mPlayer = 0;
    mEnemy = 0;
    inBlockPhase = false;
    mSelected = 0;
	}

	GfxEngine::~GfxEngine() {
		mLog->infoStream() << "shutting down";
		if (fSetup) {
			mRoot = 0;
			mSceneMgr = 0;
			mCamera = mCamera2 = mCamera3 = mCamera4 = 0;
			mViewport = 0;
			mRenderWindow = 0;

      delete attrs;

			delete mLog;
			mLog = 0;

			//if (mSceneLoader)
			//	delete mSceneLoader;

			fSetup = false;
		}
	}

	bool GfxEngine::setup() {
		if (fSetup)
			return true;

		mLog->infoStream() << "Setting up";

		mRoot         = Ogre::Root::getSingletonPtr();
		//mOverlayMgr   = OverlayManager::getSingletonPtr();
		//if (mRoot->hasSceneManager(Ogre::ST_GENERIC))
		//	mSceneMgr     = mRoot->getSceneManager( ST_GENERIC, "CombatScene" );
		//else
		mSceneMgr = mRoot->createSceneManager("OctreeSceneManager", "CombatScene");


		mCamera       = mSceneMgr->createCamera("Combat_Camera");
		//mCamera2      = mSceneMgr->createCamera("Combat_Camera_2");
		//mCamera3      = mSceneMgr->createCamera("Combat_Camera_3");
		//mCamera4      = mSceneMgr->createCamera("Combat_Camera_4");

		mRenderWindow = mRoot->getAutoCreatedWindow();

		/*mRenderWindow->reposition(1420 - mRenderWindow->getWidth(),
								  50);*/
		//mViewport     = mRenderWindow->addViewport( mCamera );

		//mRenderWindow->getViewport(<#unsigned short index#>)

		mEvtMgr = EventManager::getSingletonPtr();

		/*if (GameManager::getSingleton().currentState()->getId() == STATE_COMBAT)
		  setupCombat();*/

		bind(EventUID::EntitySelected, boost::bind(&GfxEngine::onEntitySelected, this, _1));
    bind(EventUID::StartBlockPhase, boost::bind(&GfxEngine::onStartBlockPhase, this, _1));
    bind(EventUID::Charge, boost::bind(&GfxEngine::onCharge, this, _1));
    bind(EventUID::CancelCharge, boost::bind(&GfxEngine::onCancelCharge, this, _1));
    bind(EventUID::Block, boost::bind(&GfxEngine::onBlock, this, _1));
    bind(EventUID::CancelBlock, boost::bind(&GfxEngine::onCancelBlock, this, _1));
    bind(EventUID::EndBlockPhase, boost::bind(&GfxEngine::onEndBlockPhase, this, _1));



    attrs =
      new MovableTextOverlayAttributes(
        "Attrs1",
        mCamera,
        "DejaVuSans",16,ColourValue::White,"RedTransparent");

		mPuppetPos[ME] = Vector3(0, 5, -50);
    mPuppetPos[ENEMY] =
      Vector3(mPuppetPos[ME].x,
							mPuppetPos[ME].y,
							mPuppetPos[ME].z + 100);

		setupSceneManager();
    setupViewports();
    setupCamera();
	  setupSky();
	  //setupWater();
    setupTerrain();
    setupLights();

    mTrayMgr = new SdkTrayManager("Elementum", mRenderWindow, InputManager::getSingletonPtr()->getMouse(), 0);

		mUpdate = &GfxEngine::updateNothing;
		mSelected = 0;
		fSetup = true;
		return fSetup;
	}

	bool GfxEngine::setupCombat() {

		mLog->infoStream() << "preparing combat scene";

    mPlayer = Combat::getSingleton().getPuppet();
    for (auto puppet : Combat::getSingleton().getPuppets())
      if (puppet != mPlayer) {
        mEnemy = puppet;
        break;
      }

    assert(mPlayer && mEnemy);

		//mPlayers.push_back(Combat::getSingleton().getPuppets().front()->getName());
		//mPlayers.push_back(Combat::getSingleton().getPuppets().back()->getName());

    setupNodes();
    setupWaypoints();


		std::ostringstream lNodeName;
		lNodeName << mPlayer->getName() << "_node_puppet";
		mCameraMan->setTarget(mSceneMgr->getSceneNode(lNodeName.str()));
    mCameraMan->setYawPitchDist(Ogre::Degree(180), Ogre::Degree(15), 150);
    //mCamera->yaw(Ogre::Degree(-180));

		//Combat::getSingleton().updateGfx();
		mUpdate = &GfxEngine::updateCombat;

		return true;
	}


	void GfxEngine::setCamera(const Ogre::String& inCameraName) {
		mCamera = mSceneMgr->createCamera(inCameraName);
	}

	SdkCameraMan* GfxEngine::getCameraMan() {
		return mCameraMan;
	}
	Ogre::Camera* GfxEngine::getCamera() {
		return mCamera;
	}

	Ogre::Root* GfxEngine::getRoot() {
		return mRoot;
	}

	Ogre::SceneManager* GfxEngine::getSceneMgr() {
		return mSceneMgr;
	}

  Ogre::RenderWindow* GfxEngine::getWindow() {
    return mRenderWindow;
  }

	Ogre::Viewport* GfxEngine::getViewport() {
		return mViewport;
	}

	void GfxEngine::updateNothing(unsigned long lTimeElapsed) {

	};

	void GfxEngine::updateCombat(unsigned long lTimeElapsed) {
		mCameraMan->update(lTimeElapsed);
		// update our good tray manager
    evt.timeSinceLastFrame = evt.timeSinceLastEvent = lTimeElapsed;
    mTrayMgr->frameRenderingQueued(evt);

		using namespace Ogre;

    // clean up updatees marked for removal
    updatees_t::iterator itr = mUpdatees.begin();
    /*while (!cleaned) {
      if ((itr->second) == false) {
        mUpdatees.erase(itr);
        itr = mUpdatees.begin();
      }

      if (itr == mUpdatees.end())
        cleaned = true;

      ++itr;
    }*/

    for (itr = mUpdatees.begin(); itr != mUpdatees.end(); ++itr)
      if (itr->second)
        itr->first->update(lTimeElapsed);

    RectLayoutManager m(0,0,
      mCamera->getViewport()->getActualWidth(),
      mCamera->getViewport()->getActualHeight());
    m.setDepth(0);

    int visible=0;
    MovableTextOverlay *p=0;
    std::list<Renderable*>::const_iterator r;
    for (r = mRenderables.begin(); r != mRenderables.end(); ++r) {
      (*r)->updateAnimations(lTimeElapsed);

      p = (*r)->getText();

      if (p->isHidden())
        continue;

      p->update(lTimeElapsed);

      if (p->isOnScreen()) {
      visible++;

      RectLayoutManager::Rect r(	p->getPixelsLeft(),
                p->getPixelsTop()+10,
                p->getPixelsRight(),
                p->getPixelsBottom()+10);

      RectLayoutManager::RectList::iterator it = m.addData(r);
      if (it != m.getListEnd())
      {
      //~ p->setPixelsTop((*it).getTop());
      p->enable(true);
      }
      else
      p->enable(false);
      }
      else
      p->enable(false);
    }


		//mCaelumSystem->updateSubcomponents(lTimeElapsed);
		/*mCaelumSystem->notifyCameraChanged(mCamera);
		*/


		/*
		bool mFly = false;
		if (!mFly)
		{
		 */
			// clamp to terrain
			/*Vector3 camPos = mCamera->getPosition();
			Ray ray;
			ray.setOrigin(Vector3(camPos.x, 10000, camPos.z));
			ray.setDirection(Vector3::NEGATIVE_UNIT_Y);*/

			/*TerrainGroup::RayResult rayResult = mSceneLoader->getTerrainGroup()->rayIntersects(ray);
			Real distanceAboveTerrain = 50;
			Real fallSpeed = 300;
			Real newy = camPos.y;
			if (rayResult.hit)
			{
				if (camPos.y > rayResult.position.y + distanceAboveTerrain)
				{
					mFallVelocity += lTimeElapsed * 20;
					mFallVelocity = std::min(mFallVelocity, fallSpeed);
					newy = camPos.y - mFallVelocity * lTimeElapsed;

				}
				newy = std::max(rayResult.position.y + distanceAboveTerrain, newy);
				mCamera->setPosition(camPos.x, newy, camPos.z);

			}*/
		//}
	};

	void GfxEngine::update(unsigned long lTimeElapsed) {
		processEvents();

		(this->*mUpdate)(lTimeElapsed);
	}

	bool GfxEngine::cleanup() {
	  mLog->infoStream() << "Cleaning up";

		return true;
	}


  void GfxEngine::setupSceneManager()
  {
	  mLog->debugStream() << "setting up SceneManager";



	  mViewport = mRenderWindow->addViewport(mCamera);


	  Ogre::CompositorManager& compMgr = Ogre::CompositorManager::getSingleton();
		compMgr.registerCompositorLogic("HDR", new HDRLogic);

	  Ogre::CompositorManager::getSingleton().addCompositor(mViewport, "HDR", 0);
	  Ogre::CompositorManager::getSingleton().setCompositorEnabled(mViewport, "HDR", true);

	  //mCameraMan->setStyle(OgreBites::CS_FREELOOK);

	  Ogre::Vector3 lPos = mPuppetPos[ME];
	  //mCamera->setPosition(lPos.x, lPos.y+2, lPos.z-20);
	  //mCamera->lookAt(mPuppetPos[ME]);

	  mCameraMan = new OgreBites::SdkCameraMan(mCamera);
	  //mCameraMan->setTopSpeed(50);
	  mCameraMan->setStyle(OgreBites::CS_ORBIT);

	  //mCamera->yaw(Ogre::Radian(-45));


	  //mWindow->setDebugText(getProperty("Robot","Life"));

	  //mSceneMgr->setShadowTechnique(Ogre::SHADOWTYPE_STENCIL_MODULATIVE);
  };

  void GfxEngine::setupViewports()
  {
		mLog->debugStream() << "setting up viewports";
    mViewport->setBackgroundColour(Ogre::ColourValue(0,0,0));
    // Alter the camera aspect ratio to match the viewport
    mCamera->setAspectRatio(Ogre::Real(mViewport->getActualWidth()) / Ogre::Real(mViewport->getActualHeight()));
  };

  void GfxEngine::setupLights()
  {

    mSceneMgr->setAmbientLight(Ogre::ColourValue(0,0,0));

    Ogre::ColourValue fadeColour(0, 0, 0);
    mSceneMgr->setFog(Ogre::FOG_EXP2, fadeColour, 0.0022);

    Ogre::Vector3 pos;
    Ogre::ColourValue dcol(0.9f,0.9f,0.9f);
    Ogre::ColourValue scol(0.6f,0.6f,0.6f);

    pos = mPuppetPos[ME];
    pos.y += 450;
	  mLog->debugStream() << "setting up lights";
    Ogre::Light *light;
    /* now let's setup our light so we can see the shizzle */
    light = mSceneMgr->createLight("Light1");
    light->setType(Ogre::Light::LT_POINT);
    light->setPosition(pos);
    //light->setDirection(pos.x, 0, pos.z);
    light->setDiffuseColour(dcol);
    light->setSpecularColour(scol);

    light = mSceneMgr->createLight("Lightzz");
    light->setType(Ogre::Light::LT_DIRECTIONAL);
    light->setDirection(Vector3(0,1,0));
    light->setDiffuseColour(dcol);
    light->setSpecularColour(scol);

	  light = mSceneMgr->createLight("Light2");
    light->setType(Ogre::Light::LT_SPOTLIGHT);
    pos = Vector3(0,250, 0);
    //pos.y += 250;
    light->setPosition(pos);
    light->setDiffuseColour(dcol);
    light->setSpecularColour(scol);

	  light = mSceneMgr->createLight("Light4");
    light->setType(Ogre::Light::LT_POINT);
    pos = mPuppetPos[ENEMY];
    pos.y += 250;
    pos.z -= 50;
    light->setPosition(pos);
    //light->setDirection(0,0,100);
    light->setDiffuseColour(dcol);
    light->setSpecularColour(scol);

    /*light = mSceneMgr->createLight("Light5");
    light->setType(Ogre::Light::LT_POINT);
    light->setPosition(500, 350, 100);
    //light->setDirection(500,0,100);
    light->setDiffuseColour(col);
    light->setSpecularColour(col);*/

	  /*col = Ogre::ColourValue(1, 0.4f, 0.4f);
    light = mSceneMgr->createLight("LightDirectional");
    light->setType(Ogre::Light::LT_DIRECTIONAL);
    light->setDirection(Vector3(0,-1,0));
    light->setDiffuseColour(col);
    light->setSpecularColour(col);*/

    /*pos.z -= 10;
    col = Ogre::ColourValue(1,0,0);
    light = mSceneMgr->createLight("Light3");
    light->setType(Ogre::Light::LT_POINT);
    light->setPosition(pos);
    light->setDirection(mPuppetPos[ME]);
    light->setDiffuseColour(col);
    light->setSpecularColour(col);   */
      /*light = mSceneMgr->createLight("Light2");
	 light->setType(Ogre::Light::LT_POINT);
	 light->setPosition(Vector3(500, 500, 1000));
	 light->setDirection(Vector3(0,0,1200));
	 light->setDiffuseColour(1.0, 1.0, 1.0);
	 light->setSpecularColour(1.0, 1.0, 1.0);*/
  };

  void createSphere(const std::string& strName, const float r, const int nRings = 16, const int nSegments = 16)
 {
   using namespace Ogre;
     MeshPtr pSphere = MeshManager::getSingleton().createManual(strName, ResourceGroupManager::DEFAULT_RESOURCE_GROUP_NAME);
     SubMesh *pSphereVertex = pSphere->createSubMesh();

     pSphere->sharedVertexData = new VertexData();
     VertexData* vertexData = pSphere->sharedVertexData;

     // define the vertex format
     VertexDeclaration* vertexDecl = vertexData->vertexDeclaration;
     size_t currOffset = 0;
     // positions
     vertexDecl->addElement(0, currOffset, VET_FLOAT3, VES_POSITION);
     currOffset += VertexElement::getTypeSize(VET_FLOAT3);
     // normals
     vertexDecl->addElement(0, currOffset, VET_FLOAT3, VES_NORMAL);
     currOffset += VertexElement::getTypeSize(VET_FLOAT3);
     // two dimensional texture coordinates
     vertexDecl->addElement(0, currOffset, VET_FLOAT2, VES_TEXTURE_COORDINATES, 0);
     currOffset += VertexElement::getTypeSize(VET_FLOAT2);

     // allocate the vertex buffer
     vertexData->vertexCount = (nRings + 1) * (nSegments+1);
     HardwareVertexBufferSharedPtr vBuf = HardwareBufferManager::getSingleton().createVertexBuffer(vertexDecl->getVertexSize(0), vertexData->vertexCount, HardwareBuffer::HBU_STATIC_WRITE_ONLY, false);
     VertexBufferBinding* binding = vertexData->vertexBufferBinding;
     binding->setBinding(0, vBuf);
     float* pVertex = static_cast<float*>(vBuf->lock(HardwareBuffer::HBL_DISCARD));

     // allocate index buffer
     pSphereVertex->indexData->indexCount = 6 * nRings * (nSegments + 1);
     pSphereVertex->indexData->indexBuffer = HardwareBufferManager::getSingleton().createIndexBuffer(HardwareIndexBuffer::IT_16BIT, pSphereVertex->indexData->indexCount, HardwareBuffer::HBU_STATIC_WRITE_ONLY, false);
     HardwareIndexBufferSharedPtr iBuf = pSphereVertex->indexData->indexBuffer;
     unsigned short* pIndices = static_cast<unsigned short*>(iBuf->lock(HardwareBuffer::HBL_DISCARD));

     float fDeltaRingAngle = (Math::PI / nRings);
     float fDeltaSegAngle = (2 * Math::PI / nSegments);
     unsigned short wVerticeIndex = 0 ;

     // Generate the group of rings for the sphere
     for( int ring = 0; ring <= nRings; ring++ ) {
         float r0 = r * sinf (ring * fDeltaRingAngle);
         float y0 = r * cosf (ring * fDeltaRingAngle);

         // Generate the group of segments for the current ring
         for(int seg = 0; seg <= nSegments; seg++) {
             float x0 = r0 * sinf(seg * fDeltaSegAngle);
             float z0 = r0 * cosf(seg * fDeltaSegAngle);

             // Add one vertex to the strip which makes up the sphere
             *pVertex++ = x0;
             *pVertex++ = y0;
             *pVertex++ = z0;

             Vector3 vNormal = Vector3(x0, y0, z0).normalisedCopy();
             *pVertex++ = vNormal.x;
             *pVertex++ = vNormal.y;
             *pVertex++ = vNormal.z;

             *pVertex++ = (float) seg / (float) nSegments;
             *pVertex++ = (float) ring / (float) nRings;

             if (ring != nRings) {
                                // each vertex (except the last) has six indices pointing to it
                 *pIndices++ = wVerticeIndex + nSegments + 1;
                 *pIndices++ = wVerticeIndex;
                 *pIndices++ = wVerticeIndex + nSegments;
                 *pIndices++ = wVerticeIndex + nSegments + 1;
                 *pIndices++ = wVerticeIndex + 1;
                 *pIndices++ = wVerticeIndex;
                 wVerticeIndex ++;
             }
         }; // end for seg
     } // end for ring

     // Unlock
     vBuf->unlock();
     iBuf->unlock();
     // Generate face list
     pSphereVertex->useSharedVertices = true;

     // the original code was missing this line:
     pSphere->_setBounds( AxisAlignedBox( Vector3(-r, -r, -r), Vector3(r, r, r) ), false );
     pSphere->_setBoundingSphereRadius(r);
         // this line makes clear the mesh is loaded (avoids memory leaks)
         pSphere->load();
  }
  void GfxEngine::setupSky() {

    // skyz0rs
    //mLog->noticeStream() << "Setting up sky";
    //mSceneMgr->setSkyDome(true, "Elementum/Sky", 1, 1,5000,true);
    //mSceneMgr->setSkyBox(true, "Elementum/Sky", 5000, true);

    //~ createSphere("mySphereMesh", 1000, 64, 64);
    //~ Ogre::Entity* sphereEntity = mSceneMgr->createEntity("mySphereEntity", "mySphereMesh");
    //~ Ogre::SceneNode* sphereNode = mSceneMgr->getRootSceneNode()->createChildSceneNode();
    //~ sphereEntity->setMaterialName("Elementum/Sky");
    //~ sphereNode->attachObject(sphereEntity);

    //~ sphereNode->roll(Ogre::Degree(180));
#if 0 // __DISABLED__ not using Caelum

    Ogre::ResourceGroupManager::getSingleton().initialiseResourceGroup("Caelum");

    // Pick components to create in the demo.
    // You can comment any of those and it should still work
    // Trying to disable one of these can be useful in finding problems.
    Caelum::CaelumSystem::CaelumComponent componentMask;
    componentMask = static_cast<Caelum::CaelumSystem::CaelumComponent> (
            //Caelum::CaelumSystem::CAELUM_COMPONENT_SUN |
            Caelum::CaelumSystem::CAELUM_COMPONENT_MOON |
            Caelum::CaelumSystem::CAELUM_COMPONENT_SKY_DOME |
            //Caelum::CaelumSystem::CAELUM_COMPONENT_IMAGE_STARFIELD |
            Caelum::CaelumSystem::CAELUM_COMPONENT_POINT_STARFIELD |
            Caelum::CaelumSystem::CAELUM_COMPONENT_CLOUDS |
            0);
    //componentMask = CaelumSystem::CAELUM_COMPONENTS_DEFAULT;

    // Initialise CaelumSystem.
    mCaelumSystem = new Caelum::CaelumSystem (mRoot, mSceneMgr, componentMask);
    mCaelumSystem->setSceneFogDensityMultiplier(0.001f); // or some other small value.
    //mCaelumSystem->setManageSceneFog(true);
    // Set time acceleration.
    mCaelumSystem->getUniversalClock()->setTimeScale (512);

    // Register caelum as a listener.
    mRenderWindow->addListener(mCaelumSystem);
    mRoot->addFrameListener(mCaelumSystem);

    /*mCaelumSystem =
    new CaelumSystem(mRoot, mSceneMgr, CaelumSystem::CAELUM_COMPONENTS_ALL);

    mCaelumSystem->setManageAmbientLight(false);*/

    //CaelumPlugin::getSingleton().loadCaelumSystemFromScript(mCaelumSystem, "Eclipse");

    //mCaelumSystem->attachViewport(mViewport);
#endif

  };

  /*void GfxEngine::setupWater() {
// Hydrax initialization code ---------------------------------------------
		// ------------------------------------------------------------------------

        // Create Hydrax object
		mHydrax = new Hydrax::Hydrax(mSceneMgr, mCamera, mViewport);

		// Create our projected grid module
		Hydrax::Module::ProjectedGrid *mModule
			= new Hydrax::Module::ProjectedGrid(// Hydrax parent pointer
			                                    mHydrax,
												// Noise module
			                                    new Hydrax::Noise::Real(),
												// Base plane
			                                    Ogre::Plane(Ogre::Vector3(0,1,0), Ogre::Vector3(0,0,0)),
												// Normal mode
												Hydrax::MaterialManager::NM_VERTEX,
												// Projected grid options
										        Hydrax::Module::ProjectedGrid::Options());

        // Add some waves
        static_cast<Hydrax::Noise::Real*>(mModule->getNoise())->addWave(
                                                Ogre::Vector2(1.f,0.f),
                                                0.3f,
                                                10.f);
        static_cast<Hydrax::Noise::Real*>(mModule->getNoise())->addWave(
                                                Ogre::Vector2(0.85f,0.15f),
                                                0.15f,
                                                8.f);
        static_cast<Hydrax::Noise::Real*>(mModule->getNoise())->addWave(
                                                Ogre::Vector2(0.95f,0.1f),
                                                0.1f,
                                                7.f);

		// Set our module
		mHydrax->setModule(static_cast<Hydrax::Module::Module*>(mModule));

		// Load all parameters from config file
		// Remarks: The config file must be in Hydrax resource group.
		// All parameters can be set/updated directly by code(Like previous versions),
		// but due to the high number of customizable parameters, since 0.4 version, Hydrax allows save/load config files.
		mHydrax->loadCfg("HydraxDemo.hdx");

    // Create water
    mHydrax->create();
  };*/

  void GfxEngine::setupCamera()
  {
    mLog->debugStream() << "setting up cameras";
    Vector3 lCamPos;

    mCamera->setNearClipDistance( 10 );
    mCamera->setFarClipDistance( 10000 );
    /*lCamPos = mPuppetPos[ME];
    lCamPos.y += 800;
    lCamPos.z += 1000;

    lCamPos.x = 99;
    lCamPos.y = 189;
    lCamPos.z = -150;
    mCamera->setPosition(lCamPos);
    mCamera->lookAt(mPuppetPos[ME]);*/
	};

  void GfxEngine::setupTerrain()
  {

    //mSceneMgr->setWorldGeometry("terrain.cfg");
	  //~ mSceneLoader = new DotSceneLoader();
	  //~ mSceneLoader->parseDotScene("Elementum.scene",
								  //~ "General",
								  //~ mSceneMgr);
		//mCamera = mSceneMgr->getCamera("Camera#0");
	  //parseDotScene("Elementum.xml","General",mSceneMgr);

    /*
    char* lTerrainCfgPath = (char*)malloc(sizeof(char) * (strlen(PROJECT_ROOT) + 18));
    sprintf(lTerrainCfgPath, "%s/Resources/Config", PROJECT_ROOT);
    Ogre::ResourceGroupManager::getSingleton().addResourceLocation(lTerrainCfgPath, "FileSystem");

    Ogre::ResourceGroupManager::getSingleton().initialiseAllResourceGroups();



    mLog->noticeStream() << "Setting world geometry";
    mSceneMgr->setWorldGeometry("terrain.cfg");*/

    using namespace Ogre;
    Ogre::SceneNode* node;
    // create a floor mesh resource
		MeshManager::getSingleton().createPlane("floor", ResourceGroupManager::DEFAULT_RESOURCE_GROUP_NAME,
			Plane(Vector3::UNIT_Y, 0), 1024, 1024, 10, 10, true, 1, 10, 10, Vector3::UNIT_Z);
		/*MeshManager::getSingleton().createPlane("ceiling", ResourceGroupManager::DEFAULT_RESOURCE_GROUP_NAME,
			Plane(Vector3::UNIT_Y, 1024), 1024, 1024, 10, 10, true, 1, 10, 10, Vector3::UNIT_Z);
		MeshManager::getSingleton().createPlane("lwall", ResourceGroupManager::DEFAULT_RESOURCE_GROUP_NAME,
			Plane(Vector3::UNIT_X, 512), 1024, 1024, 10, 10, true, 1, 10, 10, Vector3::UNIT_Y);
		MeshManager::getSingleton().createPlane("rwall", ResourceGroupManager::DEFAULT_RESOURCE_GROUP_NAME,
			Plane(Vector3::UNIT_X, -512), 1024, 1024, 10, 10, true, 1, 10, 10, Vector3::UNIT_Y);
    MeshManager::getSingleton().createPlane("fwall", ResourceGroupManager::DEFAULT_RESOURCE_GROUP_NAME,
			Plane(Vector3::UNIT_Z, -512), 1024, 1024, 10, 10, true, 1, 10, 10, Vector3::UNIT_Y);
    MeshManager::getSingleton().createPlane("bwall", ResourceGroupManager::DEFAULT_RESOURCE_GROUP_NAME,
			Plane(Vector3::UNIT_Z, 512), 1024, 1024, 10, 10, true, 1, 10, 10, Vector3::UNIT_Y);*/

		// create a floor entity, give it a material, and place it at the origin
    Ogre::Entity* ent = mSceneMgr->createEntity("Floor", "floor");
    ent->setMaterialName("Elementum/Terrain/Floor");
		ent->setCastShadows(false);
    mSceneMgr->getRootSceneNode()->attachObject(ent);

    /*ent = mSceneMgr->createEntity("Ceiling", "ceiling");
    ent->setMaterialName("Elementum/Terrain/Floor");
		ent->setCastShadows(false);
    mSceneMgr->getRootSceneNode()->attachObject(ent);

    ent = mSceneMgr->createEntity("LeftWall", "lwall");
    ent->setMaterialName("Elementum/Terrain/Floor");
		ent->setCastShadows(false);
    node = mSceneMgr->getRootSceneNode()->createChildSceneNode();
    node->setPosition(Vector3(0,512,0));
    node->attachObject(ent);

    ent = mSceneMgr->createEntity("RightWall", "rwall");
    ent->setMaterialName("Elementum/Terrain/Floor");
		ent->setCastShadows(false);
    node = mSceneMgr->getRootSceneNode()->createChildSceneNode();
    node->setPosition(Vector3(0,512,0));
    node->attachObject(ent);

    ent = mSceneMgr->createEntity("FrontWall", "fwall");
    ent->setMaterialName("Elementum/Terrain/Floor");
		ent->setCastShadows(false);
    node = mSceneMgr->getRootSceneNode()->createChildSceneNode();
    node->setPosition(Vector3(0,512,0));
    node->attachObject(ent);

    ent = mSceneMgr->createEntity("BackWall", "bwall");
    ent->setMaterialName("Elementum/Terrain/Floor");
		ent->setCastShadows(false);
    node = mSceneMgr->getRootSceneNode()->createChildSceneNode();
    node->setPosition(Vector3(0,512,0));
    node->attachObject(ent);*/
  };



  void GfxEngine::setupNodes()
  {
		mLog->debugStream() << "setting up nodes";
        /*
		 * Set up 22 nodes, 2 for Puppetes, and 20 for Units
		 * Each Puppet can own no more than 10 Units, and
		 * Entity positions are fixed throughout the game.
		 * Thus, the positions in which these Nodes are
		 * rendered are not to be changed.
		 * Worthy to note is that we will calculate each Node's
		 * position, and direction, using the respective owner
		 * Puppet's position.
		 */

		using Ogre::String;
    //Vector3 mPuppetScale, mUnitScale; // mPuppetPos[2], mDirection[2],

    // Define the scale to which the Models will be resized to
    mPuppetScale = Vector3(2.0f, 2.0f, 2.0f);
    mUnitScale = Vector3(1.0f, 1.0f, 1.0f);

    // Set up the direction to make each faction face the other
    mDirection[ME] = mPuppetPos[ENEMY];
    mDirection[ENEMY] = mPuppetPos[ME];
    /*Vector3 mDirectionShared =
      Vector3(mPuppetPos[ME].x,
              mPuppetPos[ME].y,
              (mPuppetPos[ME].z + mPuppetPos[ENEMY].z) / 2);*/

    // create Puppet nodes
    String ownerName, nodeName;
    Vector3 tmpPos, tmpDir;
    for (int i=0; i<2; i++)
    {
      if (i == ME)
      {
        ownerName = mPlayer->getName();
        tmpPos = mPuppetPos[ME];
        tmpDir = mDirection[ME];
      } else {
        ownerName = mEnemy->getName();
        tmpPos = mPuppetPos[ENEMY];
        tmpDir = mDirection[ENEMY];
      }
      tmpPos.y *= mPuppetScale.y;
      nodeName = ownerName + "_node_puppet";
      Ogre::SceneNode* tmpNode = createNode(nodeName, tmpPos, mPuppetScale, tmpDir);
      mLog->debugStream() << "Node: " << nodeName << " at: (" << tmpPos.x << ", " << tmpPos.y << ", " << tmpPos.z << ")";
      if (i == ENEMY) {
        tmpNode->yaw(Ogre::Degree(180));
      }
      /*
      tmpNode->pitch(Ogre::Degree(-90.0f));
      tmpNode->roll(Ogre::Degree(75.0f));
      */
    };

    // create Unit nodes
    Ogre::Real unitMargin, packMargin, posFrontier, posRear;
    for (int owner=0; owner<2; owner++)
    {
      if (owner == ME)
      {
        // start by creating client's nodes
        ownerName = mPlayer->getName();
        // define the starting position of the first node
        tmpPos = mPuppetPos[ME];
        tmpDir = mDirection[ME];
      } else {
        ownerName = mEnemy->getName();
        // define the starting position of the opponent first node
        tmpPos = mPuppetPos[ENEMY];
        tmpDir = mDirection[ENEMY];
      }

      tmpPos.x += 10; // margin from the puppet
      tmpPos.z += (owner == ME) ? -50 : 50;

      unitMargin = 10; // separate units by unitMargin space units on X axis
      packMargin = 75; // separate "packs" of units by packMargin space units on X axis
      posFrontier = tmpPos.z;
      posRear = posFrontier+15; // more on this below

      for (int i=0; i<10; i++)
      {

        /* define Unit position now;
         * Units will be grouped in packs of 5, resulting in
         * 2 "packs" of units for each Puppet, one to the left, other to the
         * right of Her. Units should be standing behind the Puppet,
         * a margin of 100 space units.
         * However, each pack of units will be split in 2 rows, Frontier and Rear;
         * this way, we have a nicely aligned pack of 3 units in front,
         * and 2 in the back, split up from the other group.
         */

				// separate our "packs"
				if (i == 5)
				{
					tmpPos.x -= packMargin;
					tmpPos.z = posFrontier;
					unitMargin *= -1; // negated so that nodes grow in the opposite direction
				}

				// split our pack into 2 rows
				tmpPos.z = (tmpPos.z == posFrontier) ? posRear : posFrontier;
				tmpPos.x += unitMargin;
				nodeName = ownerName + "_node_" + Ogre::StringConverter::toString(i);
				SceneNode* tmpNode = createNode(nodeName, tmpPos, mUnitScale, tmpDir);
        if (owner == ENEMY) {
          tmpNode->yaw(Ogre::Degree(180));
        }
				mLog->debugStream() << "Node: " << nodeName << " at: (" << tmpPos.x << ", " << tmpPos.y << ", " << tmpPos.z << ")";
      }; // end of for(i)
    }; // end of for(owner)

    // DEBUG ONLY
    // this is used because the models we use right now are bugged
    //mSceneMgr->getSceneNode("host_node_puppet")->setScale(Vector3(5,5,5));
    //mSceneMgr->getSceneNode("client_node_puppet")->setScale(Vector3(60,60,60));

    /* Create "combat" nodes now
     * These are shared throughout the game by all the Units
     * as the names describe, each node is attached to a certain - fixed -
     * position in which a certain action is taken
     */
    Vector3 offensePos, defensePos[2];
    offensePos =
    Vector3(mPuppetPos[ME].x,
            mPuppetPos[ME].y,
            (mPuppetPos[ME].z+mPuppetPos[ENEMY].z)/2);

    //~ defensePos[ME] =
    //~ Vector3(mPuppetPos[ME].x,
            //~ mPuppetPos[ME].y,
            //~ mPuppetPos[ME].z+5);

    //~ defensePos[ENEMY] =
    //~ Vector3(mPuppetPos[ENEMY].x,
            //~ mPuppetPos[ENEMY].y,
            //~ mPuppetPos[ENEMY].z-5);

    defensePos[ME] =
    Vector3(offensePos.x,
            offensePos.y,
            offensePos.z-5);

    defensePos[ENEMY] =
    Vector3(offensePos.x,
            offensePos.y,
            offensePos.z+5);

    Ogre::String tmpName[3] = {
	    "shared_node_offense",
	    mPlayer->getName() + "_node_defense",
	    mEnemy->getName() + "_node_defense"
		};

    createNode(tmpName[0],   offensePos,        mUnitScale, mDirection[ME]);
    createNode(tmpName[1],   defensePos[ME],    mUnitScale, mDirection[ENEMY]);
    createNode(tmpName[2],   defensePos[ENEMY], mUnitScale, mDirection[ME]);

  };

/* MOVING FUNCTIONS */
  void GfxEngine::createWaypoint(
    int inOwner,
    int inNode,
    std::string inOwnerName,
    std::string inOpponentName)
  {
    // Create the walking list
    // every unit "node" has 5 spots to move to
    // 1) Passive position (default, starts at it)
    // 2) Charging position (moved to when unit is told to attack/defend)
    // 3) Defensive position (1 shared node for all units under hero's control for defense)
    // 4) Offensive position (shared throughout the combat objects, there can be only 1 attacker at a time)
    // 5) Attack position (which is the defensive spot for the opponent hero in relation to the unit)
    //std::cout<<"Im setting up waypoints for"<<nodeOwner<<"@"<<nodeID<<"\n";

    std::vector<Ogre::Vector3>* mWalklist = &mWaypoints[inOwner][inNode];

    Vector3 passivePos, chargingPos, defensePos, offensePos, attackPos;
    Ogre::Real margin = 100;
    Ogre::String nodeName, heroNodeName;

    if (inOwner == ME)
    {
     //ownerName = Combat::getSingletonPtr()->getPuppetName();
     //opponentName = "enemy";
     margin *= -1;
     //~ mWalklist = &hWalklist[inIdNode];
    } else {
     //ownerName = "enemy";
     //opponentName = "player";
     //~ mWalklist = &cWalklist[inIdNode];
    }

    nodeName = inOwnerName + "_node_" + Ogre::StringConverter::toString(inNode);
    heroNodeName = inOwnerName + "_node_puppet";

    // Passive position
    passivePos = mSceneMgr->getSceneNode(nodeName)->getPosition();
    //passivePos.y /= (mPuppetScale.y);

    // Charging position
    chargingPos = passivePos;
    chargingPos.z = mSceneMgr->getSceneNode(inOwnerName + "_node_puppet")->getPosition().z;

    // Defence position
    defensePos = mSceneMgr->getSceneNode(inOwnerName + "_node_defense")->getPosition();
    defensePos.y = passivePos.y;

    // Offence position
    offensePos = mSceneMgr->getSceneNode("shared_node_offense")->getPosition();
    offensePos.y = passivePos.y;

    // Attack position
    attackPos = mSceneMgr->getSceneNode(inOpponentName + "_node_puppet")->getPosition();
    attackPos.z += (inOwner == ME) ? -5 : 5;
    attackPos.y = passivePos.y;

    std::cout
      << "Waypoint for node: " << inOwner << "#" << inNode << ":\n"
      << "\tIdle: " << passivePos.x << "," << passivePos.y << "," << passivePos.z << "\n"
      << "\tCharging: " << chargingPos.x << "," << chargingPos.y << "," << chargingPos.z << "\n"
      << "\tBlock: " << defensePos.x << "," << defensePos.y << "," << defensePos.z << "\n"
      << "\tOffense: " << offensePos.x << "," << offensePos.y << "," << offensePos.z << "\n"
      << "\tAttack: " << attackPos.x << "," << attackPos.y << "," << attackPos.z << "\n";

    (*mWalklist).push_back(passivePos);
    (*mWalklist).push_back(chargingPos);
    (*mWalklist).push_back(defensePos);
    (*mWalklist).push_back(offensePos);
    (*mWalklist).push_back(attackPos);
  };

  Ogre::SceneNode* GfxEngine::createNode(String& inName, Vector3& inPosition, Vector3& inScale, Vector3& inDirection, Ogre::SceneNode* inParent)
  {
      Ogre::SceneNode* mNode;
      if (!inParent)
          inParent = mSceneMgr->getRootSceneNode();

      mNode = inParent->createChildSceneNode(inName, inPosition);
      mNode->setScale(inScale);
      //mNode->lookAt(inDirection, Ogre::Node::TS_WORLD);
      //~ mNode->showBoundingBox(true);

      //mNode = NULL;
      return mNode;
  };

  Ogre::SceneNode* GfxEngine::renderEntity(Renderable* inRenderable)
  {
    Entity* inEntity = inRenderable->getEntity();
    mLog->debugStream() << "rendering entity " << inEntity->getName();

    bool isPuppet = inEntity->getRank() == PUPPET;
    bool found = false;
    SceneNode* mNode;
    Ogre::Entity* mEntity;

    String entityName = "", nodeName = "", ownerName = "";

    //ownerName = (inEntity->getOwner() == ME) ? "host" : "client";
    //ownerName = stringify(inEntity->getUID());
    ownerName = inEntity->getOwner()->getName();
    if (isPuppet)
    {
      entityName = ownerName + "_entity_puppet";
      nodeName = ownerName + "_node_puppet";
      /* TO_DO: add functionality to retrieve entitie's mesh pathes from factory */

      mLog->debugStream() << "puppet node name " << nodeName;
      mNode = mSceneMgr->getSceneNode(nodeName);

      mEntity = mSceneMgr->createEntity(entityName, inEntity->getMesh());

      mLog->debugStream() << "attaching user data to ogre entity";
      mEntity->setUserAny(Ogre::Any(inRenderable));

      mLog->infoStream() << "Attaching puppet entity " << mEntity->getName() << " to node " << mNode->getName();
      mNode->attachObject(mEntity);

      MovableTextOverlay *p =
        new MovableTextOverlay(mEntity->getName() + "_text"," Robot ", mEntity, attrs);
      p->enable(false); // make it invisible for now
      p->setUpdateFrequency(0.01);// set update frequency to 0.01 seconds
      inRenderable->setText(p);
      p = 0;

      inRenderable->attachSceneNode(mNode);
      inRenderable->attachSceneObject(mEntity);

      inRenderable->setup(mSceneMgr);

      static_cast<CPuppet*>(inEntity)->updateTextOverlay();

      mRenderables.push_back(inRenderable);

      return mSceneMgr->getSceneNode(nodeName);
    };

    // handle units
    entityName = ownerName + "_entity_" + stringify<int>(inEntity->getUID());
    // now we need to locate the nearest empty SceneNode
    // to render our Entity in
    int idNode;
    for (int i=0; i<10; i++)
    {
      nodeName = ownerName + "_node_" + Ogre::StringConverter::toString(i);
      mNode = mSceneMgr->getSceneNode(nodeName);
      if (mNode->numAttachedObjects() == 0) {
        idNode = i;
        if (i < 5 &&
          mSceneMgr->getSceneNode(ownerName + "_node_" + Ogre::StringConverter::toString(i+5))->numAttachedObjects() == 0
        ) {
          idNode += 5;
          mNode = mSceneMgr->getSceneNode(ownerName + "_node_" + Ogre::StringConverter::toString(i+5));
        }
        found = true;
        break;
      }
    };

    // now we've got our Node, create the Entity & attach
    if (found)
    {
      /* TO_DO */
      std::string _meshPath = "";
      mEntity = mSceneMgr->createEntity(entityName, inEntity->getMesh());
      mEntity->setUserAny(Ogre::Any(inRenderable));
      mNode->attachObject(mEntity);

      MovableTextOverlay *p =
        new MovableTextOverlay(mEntity->getName() + "_text"," Robot ", mEntity, attrs);
      p->enable(false); // make it invisible for now
      p->setUpdateFrequency(0.01);// set update frequency to 0.01 seconds
      inRenderable->setText(p);
      p = 0;

      inRenderable->attachSceneNode(mNode);
      inRenderable->attachSceneObject(mEntity);

      inRenderable->setup(mSceneMgr);

      static_cast<CUnit*>(inEntity)->
        setWaypoints(&mWaypoints[inEntity->getOwner() == mPlayer ? 0 : 1][idNode]);

      static_cast<CUnit*>(inEntity)->updateTextOverlay();

      mRenderables.push_back(inRenderable);

      return mNode;
      //mSceneMgr->getSceneNode(nodeName)->setUserAny(
    } else {
      mLog->errorStream() << "Could not attach Entity! No empty SceneNodes available";
      return NULL;
    }
    //LOG_F(__FUNCTION__);
  };


  bool GfxEngine::attachToScene(Renderable* inEntity)
  {
      //bool isPuppet = (inEntity->getRank() == 0) ? true : false;

      assert(inEntity->getEntity());
      // render the object
      renderEntity(inEntity);
  /*
      // create and attach interface stats overlay
      if (!isPuppet)
          inEntity->attachSceneOverlay( mInterface->createUnitOverlay(inEntity) );
      else
          inEntity->attachSceneOverlay( mInterface->createPuppetOverlay(inEntity) );
  */
  return true;
  };



  void GfxEngine::detachFromScene(Renderable* inRenderable)
  {
    dehighlight();

    Entity* inEntity = inRenderable->getEntity();

    Ogre::String ownerName = stringify(inEntity->getUID());// == ID_HOST) ? "host" : "client";
    Ogre::String nodeName = ownerName + "_node_";
    Ogre::String entityName = ownerName + "_entity_" + Ogre::StringConverter::toString(inEntity->getUID());
    Ogre::SceneNode* mTmpNode = NULL;

      //for (int i=0; i<10; i++)
      //{
  //LOG("I'm loooping now.");

  //nodeName = ownerName + "_node_" + Ogre::StringConverter::toString(i);
  // retrieve our node
  mTmpNode = inRenderable->getSceneNode();

  // retrieve the entity so we can destroy it after we detach it from node
  //Ogre::Entity* mOgreEntity = (Ogre::Entity*)inEntity->getSceneObject();
  //Pixy::Entity* mUnit = Ogre::any_cast<Pixy::Entity*> (mOgreEntity->getUserAny());
  // first let's check if our entity belongs to this scenenode
  //if (mUnit->getIdEntity() == inEntity->getIdEntity())
  //if (mOgreEntity->getParentSceneNode() == mTmpNode)
  //{
  // this is our node

  //Ogre::MovableObject* mOgreEntity = mTmpNode->getAttachedObject(entityName);
  // detach from node
  //mInterface->destroyUnitOverlay(inEntity->getSceneObject());

  // move the node back to its original spot
  mTmpNode->translate(static_cast<CUnit*>(inRenderable->getEntity())->mWaypoints->front());

  mLog->debugStream() << "I'm detaching Entity '" << inEntity->getName() << "' from SceneNode : " + mTmpNode->getName();
  mTmpNode->showBoundingBox(false);
  mTmpNode->detachObject(inRenderable->getSceneObject());
  // destroy entity

  //                LOG("I'm destroying Entity : " + mOgreEntity->getName());
  mSceneMgr->destroyEntity((Ogre::Entity*)inRenderable->getSceneObject());

  mRenderables.remove(inRenderable);

  if (inRenderable->getEntity()->getRank() != PUPPET)
    mUpdatees.erase(static_cast<CUnit*>(inRenderable->getEntity()));


  // translate the node back to its original position
  /*
  int idNode = parseIdNodeFromName(mTmpNode->getName());
  deque<Vector3>* mWalklist = (inEntity->getOwner() == ID_HOST) ? &hWalklist[idNode] : &cWalklist[idNode];
  mTmpNode->setPosition((*mWalklist).at(POS_PASSIVE));
  */
  //break;
  //mTmpNode->setUserAny((Ogre::Any)NULL);
  //};
  // LOG_F(__FUNCTION__);
      //};

    inEntity = 0;
  }

  void GfxEngine::setupWaypoints()
  {
    for (int i=0; i<10; i++)
    {
      createWaypoint(ME, i, mPlayer->getName(), mEnemy->getName());
      createWaypoint(ENEMY, i, mEnemy->getName(), mPlayer->getName());
    };
  };

	void GfxEngine::mouseMoved( const OIS::MouseEvent &e )
	{
		if (mCameraMan)
			mCameraMan->injectMouseMove(e);
	}

	void GfxEngine::mousePressed( const OIS::MouseEvent &e, OIS::MouseButtonID id )
	{
		if (mCameraMan)
			mCameraMan->injectMouseDown(e, id);

    //~ return;
    CEGUI::Point mousePos = CEGUI::MouseCursor::getSingleton().getPosition();

    if (id != OIS::MB_Left)
      return;

    // Setup the ray scene query, use CEGUI's mouse position
    Ogre::Ray mouseRay =
      mCamera->getCameraToViewportRay(
        mousePos.d_x/float(e.state.width),
        mousePos.d_y/float(e.state.height)
      );

    Ogre::RaySceneQuery *mRaySceneQuery = mSceneMgr->createRayQuery(Ogre::Ray());
    mRaySceneQuery->setRay(mouseRay);
    mRaySceneQuery->setSortByDistance(true);
    // Execute query
    Ogre::RaySceneQueryResult &result = mRaySceneQuery->execute();
    Ogre::RaySceneQueryResult::iterator itr;


    //dehighlight(); // remove any unit selection
    bool found = false;
    for (itr = result.begin(); itr != result.end(); itr++)
    {
      mLog->infoStream() << "Ray target name: " << itr->movable->getName();
      if (itr->movable &&
         (itr->movable->getName().substr(0,6) != "Caelum") &&
         itr->movable->getName() != "" &&
         //~ itr->movable->getName() != "mySphereEntity" &&
         itr->movable->getName() != "Floor" &&
         itr->movable->getName() != "Ceiling" &&
         itr->movable->getName() != "LeftWall" &&
         itr->movable->getName() != "RightWall" &&
         itr->movable->getName() != "FrontWall" &&
         itr->movable->getName() != "BackWall") {

        Event e(EventUID::EntitySelected);
        e.Any = ((void*)Ogre::any_cast<Pixy::Renderable*>(itr->movable->getUserAny()));
        mEvtMgr->hook(e);

        found = true;

        break;

      }
    }
    if (!found)
      // ignore any terrain selection
      dehighlight();
	}

	void GfxEngine::mouseReleased( const OIS::MouseEvent &e, OIS::MouseButtonID id )
	{
		if (mCameraMan)
			mCameraMan->injectMouseUp(e, id);
	}

	void GfxEngine::keyReleased( const OIS::KeyEvent &e ) {
	  if (mCameraMan)
			mCameraMan->injectKeyUp(e);

    if (Combat::getSingleton().getPuppet() == 0)
      return;

    SceneNode* tmp = Combat::getSingleton().getPuppet()->getRenderable()->getSceneNode();
    switch (e.key) {
      case OIS::KC_F:
        tmp->yaw(Ogre::Degree(5));
      break;
      case OIS::KC_G:
        tmp->roll(Ogre::Degree(5));
      break;
      case OIS::KC_H:
        tmp->pitch(Ogre::Degree(5));
      break;
      case OIS::KC_E:
        static_cast<CUnit*>(mSelected->getEntity())->move(POS_READY);
        break;
      case OIS::KC_R:
        static_cast<CUnit*>(mSelected->getEntity())->move(POS_CHARGING);
        break;
      case OIS::KC_T:
        static_cast<CUnit*>(mSelected->getEntity())->move(POS_DEFENCE);
        break;
      case OIS::KC_Y:
        static_cast<CUnit*>(mSelected->getEntity())->move(POS_OFFENCE);
        break;
      case OIS::KC_U:
        static_cast<CUnit*>(mSelected->getEntity())->move(POS_ATTACK);
        break;
      case OIS::KC_I:
        if (mRenderables.front()->getText()->isHidden())
          mRenderables.front()->show();
        else
          mRenderables.front()->hide();
        break;
      case OIS::KC_K:
        if (mTrayMgr->areFrameStatsVisible())
          mTrayMgr->hideFrameStats();
        else
          mTrayMgr->showFrameStats(OgreBites::TL_BOTTOMRIGHT);
        break;
        break;
    }
	}
	void GfxEngine::keyPressed( const OIS::KeyEvent &e ) {
	  if (mCameraMan)
			mCameraMan->injectKeyDown(e);
	}

	void GfxEngine::highlight(Renderable* inEntity) {
	  dehighlight();
	  mSelected = inEntity;
	  mSelected->getSceneNode()->showBoundingBox(true);
	};

	void GfxEngine::dehighlight() {
	  if (!mSelected)
	    return;

	  mSelected->getSceneNode()->showBoundingBox(false);
	  mSelected = 0;
	};

  bool GfxEngine::onEntitySelected(const Event& inEvt) {
    return inBlockPhase ? onEntitySelectedBlock(inEvt) : onEntitySelectedAttack(inEvt);
  }
  bool GfxEngine::onEntitySelectedAttack(const Event& inEvt) {
    Pixy::Renderable* lRend = static_cast<Pixy::Renderable*>(inEvt.Any);
    Pixy::Entity* lEntity = lRend->getEntity();

    std::cout << "selected a unit with UID: " << lEntity->getUID() << "\n";
     // double click on an entity
    if (mSelected && mSelected->getEntity()->getUID() == lEntity->getUID()) {
      std::cout << "got a double click\n";
      if (lEntity->getRank() != PUPPET) {
        CUnit *lUnit = static_cast<CUnit*>(lEntity);

        // make sure the unit is owned by the player not the opponent
        if (lUnit->getOwner() != mPlayer) {
          return true;
        }

        // if the unit is moving to anywhere, don't do anything
        if (lUnit->isMoving())
          return true;

        // if the unit has 0 AP, don't do anything
        if (lUnit->getAP() == 0)
          return true;

        // if the unit is passive, make it charge
        if (lUnit->getPosition() == POS_READY) {
          Event req(EventUID::Charge);
          req.setProperty("UID", lEntity->getUID());
          NetworkManager::getSingleton().send(req);


        } else if (lUnit->getPosition() == POS_CHARGING) {
          Event req(EventUID::CancelCharge);
          req.setProperty("UID", lEntity->getUID());
          NetworkManager::getSingleton().send(req);

        }

        lUnit = 0;
        dehighlight();
      }

      return true;
    }

    highlight(lRend);

    return true;
  }
  bool GfxEngine::onEntitySelectedBlock(const Event& inEvt) {
    Pixy::Renderable* lRend = static_cast<Pixy::Renderable*>(inEvt.Any);
    Pixy::Entity* lEntity = lRend->getEntity();

    // if it's a puppet, just de-select
    if (lEntity->getRank() == PUPPET) {
      highlight(lRend);
      return true;
    }

    CUnit* lUnit = static_cast<CUnit*>(lEntity);

    // if an own unit is double-clicked and is blocking, cancel the block
    if (mSelected && mSelected == lRend) {
      if (lUnit->getOwner() == mPlayer && lUnit->getPosition() == POS_CHARGING) {
        Event req(EventUID::CancelBlock);
        req.setProperty("UID", lUnit->getUID());
        NetworkManager::getSingleton().send(req);
      }
      return true;
    }

    // only when an own unit is chosen and then a charging unit is chosen
    // will we trigger the block event
    if (mSelected &&
        mSelected->getEntity()->getOwner() == mPlayer &&
        lUnit->getOwner() != mPlayer)
    {
      Event req(EventUID::Block);
      req.setProperty("B", mSelected->getEntity()->getUID());
      req.setProperty("A", lUnit->getUID());
      NetworkManager::getSingleton().send(req);
    } else
      highlight(lRend);

    return true;
  }

  bool GfxEngine::onStartBlockPhase(const Event& inEvt) {
    if (Combat::getSingleton().getActivePuppet() != mPlayer) {
      inBlockPhase = true;
      mLog->debugStream() << "entered blocking phase";
    }

    return true;
  }

  bool GfxEngine::onCharge(const Event& inEvt) {
    mLog->infoStream() << "charging with a unit";

    CUnit* lUnit = Combat::getSingleton().getUnit(convertTo<int>(inEvt.getProperty("UID")));

    lUnit->move(POS_CHARGING);

    return true;
  }

  bool GfxEngine::onCancelCharge(const Event& inEvt) {
    mLog->infoStream() << "no longer charging with a unit";

    CUnit* lUnit = Combat::getSingleton().getUnit(convertTo<int>(inEvt.getProperty("UID")));

    lUnit->move(POS_READY);

    return true;
  }

  bool GfxEngine::onBlock(const Event& inEvt) {
    mLog->infoStream() << "blocking with a unit";

    CUnit* lUnit = Combat::getSingleton().getUnit(convertTo<int>(inEvt.getProperty("B")));

    lUnit->move(POS_CHARGING);

    return true;
  }

  bool GfxEngine::onCancelBlock(const Event& inEvt) {
    mLog->infoStream() << "no longer blocking with a unit";

    CUnit* lUnit = Combat::getSingleton().getUnit(convertTo<int>(inEvt.getProperty("B")));
    lUnit->move(POS_READY);

    return true;
  }

  bool GfxEngine::onEndBlockPhase(const Event& inEvt) {
    if (Combat::getSingleton().getActivePuppet() != mPlayer) {
      inBlockPhase = false;
      mLog->debugStream() << "no longer in blocking phase";
    }

    return true;
  }

  void GfxEngine::updateMe(CUnit* inUnit) {
    if (mUpdatees.find(inUnit) != mUpdatees.end()) {
      mUpdatees.find(inUnit)->second = true;
      return;
    }

    mUpdatees.insert(std::make_pair(inUnit, true));
  }

  void GfxEngine::stopUpdatingMe(CUnit* inUnit) {
    mUpdatees.find(inUnit)->second = false;
  }


}

/*
 * Copyright (C) Shroom Studios, Inc - All Rights Reserved
 *
 * Unauthorized copying of this file, via any medium is strictly prohibited
 * Proprietary and confidential
 *
 * Written by Ahmad Amireh <ahmad@shroom-studios.com>, September 2011
 */

#include "UIEngine.h"
#include "EventManager.h"
#include "PixyUtility.h"
#include "GameManager.h"
#include <boost/filesystem.hpp>
#include "cegui/AutoRepeatKey.h"

// CEGUI
// For some reason, on OS X CEGUI.h complains that CFBundleRef is undeclared
// so we include the CoreFoundation framework to get around that
// hack date: 09/07/2011
#if PIXY_PLATFORM == PIXY_PLATFORM_APPLE
# include <CoreFoundation/CoreFoundation.h>
#endif

#if PIXY_PLATFORM == PIXY_PLATFORM_APPLE
#include <CEGUI/CEGUI.h>
#include <CEGUI/CEGUISystem.h>
#include <CEGUI/CEGUISchemeManager.h>
//#include "CEGUIBase/ScriptingModules/LuaScriptModule/CEGUILua.h"
#include "CEGUI/RendererModules/Ogre/CEGUIOgreRenderer.h"
//#include "CEGUI/WindowRendererSets/Falagard/FalStaticText.h"
#else
#include <CEGUI/CEGUI.h>
#include <CEGUI/CEGUISystem.h>
#include <CEGUI/CEGUISchemeManager.h>
//#include "CEGUI/ScriptingModules/LuaScriptModule/CEGUILua.h"
#include "CEGUI/RendererModules/Ogre/CEGUIOgreRenderer.h"
//#include "CEGUI/WindowRendererSets/Falagard/FalStaticText.h"
#endif
#include <Ogre.h>
#include "Spell.h"
//#include "Combat.h"


namespace Pixy {
	UIEngine* UIEngine::_myUIEngine = NULL;

	UIEngine* UIEngine::getSingletonPtr() {
		if( !_myUIEngine ) {
		    _myUIEngine = new UIEngine();
		}

		return _myUIEngine;
	}

	UIEngine::UIEngine()  {
		mLog = new log4cpp::FixedContextCategory(PIXY_LOG_CATEGORY, "UIEngine");
		mLog->infoStream() << "firing up";
		fSetup = false;
	}

	UIEngine::~UIEngine() {
		if (mLog)
      mLog->infoStream() << "shutting down";

    cleanup();

		if (fSetup) {
			if (mLog)
      {
        mLog->debugStream() << "destroying all windows";
        delete mLog;
      }
      mLog = 0;
			fSetup = false;
		}
	}

	bool UIEngine::cleanup() {
    if (!fSetup)
      return true;

    delete mMyInput;
    mMyInput = 0;

    CEGUI::WindowManager::getSingleton().destroyAllWindows();

    mLog->debugStream() << "destroying system";
    mOgreRenderer->destroySystem();

    mOgreRenderer = NULL; mUISystem = NULL;

    fSetup = false;

		return true;
	}

	CEGUI::MouseButton UIEngine::convertButton(OIS::MouseButtonID buttonID)
	{
		switch (buttonID)
		{
			case OIS::MB_Left:
				return CEGUI::LeftButton;

			case OIS::MB_Right:
				return CEGUI::RightButton;

			case OIS::MB_Middle:
				return CEGUI::MiddleButton;

			default:
				return CEGUI::LeftButton;
		}
	}

	const char* UIEngine::getDataPathPrefix() const
	{
		static char dataPathPrefix[PATH_MAX];

		return dataPathPrefix;
	}

	bool UIEngine::setup() {
		if (fSetup)
			return true;

		mLog->infoStream() << "Setting up";

		mEvtMgr = EventManager::getSingletonPtr();

		mLog->infoStream() << "initting CEGUI system & CEGUIOgreRenderer.";

		if (CEGUI::System::getSingletonPtr())
			throw CEGUI::InvalidRequestException("OgreRenderer::bootstrapSystem: "
										  "CEGUI::System object is already initialised.");


		CEGUI::DefaultLogger* lUILog = new CEGUI::DefaultLogger();

    using boost::filesystem::path;
		std::string lUILogPath =
      (path(GameManager::getSingleton().getLogPath()) / "CEGUI.log").make_preferred().string();
		lUILog->setLogFilename(lUILogPath, true);

		mOgreRenderer = &CEGUI::OgreRenderer::bootstrapSystem();
		mUISystem = &CEGUI::System::getSingleton();

    lUILog = 0;

		// load resources
		loadResources();

		// load GUI scheme

		//~ CEGUI::SchemeManager::getSingleton().create( "TaharezLook.scheme" );
		//~ CEGUI::SchemeManager::getSingleton().create( "VanillaSkin.scheme" );
		//~ CEGUI::SchemeManager::getSingleton().create( "WindowsLook.scheme" );


		// load font and setup default if not loaded via scheme
		//CEGUI::FontManager::getSingleton().create("DejaVuSans-10.font");

		// set up defaults
		//~ CEGUI::System::getSingleton().setDefaultMouseCursor("TaharezLook", "MouseArrow");
		//CEGUI::System::getSingleton().setDefaultFont("DejaVuSans-10");

    // Create default ToolTip item
    CEGUI::System::getSingleton().setDefaultTooltip( (CEGUI::utf8*)"TaharezLook/Tooltip" );
    //~ CEGUI::Tooltip* ttip = CEGUI::System::getSingleton().getDefaultTooltip();
    //~ ttip->setMaxSize(CEGUI::UVector2(CEGUI::UDim(0, 600), CEGUI::UDim(0, 400)));

		mLog->infoStream() << "Set up!";

		//Combat::getSingletonPtr()->updateMe(getSingletonPtr());

    mMyInput = new MyInput();

		fSetup = true;
		return true;
	}


	bool UIEngine::loadResources() {

		mLog->infoStream() << "Loading resources...";
		using namespace Ogre;
		ResourceGroupManager& rgm = ResourceGroupManager::getSingleton();

		// add resource groups that we use
		//rgm.createResourceGroup("imagesets");
		//rgm.createResourceGroup("fonts");
		//rgm.createResourceGroup("layouts");
		//rgm.createResourceGroup("schemes");
		//rgm.createResourceGroup("looknfeels");
		//rgm.createResourceGroup("schemas");

		// add CEGUI sample framework datafile dirs as resource locations
		//ResourceGroupManager::getSingleton().addResourceLocation("./", "FileSystem");

		//char* dataPathPrefix = (char*)malloc(sizeof(char) * (strlen(PROJECT_ROOT) + 1 + strlen(PROJECT_RESOURCES) + 1 + strlen("ui") + 1));
		//sprintf(dataPathPrefix, "%s%s/ui", PROJECT_ROOT, PROJECT_RESOURCES);
		//char resourcePath[PATH_MAX];

		// for each resource type, set a resource group directory
		//sprintf(resourcePath, "%s/%s", dataPathPrefix, "schemes/");
		//ResourceGroupManager::getSingleton().addResourceLocation(resourcePath, "FileSystem", "schemes");
		//sprintf(resourcePath, "%s/%s", dataPathPrefix, "imagesets/");
		//ResourceGroupManager::getSingleton().addResourceLocation(resourcePath, "FileSystem", "imagesets");
		//sprintf(resourcePath, "%s/%s", dataPathPrefix, "fonts/");
		//ResourceGroupManager::getSingleton().addResourceLocation(resourcePath, "FileSystem", "fonts");
		//sprintf(resourcePath, "%s/%s", dataPathPrefix, "layouts/");
		//ResourceGroupManager::getSingleton().addResourceLocation(resourcePath, "FileSystem", "layouts");
		//sprintf(resourcePath, "%s/%s", dataPathPrefix, "looknfeel/");
		//ResourceGroupManager::getSingleton().addResourceLocation(resourcePath, "FileSystem", "looknfeels");
		//sprintf(resourcePath, "%s/%s", dataPathPrefix, "xml_schemas/");
		//ResourceGroupManager::getSingleton().addResourceLocation(resourcePath, "FileSystem", "schemas");

		//free(dataPathPrefix);

		// set the default resource groups to be used
		CEGUI::Imageset::setDefaultResourceGroup("imagesets");
		CEGUI::Font::setDefaultResourceGroup("fonts");
		CEGUI::Scheme::setDefaultResourceGroup("schemes");
		CEGUI::WidgetLookManager::setDefaultResourceGroup("looknfeels");
		CEGUI::WindowManager::setDefaultResourceGroup("layouts");

		// setup default group for validation schemas
		CEGUI::XMLParser* parser = CEGUI::System::getSingleton().getXMLParser();
		if (parser->isPropertyPresent("SchemaDefaultResourceGroup"))
			parser->setProperty("SchemaDefaultResourceGroup", "schemas");

		//ResourceGroupManager::getSingleton().initialiseAllResourceGroups();
		//ResourceGroupManager::getSingleton().initialiseResourceGroup("schemes");
		//ResourceGroupManager::getSingleton().initialiseResourceGroup("imagesets");
		//ResourceGroupManager::getSingleton().initialiseResourceGroup("fonts");
		//ResourceGroupManager::getSingleton().initialiseResourceGroup("layouts");
		//ResourceGroupManager::getSingleton().initialiseResourceGroup("looknfeels");
		//ResourceGroupManager::getSingleton().initialiseResourceGroup("schemas");

    CEGUI::AnimationManager::getSingleton().loadAnimationsFromXML("animations.xml");

		// load LUA script
		return true;
	}


	void UIEngine::update( unsigned long lTimeElapsed ) {
		mUISystem->injectTimePulse(lTimeElapsed * 0.001f);
    mMyInput->update(lTimeElapsed * 0.001f);

		processEvents();
	}

	bool UIEngine::mouseMoved( const OIS::MouseEvent &e )
	{
		return mUISystem->injectMouseMove(e.state.X.rel, e.state.Y.rel);
	}

	bool UIEngine::mousePressed( const OIS::MouseEvent &e, OIS::MouseButtonID id ) {
		return mUISystem->injectMouseButtonDown(convertButton(id));
    // CEGUI::Window *window = CEGUI::System::getSingletonPtr()->getWindowContainingMouse();
    // return (window && window->isVisible() && window->getAlpha() > 0.0f && (window->getType() != "DefaultWindow"));
	}

	bool UIEngine::mouseReleased( const OIS::MouseEvent &e, OIS::MouseButtonID id ) {
		return mUISystem->injectMouseButtonUp(convertButton(id));
    //CEGUI::Window *window = CEGUI::System::getSingletonPtr()->getWindowContainingMouse();
    //return (window && window->isVisible() && window->getAlpha() > 0.0f && (window->getType() != "DefaultWindow"));
  }

	void UIEngine::keyReleased( const OIS::KeyEvent &e ) {
    //mUISystem->injectKeyReleased(e);
    mMyInput->injectOISKey(false, e);
	}
	void UIEngine::keyPressed( const OIS::KeyEvent &e ) {
		//mUISystem->injectKeyDown(e.key);
		//mUISystem->injectChar(e.text);
    mMyInput->injectOISKey(true, e);
	}

  void UIEngine::setMargin(CEGUI::Window* win, CEGUI::UBox& margin) {
    using CEGUI::UDim;
    //win->setMargin(CEGUI::UBox(UDim(0,0),UDim(0,0),UDim(0,0),UDim(0,5)));
    win->setMargin(margin);
  }

  void UIEngine::refreshSize() {
    //~ using namespace CEGUI;
    //~ Window* sp = CEGUI::WindowManager::getSingletonPtr()->getWindow("Elementum/Combat/SpellPanel/Player");
    //~ UVector2 sz(sp->getSize());
    //~ sp->setSize(UVector2(UDim(0,0),UDim(0,0)));
    //~ sp->setSize(sz);
  }



  void UIEngine::connectAnimation(CEGUI::Window* inWindow, std::string inAnim) {

    CEGUI::AnimationInstance* instance = CEGUI::AnimationManager::getSingleton().instantiateAnimation(inAnim);
    // after we instantiate the animation, we have to set its target window
    instance->setTargetWindow(inWindow);
    inWindow->subscribeEvent(
      CEGUI::Window::EventShown,
      CEGUI::Event::Subscriber(&CEGUI::AnimationInstance::handleStart, instance));

  }

  void UIEngine::refreshTooltipSize(CEGUI::Window* inWindow, Spell* inSpell) {

    //std::cout << inWindow->getProperty("HorzExtent") << ", " << inWindow->getProperty("VertExtent") << "\n";
  }

  void UIEngine::reportLuaError(std::string const& inMsg)
  {
    std::string lWindowName = "Elementum/LuaErrorBox";
    CEGUI::WindowManager *lWindowMgr = CEGUI::WindowManager::getSingletonPtr();
    if (!lWindowMgr->isWindowPresent(lWindowName))
    {
      mLog->errorStream() << "Could not find LuaErrorBox window, unable to report message: " << inMsg;
      return;
    }

    CEGUI::Window* lErrorBox = lWindowMgr->getWindow("Elementum/LuaErrorBox");
    lErrorBox->show();
    lErrorBox->getChildAtIdx(0)->setText(inMsg);
  }

}

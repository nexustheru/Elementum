/*
 *  UIEngine.h
 *  Elementum
 *
 *  Created by Ahmad Amireh on 2/11/10.
 *  Copyright 2010 JU. All rights reserved.
 *
 */

#ifndef H_UIEngine_H
#define H_UIEngine_H

#include "Engine.h"
#include "EventListener.h"
#include "InputListener.h"

#include <CEGUI/CEGUIInputEvent.h>

namespace CEGUI {
  class System;
  class OgreRenderer;
}

namespace Pixy {

  class Event;
  class EventManager;
	/*	\class UIEngine
	 *	\brief
	 *	Loads and unloads UISheets, manages CEGUI system, and handles UI related operations
	 *
	 *	\remark
	 *	At the moment, the UIEngine acts as a manager for UISheets, however,
	 *	the sheets are ought to be handled from within the LUA subsystem.
	 */
	class UIEngine : public Engine, public EventListener, public InputListener {

	public:
		virtual ~UIEngine();
		static UIEngine* getSingletonPtr();

		virtual bool setup();
		virtual void update(unsigned long lTimeElapsed);
		virtual bool cleanup();

		void keyPressed( const OIS::KeyEvent &e );
		void keyReleased( const OIS::KeyEvent &e );

		void mouseMoved( const OIS::MouseEvent &e );
		void mousePressed( const OIS::MouseEvent &e, OIS::MouseButtonID id );
		void mouseReleased( const OIS::MouseEvent &e, OIS::MouseButtonID id );

		CEGUI::MouseButton convertButton(OIS::MouseButtonID buttonID);

	protected:
		const char* getDataPathPrefix() const;

		bool loadResources();

		CEGUI::OgreRenderer		*mOgreRenderer;
		CEGUI::System			*mUISystem;
		EventManager			*mEvtMgr;

	private:
		static UIEngine* _myUIEngine;
		UIEngine();
		UIEngine(const UIEngine& src);
		UIEngine& operator=(const UIEngine& rhs);
	};
}
#endif

/*
 * Copyright (C) Shroom Studios, Inc - All Rights Reserved
 *
 * Unauthorized copying of this file, via any medium is strictly prohibited
 * Proprietary and confidential
 *
 * Written by Ahmad Amireh <ahmad@shroom-studios.com>, September 2011
 */

%module Pixy
%{
  #include "Renderable.h"
%}

namespace Pixy
{
	class Renderable
	{
    public:

		Renderable();
		Renderable& operator=(const Renderable& rhs);
		Renderable(const Renderable& src);
		virtual ~Renderable();

		void attachSceneNode(Ogre::SceneNode* inNode);
		Ogre::SceneNode* getSceneNode() const;

		void attachSceneObject(Ogre::Entity* inObject);
		Ogre::Entity* getSceneObject() const;

    Ogre::Entity* attachExtension(std::string const& inMesh, std::string const& inBone);

    std::string const& getMesh() const;
    std::string const& getMaterial() const;
    void setMaterial(std::string const&);
    void setMesh(std::string const&);

	}; // end of Renderable class
} // end of Pixy namespace

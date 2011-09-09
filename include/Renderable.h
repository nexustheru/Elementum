/* -----------------------------------------------
 *  Filename: Entity.h
 *  Date Created: ??/2009
 *
 *  Original Author:
 *      Ahmad Amireh ( ahmad.amireh@gmail.com )
 *
 *  Last Update:
 *      Date:   20/4/2009
 *      By:     Ahmad Amireh
 * ----------------------------------------------- */

#ifndef H_Renderable_H
#define H_Renderable_H

#include "Pixy.h"
#include <Ogre.h>

//#define NUM_ANIMS 14           // number of animations the character has
//#define CHAR_HEIGHT 5          // height of character's center of mass above ground
//#define CAM_HEIGHT 2           // height of camera above character's center of mass
//#define RUN_SPEED 17           // character running speed in units per second
//#define TURN_SPEED 500.0f      // character turning in degrees per second
//#define ANIM_FADE_SPEED 7.5f   // animation crossfade speed in % of full weight per second
//#define JUMP_ACCEL 30.0f       // character jump acceleration in upward units per squared second
//#define GRAVITY 90.0f          // gravity in downward units per squared second

/*namespace Ogre {
  class SceneNode;
  class MovableObject;
  class Entity;
  class RibbonTrail;
  class AnimationState;
}*/

class MovableTextOverlay;

namespace Pixy
{
  class Entity;
  class CUnit;
  class CPuppet;
	/*! \class Entity Entity.h "src/Entity.h"
	 *
	 * \brief Defines base attributes and actions for GameObjects.
	 *
	 * \note Please note that for the creation of GameObjects
	 * refer to their respective Factories, do NOT use this
	 * directly.
	 */
	class Renderable
	{

    public:

    // all the animations our character has, and a null ID
    // some of these affect separate body parts and will be blended together
    enum AnimID
    {
      ANIM_NONE=0,
      ANIM_LIVE,
      ANIM_IDLE,
      ANIM_WALK,
      ANIM_RUN,
      ANIM_ATTACK,
      ANIM_HIT,
      ANIM_REST,
      ANIM_GETUP,
      ANIM_DIE
    };

		Renderable(Entity* inOwner);
		virtual ~Renderable();

		virtual void attachSceneNode(Ogre::SceneNode* inNode);
		virtual Ogre::SceneNode* getSceneNode();

		virtual void attachSceneObject(Ogre::Entity* inObject);
		virtual Ogre::Entity* getSceneObject();

    void setup(Ogre::SceneManager*);
    void updateAnimations(unsigned long deltaTime);
    void updateBody(unsigned long deltaTime);

    virtual Entity* getEntity();

    MovableTextOverlay* getText() const;
    void setText(MovableTextOverlay* inT);
    Ogre::Vector3 const& getScale() const;

    void hide();
    void show();

    int nrHandlers;

    void registerAnimationState(AnimID inId, std::string inState, bool doLoop = true);
    void setRunSpeed(float inWorldUnitsPerSecond);
    void setScale(Ogre::Vector3 inScale);
    void setScale(float inScale);

    float animateLive();
    float animateDie();
    float animateIdle();
    float animateWalk();
    float animateRun();
    float animateAttack();
    float animateHit();
    float animateRest();
    float animateGetUp();

    // if override is true, then the given animation will instantly
    // play and override any currently playing one (even mini ones)
    //
    // note: only applies to Mini animations
    float _animate(AnimID inId, bool override = false);

    Ogre::Entity* attachExtension(std::string inMesh, std::string inBone);

    void trackEnemyPuppet();
    void trackEnemyUnit(CUnit* inUnit);

    void setOrientation(Ogre::Quaternion inQuat);
    void resetOrientation();

    void rotateToEnemy();
    void rotateTo(const Ogre::Vector3& inDest);

    static void setAnimFadeSpeed(float inSpeed);
    static float getAnimFadeSpeed();

    static void setRotationFactor(float inFactor);

		protected:

    friend class CPuppet;
    friend class CUnit;

    void setOwner(Pixy::Entity* inOwner) {
      mOwner = inOwner;
    }

    //~ void setBaseAnimation(AnimID id, bool reset = false);
    //~ void setTopAnimation(AnimID id, bool reset = false);

    //~ AnimID mCurrentAnimID;
    //~ AnimID mLoopAnimID;

    Ogre::Real mTimer;
    //Ogre::AnimationState* mAnims[NUM_ANIMS];    // master animation list

    private:

    typedef struct {

      AnimID ID;
      Ogre::AnimationState* State;
      bool FadingIn;
      bool FadingOut;

      inline bool isMini() const {
        return (!State->getLoop());
      }

    } Animation;

    Animation *mCurrentAnim;
    Animation *mLoopAnim;

    typedef std::vector<Animation*> anims_t;
    typedef std::map<AnimID, anims_t > anim_map_t;
    anim_map_t mAnims;

    std::list<Animation*> mAnimQueue;

    void _applyNextAnimation();

    void setupBody();
    void setupAnimations();

    void fadeAnimations(Ogre::Real deltaTime);

		Ogre::SceneNode     *mSceneNode;
    Ogre::Vector3       mScale;
		Ogre::Entity        *mSceneObject;
    MovableTextOverlay  *mText;
    Pixy::Entity        *mOwner;
    Ogre::SceneManager  *mSceneMgr;
    std::vector<Ogre::Entity*> mExtensions;
    static float mAnimFadeSpeed;

    //bool mFadingIn[NUM_ANIMS];            // which animations are fading in
    //bool mFadingOut[NUM_ANIMS];           // which animations are fading out

    // for rotation
    Ogre::Quaternion mOrientSrc;               // Initial orientation
    Ogre::Quaternion mOrientDest;              // Destination orientation
    Ogre::Real mRotProgress;                   // How far we've interpolated
    static Ogre::Real mRotFactor;                     // Interpolation step
    bool fRotating;

		Renderable& operator=(const Renderable& rhs);
		Renderable(const Renderable& src);

	}; // end of Entity class
} // end of Pixy namespace
#endif

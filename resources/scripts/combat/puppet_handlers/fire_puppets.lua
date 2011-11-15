local create_puppet = function(puppet)
  puppet:setMesh("DarkKnight.mesh")
  puppet:setMaterial("Elementum/DarkKnight/Fire")

  rnd = puppet
  rnd:getSceneNode():setScale(5)
  GfxEngine:attachToScene(rnd)
  local sword = rnd:attachExtension("DarkKnightSword.mesh", "Bone01")
  sword:setMaterialName("Elementum/DarkKnight/Fire/Sword")

  rnd:registerAnimationState(Pixy.Renderable.ANIM_IDLE, "Idle_1")
  rnd:registerAnimationState(Pixy.Renderable.ANIM_IDLE, "Idle_2")
  rnd:registerAnimationState(Pixy.Renderable.ANIM_DIE, "Death_1", false)
  rnd:registerAnimationState(Pixy.Renderable.ANIM_DIE, "Death_2", false)
  rnd:registerAnimationState(Pixy.Renderable.ANIM_HIT, "Hit_1", false)
  rnd:registerAnimationState(Pixy.Renderable.ANIM_HIT, "Hit_2", false)
  rnd:registerAnimationState(Pixy.Renderable.ANIM_HIT, "Hit_3", false)

  -- undefined
  --~ rnd:registerAnimationState(Pixy.Renderable.ANIM_REST, "Idle_1")
  --~ rnd:registerAnimationState(Pixy.Renderable.ANIM_GETUP, "Idle_1")
  --~ rnd:registerAnimationState(Pixy.Renderable.ANIM_LIVE, "Idle_1")
  --~ rnd:registerAnimationState(Pixy.Renderable.ANIM_LIVE, "Idle_1")
  --~ rnd:registerAnimationState(Pixy.Renderable.ANIM_ATTACK, "Attack_1")
  --~ rnd:registerAnimationState(Pixy.Renderable.ANIM_ATTACK, "Attack_2")
  --~ rnd:registerAnimationState(Pixy.Renderable.ANIM_WALK, "Walk")
  --~ rnd:registerAnimationState(Pixy.Renderable.ANIM_RUN, "Run")

  rnd:animateIdle()
end

subscribe_puppet(Pixy.Race.Fire, create_puppet)

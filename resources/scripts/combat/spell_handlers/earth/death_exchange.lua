local DeathExchange = {}
function DeathExchange:cast()
	Pixy.Log("I'm casting Death Exchange!")
  FxEngine:playEffect("BloodForBlood", self.Caster)

  self.Target:die()
  SCT.ShowScrollingMessage("+2 channels (Death Exchange)", true, self.Caster)
  Gfx.Dehighlight()

	return true
end

-- Death exchange can't be cast on puppets
SpellValidators["Death Exchange"] = function(spell)
  if Selected and Selected:isPuppet() then
    UI.setValidationMsg("This spell can only be cast on friendly units.")
    return false
  end

  return true
end

subscribe_spell("Death Exchange", DeathExchange)

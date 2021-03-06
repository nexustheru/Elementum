Profiles.Listing = UISheet:new("intro/profiles/listing.layout")

local isSetup = false
local Listbox = nil
local Listing = nil
local Buttons = {
  JoinLobby = nil,
  Back = nil,
  CreatePuppet = nil,
  DeletePuppet = nil
}
local Labels = { Name = nil, Level = nil }

-- listbox Items To Puppets mapping
local I2P = {}
local last_selected = nil

local scale = nil
local populate = function()
  -- clear all current items before adding any
  Listbox:resetList()

  local selected = nil
  for puppet in list_iter(Intro.Puppets) do
    local item = CEGUI.createListboxTextItem(puppet:getName())
    item:setTextParsingEnabled(true)
    item:setSelectionBrushImage("TaharezLook", "MultiListSelectionBrush")
    Listbox:addItem(item)
    Listing:add(item)
    I2P[item] = puppet

    if not selected then
      Listbox:setItemSelectState(item, true)
      selected = item
      last_selected = selected
      Selected = puppet
    end
  end
end

local showKnight = function(rnd)
  rnd:show()
end

local doSelect = function(puppet)
  Selected = puppet

  Labels.Name:setText(puppet:getName())
  Labels.Level:setText("Level " .. puppet:getLevel())

  Profiles.HideKnights()
  local knight = Profiles.Knights[raceToString(puppet:getRace())]
  showKnight(knight)
  --~ GfxEngine:getCameraMan():setTarget(knight:getSceneNode())
  --~ GfxEngine:getCameraMan():setYawPitchDist(Ogre.Radian(0), Ogre.Radian(0), -40)
  local node = knight:getSceneNode()
  node:setPosition(Ogre.Vector3(0,0,0))
  node:lookAt(Ogre.Vector3(0,0,-1), Ogre.Node.TS_WORLD)
  node:setScale(scale)
  local pos = node:getPosition()
  GfxEngine:getCamera():setPosition(Ogre.Vector3(0, 5, 20))
  GfxEngine:getCamera():lookAt(pos.x, pos.y + 7.5, pos.z)

  Buttons.DeletePuppet:enable()
  return true
end

local onItemSelectionChanged = function(args)
  print("im selecting a puppet!")

  local item = Listbox:getFirstSelectedItem()

  -- make sure the selection is never cleared
  if item then last_selected = item else
    if last_selected then
      Listbox:setItemSelectState(last_selected, true)
    end
    return true
  end

  assert( I2P[item] )

  return doSelect(I2P[item])
end

function Profiles.Listing:attach()
  UISheet.attach(self)

  --~ Profiles.Layout = Pixy.UI.attach("intro/profiles/listing.layout")
  Buttons.JoinLobby = CEWindowMgr:getWindow("Elementum/Intro/Buttons/JoinLobby")
  Buttons.Back = CEWindowMgr:getWindow("Elementum/Intro/Buttons/BackToLogin")
  Buttons.CreatePuppet = CEWindowMgr:getWindow("Elementum/Intro/Buttons/CreatePuppet")
  Buttons.DeletePuppet = CEWindowMgr:getWindow("Elementum/Intro/Buttons/DeletePuppet")
  Labels.Name = CEWindowMgr:getWindow("Elementum/Intro/Labels/Name")
  Labels.Level = CEWindowMgr:getWindow("Elementum/Intro/Labels/Level")
  Listbox = CEGUI.toListbox(CEWindowMgr:getWindow("Elementum/Intro/Listboxes/Puppets"))
  Listbox:subscribeEvent("ItemSelectionChanged", onItemSelectionChanged)

  Listing = Cyclable:new()

  Input.KeyRelease.bind(OIS.KC_UP, Profiles.Listing.selectNext)
  Input.KeyRelease.bind(OIS.KC_DOWN, Profiles.Listing.selectPrev)
  Input.KeyRelease.bind(OIS.KC_RETURN, Profiles.JoinLobby)
  Input.KeyRelease.bind(OIS.KC_ESCAPE, Profiles.Back)

  Profiles.CurrentScreen = "Listing"
  Pixy.Log("Attaching Profiles[Listing]")

  scale = Profiles.CalcScale(2.75)

  GfxEngine:getCamera():setPosition(Ogre.Vector3(0, 5, 30))
  GfxEngine:getCameraMan():setStyle(OgreBites.CS_FREELOOK)

	--if isSetup then return true end

  do
    -- rotate the ones on the edges to give them a look of standing in a semi-circle
    Profiles.Knights.Earth:getSceneNode():lookAt(Ogre.Vector3(0,0, -1), Ogre.Node.TS_WORLD)
    Profiles.Knights.Air:getSceneNode():lookAt(Ogre.Vector3(0,0, -1), Ogre.Node.TS_WORLD)
    Profiles.Knights.Fire:getSceneNode():lookAt(Ogre.Vector3(0,0, -1), Ogre.Node.TS_WORLD)
    Profiles.Knights.Water:getSceneNode():lookAt(Ogre.Vector3(0,0, -1), Ogre.Node.TS_WORLD)
  end

  populate()


  --isSetup = true
end

function Profiles.Listing:detach()
  UISheet.detach(self)

  Listing:destroy()

  Input.KeyRelease.unbind(OIS.KC_ESCAPE, Profiles.Back)
  Input.KeyRelease.unbind(OIS.KC_UP, Profiles.Listing.selectNext)
  Input.KeyRelease.unbind(OIS.KC_DOWN, Profiles.Listing.selectPrev)
  Input.KeyRelease.unbind(OIS.KC_RETURN, Profiles.JoinLobby)

  Buttons = {}
  RaceText = nil
  Listbox:removeEvent("ItemSelectionChanged")

  Pixy.Log("Detaching Profiles[Listing]")
end

Profiles.DeletePuppet = function(args)
  assert(Selected)

  local e = Pixy.Event(Pixy.EventUID.RemovePuppet)
  e:setProperty("Name", Selected:getName())
  NetMgr:send(e)
end

Profiles.Listing.selectNext = function()
  Listbox:clearAllSelections();
  Listbox:setItemSelectState(Listing:next(), true)
  doSelect(I2P[Listing:get()])
end

Profiles.Listing.selectPrev = function()
  Listbox:clearAllSelections();
  Listbox:setItemSelectState(Listing:prev(), true)
  doSelect(I2P[Listing:get()])
end


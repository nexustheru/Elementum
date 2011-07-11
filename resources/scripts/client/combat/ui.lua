--[[
	combat: ui.lua

	cegui event handlers and UI bootstrapping goes here
]]
if (Pixy.UI.Combat == nil) then
  Pixy.UI.Combat = { Buttons = {}, Labels = {}, Config = {} }

end

Pixy.UI.Combat.configure = function()
	-- register the globals
	cfg = Pixy.UI.Combat.Config

  -- path to our layout sheet
  cfg.LayoutPath = "combat/ui.layout"

	-- spell button config
	cfg.SpellButton = {}
	cfg.SpellButton["Height"] = 96
	cfg.SpellButton["Width"] = 96

	-- create our imagesets used for spell buttons
	CEImagesetMgr:create( "spells_earth.imageset" )
  CEImagesetMgr:create( "huds.imageset" )

end

Pixy.UI.Combat.registerGlobals = function()
  Pixy.UI.Combat.Labels["Turns"] = CEWindowMgr:getWindow("Elementum/Scenes/Combat/Effects/Turns/Text")
  Pixy.UI.Combat.Labels["Tooltip"] = CEWindowMgr:getWindow("Elementum/Combat/Labels/Tooltip")


end

-- effectively reloads the UI components and sheets
-- NOTE: this destroys all currently drawn spell buttons too!
Pixy.UI.Combat.reload = function()
	CEWindowMgr:destroyWindow(CEWindowMgr:getWindow("Elementum/Scenes/Combat"))

	local lLayout = CEWindowMgr:loadWindowLayout(Pixy.UI.Combat.Config.LayoutPath)
	lLayout:setAlwaysOnTop(true)
	CESystem:setGUISheet(lLayout)

	-- TODO repopulate puppet hand after reloading ui
end

firstButton = true

-- draws a CEGUI::ImageButton element representing the given spell
-- and attaches it to the Puppet's hand
Pixy.UI.Combat.drawSpell = function(inSpell)

	local lButton = {}

  local margin = 5
    if (firstButton) then margin = 0; firstButton = false end

	-- calculate position
	lButton["Position"] =
		CEGUI.UVector2:new(
			--CEGUI.UDim(0, cfg.SpellButton["Width"] * (SelfPuppet:nrSpellsInHand() - 2) + (margin*SelfPuppet:nrSpellsInHand())),
      CEGUI.UDim(0, cfg.SpellButton["Width"] * (SelfPuppet:nrSpellsInHand() - 2)),
			CEGUI.UDim(0, 0)
		)

	-- assign dimensions
	lButton["Dimensions"] =
		CEGUI.UVector2:new(
			CEGUI.UDim(0.147,0),-- cfg.SpellButton["Width"]),
			CEGUI.UDim(0.67,0)-- cfg.SpellButton["Height"])
		)

	-- generate the button's name
	lButton["Name"] = "Elementum/Scenes/Combat/SpellPanel/Player/" .. inSpell:getUID()

	Pixy.Log("creating a window named " .. lButton["Name"])

	-- create the actual button element
  --local list_item = CEWindowMgr:createWindow("TaharezLook/ListboxItem", lButton["Name"] .. "/ListItem")
	lButton["Window"] = CEWindowMgr:createWindow("TaharezLook/ImageButton", lButton["Name"])
  --list_item:addChildWindow(lButton["Window"])

	-- attach the button to the Pixy::Spell object
	inSpell:setButton(lButton["Window"])
  inSpell:setImageSet("Spells_" .. raceToString(inSpell:getRace()))
  -- sanitize spell name to match image set name
  local sane_name = string.gsub(inSpell:getName(), "%s", "_")
  inSpell:setImageName(sane_name)

  -- get spell properties
  --local lAspect = aspectToString(inSpell:getAspect())
  --local lRace = raceToString(inSpell:getRace())
  --local lSpellData = Pixy.Spells[lRace][lAspect][inSpell:getName()]
  --local lSpellData = Pixy.Spells.Earth.Matter["Summon: Fetish Zij"]
	-- draw the image button
	lButton["Image"] = "set:" .. inSpell:getImageSet() .. " image:" .. inSpell:getImageName()
	lButton["Window"]:setProperty("NormalImage", lButton["Image"] .. "_Normal")
	lButton["Window"]:setProperty("HoverImage", lButton["Image"] .. "_Hover")
	lButton["Window"]:setProperty("PushedImage", lButton["Image"] .. "_Pushed")
	lButton["Window"]:setProperty("DisabledImage", lButton["Image"] .. "_Disabled")

	-- attach our spell object to the button...
	lButton["Window"]:setUserString("Spell", inSpell:getUID())

	-- create tooltip
  --lButton["Window"]:setTooltipText(inSpell:getTooltip())
	-- ...

	-- attach the window to our layout
	CEWindowMgr:getWindow("Elementum/Scenes/Combat/SpellPanel/Player/Hand"):addChildWindow(lButton["Window"])

  --local item = lButton["Window"]
	--CEGUI.toItemEntry(list_item)
  --local list = CEWindowMgr:getWindow("Elementum/Scenes/Combat/SpellPanel/Player/Hand")
  --CEGUI.toListbox(list)
  --list:addItem(list_item)
	lButton["Window"]:setSize(lButton["Dimensions"])
  UIEngine:setMargin(lButton["Window"],
    CEGUI.UBox(
      CEGUI.UDim(0,0),
      CEGUI.UDim(0,0),
      CEGUI.UDim(0,0),
      CEGUI.UDim(0,5)))
  --list_item:setSize(lButton["Dimensions"])
	--lButton["Window"]:setPosition(lButton["Position"])
	lButton["Window"]:moveToFront()
	lButton["Window"]:show()

	-- finally, subscribe the button to its event handlers
	lButton["Window"]:subscribeEvent("Clicked", "Pixy.Combat.reqCastSpell")
  lButton["Window"]:subscribeEvent("MouseEnter", "Pixy.UI.Combat.ShowTooltip")
  lButton["Window"]:subscribeEvent("MouseLeave", "Pixy.UI.Combat.HideTooltip")

  table.insert(Pixy.UI.Combat.Buttons, inSpell:getButton())
end

Pixy.UI.Combat.dropSpell = function(inSpell)
  removeByValue(Pixy.UI.Combat.Buttons, inSpell:getButton())
  CEWindowMgr:destroyWindow(inSpell:getButton())
  for button in list_iter(Pixy.UI.Combat.Buttons) do
    local pos = CEGUI.UVector2:new(
    	CEGUI.UDim(0, button:getPosition().x.offset - cfg.SpellButton["Width"]),
			button:getPosition().y)

    button:setPosition(pos)
  end
end

Pixy.UI.Combat.onStartTurn = function()
  Pixy.UI.Combat.Labels["Turns"]:setText("Your turn")

  --CEWindowMgr:getWindow("Elementum/Scenes/Combat/SpellPanel/Player"):enable()
  for button in list_iter(Pixy.UI.Combat.Buttons) do
    button:enable()
  end
end

Pixy.UI.Combat.onTurnStarted = function()
  Pixy.UI.Combat.Labels["Turns"]:setText("Enemy's turn")

  --CEWindowMgr:getWindow("Elementum/Scenes/Combat/SpellPanel/Player"):disable()
  for button in list_iter(Pixy.UI.Combat.Buttons) do
    button:disable()
  end
end

Pixy.UI.Combat.ShowTooltip = function(e)
	local win = CEGUI.toWindowEventArgs(e).window
	local spell = SelfPuppet:getSpell(win:getUserString("Spell"))
  assert(spell)

  Pixy.UI.Combat.Labels["Tooltip"]:setText(spell:getTooltip())
end
Pixy.UI.Combat.HideTooltip = function(e)
  Pixy.UI.Combat.Labels["Tooltip"]:setText("")
end
-- configure
Pixy.UI.Combat.configure()

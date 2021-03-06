-- resolve script paths
do
  ScriptPrefix = Pixy.ScriptEngine_getSingletonPtr():getScriptPathPrefix()
  package.path = ScriptPrefix .. "/?.lua;" .. package.path
  ModulePrefix = Pixy.ScriptEngine_getSingletonPtr():getModulePathPrefix()
  package.cpath = ModulePrefix .. "/?.so;" .. package.cpath

  print("Scripts path: " .. ScriptPrefix .. "\nModule path: " .. ModulePrefix)
end

require "shared/helpers"
require "shared/globals"
require "shared/dispatcher"
require "shared/cyclable"
require "shared/input"
require "shared/ui"
require "shared/fx"

function arbitrary(name, ...)
  -- construct the method pointer
  local _p = _G
  for word in list_iter(all_words(name)) do
    _p = _p[word]
    if not _p then
      return error("attempting to call an invalid arbitrary method: " .. name)
    end
  end

  return _p(unpack(arg))
end

WantUpdates = {}
Pixy.doUpdateMe = function(inCallback)
	table.insert(WantUpdates, inCallback)
end

Pixy.dontUpdateMe = function(inCallback)
	local i = 1
	for method in list_iter(WantUpdates) do
	  if method == inCallback then
	    table.remove(WantUpdates, i)
			break
	  end
	  i = i+1
	end
end

Pixy.update = function(dt)
	for callback in list_iter(WantUpdates) do
		callback(dt)
	end
end

-- this should be called once during the client's lifetime
-- for state bootstrapping, see State.onEnter
local isSetup = false
Pixy.onEnter = function()
  if isSetup then return true end

  print("-- in Pixy.onEnter --")
  local i = 0
  for k in pairs(Pixy) do print("# " .. i .. " => " .. k); i = i + 1 end

  print(Pixy.Race.Fire)
  if Pixy then
    print("SWIG Exporter is working!!")
  else
    print("SWIG Exporter isn't working")
  end

  Pixy.registerGlobals()
  require "shared/aliases"

  Dispatcher.setup()
  Pixy.UI.setup()
  Input.setup()
  Fx.setup()

  CombatState:setIsDebugging(true)

  local exporter = Pixy.SpellListExporter()
  local unit = Pixy.Unit()
  exporter:export(unit:getSpells(), "Pixy::Spell", "__Temp")
  for i in list_iter(__Temp) do
    print("I got a spell")
  end
  __Temp = nil
  unit = nil
  collectgarbage()

  isSetup = true
  return true
end

-- this should be called only when a shutdown is performed
-- for state cleanups, see State.onExit
Pixy.onExit = function()
  Pixy.Models.Spells = {}
  Pixy.Models.Units = {}
  Pixy.Models = {}

  Fx.cleanup()
  Input.cleanup()
  Pixy.UI.cleanup()
  Dispatcher.cleanup()
end

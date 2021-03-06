-- Setup paths
package.path = package.path .. ';dependencies/?.lua' 
package.path = package.path .. ';dependencies/socket/?.lua' 

-- Surpress noisy input
require("Logging").set_log_level(os.getenv("LANARTS_LOG") or "WARN")

-- Include necessary global modifications
require("GlobalVariableSetup")(--[[Surpress loading draw-related globals?]] os.getenv("LANARTS_HEADLESS") ~= nil)
require("moonscript.base").insert_loader()

local argparse = require "argparse"

local function main(raw_args)
    local parser = argparse("lanarts", "Lanarts shell. By default, starts lanarts.")
    parser:option("-L --lua", "Run lua files."):count("*")
    parser:option("-R --require", "Require lua modules."):count("*")
    -- Parse arguments
    local args = parser:parse(raw_args)
    local req_call = function(modulename) 
        local module = require(modulename)
        if type(module) == "table" and rawget(module, "main") then
            module.main(raw_args)
        end
    end
    for _, filename in ipairs(args.lua) do
        local modulename = filename:gsub(".moon", ""):gsub(".lua", ""):gsub("/", ".")
        req_call(modulename)
    end
    for _, modulename in ipairs(args.require) do
        req_call(modulename)
    end
end

return main

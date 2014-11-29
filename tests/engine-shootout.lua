--[[
engine-shootout.lua

%CUTECHESS% -pgnout %PGN_DATABASE% -engine name=superpawn proto=uci cmd=superpawn.exe dir=..\build\win\x64\release -engine name=testina proto=xboard cmd=testina.exe dir=engines\testina -each tc=60/180 -games 10 -wait 1
]]--

games = 1

game_options = "-tournament gauntlet -each tc=1/10 "
game_options = game_options .. " -games " .. games
game_options = game_options .. " -concurrency 2 -wait 1"

opponents = { 
	"ACE/ACE.exe/uci",
	"DesasterArea/DesasterArea-1.54.exe/uci",
	"Dika/Dikabi.exe/xboard",
--	"Marquis/marquis.exe/xboard",
--	"Numpty/Numpty_Recharged_64.exe/xboard",
	"Piranha/piranha.exe/uci",
	"Senpai/senpai1.0_sse42.exe/uci",
	"Stockfish/stockfish_14053109_32bit.exe/uci",
	"Testina/Testina.exe/xboard",
	}

platform = "win32"
platform_generic = "win"
subplatform = "x64"
hero_engine_name = "Superpawn"
hero_engine_build_type = "Release"
hero_engine_command = "superpawn.exe"

current_dir = io.popen "cd" : read '*l'
build_dir = current_dir .. "/../build/" 
engines_dir = current_dir .. "/engines/"
hero_engine_path = build_dir .. platform_generic .. "/" .. subplatform .. "/" .. hero_engine_build_type .. "/"

current_date = os.date("%Y-%m-%d-%H-%M-%S")

print("Current directory is " .. current_dir)
print("Current time is " .. current_date)

cutechess = current_dir .. "/../tools/" .. platform .. "/cutechess-cli/cutechess-cli.exe"
pgn_database_dir = build_dir .. "tests/"
pgn_database = pgn_database_dir .. "engine-test-" .. current_date .. ".pgn"

command_line = ""
command_line = command_line .. cutechess .. " "

-- PGN database output
command_line = command_line .. "-pgnout " .. pgn_database

-- Hero engine information
command_line = command_line .. " -engine name=" .. hero_engine_name .. " proto=uci "
command_line = command_line .. "cmd=" .. hero_engine_command .. " "
command_line = command_line .. "dir=" .. hero_engine_path .. " "
-- command_line = command_line .. "dir=c:/git/superpawn/build/win/x64/release "

-- Information for each successive engine
for key, opponent in pairs( opponents ) do
	print( "opponent = " .. opponent )
	opponent_dir, opponent_exe, opponent_protocol = opponent:match("([^/]+)/([^/]+)/([^/]+)")
	print( "opponent_dir = " .. opponent_dir )
	print( "opponent_exe = " .. opponent_exe )
	print( "opponent_protocol = " .. opponent_protocol )
	
	command_line = command_line .. "-engine name=" .. opponent_dir .. " "
	command_line = command_line .. "proto=" .. opponent_protocol .. " "
	command_line = command_line .. "cmd=" .. opponent_exe .. " "
	command_line = command_line .. "dir=" .. engines_dir .. opponent_dir .. "/" .. " "
end

command_line = command_line .. game_options .. " "

print "---"
print( command_line )
print "---"
os.execute( command_line )

-- create elostat instruction file



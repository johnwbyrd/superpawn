--[[
gauntlet-test.lua
]]--

require "cmdline"
local getparam = cmdline.getparam
local inspect = require('inspect')

print( inspect( arg ))
t_out = getparam( arg )
print( inspect( t_out ))

current_date = os.date("%Y-%m-%d-%H-%M-%S")

build_id = t_out.BUILD_ID or current_date
build_number = t_out.BUILD_NUMBER or current_date
build_tag = t_out.BUILD_TAG or current_date
print( "Build tag: " .. build_tag )

-- Number of rounds per engine
games = 20

-- Actually run cutechess or just simulate?
run_cutechess = true
-- Actually run analysis after?
run_analyze = true

game_options = "-tournament gauntlet -each tc=40/60+5"
game_options = game_options .. " -games " .. games
game_options = game_options .. " -concurrency 2 -wait 1 " 
game_options = game_options .. " -event " .. build_tag .. " "
game_options = game_options .. " -site " .. build_tag .. " "
-- Testina seems to crash a lot.
game_options = game_options .. " -recover "

opponents = { 
	"ACE/ACE.exe/uci",
--	"DesasterArea/DesasterArea-1.54.exe/uci",
	"Dika/Dikabi.exe/xboard",
	"GiuChess/giuchess.exe/xboard",
--	"Piranha/piranha.exe/uci",
--	"Senpai/senpai1.0_sse42.exe/uci",
--	"Stockfish/stockfish_14053109_32bit.exe/uci",
	"Testina/Testina.exe/xboard",
--	"TSCP/tscp181.exe/xboard"
	}
	
platform = "win32"
platform_generic = "win"
subplatform = "x64"
hero_engine_name = "Superpawn"
hero_engine_build_type = "Debug"
hero_engine_command = "superpawn.exe"

current_dir = io.popen "cd" : read '*l'
root_dir = current_dir .. "/../../"
build_dir = root_dir .. "build/"
build_tests_dir = build_dir .. "tests/"
tests_dir = root_dir .. "tests/"
engines_dir = tests_dir .. "engines/"
hero_engine_path = build_dir .. platform_generic .. "/" .. subplatform .. "/" .. hero_engine_build_type .. "/"

build_tests_dir = string.gsub( build_tests_dir, "/", "\\")
makedir_cmd = "if not exist " .. build_tests_dir .. "nul mkdir ".. build_tests_dir
print( "Trying to make directory: " .. makedir_cmd )
os.execute( makedir_cmd )

print("Current directory: " .. current_dir)
print("Current time:      " .. current_date)

tools_dir = root_dir .. "tools/"
tools_platform_dir = tools_dir .. platform .. "/"
cutechess = tools_platform_dir .. "/cutechess-cli/cutechess-cli.exe"

pgn_database_dir = build_dir .. "tests/"
pgn_database_root = pgn_database_dir .. build_tag
pgn_database = pgn_database_root .. ".pgn"
pgn_report_fn = pgn_database_root .. ".txt"

command_line = ""
command_line = command_line .. cutechess .. " "

-- PGN database output
command_line = command_line .. "-pgnout " .. pgn_database

-- Hero engine information
command_line = command_line .. " -engine name=" .. hero_engine_name .. " proto=uci "
command_line = command_line .. "cmd=" .. hero_engine_command .. " "
command_line = command_line .. "dir=" .. hero_engine_path .. " "

-- Information for each successive engine
for key, opponent in pairs( opponents ) do
	opponent_dir, opponent_exe, opponent_protocol = opponent:match("([^/]+)/([^/]+)/([^/]+)")
	print( "Opponent:              " .. opponent_dir )
	print( "Opponent executable: = " .. opponent_exe )
	print( "Protocol:            = " .. opponent_protocol )
	print( " " )
	
	command_line = command_line .. "-engine name=" .. opponent_dir .. " "
	command_line = command_line .. "proto=" .. opponent_protocol .. " "
	command_line = command_line .. "cmd=" .. opponent_exe .. " "
	command_line = command_line .. "dir=" .. engines_dir .. opponent_dir .. "/" .. " "
end

command_line = command_line .. game_options .. " "

print "---"
print( command_line )
print "---"

if ( run_cutechess ) then
	os.execute( command_line )
end

print "cutechess execution finished."

-- create elostat instruction file
print "Creating elostat instruction file..."

pgn_database_base_fn = string.gsub( pgn_database, ".pgn", "" )

insns_file_name = current_date .. ".tmp"
insns = io.open( insns_file_name, "w" )
insns:write( "1\n" )
insns:write( pgn_database_base_fn .. "\n" )
insns:write( "1500\n" )
insns:write( "1\n" )
insns:close()

print "File created."

analysis_dir = tools_platform_dir .. "elostat/"
analysis_exe = analysis_dir .. "elostat_13.exe"

if ( run_analyze ) then
	analysis_cmdline =  "type " .. insns_file_name .. " | " .. analysis_exe
	-- darn backslashes
	analysis_cmdline = string.gsub( analysis_cmdline, "/", "\\" )

	print ("---")
	print (analysis_cmdline)
	print ("---")
	os.execute( analysis_cmdline )
	print "Analysis completed."
end

os.remove( insns_file_name )

-- save report along with pgn
pgn_report_fn = string.gsub( pgn_report_fn, "/", "\\" )
report_cmdline = "copy rating.dat+programs.dat " .. pgn_report_fn
print( report_cmdline )
os.execute( report_cmdline )
os.execute( "del *.dat /q" )








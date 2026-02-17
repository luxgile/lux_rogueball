add_rules("mode.debug", "mode.release")
add_requires("flecs", "glm", "stb", "spdlog", "box2d", "imgui v1.92.5-docking")

target("rogue_ball")
set_kind("binary")
add_rules("sokol.shdc")
set_languages("cxx20")
add_files("src/*.cpp")
add_files("src/server/*.cpp")
add_files("src/game/*.cpp")
add_files("src/modules/*.cpp")
add_files("src/shaders/*.glsl")
add_packages("flecs", "glm", "stb", "spdlog", "box2d", "imgui")
add_includedirs("libs/sokol")
add_includedirs("libs/sokol/util")

-- Adding sol2 for lua
add_includedirs("libs/sol2/include")
add_includedirs("libs/lua-5.4.8/src")
add_links("lua")

-- To debug includes
add_cxflags("-H")

if is_plat("linux") then
	add_syslinks("GL", "dl", "pthread", "X11", "Xi", "Xcursor")
end

after_build(function(target)
	-- where the executable is placed
	local outdir = target:targetdir()
	local linkpath = path.join(outdir, "assets")

	-- absolute path to project root assets
	local projdir = os.projectdir()
	local assetsdir = path.join(projdir, "assets")

	if os.exists(linkpath) then
		os.rm(linkpath)
	end

	os.runv("ln", { "-s", assetsdir, linkpath })
end)

rule("sokol.shdc")
set_extensions(".glsl")
on_build_file(function(target, sourcefile, opt)
	import("core.project.depend")
	import("utils.progress")

	-- Define the output header path (e.g., build/shaders/myshader.glsl.h)
	local headerfile = sourcefile .. ".h"

	-- Only run the command if the .glsl file has actually changed
	depend.on_changed(function()
		progress.show(opt.progress, "${color.build.object}compiling.shader %s", sourcefile)

		local slang = "glsl430"
		os.vrunv("./sokol-shdc", {
			"-i",
			sourcefile,
			"-o",
			headerfile,
			"-l",
			slang,
			"-f",
			"sokol",
		})
	end, { files = sourcefile })
end)

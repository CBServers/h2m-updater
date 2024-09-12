dependencies = {
	basePath = "./deps"
}

function dependencies.load()
	dir = path.join(dependencies.basePath, "premake/*.lua")
	deps = os.matchfiles(dir)

	for i, dep in pairs(deps) do
		dep = dep:gsub(".lua", "")
		require(dep)
	end
end

function dependencies.imports()
	for i, proj in pairs(dependencies) do
		if type(i) == 'number' then
			proj.import()
		end
	end
end

function dependencies.projects()
	for i, proj in pairs(dependencies) do
		if type(i) == 'number' then
			proj.project()
		end
	end
end

dependencies.load()

workspace "h2m-updater"
startproject "h2m-updater"
location "./build"
objdir "%{wks.location}/obj"
targetdir "%{wks.location}/bin/%{cfg.platform}/%{cfg.buildcfg}"

configurations {"Debug", "Release"}

if os.istarget("darwin") then
	platforms {"x64", "arm64"}
else
	platforms {"x86", "x64"}
end

filter "platforms:x86"
architecture "x32"

filter "platforms:x64"
architecture "x86_64"

filter "platforms:arm64"
architecture "ARM64"

filter {"language:C++", "toolset:not msc*"}
	buildoptions {
		"-std=c++17"
	}
filter "toolset:msc*"
	buildoptions {
		"/std:c++17"
	}
filter {}

filter {"system:windows"}
	systemversion "latest"
filter {}

symbols "On"
staticruntime "On"
editandcontinue "Off"
warnings "Extra"
characterset "ASCII"

if os.istarget("linux") or os.istarget("darwin") then
	buildoptions "-pthread"
	linkoptions "-pthread"
end

if _OPTIONS["dev-build"] then
	defines {"DEV_BUILD"}
end

if os.getenv("CI") then
	defines "CI"
end

flags {"NoIncrementalLink", "NoMinimalRebuild", "MultiProcessorCompile", "No64BitChecks"}

filter "configurations:Release"
	optimize "Speed"
	defines {"NDEBUG"}
	flags {"FatalCompileWarnings"}
filter {}

filter "configurations:Debug"
	optimize "Debug"
	defines {"DEBUG", "_DEBUG"}
filter {}

project "h2m-updater"
kind "ConsoleApp"
language "C++"

pchheader "std_include.hpp"
pchsource "src/std_include.cpp"

files {"./src/**.rc", "./src/**.hpp", "./src/**.cpp"}

includedirs {"./src", "%{prj.location}/src"}

filter "system:windows"
	files {
		"./src/**.rc",
	}
filter { "system:windows", "toolset:not msc*" }
	resincludedirs {
		"%{_MAIN_SCRIPT_DIR}/src"
	}
filter { "system:windows", "toolset:msc*" }
	linkoptions {"/IGNORE:4254", "/SAFESEH:NO", "/LARGEADDRESSAWARE", "/PDBCompress"}
	resincludedirs {
		"$(ProjectDir)src" -- fix for VS IDE
	}
filter {}

dependencies.imports()


group "Dependencies"
dependencies.projects()

rule "ProtobufCompiler"
	display "Protobuf compiler"
	location "./build"
	fileExtension ".proto"
	buildoutputs {
		'%{prj.location}/src/proto/%{file.basename}.pb.cc',
		'%{prj.location}/src/proto/%{file.basename}.pb.h',
	}
	filter "action:vs*"
		buildmessage "Compiling %(Identity) with protoc..."
		buildcommands {
			'@echo off',
			'path "$(SolutionDir)..\\tools\\windows"',
			'if not exist "$(ProjectDir)\\src\\proto" {MKDIR} "$(ProjectDir)\\src\\proto"',
			'protoc --error_format=msvs -I=%(RelativeDir) --cpp_out=src\\proto %(Identity)',
		}

	filter "action:not vs*"
		protocbin = "%{_MAIN_SCRIPT_DIR}/tools/"  .. os.host() .. "/protoc"
		buildcommands {
			'mkdir -p "%{prj.location}/src/proto"',
			'"%{protocbin}" "-I=%{path.getdirectory(file.relpath)}" "--cpp_out=%{prj.location}/src/proto" "%{file.relpath}"',
		}

	filter {}

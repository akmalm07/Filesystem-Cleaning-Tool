workspace "cleanCLI"
    architecture "x64"
    startproject "cleanCLI"

    configurations { "Debug", "Release" }

outputdir = "%{cfg.buildcfg}-%{cfg.system}-%{cfg.architecture}"

-- Include directories relative to root folder
IncludeDir = {}
IncludeDir["item"] = "vendors/item/include"


LibDir = {}
LibDir["item"] = "vendors/item/lib"



group "cleanCLI"
    include "clean/clean.lua"
set_languages("cxx17")
set_arch("x86")
option("SMPATH")
option("MMPATH")
option("HL2SDKPATH")
option("DEBUG")

target("modeloader")
	set_kind("shared")
	set_prefixname("")
	set_suffixname(".ext")
	
	add_files(
		"extension/*.cpp",
		"$(SMPATH)/public/smsdk_ext.cpp")

	add_includedirs(
		"extension",
		"$(SMPATH)/public",
		"$(SMPATH)/public/extensions",
		"$(SMPATH)/sourcepawn/include",
		"$(SMPATH)/public/amtl",
		"$(SMPATH)/public/amtl/amtl",
		"$(MMPATH)/core",
		"$(MMPATH)/core/sourcehook",
		"$(HL2SDKPATH)/public",
		"$(HL2SDKPATH)/public/tier0",
		"$(HL2SDKPATH)/public/tier1",
		"$(HL2SDKPATH)/public/engine",
		"$(HL2SDKPATH)/public/mathlib")

	add_defines(
		"SE_EPISODEONE=1",
		"SE_DARKMESSIAH=2",
		"SE_ORANGEBOX=3",
		"SE_BLOODYGOODTIME=4",
		"SE_EYE=5",
		"SE_CSS=6",
		"SE_HL2DM=7",
		"SE_DODS=8",
		"SE_SDK2013=9",
		"SE_PVKII=10",
		"SE_BMS=11",
		"SE_TF2=12",
		"SE_LEFT4DEAD=13",
		"SE_NUCLEARDAWN=14",
		"SE_CONTAGION=15",
		"SE_LEFT4DEAD2=16",
		"SE_ALIENSWARM=17",
		"SE_PORTAL2=18",
		"SE_BLADE=19",
		"SE_INSURGENCY=20",
		"SE_DOI=21",
		"SE_MCV=22",
		"SE_CSGO=23",
		"SE_DOTA=24",
		"SE_CS2=25",
		"SE_MOCK=26")
	
	add_defines("SOURCE_ENGINE=16")
	add_defines("NDEBUG")
	

	if is_plat("windows") then
		set_toolchains("msvc")
		add_defines(
			"_WINDOWS", "WIN32",
			'_CRT_SECURE_NO_WARNINGS')
		add_cxflags("/W3", "/wd4819", "/Ox", "/Oy-", "/EHsc", "/MT", "/Z7")

		add_shflags("/DEBUG") --Always generate pdb files
		add_shflags("/OPT:ICF", "/OPT:REF")
		add_linkdirs("$(HL2SDKPATH)/lib/public");
		add_links("mathlib", "tier0", "tier1", "vstdlib", "kernel32", "legacy_stdio_definitions")
		
	else
		set_toolchains("clang")
		add_defines(
			"_LINUX","POSIX",
			"NO_HOOK_MALLOC", "NO_MALLOC_OVERRIDE",
			"stricmp=strcasecmp",
			"_stricmp=strcasecmp",
			"strcmpi=strcasecmp",
			"_strnicmp=strncasecmp",
			"strnicmp=strncasecmp ",
			"_snprintf=snprintf",
			"_vsnprintf=vsnprintf",
			"_alloca=alloca")
		add_cxflags(
			"-Wall", 
			"-Wshadow",
			"-Wno-implicit-int-float-conversion", 
			"-Wno-overloaded-virtual", 
			"-Wno-deprecated-register", 
			"-Wno-register",
			"-Wno-non-virtual-dtor", 
			"-Wno-expansion-to-defined",
			"-fno-strict-aliasing", 
			"-fno-exceptions", 
			"-fvisibility=hidden", 
			"-fvisibility-inlines-hidden", 
			"-fPIC", "-O3", "-g3")

		if has_config("DEBUG") then
			add_cxflags("-UNDEBUG", "-O0")
		end

		add_linkdirs("$(HL2SDKPATH)/lib/linux");
		add_links("$(HL2SDKPATH)/lib/linux/tier1_i486.a", "$(HL2SDKPATH)/lib/linux/mathlib_i486.a", "vstdlib_srv", "tier0_srv")
		add_shflags("-static-libstdc++")
	end
	
	after_build(function (target)
		os.tryrm("release")
		os.mkdir("release/addons/sourcemod/extensions")
		os.mkdir("release/cfg/modeloader")
		os.cp(path.join(target:targetdir(), target:filename()), "release/addons/sourcemod/extensions")
		io.writefile("release/addons/sourcemod/extensions/modeloader.autoload", "")
		os.cp("example/cfg/modeloader/modeloader.cfg", "release/cfg/modeloader/modeloader.cfg.example")
    end)


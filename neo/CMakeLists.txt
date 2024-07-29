cmake_minimum_required( VERSION 3.27.8 )

# Check if vcpkg exist in our source folder
if ( EXISTS "${CMAKE_SOURCE_DIR}/../vcpkg/scripts/buildsystems/vcpkg.cmake" )
	set( CMAKE_TOOLCHAIN_FILE "${CMAKE_SOURCE_DIR}/../vcpkg/scripts/buildsystems/vcpkg.cmake" CACHE STRING "Vcpkg toolchain file" )
else()
	message( FATAL_ERROR "Vcpkg wasn't not found, aborting, make sure to git clone this with all submodules!")
endif()

# Enable folder groups
set_property( GLOBAL PROPERTY PREDEFINED_TARGETS_FOLDER  "" )
set_property( GLOBAL PROPERTY USE_FOLDERS ON )

set( CMAKE_MODULE_PATH "${CMAKE_SOURCE_DIR}/CMakeModules" )

project( dhewm3 CXX C)

# build type
if( NOT CMAKE_BUILD_TYPE )
	set( CMAKE_BUILD_TYPE "RelWithDebInfo" )
endif()

set( DHEWM3BINARY "D3ModdingKit" )

include( CheckCXXCompilerFlag )
include( GNUInstallDirs OPTIONAL RESULT_VARIABLE GNUINSTALLDIRS )
include( GitBranch )
include( GlobbedHeaders )

option( ENGINE						"Build the main executable." ON )
option( DEDICATED					"Build the dedicated server executable." ON )
option( TOOLS						"Build the include tools into the main executable." ON )
option( MAYA_IMPORT					"Build the include maya import tool (Requires MayaSDK)." ON )
option( BASE						"Build the base game code" ON )
option( D3XP						"Build the d3xp game code" ON )
option( ONATIVE						"Optimize for the host CPU" OFF )
option( IMGUI						"Build with Dear ImGui integration - requires SDL2 and C++11" ON )
option( REPRODUCIBLE_BUILD			"Replace __DATE__ and __TIME__ by hardcoded values for reproducible builds." OFF )
option( LINUX_RELEASE_BINS			"Set RPATH to \$ORIGIN/libs/ for Linux binary releases" OFF )
option( HARDLINK_GAME				"Compile gamecode into executable (no game DLLs)" OFF )
option( FREETYPE					"Enable Freetype support." ON )
if( NOT MSVC ) # GCC/clang or compatible, hopefully
	option( FORCE_COLORED_OUTPUT	"Always produce ANSI-colored compiler warnings/errors (GCC/Clang only; esp. useful with ninja)." OFF )
	option( ASAN					"Enable GCC/Clang Adress Sanitizer (ASan)" OFF) # TODO: MSVC might also support this, somehow?
	option( UBSAN					"Enable GCC/Clang Undefined Behavior Sanitizer (UBSan), implies HARDLINK_GAME" OFF )

	if( UBSAN AND NOT HARDLINK_GAME )
		message( STATUS "UBSAN requires linking the gamecode into the executable, will enable HARDLINK_GAME" )
		set( HARDLINK_GAME ON )
	endif()
endif()

if ( LINUX_RELEASE_BINS )
	message( STATUS "Setting RPATH to \$ORIGIN/libs/ so you can put dependencies in there" )
	set( CMAKE_SKIP_RPATH OFF CACHE BOOL "Skip RPATH" FORCE )
	set( CMAKE_INSTALL_RPATH_USE_LINK_PATH FALSE )
	set( CMAKE_BUILD_WITH_INSTALL_RPATH TRUE )
	set( CMAKE_INSTALL_RPATH "$ORIGIN/libs" )
	set( ldflags "${ldflags} -Wl,--large-address-aware,-z,origin" ) # not sure this is even needed, but shouldn't hurt
else()
	set( CMAKE_SKIP_RPATH ON CACHE BOOL "Skip RPATH" FORCE )
endif()

if( REPRODUCIBLE_BUILD )
	# don't use __DATE__ and __TIME__ macros so builds are reproducible
	add_definitions( -DID_REPRODUCIBLE_BUILD )
endif()

# Set these if we found either MFC or any UI toolkit, turn off TOOLS if we didn't found neither of theses
set( MFC_TOOLS OFF ) # (Visual Studio + SDL2 only)

# Define our cpu variable
include( CPUDefine )

# Nicely tied our thirdparty libraries here
set( sys_thirdparty_libs )

# libs
find_package( Stb REQUIRED )
include_directories( ${Stb_INCLUDE_DIR} )

find_package( OpenAL REQUIRED )
include_directories( ${OPENAL_INCLUDE_DIR} )
list( APPEND sys_thirdparty_libs ${OPENAL_LIBRARY} )

find_package( SDL2 REQUIRED )
include_directories( ${SDL2_INCLUDE_DIR} )
list( APPEND sys_thirdparty_libs SDL2::SDL2 )

if ( FREETYPE )
	find_package( Freetype REQUIRED )
	find_path( FREETYPE_INCLUDE_DIR "fttypes.h" PATH_SUFFIXES "freetype" )
	include_directories( ${FREETYPE_INCLUDE_DIR} )
	add_definitions( -DID_BUILD_FREETYPE )
	list( APPEND sys_thirdparty_libs Freetype::Freetype )
endif()

if( IMGUI )
	# we need C++11 for ImGui
	set ( CMAKE_CXX_STANDARD 11 )
	message( STATUS "Dear ImGui integration enabled" )
else()
	message( STATUS "Dear ImGui integration disabled" )
	add_definitions( -DIMGUI_DISABLE )
endif()

# Curl
find_package( CURL QUIET )
if( CURL_FOUND )
	set( ID_ENABLE_CURL ON )
	include_directories( ${CURL_INCLUDE_DIR} )
	message( STATUS "libcurl found and enabled" )
	list( APPEND sys_thirdparty_libs ${CURL_LIBRARY} )
else()
	message( WARNING "libcurl not found, server downloads won't be available (apart from that the engine will work)" )
	set( ID_ENABLE_CURL OFF )
	set( CURL_LIBRARY "" )
endif()

# MayaImport currently doesn't work in Linux/Mac
if ( NOT WIN32 OR NOT TOOLS )
	set( MAYA_IMPORT OFF )
endif()

if( TOOLS )
	if ( MAYA_IMPORT )
		include( ExternalProject )

		# Locate and download MayaSDK, change MAYASDK_URL to one of the urls listed here: https://aps.autodesk.com/developer/overview/maya
		# ( Rigth click in anyone > Copy Url)
		set( MAYASDK_URL "https://autodesk-adn-transfer.s3-us-west-2.amazonaws.com/ADN+Extranet/M%26E/Maya/devkit+2019/Autodesk_Maya_2019_3_Update_DEVKIT_Windows.zip" CACHE STRING "" )
	
		# The downloaded Hash & MD5 of the MAYASDK_URL download
		# These ones are from 2019 make sure to generate one for newer releases!
		set( MAYASDK_HASH "2332C6C79B8C7974A5645523E395962BBB19901289AAAEE129D3BF2B630829C7" CACHE STRING "" )
		set( MAYASDK_MD5 "6BC28A0047709DEECC3DBB0D4A0B4D14" CACHE STRING "" )

		# Set the maya version (mainly for the project creation + reference)
		set( MAYA_VERSION 2019 )
	
		ExternalProject_Add( MayaDevKit
			URL ${MAYASDK_URL}
			URL_HASH SHA256=${MAYASDK_HASH}
			URL_MD5 ${MAYASDK_MD5}
			DOWNLOAD_DIR ${CMAKE_BINARY_DIR}/downloads
			SOURCE_DIR ${CMAKE_SOURCE_DIR}/libs/MayaSDKDevKit
			CONFIGURE_COMMAND ""
			BUILD_COMMAND ""
			INSTALL_COMMAND ""
		)

		add_definitions( -DID_MAYA_IMPORT_TOOL )

		set_target_properties( MayaDevKit PROPERTIES FOLDER "libs" )
	endif()

	if( MSVC )
		# This is required for tools on windows.
		find_package( MFC )
		if( NOT MFC_FOUND )
			message( WARNING "MFC ('Microsoft Foundation Classes for C++') couldn't be found, but is needed for TOOLS!" )
			message( WARNING "If you're using VS2013, you'll also need the 'Multibyte MFC Library for Visual Studio 2013': https://www.microsoft.com/en-us/download/details.aspx?id=40770 (VS2015 and 2017 include that in the default MFC package)" )
		else()
			set( MFC_TOOLS ON )
		endif()
	endif()
endif()

set( D3_COMPILER_IS_CLANG FALSE )
set( D3_COMPILER_IS_GCC_OR_CLANG FALSE )

if( NOT MSVC )
	if( NOT WIN32 )
		# libbacktrace support - TODO: might work with MinGW? we don't have a crash handler for win32 though..
		include( CheckCSourceCompiles )
		set( CMAKE_REQUIRED_LIBRARIES backtrace )
		check_c_source_compiles( "#include <backtrace.h>
		int main() { backtrace_create_state(NULL, 0, NULL, NULL); return 0; }" HAVE_LIBBACKTRACE )
		unset( CMAKE_REQUIRED_LIBRARIES )

		if( HAVE_LIBBACKTRACE )
			set( sys_libs ${sys_libs} backtrace )
			add_definitions( -DD3_HAVE_LIBBACKTRACE )
			message( STATUS "Using libbacktrace" )
		else()
			message( WARNING "libbacktrace wasn't found. It's not required but recommended, because it provides useful backtraces if engine/game crashes" )
		endif()
	endif() # NOT WIN32

	# check if our SDL2 supports X11 in SDL_syswm so we can use it for DPI scaling ImGui
	set( CMAKE_REQUIRED_LIBRARIES SDL2 )
	check_c_source_compiles( "#include <SDL_syswm.h>
		int main() { SDL_SysWMinfo wmInfo = {}; wmInfo.info.x11.display = NULL; return 0; }" HAVE_SDL_X11 )
	unset( CMAKE_REQUIRED_LIBRARIES )
		
	if ( HAVE_SDL_X11 )
		message( STATUS "This SDL2 has X11 support" )
		add_definitions( -DD3_SDL_X11 )
	endif()

	# check if this is some kind of clang (Clang, AppleClang, whatever)
	# (convert compiler ID to lowercase so we match Clang, clang, AppleClang etc, regardless of case)
	string( TOLOWER ${CMAKE_CXX_COMPILER_ID} compiler_id_lower )
	if( compiler_id_lower MATCHES ".*clang.*" )
		message( STATUS "Compiler \"${CMAKE_CXX_COMPILER_ID}\" detected as some kind of clang" )
		set( D3_COMPILER_IS_CLANG TRUE )
		set( D3_COMPILER_IS_GCC_OR_CLANG TRUE )
	elseif( CMAKE_COMPILER_IS_GNUCC )
		set( D3_COMPILER_IS_GCC_OR_CLANG TRUE )
	endif()
	unset( compiler_id_lower )
endif() # not MSVC

# compiler specific flags
if( D3_COMPILER_IS_GCC_OR_CLANG )
	add_compile_options( -pipe )
	add_compile_options( -Wall )

	if( NOT CMAKE_CROSSCOMPILING AND ONATIVE )
		add_compile_options( -march=native)
	elseif( NOT APPLE AND cpu STREQUAL "x86" )
		add_compile_options( -march=pentium3 )
	endif()

	if( FORCE_COLORED_OUTPUT)
		if( CMAKE_COMPILER_IS_GNUCC )
		   add_compile_options ( -fdiagnostics-color=always )
		elseif ( D3_COMPILER_IS_CLANG )
		   add_compile_options ( -fcolor-diagnostics )
		endif ()
	endif ()

	set( CMAKE_C_FLAGS_DEBUG "-g -ggdb -D_DEBUG -O0" )
	set( CMAKE_C_FLAGS_DEBUGALL "-g -ggdb -D_DEBUG" )
	set( CMAKE_C_FLAGS_PROFILE "-g -ggdb -D_DEBUG -O1 -fno-omit-frame-pointer" )
	set( CMAKE_C_FLAGS_RELEASE "-O2 -fno-math-errno -fno-trapping-math -fomit-frame-pointer" )
	set( CMAKE_C_FLAGS_RELWITHDEBINFO "-g -ggdb -O2 -fno-math-errno -fno-trapping-math -fno-omit-frame-pointer" )
	set( CMAKE_C_FLAGS_MINSIZEREL "-Os -fno-math-errno -fno-trapping-math -fomit-frame-pointer" )

	set( CMAKE_CXX_FLAGS_DEBUGALL ${CMAKE_C_FLAGS_DEBUGALL} )
	set( CMAKE_CXX_FLAGS_PROFILE ${CMAKE_C_FLAGS_PROFILE} )

	add_compile_options(-fno-strict-aliasing)

	# dear idiot compilers, don't fuck up math code with useless FMA "optimizations"
	# (https://gcc.gnu.org/bugzilla/show_bug.cgi?id=100839)
	CHECK_CXX_COMPILER_FLAG("-ffp-contract=off" cxx_has_fp-contract)
	if(cxx_has_fp-contract)
		add_compile_options(-ffp-contract=off)
	endif()
	
	if( ASAN )
		# if this doesn't work, ASan might not be available on your platform, don't set ASAN then..
		add_compile_options( -fsanitize=address )
		set( sys_libs ${sys_libs} -fsanitize=address )
	endif()

	if( UBSAN )
		# if this doesn't work, UBSan might not be available on your platform, don't set UBSAN then..
		add_compile_options( -fsanitize=undefined )
		set( sys_libs ${sys_libs} -fsanitize=undefined )
	endif()

	CHECK_CXX_COMPILER_FLAG( "-fvisibility=hidden" cxx_has_fvisibility )
	if( NOT cxx_has_fvisibility )
		message( FATAL_ERROR "Compiler does not support -fvisibility" )
	endif()
	add_compile_options( -fvisibility=hidden )

	# TODO fix these warnings
	add_compile_options( -Wno-sign-compare )
	add_compile_options( -Wno-switch )

	CHECK_CXX_COMPILER_FLAG( "-Woverloaded-virtual" cxx_has_Woverload_virtual )
	if( cxx_has_Woverload_virtual )
		set( CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} -Woverloaded-virtual" )
	endif()
	
	# shut up about using memcpy() on classes, in the cases doom3 uses it it seems to be fine
	CHECK_CXX_COMPILER_FLAG( "-Wno-class-memaccess" cxx_has_Wno-class-memaccess )
	if( cxx_has_Wno-class-memaccess )
		set( CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} -Wno-class-memaccess" )
	endif()

	# ignore warnings about variables named "requires" for now (in C++20 it's a keyword,
	#  but currently we don't even use C++11 features)
	CHECK_CXX_COMPILER_FLAG( "-Wno-c++20-compat" cxx_has_Wno-cpp20-compat )
	if( cxx_has_Wno-cpp20-compat )
		set( CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} -Wno-c++20-compat" )
	endif()
	
	if ( NOT MSVC )
		find_package( OpenSSL REQUIRED )
	endif()

	if(APPLE)
		add_definitions(-DMACOS_X=1)

		if( cpu STREQUAL "x86_64" )
			add_compile_options( -arch x86_64 -mmacosx-version-min=10.9 )
			set( ldflags "${ldflags} -arch x86_64 -mmacosx-version-min=10.9" )
		elseif( cpu STREQUAL "arm64" )
			add_compile_options( -arch arm64 -mmacosx-version-min=11.0 )
			set( ldflags "${ldflags} -arch arm64 -mmacosx-version-min=11.0" )
		elseif( cpu STREQUAL "x86" )
			CHECK_CXX_COMPILER_FLAG( "-arch i386" cxx_has_arch_i386 )
			if( cxx_has_arch_i386 )
				add_compile_options( -arch i386 )
				set( ldflags "${ldflags} -arch i386" )
			endif()

			add_compile_options( -mmacosx-version-min=10.4 )
			set( ldflags "${ldflags} -mmacosx-version-min=10.4" )
		elseif( cpu STREQUAL "ppc" )
			CHECK_CXX_COMPILER_FLAG( "-arch ppc" cxx_has_arch_ppc )
			if( cxx_has_arch_ppc )
				add_compile_options( -arch ppc -mone-byte-bool )
				set( ldflags "${ldflags} -arch ppc -mone-byte-bool" )
			endif()

			add_compile_options( -mmacosx-version-min=10.4 )
			set( ldflags "${ldflags} -mmacosx-version-min=10.4" )
		else()
			message( FATAL_ERROR "Unsupported CPU architecture for OSX" )
		endif()

		set( sys_libs ${sys_libs} "OpenSSL::SSL OpenSSL::Crypto -framework Carbon -framework Cocoa -framework IOKit" )
	elseif( WIN32 )
		# use 8MB stack instead of 1MB, so big models don't overflow it with _alloca()
		set( ldflags "${ldflags} -static-libgcc -static-libstdc++ -Wl, --large-address-aware --stack,8388608" )
	elseif( os STREQUAL "linux" )
		set( sys_libs ${sys_libs} dl OpenSSL::SSL OpenSSL::Crypto rt )
	elseif( os STREQUAL "freebsd" )
		find_library( EXECINFO_LIBRARIES execinfo /usr/lib /usr/local/lib )
		set( sys_libs ${sys_libs} OpenSSL::SSL OpenSSL::Crypto rt ${EXECINFO_LIBRARIES} )
	endif()
elseif( MSVC )
	add_compile_options( /MP ) # parallel build (use all cores, or as many as configured in VS)
	add_compile_options( /W3 ) # TODO: was /W4, caused trouble with VS2019 (and/or its integrated CMake? or only HarrieVG's setup?)

	add_compile_options( /we4840 ) # treat as error when passing a class to a vararg-function (probably printf-like)
	add_compile_options( /we4306 /we4311 /we4312 /we4302 )	# treat several kinds of truncating int<->pointer conversions as errors (for more 64bit-safety)
	# ignore the following warnings:
	add_compile_options( /wd4100 ) # unreferenced formal parameter
	add_compile_options( /wd4127 ) # conditional expression is constant
	add_compile_options( /wd4244 ) # possible loss of data
	add_compile_options( /wd4245 ) # signed/unsigned mismatch
	add_compile_options( /wd4267 ) # possible loss of data
	add_compile_options( /wd4714 ) # 'function' marked as __forceinline not inlined
	add_compile_options( /wd4996 ) # 'function': was declared deprecated
	add_compile_options( /wd4068 ) # unknown pragma
	add_compile_options( /wd4458 ) # declaration of 'variable' hides class member
	add_compile_options( /wd4018 ) # 'math symbol': signed/unsigned mismatch
	add_compile_options( /wd4018 ) # 'math symbol': signed/unsigned mismatch
	add_compile_options( /wd26495 ) # variable 'variable' is uninitialized. Always initialize a member variable (type.6).
	add_compile_options( /wd4146 ) # unary minus operator applied to unsigned type, result still unsigned
	add_compile_options( /wd4805 ) # 'symbol' unsafe mix of type 'type a' and type 'type b' in operation
	add_compile_options( /wd4273 ) # 'function': inconsistent dll linkage
	add_compile_options( /wd4838 ) # conversion from 'type' to 'type' requires a narrowing conversion

	
	add_definitions( -D_ALLOW_KEYWORD_MACROS ) # because of the "#define private public" and "#define protected public" in TypeInfo.cpp
	
	set( CMAKE_C_FLAGS_DEBUG "-D_DEBUG /Od /Zi /MDd" )
	set( CMAKE_C_FLAGS_RELEASE "/Ox /Oy /MD" )
	set( CMAKE_C_FLAGS_RELWITHDEBINFO "/Ox /Oy /Zi /MD" )
	set( CMAKE_C_FLAGS_MINSIZEREL "/Ox /Oy /Os /MD" )
	
	# use 8MB stack instead of 1MB, so big models don't overflow it with _alloca()
	set( ldflags "${ldflags} /STACK:8388608" )
	
	# let the 32bit app use more than 2gb of ram
	set( ldflags "${ldflags} /LARGEADDRESSAWARE" )
	
	# Don't let the linker generate manifest files for shared dlls
	set( CMAKE_SHARED_LINKER_FLAGS "${CMAKE_SHARED_LINKER_FLAGS} /MANIFEST:NO" )
else()
	message( FATAL_ERROR "Unsupported compiler" )
endif()

# Drop the binaries in the output/${os} folder
set( OUTPUT_FOLDER ${CMAKE_SOURCE_DIR}/../output/${os} )

set( CMAKE_CXX_FLAGS_DEBUG ${CMAKE_C_FLAGS_DEBUG} )
set( CMAKE_CXX_FLAGS_RELWITHDEBINFO ${CMAKE_C_FLAGS_RELWITHDEBINFO} )

# disable assertions in release builds
set( CMAKE_CXX_FLAGS_RELEASE "-DNDEBUG ${CMAKE_C_FLAGS_RELEASE}" )
set( CMAKE_CXX_FLAGS_MINSIZEREL "-DNDEBUG ${CMAKE_C_FLAGS_MINSIZEREL}" )

# mingw and msvc
if( WIN32 )
	add_definitions( -DWINVER=0x0501 )
	add_definitions( -D_WIN32_WINNT=0x0501 )

	set( sys_libs ${sys_libs}
		winmm
		iphlpapi
		wsock32
		ole32
	)
endif()

# Inform us of the current branch we're on.
get_git_branch( branch )
message( STATUS "Setting project on git branch: ${branch}" )

# optimize this source file even in debug builds, to make speed (esp. loading times) more bearable
# used for libs integrated here like stb_* and miniz(ip)
if(CMAKE_MAJOR_VERSION LESS 3 OR ( CMAKE_MAJOR_VERSION EQUAL 3 AND CMAKE_MINOR_VERSION LESS 11 ))
	function(always_optimize_sourcefile srcfilename)
		if(MSVC)
			set_source_files_properties(${srcfilename} PROPERTIES COMPILE_FLAGS "/Ox")
		else()
			set_source_files_properties(${srcfilename} PROPERTIES COMPILE_FLAGS "-O2")
		endif()
	endfunction()
else()
	function(always_optimize_sourcefile srcfilename)
		if(MSVC)
			set_source_files_properties(${srcfilename} PROPERTIES COMPILE_OPTIONS "/Ox")
		else()
			set_source_files_properties(${srcfilename} PROPERTIES COMPILE_OPTIONS "-O2")
		endif()
	endfunction()
endif()

set( bindir		"${CMAKE_INSTALL_FULL_BINDIR}" )
set( libdir		"${CMAKE_INSTALL_FULL_LIBDIR}/dhewm3" )
set( datadir	"${CMAKE_INSTALL_FULL_DATADIR}/dhewm3" )

configure_file(
	"${CMAKE_SOURCE_DIR}/config.h.in"
	"${CMAKE_BINARY_DIR}/config.h"
)

if( NOT MSVC )
	message( STATUS "Building ${CMAKE_BUILD_TYPE} for ${os}-${cpu}" )
endif()

if( NOT APPLE AND NOT WIN32 )
	message( STATUS "The install target will use the following directories:" )
	message( STATUS "  Binary directory:  ${bindir}" )
	message( STATUS "  Library directory: ${libdir}" )
	message( STATUS "  Data directory:    ${datadir}" )
endif()

set(src_aas_file
	libs/aasfile/AASFile.cpp
	libs/aasfile/AASFile_optimize.cpp
	libs/aasfile/AASFile_sample.cpp
	libs/aasfile/AASFileManager.cpp
)

add_globbed_headers(src_aas_file "libs/aasfile")

set(src_renderer
	renderer/Cinematic.cpp
	renderer/GuiModel.cpp
	renderer/Image_files.cpp
	renderer/Image_init.cpp
	renderer/Image_load.cpp
	renderer/Image_process.cpp
	renderer/Image_program.cpp
	renderer/Interaction.cpp
	renderer/Material.cpp
	renderer/MegaTexture.cpp
	renderer/Model.cpp
	renderer/ModelDecal.cpp
	renderer/ModelManager.cpp
	renderer/ModelOverlay.cpp
	renderer/Model_beam.cpp
	renderer/Model_ase.cpp
	renderer/Model_liquid.cpp
	renderer/Model_lwo.cpp
	renderer/Model_ma.cpp
	renderer/Model_md3.cpp
	renderer/Model_md5.cpp
	renderer/Model_obj.cpp
	renderer/Model_prt.cpp
	renderer/Model_sprite.cpp
	renderer/RenderEntity.cpp
	renderer/RenderSystem.cpp
	renderer/RenderSystem_init.cpp
	renderer/RenderWorld.cpp
	renderer/RenderWorld_demo.cpp
	renderer/RenderWorld_load.cpp
	renderer/RenderWorld_portals.cpp
	renderer/VertexCache.cpp
	renderer/draw_arb2.cpp
	renderer/draw_common.cpp
	renderer/tr_backend.cpp
	renderer/tr_deform.cpp
	renderer/tr_font.cpp
	renderer/tr_guisurf.cpp
	renderer/tr_light.cpp
	renderer/tr_lightrun.cpp
	renderer/tr_main.cpp
	renderer/tr_orderIndexes.cpp
	renderer/tr_polytope.cpp
	renderer/tr_render.cpp
	renderer/tr_rendertools.cpp
	renderer/tr_shadowbounds.cpp
	renderer/tr_stencilshadow.cpp
	renderer/tr_subview.cpp
	renderer/tr_trace.cpp
	renderer/tr_trisurf.cpp
	renderer/tr_turboshadow.cpp
)

add_globbed_headers(src_renderer "renderer")

if ( NOT MAYA_IMPORT )
	list( REMOVE_ITEM src_renderer
		renderer/Model_ma.cpp
		renderer/Model_ma.h
	)
endif()

set(src_framework
	framework/CVarSystem.cpp
	framework/CmdSystem.cpp
	framework/Common.cpp
	framework/Compressor.cpp
	framework/Console.cpp
	framework/DemoFile.cpp
	framework/DeclAF.cpp
	framework/DeclEntityDef.cpp
	framework/DeclFX.cpp
	framework/DeclManager.cpp
	framework/DeclParticle.cpp
	framework/DeclPDA.cpp
	framework/DeclSkin.cpp
	framework/DeclTable.cpp
	framework/Dhewm3SettingsMenu.cpp
	framework/EditField.cpp
	framework/EventLoop.cpp
	framework/File.cpp
	framework/FileSystem.cpp
	framework/KeyInput.cpp
	framework/UsercmdGen.cpp
	framework/Session_menu.cpp
	framework/Session.cpp
	framework/async/AsyncClient.cpp
	framework/async/AsyncNetwork.cpp
	framework/async/AsyncServer.cpp
	framework/async/MsgChannel.cpp
	framework/async/NetworkSystem.cpp
	framework/async/ServerScan.cpp
	framework/miniz/miniz.c
	framework/minizip/ioapi.c
	framework/minizip/unzip.cpp
)

# Don't use precompiled headers for miniz/minizip sources.
set_source_files_properties(
	framework/miniz/miniz.c
	framework/minizip/ioapi.c
	PROPERTIES
	SKIP_PRECOMPILE_HEADERS ON
	LANGUAGE C
)

always_optimize_sourcefile(framework/miniz/miniz.c)
always_optimize_sourcefile(framework/minizip/ioapi.c)
always_optimize_sourcefile(framework/minizip/unzip.cpp)

add_globbed_headers(src_framework "framework")

set(src_cm
	cm/CollisionModel_contacts.cpp
	cm/CollisionModel_contents.cpp
	cm/CollisionModel_debug.cpp
	cm/CollisionModel_files.cpp
	cm/CollisionModel_load.cpp
	cm/CollisionModel_rotate.cpp
	cm/CollisionModel_trace.cpp
	cm/CollisionModel_translate.cpp
)

add_globbed_headers(src_cm "cm")

set(src_dmap
	tools/compilers/dmap/dmap.cpp
	tools/compilers/dmap/facebsp.cpp
	tools/compilers/dmap/leakfile.cpp
	tools/compilers/dmap/map.cpp
	tools/compilers/dmap/optimize.cpp
	tools/compilers/dmap/output.cpp
	tools/compilers/dmap/portals.cpp
	tools/compilers/dmap/shadowopt3.cpp
	tools/compilers/dmap/tritjunction.cpp
	tools/compilers/dmap/tritools.cpp
	tools/compilers/dmap/ubrush.cpp
	tools/compilers/dmap/usurface.cpp
)

add_globbed_headers(src_dmap "tools/compilers/dmap")

set(src_aas
	tools/compilers/aas/AASBuild.cpp
	tools/compilers/aas/AASBuild_file.cpp
	tools/compilers/aas/AASBuild_gravity.cpp
	tools/compilers/aas/AASBuild_ledge.cpp
	tools/compilers/aas/AASBuild_merge.cpp
	tools/compilers/aas/AASCluster.cpp
	tools/compilers/aas/AASReach.cpp
	tools/compilers/aas/Brush.cpp
	tools/compilers/aas/BrushBSP.cpp
)

add_globbed_headers(src_aas "tools/compilers/aas")

set(src_roq
	tools/compilers/roqvq/NSBitmapImageRep.cpp
	tools/compilers/roqvq/codec.cpp
	tools/compilers/roqvq/roq.cpp
	tools/compilers/roqvq/roqParam.cpp
)

add_globbed_headers(src_roq "tools/compilers/roqvq")

set(src_renderbump
	tools/compilers/renderbump/renderbump.cpp
)

add_globbed_headers(src_renderbump "tools/compilers/renderbump")

set(src_matbuild
	tools/compilers/matbuild/matbuild.cpp
)

add_globbed_headers(src_matbuild "tools/compilers/matbuild")

set(src_snd
	sound/snd_cache.cpp
	sound/snd_decoder.cpp
	sound/snd_efxfile.cpp
	sound/snd_emitter.cpp
	sound/snd_reverb.cpp
	sound/snd_shader.cpp
	sound/snd_system.cpp
	sound/snd_wavefile.cpp
	sound/snd_world.cpp
)

add_globbed_headers(src_snd "sound")

set(src_ui
	ui/BindWindow.cpp
	ui/ChoiceWindow.cpp
	ui/DeviceContext.cpp
	ui/EditWindow.cpp
	ui/FieldWindow.cpp
	ui/GameBearShootWindow.cpp
	ui/GameBustOutWindow.cpp
	ui/GameSSDWindow.cpp
	ui/GuiScript.cpp
	ui/ListGUI.cpp
	ui/ListWindow.cpp
	ui/MarkerWindow.cpp
	ui/RegExp.cpp
	ui/RenderWindow.cpp
	ui/SimpleWindow.cpp
	ui/SliderWindow.cpp
	ui/UserInterface.cpp
	ui/Window.cpp
	ui/Winvar.cpp
)

add_globbed_headers(src_ui "ui")

set(src_idlib
	idlib/bv/Bounds.cpp
	idlib/bv/Frustum.cpp
	idlib/bv/Sphere.cpp
	idlib/bv/Box.cpp
	idlib/geometry/DrawVert.cpp
	idlib/geometry/Winding2D.cpp
	idlib/geometry/Surface_SweptSpline.cpp
	idlib/geometry/Winding.cpp
	idlib/geometry/Surface.cpp
	idlib/geometry/Surface_Patch.cpp
	idlib/geometry/TraceModel.cpp
	idlib/geometry/JointTransform.cpp
	idlib/hashing/CRC32.cpp
	idlib/hashing/MD4.cpp
	idlib/hashing/MD5.cpp
	idlib/math/Angles.cpp
	idlib/math/Lcp.cpp
	idlib/math/Math.cpp
	idlib/math/Matrix.cpp
	idlib/math/Ode.cpp
	idlib/math/Plane.cpp
	idlib/math/Pluecker.cpp
	idlib/math/Polynomial.cpp
	idlib/math/Quat.cpp
	idlib/math/Rotation.cpp
	idlib/math/Simd.cpp
	idlib/math/Simd_Generic.cpp
	idlib/math/Simd_MMX.cpp
	idlib/math/Simd_SSE.cpp
	idlib/math/Simd_SSE2.cpp
	idlib/math/Simd_SSE3.cpp
	idlib/math/Vector.cpp
	idlib/BitMsg.cpp
	idlib/LangDict.cpp
	idlib/Lexer.cpp
	idlib/Lib.cpp
	idlib/containers/HashIndex.cpp
	idlib/Dict.cpp
	idlib/Str.cpp
	idlib/Parser.cpp
	idlib/precompiled.cpp
	idlib/MapFile.cpp
	idlib/CmdArgs.cpp
	idlib/Token.cpp
	idlib/Base64.cpp
	idlib/Timer.cpp
	idlib/Heap.cpp
)

add_globbed_headers(src_idlib "idlib")


set(src_mikktspace
	libs/mikktspace/mikktspace.c
)

# Don't use precompiled headers for mikktspace sources.
set_source_files_properties(
	libs/mikktspace/mikktspace.c
	PROPERTIES
	SKIP_PRECOMPILE_HEADERS ON
	LANGUAGE C
)

add_globbed_headers(src_mikktspace "mikktspace")

if( IMGUI )
	set(src_imgui
		libs/imgui/backends/imgui_impl_sdl2.cpp
		libs/imgui/backends/imgui_impl_opengl2.cpp
		
		libs/imgui/imgui.h
		libs/imgui/imgui.cpp
		libs/imgui/imgui_draw.cpp
		libs/imgui/imgui_tables.cpp
		libs/imgui/imgui_widgets.cpp
		
		libs/imgui/imgui_demo.cpp
		
		sys/sys_imgui.h
		sys/sys_imgui.cpp
		sys/imgui_savestyle.cpp
	)

	# Don't use precompiled headers for imgui sources.
	set_source_files_properties(
		libs/imgui/backends/imgui_impl_sdl2.cpp
		libs/imgui/backends/imgui_impl_opengl2.cpp
		libs/imgui/imgui.cpp
		libs/imgui/imgui_draw.cpp
		libs/imgui/imgui_tables.cpp
		libs/imgui/imgui_widgets.cpp
		libs/imgui/imgui_demo.cpp
		PROPERTIES
		SKIP_PRECOMPILE_HEADERS ON
	)
else()
	set(src_imgui
		sys/sys_imgui.h
	)
endif()

set(src_game
	game/AF.cpp
	game/AFEntity.cpp
	game/AnimatedEntity.cpp
	game/AnimState.cpp
	game/Actor.cpp
	game/Actor_script.cpp
	game/Camera.cpp
	game/Entity.cpp
	game/Entity_script.cpp
	game/BrittleFracture.cpp
	game/Fx.cpp
	game/GameEdit.cpp
	game/Game_local.cpp
	game/Game_network.cpp
	game/Item.cpp
	game/IK.cpp
	game/Light.cpp
	game/Misc.cpp
	game/Mover.cpp
	game/Moveable.cpp
	game/MultiplayerGame.cpp
	game/Player.cpp
	game/PlayerIcon.cpp
	game/PlayerView.cpp
	game/Projectile.cpp
	game/Pvs.cpp
	game/SecurityCamera.cpp
	game/SmokeParticles.cpp
	game/Sound.cpp
	game/Target.cpp
	game/Trigger.cpp
	game/Weapon.cpp
	game/Weapon_script.cpp
	game/WorldSpawn.cpp
	game/aas/AAS.cpp
	game/aas/AAS_debug.cpp
	game/aas/AAS_pathing.cpp
	game/aas/AAS_routing.cpp
	game/ai/AI.cpp
	game/ai/AI_events.cpp
	game/ai/AI_pathing.cpp
	game/ai/AI_script.cpp
	game/ai/AI_Vagary.cpp
	game/gamesys/DebugGraph.cpp
	game/gamesys/Class.cpp
	game/gamesys/Event.cpp
	game/gamesys/SaveGame.cpp
	game/gamesys/State.cpp
	game/gamesys/SysCmds.cpp
	game/gamesys/SysCvar.cpp
	game/anim/Anim.cpp
	game/anim/Anim_Blend.cpp
	game/anim/Anim_Import.cpp
	game/anim/Anim_Testmodel.cpp
	game/script/Script_Compiler.cpp
	game/script/Script_Interpreter.cpp
	game/script/Script_Program.cpp
	game/script/Script_Thread.cpp
	game/physics/Clip.cpp
	game/physics/Force.cpp
	game/physics/Force_Constant.cpp
	game/physics/Force_Drag.cpp
	game/physics/Force_Field.cpp
	game/physics/Force_Spring.cpp
	game/physics/Physics.cpp
	game/physics/Physics_AF.cpp
	game/physics/Physics_Actor.cpp
	game/physics/Physics_Base.cpp
	game/physics/Physics_Monster.cpp
	game/physics/Physics_Liquid.cpp
	game/physics/Physics_Parametric.cpp
	game/physics/Physics_Player.cpp
	game/physics/Physics_RigidBody.cpp
	game/physics/Physics_Static.cpp
	game/physics/Physics_StaticMulti.cpp
	game/physics/Push.cpp
)

add_globbed_headers(src_game "game")

if ( NOT MAYA_IMPORT )
	list( REMOVE_ITEM src_game
		game/anim/Anim_Import.cpp
		game/anim/Anim_Import.h
	)
endif()

set(src_d3xp
	d3xp/AF.cpp
	d3xp/AFEntity.cpp
	d3xp/AnimatedEntity.cpp
	d3xp/AnimState.cpp
	d3xp/Actor.cpp
	d3xp/Actor_script.cpp
	d3xp/Camera.cpp
	d3xp/Entity.cpp
	d3xp/Entity_script.cpp
	d3xp/BrittleFracture.cpp
	d3xp/Fx.cpp
	d3xp/GameEdit.cpp
	d3xp/Game_local.cpp
	d3xp/Game_network.cpp
	d3xp/Item.cpp
	d3xp/IK.cpp
	d3xp/Light.cpp
	d3xp/Misc.cpp
	d3xp/Mover.cpp
	d3xp/Moveable.cpp
	d3xp/MultiplayerGame.cpp
	d3xp/Player.cpp
	d3xp/PlayerIcon.cpp
	d3xp/PlayerView.cpp
	d3xp/Projectile.cpp
	d3xp/Pvs.cpp
	d3xp/SecurityCamera.cpp
	d3xp/SmokeParticles.cpp
	d3xp/Sound.cpp
	d3xp/Target.cpp
	d3xp/Trigger.cpp
	d3xp/Weapon.cpp
	d3xp/Weapon_script.cpp
	d3xp/WorldSpawn.cpp
	d3xp/aas/AAS.cpp
	d3xp/aas/AAS_debug.cpp
	d3xp/aas/AAS_pathing.cpp
	d3xp/aas/AAS_routing.cpp
	d3xp/ai/AI.cpp
	d3xp/ai/AI_events.cpp
	d3xp/ai/AI_pathing.cpp
	d3xp/ai/AI_script.cpp
	d3xp/ai/AI_Vagary.cpp
	d3xp/gamesys/DebugGraph.cpp
	d3xp/gamesys/Class.cpp
	d3xp/gamesys/Event.cpp
	d3xp/gamesys/SaveGame.cpp
	d3xp/gamesys/State.cpp
	d3xp/gamesys/SysCmds.cpp
	d3xp/gamesys/SysCvar.cpp
	d3xp/anim/Anim.cpp
	d3xp/anim/Anim_Blend.cpp
	d3xp/anim/Anim_Import.cpp
	d3xp/anim/Anim_Testmodel.cpp
	d3xp/script/Script_Compiler.cpp
	d3xp/script/Script_Interpreter.cpp
	d3xp/script/Script_Program.cpp
	d3xp/script/Script_Thread.cpp
	d3xp/physics/Clip.cpp
	d3xp/physics/Force.cpp
	d3xp/physics/Force_Constant.cpp
	d3xp/physics/Force_Drag.cpp
	d3xp/physics/Force_Field.cpp
	d3xp/physics/Force_Spring.cpp
	d3xp/physics/Physics.cpp
	d3xp/physics/Physics_AF.cpp
	d3xp/physics/Physics_Actor.cpp
	d3xp/physics/Physics_Base.cpp
	d3xp/physics/Physics_Monster.cpp
	d3xp/physics/Physics_Liquid.cpp
	d3xp/physics/Physics_Parametric.cpp
	d3xp/physics/Physics_Player.cpp
	d3xp/physics/Physics_RigidBody.cpp
	d3xp/physics/Physics_Static.cpp
	d3xp/physics/Physics_StaticMulti.cpp
	d3xp/physics/Push.cpp
	d3xp/Grabber.cpp
	d3xp/physics/Force_Grab.cpp
)

add_globbed_headers(src_d3xp "d3xp")

if ( NOT MAYA_IMPORT )
	list( REMOVE_ITEM src_d3xp
		d3xp/anim/Anim_Import.cpp
		d3xp/anim/Anim_Import.h
	)
endif()

set(src_debuggerServer
	tools/debugger/DebuggerBreakpoint.h
	tools/debugger/DebuggerBreakpoint.cpp
	tools/debugger/DebuggerServer.h
	tools/debugger/DebuggerServer.cpp
	tools/debugger/DebuggerScript.h
	tools/debugger/DebuggerScript.cpp
	tools/debugger/DebuggerMessages.h
	tools/debugger/debugger.cpp
)

set(src_core
	${src_aas_file}
	${src_renderer}
	${src_mikktspace}
	${src_framework}
	${src_cm}
	${src_dmap}
	${src_aas}
	${src_roq}
	${src_renderbump}
	${src_matbuild}
	${src_snd}
	${src_ui}
	${src_tools}
)

set(src_stub_openal		sys/stub/openal_stub.cpp)
set(src_stub_gl			sys/stub/stub_gl.cpp)


set(src_tools
	tools/guied/GEWindowWrapper_stub.cpp
)

# TODO: add_globbed_headers(src_tools "tools/guied")

# Begin normal (MFC) tools code
if ( TOOLS AND MFC_TOOLS AND MSVC )
	set(ALLOWRESOURCES ON)
	# Common files.
	file(GLOB src_comafx "tools/comafx/*.cpp")
	add_globbed_headers(src_comafx "tools/comafx")
	file(GLOB_RECURSE src_tools_common "tools/common/*.cpp")
	add_globbed_headers(src_tools_common "tools/common")

	#Compilers.
	# DG: this is always added, why add it again? file(GLOB src_compiler_renderbump "neo/tools/compilers/renderbump/*.cpp")
	file(GLOB src_compiler_roqvq	  "tools/compilers/roqvq/*.cpp") # FIXME: some of those were added before
	add_globbed_headers(src_compiler_roqvq "tools/compilers/roqvq")
	#tools
	# Articulated Frame editor.
	file(GLOB src_afeditor "tools/af/*.cpp")
	add_globbed_headers(src_afeditor "tools/af")
	# Declaration editor
	file(GLOB src_decleditor "tools/decl/*.cpp")
	add_globbed_headers(src_decleditor "tools/decl")
	# GUI Editor.
	file(GLOB src_gui_editor "tools/guied/*.cpp")
	add_globbed_headers(src_gui_editor "tools/guied")
	# Material editor
	file(GLOB src_materialeditor "tools/materialeditor/*.cpp")
	add_globbed_headers(src_materialeditor "tools/materialeditor")
	# Particle Editor
	file(GLOB src_particleeditor "tools/particle/*.cpp")
	add_globbed_headers(src_particleeditor "tools/particle")
	# PDA editor
	file(GLOB src_pdaeditor "tools/pda/*.cpp")
	add_globbed_headers(src_pdaeditor "tools/pda")
	# Radiant ( Map editor )
	file(GLOB src_map_editor "tools/radiant/*.cpp")
	add_globbed_headers(src_map_editor "tools/radiant")
	# Script editor
	file(GLOB src_script_editor "tools/script/*.cpp")
	add_globbed_headers(src_script_editor "tools/script")
	# Script Debugger
	set(src_debuggerClient
		tools/debugger/DebuggerClient.h
		tools/debugger/DebuggerClient.cpp
		tools/debugger/DebuggerApp.h
		tools/debugger/DebuggerApp.cpp
		tools/debugger/DebuggerQuickWatchDlg.h
		tools/debugger/DebuggerQuickWatchDlg.cpp
		tools/debugger/DebuggerWindow.h
		tools/debugger/DebuggerWindow.cpp
		tools/debugger/DebuggerFindDlg.h
		tools/debugger/DebuggerFindDlg.cpp
	)
	set(src_script_debugger
		${src_debuggerServer}
		${src_debuggerClient}
	)
	# sound editor?
	file(GLOB src_sound_editor "tools/sound/*.cpp")
	add_globbed_headers(src_sound_editor "tools/sound")
	

	# The numerous tools in a nice list.
	list(APPEND src_editor_tools
		${src_comafx}
		${src_tools_common}
		${src_compiler_renderbump}
		${src_afeditor}
		${src_decleditor}
		${src_gui_editor}
		${src_materialeditor}
		${src_particleeditor}
		${src_pdaeditor}
		${src_map_editor}
		${src_script_editor}
		${src_sound_editor}
		${src_script_debugger}
		"tools/edit_public.h"
		)
	SET(CMAKE_MFC_FLAG 2)
	set(TOOLS_DEFINES "ID_ALLOW_TOOLS;_AFXDLL")
else()
	set(src_editor_tools "tools/edit_stub.cpp" "tools/edit_public.h" )
	list(APPEND src_editor_tools
		${src_debuggerServer}
	)
endif()

if(APPLE)
	set(OSX_RESOURCE_FILES
		"${CMAKE_SOURCE_DIR}/sys/osx/Doom3.icns"
		"${CMAKE_SOURCE_DIR}/sys/osx/Doom 3.rsrc"
	)

	set_source_files_properties(${OSX_RESOURCE_FILES} PROPERTIES MACOSX_PACKAGE_LOCATION Resources)

	set(src_sys_base
		sys/cpu.cpp
		sys/threads.cpp
		sys/events.cpp
		sys/sys_local.cpp
		sys/posix/posix_net.cpp
		sys/posix/posix_main.cpp
	)

	set(src_sys_core
		sys/glimp.cpp
		sys/osx/DOOMController.mm
		sys/osx/macosx_misc.mm
		sys/osx/SDLMain.m
		${OSX_RESOURCE_FILES}
	)
elseif(WIN32)

	set(src_sys_base
		sys/cpu.cpp
		sys/threads.cpp
		sys/events.cpp
		sys/sys_local.cpp
		sys/win32/win_input.cpp
		sys/win32/win_main.cpp
		sys/win32/win_net.cpp
		sys/win32/win_shared.cpp
		sys/win32/win_syscon.cpp
		sys/win32/SDL_win32_main.c
	)
	
	# Don't use precompiled headers for our SDL Win32 c sources.
	set_source_files_properties(
		sys/win32/SDL_win32_main.c
		PROPERTIES
		SKIP_PRECOMPILE_HEADERS ON
		LANGUAGE C
	)

	# adding the few relevant headers in sys/ manually..
	set(src_sys_base ${src_sys_base}
		sys/platform.h
		sys/sys_local.h
		sys/sys_public.h
		sys/win32/win_local.h
	)

	set(src_sys_core
		sys/glimp.cpp
		"sys/win32/rc/windows_default.manifest"
		"sys/win32/rc/windows_dpiaware.manifest"
		"sys/win32/rc/windows_visualstyles.manifest"
	)

	if(TOOLS)
		file(GLOB src_sys_tools_images "sys/win32/rc/res/*.bmp" "sys/win32/rc/res/*.ico" "sys/win32/rc/res/*.cur")
		set(src_sys_core
			${src_sys_core}
			${src_sys_tools_images}
			"sys/win32/rc/doom.rc"
			"sys/win32/rc/resource.h"
		)
	else()
		# adding both .rc files breaks the build, but I think it only contains the icon
		# and doom.rc also contains that (+icons for editor etc, AFAIK), so this is fine
		set(src_sys_core
			${src_sys_core}
			"sys/win32/rc/res/doom3.ico"
			"sys/win32/rc/dhewm3.rc"
			"sys/win32/rc/dhewm3_resource.h"
		)
	endif()

else()
	set(src_sys_base
		sys/cpu.cpp
		sys/threads.cpp
		sys/events.cpp
		sys/sys_local.cpp
		sys/posix/posix_net.cpp
		sys/posix/posix_main.cpp
		sys/linux/main.cpp
	)

	set(src_sys_core
		sys/glimp.cpp
	)
endif()

include_directories(${CMAKE_BINARY_DIR})
include_directories(${CMAKE_SOURCE_DIR})

# ================================= Idlib ======================================

add_library( idlib STATIC ${src_idlib} )
if( D3_COMPILER_IS_GCC_OR_CLANG AND NOT MINGW )
	set_target_properties( idlib PROPERTIES COMPILE_FLAGS "-fPIC" )
endif()

if ( MSVC )
	target_sources( idlib INTERFACE "sys/win32/natvis/idlib.natvis" )
	source_group( TREE ${CMAKE_CURRENT_SOURCE_DIR} PREFIX neo FILES ${src_idlib} )
endif()

target_include_directories( idlib PRIVATE "${CMAKE_SOURCE_DIR}/idlib" )
target_precompile_headers( idlib PRIVATE "${CMAKE_SOURCE_DIR}/idlib/precompiled.h" )

set_target_properties( idlib PROPERTIES FOLDER "libs" )

# ================================= Engine ======================================

if( HARDLINK_GAME )
	if( BASE )
		set( src_core ${src_core} ${src_game} )
		set( game_includes "${CMAKE_SOURCE_DIR}/game" )
		if( D3XP )
			message( WARNING "hardlinking the base game, d3xp will not be supported (disable BASE for D3XP support with HARDLINK_GAME)" )
		endif()	
	elseif( D3XP )
		set( src_core ${src_core} ${src_d3xp} )
		set( game_includes "${CMAKE_SOURCE_DIR}/d3xp" )
	endif()
endif()

if( ENGINE )
	add_executable( ${DHEWM3BINARY} WIN32 MACOSX_BUNDLE
		${src_core}
		${src_sys_base}
		${src_sys_core}
		${src_imgui}
		${src_editor_tools}
	)
	
	if( MSVC )
		# set MSVC default debug directory on executable	
		set_target_properties( ${DHEWM3BINARY} PROPERTIES VS_STARTUP_PROJECT ${DHEWM3BINARY} )
		set_target_properties( ${DHEWM3BINARY} PROPERTIES VS_DEBUGGER_COMMAND "${OUTPUT_FOLDER}/${DHEWM3BINARY}.exe" )
		set_target_properties( ${DHEWM3BINARY} PROPERTIES VS_DEBUGGER_WORKING_DIRECTORY "${OUTPUT_FOLDER}" )
		set_target_properties( ${DHEWM3BINARY} PROPERTIES VS_DEBUGGER_COMMAND_ARGUMENTS "+set developer 1" )

		source_group( TREE ${CMAKE_CURRENT_SOURCE_DIR} PREFIX neo FILES ${src_core} ${src_sys_base} ${src_sys_core} ${src_imgui} ${src_editor_tools} )
	endif()

	target_precompile_headers( ${DHEWM3BINARY} PRIVATE "${CMAKE_SOURCE_DIR}/idlib/precompiled.h" )
	
	target_include_directories( ${DHEWM3BINARY} PRIVATE "${CMAKE_SOURCE_DIR}/idlib" "${CMAKE_SOURCE_DIR}/libs/imgui" )

	if( HARDLINK_GAME )
		set_target_properties( ${DHEWM3BINARY} PROPERTIES COMPILE_DEFINITIONS "${TOOLS_DEFINES}" )
		target_include_directories( ${DHEWM3BINARY} PRIVATE ${game_includes} )
	
		# Hack to get .natvis added
		if ( MSVC )
			target_sources( ${DHEWM3BINARY} PRIVATE "sys/win32/natvis/game.natvis" )
		endif()
	else()
		set_target_properties( ${DHEWM3BINARY} PROPERTIES COMPILE_DEFINITIONS "__DOOM_DLL__;${TOOLS_DEFINES}" )
	endif()

	if ( MSVC )
		target_sources( ${DHEWM3BINARY} PRIVATE "sys/win32/natvis/engine.natvis" )
	endif()

	set_target_properties( ${DHEWM3BINARY} PROPERTIES LINK_FLAGS "${ldflags}" )
	set_target_properties( ${DHEWM3BINARY} PROPERTIES MACOSX_BUNDLE_INFO_PLIST ${PROJECT_SOURCE_DIR}/sys/osx/Info.plist )
	set_target_properties( ${DHEWM3BINARY} PROPERTIES RUNTIME_OUTPUT_DIRECTORY $<1:>${OUTPUT_FOLDER} )
	set_target_properties( ${DHEWM3BINARY} PROPERTIES FOLDER "exes" )

	target_link_libraries( ${DHEWM3BINARY}
		idlib
		${sys_thirdparty_libs}
		${sys_libs}
	)

	if( NOT APPLE AND NOT WIN32 )
		install(TARGETS ${DHEWM3BINARY}
				RUNTIME DESTINATION "${bindir}"
				LIBRARY DESTINATION "${libdir}"
				ARCHIVE DESTINATION "${libdir}"
		)
	endif()
endif()

# ============================== Dedicate Server ================================

if( DEDICATED )
	add_executable( ${DHEWM3BINARY}ded WIN32 MACOSX_BUNDLE
		${src_core}
		${src_stub_openal}
		${src_stub_gl}
		${src_sys_base}
		${src_debuggerServer}
	)
	
	if( MSVC )
		source_group( TREE ${CMAKE_CURRENT_SOURCE_DIR} PREFIX neo FILES ${src_core} ${src_sys_base} ${src_stub_openal} ${src_stub_gl} ${src_debuggerServer} )
	endif()
	
	target_precompile_headers( ${DHEWM3BINARY}ded PRIVATE "${CMAKE_SOURCE_DIR}/idlib/precompiled.h" )
	
	target_include_directories( ${DHEWM3BINARY}ded PRIVATE "${CMAKE_SOURCE_DIR}/idlib" )

	if( HARDLINK_GAME )
		set_target_properties( ${DHEWM3BINARY}ded PROPERTIES COMPILE_DEFINITIONS "IMGUI_DISABLE;ID_DEDICATED" )
		target_include_directories( ${DHEWM3BINARY}ded PRIVATE ${game_includes} )
	
		# Hack to get .natvis added
		if ( MSVC )
			target_sources( ${DHEWM3BINARY} PRIVATE "sys/win32/natvis/game.natvis" )
		endif()
	else()
		set_target_properties( ${DHEWM3BINARY}ded PROPERTIES COMPILE_DEFINITIONS "IMGUI_DISABLE;ID_DEDICATED;__DOOM_DLL__" )
	endif()

	if ( MSVC )
		target_sources( ${DHEWM3BINARY} PRIVATE "sys/win32/natvis/engine.natvis" )
	endif()

	set_target_properties( ${DHEWM3BINARY}ded PROPERTIES LINK_FLAGS "${ldflags}" )
	set_target_properties( ${DHEWM3BINARY}ded PROPERTIES RUNTIME_OUTPUT_DIRECTORY $<1:>${OUTPUT_FOLDER} )
	set_target_properties( ${DHEWM3BINARY}ded PROPERTIES FOLDER "exes" )
	
	list( REMOVE_ITEM sys_thirdparty_libs ${OPENAL_LIBRARY} )

	target_link_libraries( ${DHEWM3BINARY}ded
		idlib
		${sys_thirdparty_libs}
		${sys_libs}
	)

	if( NOT APPLE AND NOT WIN32 )
		install( TARGETS ${DHEWM3BINARY}ded
				RUNTIME DESTINATION "${bindir}"
				LIBRARY DESTINATION "${libdir}"
				ARCHIVE DESTINATION "${libdir}"
		)
	endif()
endif()

# ============================== Maya Import ================================

if( TOOLS AND MAYA_IMPORT )
	set( src_mayaimport 
		MayaImport/exporter.h
		MayaImport/maya_main.cpp
		MayaImport/maya_main.h
	)

	set ( src_toolsupportsrc 
		framework/CmdSystem.cpp
		framework/CVarSystem.cpp
		framework/File.cpp
		framework/FileSystem.cpp
		framework/miniz/miniz.c
		framework/minizip/ioapi.c
		framework/minizip/ioapi.h
		framework/minizip/MiniZip64_info.txt
		framework/minizip/unzip.cpp
		framework/minizip/unzip.h
	)

	set ( src_enginestub 
		tools/tool_idcommon_stub.cpp
	)

	add_library( MayaImport${MAYA_VERSION} SHARED
		${src_mayaimport}
		${src_toolsupportsrc}
		${src_enginestub}
	)

	add_dependencies( MayaImport${MAYA_VERSION} MayaDevKit )
	
	if( MSVC )
		source_group( TREE ${CMAKE_CURRENT_SOURCE_DIR} PREFIX neo FILES ${src_mayaimport} ${src_toolsupportsrc} ${src_enginestub} )
	endif()
	
	target_precompile_headers( MayaImport${MAYA_VERSION} PRIVATE "${CMAKE_SOURCE_DIR}/idlib/precompiled.h" )
	
	target_include_directories( MayaImport${MAYA_VERSION} PRIVATE "${CMAKE_SOURCE_DIR}/idlib" )

	if( HARDLINK_GAME )
		if ( MSVC )
			set_target_properties( MayaImport${MAYA_VERSION} PROPERTIES COMPILE_DEFINITIONS "ID_MAYA_IMPORT;ID_MAYA_IMPORT_API=__declspec(dllexport)" )
		else()
			set_target_properties( MayaImport${MAYA_VERSION} PROPERTIES COMPILE_DEFINITIONS "ID_MAYA_IMPORT;ID_MAYA_IMPORT_API=__attribute__ ((visibility (\"default\")))" )
		endif()
		target_include_directories( MayaImport${MAYA_VERSION} PRIVATE ${game_includes} )
	else()
		if ( MSVC )
			set_target_properties( MayaImport${MAYA_VERSION} PROPERTIES COMPILE_DEFINITIONS "ID_MAYA_IMPORT;__DOOM_DLL__;ID_MAYA_IMPORT_API=__declspec(dllexport)" )
		else()
			set_target_properties( MayaImport${MAYA_VERSION} PROPERTIES COMPILE_DEFINITIONS "ID_MAYA_IMPORT;__DOOM_DLL__;ID_MAYA_IMPORT_API=__attribute__ ((visibility (\"default\")))" )
		endif()
	endif()
	set_target_properties( MayaImport${MAYA_VERSION} PROPERTIES LINK_FLAGS "${ldflags}" )
	target_include_directories( MayaImport${MAYA_VERSION} PRIVATE "${CMAKE_CURRENT_SOURCE_DIR}/libs/MayaSDKDevKit/include" )
	target_link_directories( MayaImport${MAYA_VERSION} PRIVATE "${CMAKE_CURRENT_SOURCE_DIR}/libs/MayaSDKDevKit/lib" )
	target_link_libraries( MayaImport${MAYA_VERSION} idlib Foundation OpenMaya OpenMayaAnim )
	set_target_properties( MayaImport${MAYA_VERSION} PROPERTIES FOLDER "dlls" )
	set_target_properties( MayaImport${MAYA_VERSION} PROPERTIES RUNTIME_OUTPUT_DIRECTORY $<1:>${OUTPUT_FOLDER} LIBRARY_OUTPUT_DIRECTORY $<1:>${OUTPUT_FOLDER} )

	if( NOT APPLE AND NOT WIN32 )
		install( TARGETS MayaImport${MAYA_VERSION}
				RUNTIME DESTINATION "${bindir}"
				LIBRARY DESTINATION "${libdir}"
				ARCHIVE DESTINATION "${libdir}"
		)
	endif()
endif()

# ============================== Doom 3 ================================

if( BASE AND NOT HARDLINK_GAME )
	add_library( base SHARED ${src_game} )
	
	if ( MSVC )
		target_sources( base PRIVATE "sys/win32/natvis/game.natvis" )
		source_group( TREE ${CMAKE_CURRENT_SOURCE_DIR} PREFIX neo FILES ${src_game} )
	endif()
	
	target_precompile_headers( base PRIVATE "${CMAKE_SOURCE_DIR}/idlib/precompiled.h" )
	
	target_include_directories( base PRIVATE "${CMAKE_SOURCE_DIR}/idlib" )
	
	set_target_properties( base PROPERTIES PREFIX "" )
	set_target_properties( base PROPERTIES COMPILE_DEFINITIONS "GAME_DLL" )
	target_include_directories( base PRIVATE "${CMAKE_SOURCE_DIR}/game" )
	set_target_properties( base PROPERTIES LINK_FLAGS "${ldflags}" )
	set_target_properties( base PROPERTIES INSTALL_NAME_DIR "@executable_path" )
	target_link_libraries( base idlib )
	set_target_properties( base PROPERTIES RUNTIME_OUTPUT_DIRECTORY $<1:>${OUTPUT_FOLDER} LIBRARY_OUTPUT_DIRECTORY $<1:>${OUTPUT_FOLDER} )
	set_target_properties( base PROPERTIES FOLDER "dlls" )

	if( NOT APPLE AND NOT WIN32 )
		install(TARGETS base
				RUNTIME DESTINATION "${bindir}"
				LIBRARY DESTINATION "${libdir}"
				ARCHIVE DESTINATION "${libdir}"
		)
	endif()
endif()

# ============================== Doom 3: RoE ================================

if( D3XP AND NOT HARDLINK_GAME )
	add_library( d3xp SHARED ${src_d3xp} )

	if ( MSVC )
		target_sources( d3xp PRIVATE "sys/win32/natvis/game.natvis" )
		source_group( TREE ${CMAKE_CURRENT_SOURCE_DIR} PREFIX neo FILES ${src_d3xp} )
	endif()

	target_precompile_headers( d3xp PRIVATE "${CMAKE_SOURCE_DIR}/idlib/precompiled.h" )
	
	target_include_directories( d3xp PRIVATE "${CMAKE_SOURCE_DIR}/idlib" )

	set_target_properties( d3xp PROPERTIES PREFIX "")
	set_target_properties( d3xp PROPERTIES COMPILE_DEFINITIONS "GAME_DLL;_D3XP;CTF" )
	target_include_directories( d3xp PRIVATE "${CMAKE_SOURCE_DIR}/d3xp" )
	set_target_properties( d3xp PROPERTIES LINK_FLAGS "${ldflags}" )
	set_target_properties( d3xp PROPERTIES INSTALL_NAME_DIR "@executable_path" )
	target_link_libraries( d3xp idlib )
	set_target_properties( d3xp PROPERTIES RUNTIME_OUTPUT_DIRECTORY $<1:>${OUTPUT_FOLDER} LIBRARY_OUTPUT_DIRECTORY $<1:>${OUTPUT_FOLDER} )
	set_target_properties( d3xp PROPERTIES FOLDER "dlls" )

	if( NOT APPLE AND NOT WIN32 )
		install( TARGETS d3xp
				RUNTIME DESTINATION "${bindir}"
				LIBRARY DESTINATION "${libdir}"
				ARCHIVE DESTINATION "${libdir}"
		)
	endif()
endif()

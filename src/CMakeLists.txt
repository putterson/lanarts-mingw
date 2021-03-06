cmake_minimum_required( VERSION 2.6 )
PROJECT(project_lanarts)

FIND_LIBRARY(OpenGL REQUIRED)
include(FindFreetype)

set( LIBS ../libs)
set( DEPS ../dependencies)
set( BUILTDEPS ${CMAKE_BINARY_DIR}/dependencies )

add_definitions( -std=c++14 -Wno-deprecated-declarations)

if (${USE_BACKTRACE})
    set( BACKTRACE_LIBRARIES bfd )
else()
    set( BACKTRACE_LIBRARIES )
endif()

if (${USE_LUAJIT})
    set( lua_includes ${DEPS}/luajit/src )
else()
    set( lua_includes ${DEPS}/lua  )
endif()

set( lua_libraries lua )

include_directories(
    "."
    "${DEPS}/sdl2/include"
    "${DEPS}/sdl2-image"
    "${DEPS}/backward-cpp"
    ${OpenGL_INCLUDE_DIRS} 		
    ${FREETYPE_INCLUDE_DIRS}
    "${LIBS}/common-lib/include"
    "${LIBS}/draw-lib/include"
    "${LIBS}/dungeon-generation-lib/include"
    ${LIBS} # for net
    "${LIBS}/luawrap-lib/include"
    "${LIBS}/sound-lib/include"
    ${DEPS}/UnitTest++ 
    "${DEPS}/yaml-cpp/include"
    "${DEPS}/physfs/src"
    "${DEPS}/physfs/extras"
    ${lua_includes} 
    ${DEPS} # for rvo2 & luasocket
)

file(GLOB lanarts_src RELATIVE ${CMAKE_CURRENT_SOURCE_DIR} 
    include/* 
 
    collision_avoidance/*
 
    data/*
    data/loaddata/*
 
    display/*
    display/loaddata/*
 
    draw/*
    draw/loaddata/*
 
    fov/*
    fov/impl/*
 
    gamestate/*
    gamestate/loaddata/*
 
    interface/*
    interface/sidebar/*
 
    dungeon_generation/*
    dungeon_generation/loaddata/*
 
    lua/*
    lua_api/*
 
    menu/*
    net/*

    objects/*
    objects/enemy/*
    objects/loaddata/*
    objects/player/*
    objects/store/*

    pathfind/*

    stats/*
    stats/items/*
    stats/loaddata/*

    util/*

    lanarts_defines.cpp

    main.cpp
)

file(GLOB lanarts_tests RELATIVE ${CMAKE_CURRENT_SOURCE_DIR} 
       tests/*
)

add_executable( lanarts ${lanarts_src} ${lanarts_tests})


set (libraries 
        luayaml
   mingw32
	ldraw
	lcommon
        lua_box2d
	lanarts_dungeon_gen
	lsound
	${lua_libraries}
	luasocket
	yaml-cpp
        luayaml
	luawrap
	lanarts_net
        lpeg
        enet
        ws2_32
        mingw32
        winmm
	RVO
        #physfs-static
        libxmi
	${OPENGL_gl_LIBRARY}
	${FREETYPE_LIBRARIES}
        UnitTest++
        SDL2-static
        SDL2-image
        #SDL2-mixer
                /usr/i686-w64-mingw32/sys-root/mingw/lib/libSDL2_mixer.dll.a
        #        /usr/i686-w64-mingw32/sys-root/mingw/lib/libSDL2_image.dll.a
        #        /usr/i686-w64-mingw32/sys-root/mingw/lib/libSDL2.dll.a
        backward_object
        SDL2-static
        ${BACKTRADE_DEP}
	yaml-cpp
        luayaml
   mingw32
	ldraw
	lcommon
        lua_box2d
	lanarts_dungeon_gen
	lsound
	${lua_libraries}
	luasocket
	yaml-cpp
        lua_enet
        luayaml
	luawrap
	lanarts_net
        lpeg
        enet
        ws2_32
        mingw32
        winmm
	RVO
        #physfs-static
        libxmi
	${OPENGL_gl_LIBRARY}
	${FREETYPE_LIBRARIES}
        UnitTest++
        SDL2-static
        SDL2-image
        SDL2-static
        #SDL2-mixer
                /usr/i686-w64-mingw32/sys-root/mingw/lib/libSDL2_mixer.dll.a
        #        /usr/i686-w64-mingw32/sys-root/mingw/lib/libSDL2_image.dll.a
        #        /usr/i686-w64-mingw32/sys-root/mingw/lib/libSDL2.dll.a
        backward_object
        SDL2-static
        ${BACKTRADE_DEP}
	yaml-cpp
        luayaml
)
target_link_libraries( lanarts 
    ${libraries}
)

enable_testing()
add_test(NAME lanarts_tests COMMAND 
    cd ${CMAKE_SOURCE_DIR} ;
    ${EXECUTABLE_OUTPUT_PATH}/lanarts --tests )

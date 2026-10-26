cmake_minimum_required(VERSION 3.10)
project(Anubis-RPG-Studio VERSION 1.0)

add_executable(${PROJECT_NAME} 
    Anubis-RPG-Studio/src/main.cpp
    Anubis-RPG-Studio/ico/app.rc
)

# SOURCES
file(GLOB_RECURSE SOURCES
    Anubis-RPG-Studio/src/*.cpp
)

# HEADERS
file(GLOB_RECURSE HEADERS
    Anubis-RPG-Studio/include/*.hpp
)

target_sources(${PROJECT_NAME} PRIVATE ${SOURCES})

source_group(TREE ${CMAKE_SOURCE_DIR}/Anubis-RPG-Studio/src
             PREFIX "Source Files"
             FILES ${SOURCES})

target_sources(${PROJECT_NAME} PRIVATE ${HEADERS})

source_group(TREE ${CMAKE_SOURCE_DIR}/Anubis-RPG-Studio/include
             PREFIX "Header Files"
             FILES ${HEADERS})

target_include_directories(${PROJECT_NAME} PRIVATE
    ${CMAKE_SOURCE_DIR}/Anubis-RPG-Studio/include
)

set_target_properties(${PROJECT_NAME} PROPERTIES
  CXX_STANDARD 23
  CXX_STANDARD_REQUIRED ON
)

target_compile_features(${PROJECT_NAME} PRIVATE cxx_std_23)

# set the SFML lib
set(SFML_DIR "C:/SFML-3.1.0/lib/cmake/SFML")

# set the packages OpenGL
find_package(OpenGL REQUIRED)

# set the packages SFML
find_package(SFML 3.1.0 COMPONENTS Graphics Window System Audio REQUIRED)
target_link_libraries(${PROJECT_NAME} 
	SFML::Graphics
        SFML::Window
        SFML::System
	SFML::Audio
	OpenGL::GL
	glu32
)

# set the working directory (assets)
set_target_properties(${PROJECT_NAME} PROPERTIES
    VS_DEBUGGER_WORKING_DIRECTORY_DEBUG "${CMAKE_SOURCE_DIR}/Anubis-RPG-Studio"
    VS_DEBUGGER_WORKING_DIRECTORY_RELEASE "${CMAKE_SOURCE_DIR}/Anubis-RPG-Studio"
)

# set the path for SFML bin
set_target_properties(${PROJECT_NAME} PROPERTIES
    VS_DEBUGGER_ENVIRONMENT "PATH=%PATH%;C:\\SFML-3.1.0\\bin"
)
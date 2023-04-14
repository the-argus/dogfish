cmake_minimum_required(VERSION 3.11) # FetchContent is available in 3.11+
project(dogfish)

# Generate compile_commands.json
set(CMAKE_EXPORT_COMPILE_COMMANDS ON)

# Dependencies
set(RAYLIB_VERSION 4.2.0)
find_package(raylib ${RAYLIB_VERSION} QUIET) # QUIET or REQUIRED
if (NOT raylib_FOUND) # If there's none, fetch and build raylib
  include(FetchContent)
  FetchContent_Declare(
    raylib
    URL https://github.com/raysan5/raylib/archive/refs/tags/${RAYLIB_VERSION}.tar.gz
  )
  FetchContent_GetProperties(raylib)
  if (NOT raylib_POPULATED) # Have we downloaded raylib yet?
    set(FETCHCONTENT_QUIET NO)
    FetchContent_Populate(raylib)
    set(BUILD_EXAMPLES OFF CACHE BOOL "" FORCE) # don't build the supplied examples
    add_subdirectory(${raylib_SOURCE_DIR} ${raylib_BINARY_DIR})
  endif()
endif()

set(ODE_VERSION 0.16.2)
find_package(ode ${ODE_VERSION} QUIET)
if (NOT ode_FOUND)
  include(FetchContent)
  FetchContent_Declare(ode URL https://bitbucket.org/odedevs/ode/downloads/ode-${ODE_VERSION}.tar.gz)
  FetchContent_GetProperties(ode)
  if (NOT ode_POPULATED) # Have we downloaded raylib yet?
    set(FETCHCONTENT_QUIET NO)
    FetchContent_Populate(ode)
    set(ODE_WITH_DEMOS OFF ODE_WITH_TESTS OFF BUILD_SHARED_LIBS OFF)
    add_subdirectory(${ode_SOURCE_DIR} ${ode_BINARY_DIR})
  endif()
endif()

# Our Project
FILE(GLOB DogfishSources src/*.c)
FILE(GLOB DogfishHeaders src/*.h)
FILE(GLOB DogfishHeaders2 src/include/*.h)
set(SOURCES ${DogfishHeaders} ${DogfishHeaders2} ${DogfishSources} )
add_executable(${PROJECT_NAME} ${SOURCES})
#set(raylib_VERBOSE 1)
target_link_libraries(${PROJECT_NAME} raylib ODE)
target_include_directories(${PROJECT_NAME} PRIVATE src src/include)

# Checks if OSX and links appropriate frameworks (Only required on MacOS)
if (APPLE)
    target_link_libraries(${PROJECT_NAME} "-framework IOKit")
    target_link_libraries(${PROJECT_NAME} "-framework Cocoa")
    target_link_libraries(${PROJECT_NAME} "-framework OpenGL")
endif()

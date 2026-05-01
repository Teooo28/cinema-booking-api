cmake_minimum_required(VERSION 3.15)

project(oop)

cmake_policy(SET CMP0135 NEW)

set(CMAKE_CXX_STANDARD 20)
set(CMAKE_CXX_STANDARD_REQUIRED ON)
set(CMAKE_CXX_EXTENSIONS OFF)

include(FetchContent)

# 1. Librăria JSON
FetchContent_Declare(json
    GIT_REPOSITORY https://github.com/nlohmann/json.git
    GIT_TAG v3.11.3
)

# 2. Descărcăm manual ASIO
FetchContent_Declare(asio
    GIT_REPOSITORY https://github.com/chriskohlhoff/asio.git
    GIT_TAG asio-1-28-0
)

# 3. Librăria CROW
FetchContent_Declare(Crow
    GIT_REPOSITORY https://github.com/CrowCpp/Crow.git
    GIT_TAG v1.2.0
)

set(CROW_BUILD_EXAMPLES OFF CACHE BOOL "Skip Crow Examples" FORCE)
set(CROW_BUILD_TESTS OFF CACHE BOOL "Skip Crow Tests" FORCE)

FetchContent_MakeAvailable(json asio)

set(ASIO_INCLUDE_DIR ${asio_SOURCE_DIR}/asio/include CACHE PATH "ASIO include dir" FORCE)

FetchContent_MakeAvailable(Crow)

if(DEFINED WARNINGS_AS_ERRORS)
elseif(DEFINED ENV{WARNINGS_AS_ERRORS})
    set(WARNINGS_AS_ERRORS $ENV{WARNINGS_AS_ERRORS})
else()
    set(WARNINGS_AS_ERRORS ON)
endif()

set(SOURCES
    src/main.cpp
    src/Event.cpp
    src/Movie2D.cpp
    src/Movie3D.cpp
    src/EventFactory.cpp
    src/DiscountStrategy.cpp
    src/CinemaRepository.cpp
)
add_executable(${PROJECT_NAME} ${SOURCES}
    include/CinemaRepository.h)

target_include_directories(${PROJECT_NAME} PUBLIC include)
target_link_libraries(${PROJECT_NAME} PRIVATE nlohmann_json::nlohmann_json Crow ws2_32 mswsock)

if(GITHUB_ACTIONS)
    message("NOTE: GITHUB_ACTIONS defined")
    target_compile_definitions(${PROJECT_NAME} PRIVATE GITHUB_ACTIONS)
endif()

# Compiler flags
message("Compiler: ${CMAKE_CXX_COMPILER_ID} version ${CMAKE_CXX_COMPILER_VERSION}")
if(MSVC)
    target_compile_options(${PROJECT_NAME} PRIVATE /W4 /permissive- /wd4244 /wd4267 /wd4996 /external:anglebrackets /external:W0)
    if(WARNINGS_AS_ERRORS)
        target_compile_options(${PROJECT_NAME} PRIVATE /WX)
    endif()
else()
    target_compile_options(${PROJECT_NAME} PRIVATE -Wall -Wextra -pedantic)
    if(WARNINGS_AS_ERRORS)
        target_compile_options(${PROJECT_NAME} PRIVATE -Werror)
    endif()
endif()

# Sanitizers Configuration
set(is_debug "$<CONFIG:Debug>")
set(is_rel_with_deb "$<CONFIG:RelWithDebInfo>")
set(debug_mode "$<OR:${is_debug},${is_rel_with_deb}>")

if(MSVC)
    target_compile_options(${PROJECT_NAME} PRIVATE "$<${debug_mode}:/fsanitize=address>")
elseif(APPLE)
    target_compile_options(${PROJECT_NAME} PRIVATE "$<${debug_mode}:-fsanitize=address,undefined>")
    target_link_options(${PROJECT_NAME} PRIVATE "$<${debug_mode}:-fsanitize=address,undefined>")
elseif(UNIX)
    if("${CMAKE_CXX_COMPILER_ID}" MATCHES "GNU")
        # We leave GCC "clean" so Valgrind can run without shadow memory conflicts
        message("GCC detected: Sanitizers disabled to allow Valgrind usage.")
    elseif("${CMAKE_CXX_COMPILER_ID}" MATCHES "Clang")
        # Enable ASan for Clang 14 and all newer versions (15, 16, 17, 18, etc.)
        if("${CMAKE_CXX_COMPILER_VERSION}" VERSION_GREATER_EQUAL "14.0")
            target_compile_options(${PROJECT_NAME} PRIVATE "$<${debug_mode}:-fsanitize=address,undefined>")
            target_link_options(${PROJECT_NAME} PRIVATE "$<${debug_mode}:-fsanitize=address,undefined>")
        else()
            message("Legacy Clang detected: Sanitizers skipped.")
        endif()
    endif()
endif()

install(TARGETS ${PROJECT_NAME} DESTINATION bin)
cmake_minimum_required(VERSION 3.20)

set(CMAKE_C_COMPILER "gcc")
set(CMAKE_CXX_COMPILER "g++")

project(ObliqueEngine LANGUAGES CXX C)

set(CMAKE_CXX_STANDARD 23)
set(CMAKE_CXX_STANDARD_REQUIRED ON)
set(CMAKE_EXPORT_COMPILE_COMMANDS ON)

option(OBLIQUE_PROD_BUILD "build for production" OFF)

file(GLOB_RECURSE ENGINE_SOURCES "src/*.cpp" "thirdparty/volk/volk.c")

if(NOT OBLIQUE_PROD_BUILD)
  set(FETCHCONTENT_QUIET OFF)
  include(FetchContent)
  FetchContent_Declare(
        imgui
        GIT_REPOSITORY https://github.com/ocornut/imgui.git
        GIT_TAG        docking
        GIT_SHALLOW    TRUE
        )
  FetchContent_MakeAvailable(imgui)

  FetchContent_Declare(
    spdlog
    GIT_REPOSITORY https://github.com/gabime/spdlog.git
    GIT_TAG        v1.x
    GIT_SHALLOW    TRUE
)
  set(CMAKE_POSITION_INDEPENDENT_CODE ON)

  FetchContent_MakeAvailable(spdlog)
  list(APPEND IMGUI_SOURCES
        ${imgui_SOURCE_DIR}/imgui.cpp
        ${imgui_SOURCE_DIR}/imgui_demo.cpp
        ${imgui_SOURCE_DIR}/imgui_draw.cpp
        ${imgui_SOURCE_DIR}/imgui_tables.cpp
        ${imgui_SOURCE_DIR}/imgui_widgets.cpp
        ${imgui_SOURCE_DIR}/backends/imgui_impl_glfw.cpp
        ${imgui_SOURCE_DIR}/backends/imgui_impl_vulkan.cpp
    )
else()
  add_compile_definitions(OBLIQUE_PROD_BUILD)
endif()

FetchContent_Declare(
    EnTT
    GIT_REPOSITORY https://github.com/skypjack/entt.git
    GIT_TAG        v3.13.0 # Or your preferred stable version
)

FetchContent_MakeAvailable(EnTT)

# Add GLM FetchContent Declaration right under EnTT
FetchContent_Declare(
    glm
    GIT_REPOSITORY https://github.com/g-truc/glm.git
    GIT_TAG        1.0.1
    GIT_SHALLOW    TRUE
)
FetchContent_MakeAvailable(glm)

set(SHADER_SRC_DIR "${CMAKE_CURRENT_SOURCE_DIR}/assets/shaders")
set(VERT_SRC "${SHADER_SRC_DIR}/forward_3d.vert")
set(FRAG_SRC "${SHADER_SRC_DIR}/forward_3d.frag")

set(VERT_SPV "${SHADER_SRC_DIR}/forward_3d.vert.spv")
set(FRAG_SPV "${SHADER_SRC_DIR}/forward_3d.frag.spv")

add_custom_command(
    OUTPUT ${VERT_SPV}
    COMMAND ${CMAKE_COMMAND} -E make_directory ${SHADER_SRC_DIR}
    COMMAND glslc ${VERT_SRC} -o ${VERT_SPV}
    DEPENDS ${VERT_SRC}
    COMMENT "Compiling Vertex Shader: forward_3d.vert -> SPIR-V"
)

add_custom_command(
    OUTPUT ${FRAG_SPV}
    COMMAND ${CMAKE_COMMAND} -E make_directory ${SHADER_SRC_DIR}
    COMMAND glslc ${FRAG_SRC} -o ${FRAG_SPV}
    DEPENDS ${FRAG_SRC}
    COMMENT "Compiling Fragment Shader: forward_3d.frag -> SPIR-V"
)
add_custom_target(CompileShaders DEPENDS ${VERT_SPV} ${FRAG_SPV})

add_library(Oblique SHARED ${ENGINE_SOURCES} ${IMGUI_SOURCES})
add_dependencies(Oblique CompileShaders)

target_compile_definitions(Oblique PRIVATE
    OBLIQUE_PROJECT_ROOT="${CMAKE_CURRENT_SOURCE_DIR}/"
)

target_include_directories(Oblique PUBLIC
    includes
    thirdparty/volk
    thirdparty/vma
)
target_include_directories(Oblique PRIVATE
    includes
    thirdparty/volk
    thirdparty/vma
)

if(NOT OBLIQUE_PROD_BUILD)
  target_include_directories(Oblique PRIVATE
        ${imgui_SOURCE_DIR}
        ${imgui_SOURCE_DIR}/backends
    )
endif()

target_link_libraries(
  Oblique PRIVATE

  glfw ${CMAKE_DL_LIBS}
  spdlog::spdlog
)
target_link_libraries(Oblique PUBLIC EnTT::EnTT glm::glm)

add_executable(ObliqueSandbox sandbox/main.cpp)
target_link_libraries(ObliqueSandbox PRIVATE Oblique)



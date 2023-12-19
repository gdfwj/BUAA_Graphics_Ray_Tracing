cmake_minimum_required(VERSION 3.16)

SET(-DCMAKE_TOOLCHAIN_FILE D:/works/grade3/graphic/vcpkg/scripts/buildsystems/vcpkg.cmake CACHE STRING "")

project(tcode)

set(CMAKE_CXX_STANDARD 17)
set(CMAKE_EXE_LINKER_FLAGS "-static-libgcc -static-libstdc++")
set(CMAKE_CXX_STANDARD_REQUIRED ON)
set(CMAKE_INCLUDE_CURRENT_DIR ON)

#如果find 失败，删除cmake-build-debug，重新reload cmake
find_package(glfw3 REQUIRED)
find_package(GLEW REQUIRED)
find_package(glm REQUIRED)
find_package(GLUT REQUIRED)

add_executable(tcode main.cpp
        ${SHADER_SRCS} my_math.h objects.h)

target_link_libraries(tcode PRIVATE glfw)
target_link_libraries(tcode PRIVATE GLEW::GLEW)
target_link_libraries(tcode PRIVATE glm::glm)
target_link_libraries(tcode PRIVATE GLUT::GLUT)
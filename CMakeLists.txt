cmake_minimum_required(VERSION 3.11)

set(CMAKE_TOOLCHAIN_FILE "D:/vcpkg/scripts/buildsystems/vcpkg.cmake")

project(search_system)
set(CMAKE_CXX_STANDARD 20)
set(CMAKE_CXX_STANDARD_REQUIRED ON)

find_package(libpqxx CONFIG REQUIRED)

add_executable(search_system main.cpp indexer.cpp search_system.cpp search_service.cpp)
target_link_libraries(search_system PRIVATE libpqxx::pqxx)

find_package(unofficial-inih CONFIG REQUIRED)

target_link_libraries(search_system PRIVATE 
    unofficial::inih::inireader
)
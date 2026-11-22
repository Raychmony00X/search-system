cmake_minimum_required(VERSION 3.11)

set(CMAKE_TOOLCHAIN_FILE "D:/vcpkg/scripts/buildsystems/vcpkg.cmake" CACHE STRING "Vcpkg toolchain file")

project(search_system)

set(CMAKE_CXX_STANDARD 20)
set(CMAKE_CXX_STANDARD_REQUIRED ON)

find_package(libpqxx CONFIG REQUIRED)
find_package(unofficial-inih CONFIG REQUIRED)

add_library(core STATIC
    search_system.cpp
    search_system.h
)
target_include_directories(core PUBLIC ${CMAKE_CURRENT_SOURCE_DIR})
target_link_libraries(core PUBLIC 
    libpqxx::pqxx
    unofficial::inih::inireader
)

add_executable(indexer_app 
    main_indexer.cpp 
    indexer.cpp 
    indexer.h
)
target_link_libraries(indexer_app PRIVATE core)

add_custom_command(TARGET indexer_app POST_BUILD
    COMMAND ${CMAKE_COMMAND} -E copy_if_different
        "${CMAKE_CURRENT_SOURCE_DIR}/file.ini"
        $<TARGET_FILE_DIR:indexer_app>
    COMMENT "Copying file.ini for indexer_app"
)

add_executable(searcher_app 
    main_search.cpp 
    search_service.cpp 
    search_service.h
)
target_link_libraries(searcher_app PRIVATE core)

add_custom_command(TARGET searcher_app POST_BUILD
    COMMAND ${CMAKE_COMMAND} -E copy_if_different
        "${CMAKE_CURRENT_SOURCE_DIR}/file.ini"
        $<TARGET_FILE_DIR:searcher_app>
    COMMENT "Copying file.ini for searcher_app"
)
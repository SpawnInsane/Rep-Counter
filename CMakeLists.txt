cmake_minimum_required(VERSION 3.20)

project(rep_counter VERSION 0.4.0 LANGUAGES CXX)

add_executable(rep_counter WIN32 "Rep Counter UI.cpp")
target_compile_features(rep_counter PRIVATE cxx_std_20)
target_compile_definitions(rep_counter PRIVATE REP_COUNTER_VERSION="0.4-alpha")

if(WIN32)
    target_link_libraries(rep_counter PRIVATE winmm)
endif()

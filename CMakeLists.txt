cmake_minimum_required(VERSION 3.10)
project(CacheSimulator)

set(CMAKE_CXX_STANDARD 17)

# Ensure Debug Build Type
if(NOT CMAKE_BUILD_TYPE)
    set(CMAKE_BUILD_TYPE Debug CACHE STRING "Build type" FORCE)
endif()

# Enable Debug Symbols
set(CMAKE_CXX_FLAGS_DEBUG "-g -O0")

add_executable(CacheSimulator 
    main.cpp 
    Replica.cpp 
    LRUCache.cpp 
    ReplicaManager.cpp 
    Metrics.cpp
    CostBenefitAnalyzer.cpp
    ConfigManager.cpp
    RequestProcessor.cpp
)

# Link nlohmann-json if installed via package manager
find_package(nlohmann_json REQUIRED)
target_link_libraries(CacheSimulator PRIVATE nlohmann_json::nlohmann_json)

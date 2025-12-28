cmake_minimum_required(VERSION 3.10)
project(HFT_CPP_32)

set(CMAKE_CXX_STANDARD 17)
set(CMAKE_CXX_STANDARD_REQUIRED ON)

# Add source files
set(SOURCES
    # Add your source files here
    # src/main.cpp
    src\main.cpp
    src\HFT_ITCH.hpp
    src\hash.hpp
    src\hash.cpp
    

)

# Create executable
add_executable(${PROJECT_NAME} ${SOURCES})

# Include directories
target_include_directories(${PROJECT_NAME} PRIVATE
    # ${CMAKE_CURRENT_SOURCE_DIR}/include
)

# Link lwIP library
target_link_libraries(${PROJECT_NAME} PRIVATE lwip)

# Add lwIP include directory (adjust path as needed)
target_include_directories(${PROJECT_NAME} PRIVATE
    ${CMAKE_CURRENT_SOURCE_DIR}/path/to/lwip/include
)
# target_link_libraries(${PROJECT_NAME} PRIVATE somelib)
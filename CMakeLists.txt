cmake_minimum_required(VERSION 3.31)
project(CChat)

set(CMAKE_CXX_STANDARD 20)
set(CMAKE_CXX_STANDARD_REQUIRED ON)

find_package(OpenCV REQUIRED)

# Create executable
add_executable(CChat simple_video_chat.cpp
        simple_video_chat.cpp
)

# Link libraries
target_link_libraries(CChat ${OpenCV_LIBS} pthread)

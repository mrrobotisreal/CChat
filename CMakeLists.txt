cmake_minimum_required(VERSION 3.31)
project(CChat)

set(CMAKE_CXX_STANDARD 20)
set(CMAKE_CXX_STANDARD_REQUIRED ON)

# Add Homebrew include and lib directories (for Apple Silicon)
if(APPLE AND EXISTS /opt/homebrew)
    # Apple Silicon location
    set(OpenCV_DIR "/opt/homebrew/Cellar/opencv/4.11.0_1/lib/cmake/opencv4")
    include_directories("/opt/homebrew/include")
    link_directories("/opt/homebrew/lib")
elseif(APPLE AND EXISTS /usr/local/Cellar)
    # Intel Mac location
    set(OpenCV_DIR "/usr/local/Cellar/opencv/4.11.0_1/lib/cmake/opencv4")
    include_directories("/usr/local/include")
    link_directories("/usr/local/lib")
endif()

find_package(OpenCV REQUIRED)

# Create executable
add_executable(CChat simple_video_chat.cpp
        simple_video_chat.cpp
)

# Link libraries
target_link_libraries(CChat ${OpenCV_LIBS} pthread)

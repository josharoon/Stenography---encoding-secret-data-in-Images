cmake_minimum_required(VERSION 3.21)

project(Stenography)
set(CMAKE_CXX_STANDARD 17)
set(OpenCV_DIR PATH D:/opencv/build/x64/vc15/bin)
include_directories( D:/opencv/build/include  D:/opencv/build/include/opencv2 )
add_executable(Stenography main.cpp)
target_link_libraries( Stenography D:/opencv/build/x64/vc15/lib/opencv_world454d.lib )

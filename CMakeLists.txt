cmake_minimum_required(VERSION 3.21)
project(Stenography)

set(CMAKE_CXX_STANDARD 17)

set(OpenCV_DIR PATH C:/opencv/build/x64/vc15/bin)
include_directories( C:/opencv/build/include  C:/opencv/build/include/opencv2 )
add_executable(PROJECT_NAME main.cpp)
target_link_libraries( PROJECT_NAME C:/opencv/build/x64/vc15/lib/opencv_world454d.lib )

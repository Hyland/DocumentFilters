cmake_minimum_required(VERSION 3.15)

add_executable (WatermarkDocument "WatermarkDocument.cpp")
target_link_libraries (WatermarkDocument PUBLIC DocumentFiltersSamples)
install(TARGETS WatermarkDocument DESTINATION bin)
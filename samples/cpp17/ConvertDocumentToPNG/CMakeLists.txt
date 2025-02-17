cmake_minimum_required(VERSION 3.15)
set (PROJECT_NAME "ConvertDocumentToPNG")

add_executable (${PROJECT_NAME} "ConvertDocumentToPNG.cpp")
target_compile_features(${PROJECT_NAME} PRIVATE cxx_std_17)
target_compile_definitions(${PROJECT_NAME} PRIVATE _SILENCE_ALL_CXX17_DEPRECATION_WARNINGS)
target_link_libraries (${PROJECT_NAME} PRIVATE DocumentFilters DocumentFiltersSamples CLI11::CLI11)
set_target_properties(${PROJECT_NAME} PROPERTIES FOLDER "Samples")

cmake_minimum_required(VERSION 3.16)

project(DocumentFilters
    VERSION 25.1.0
    DESCRIPTION "Document Filters - c++11 bindings"
    LANGUAGES CXX
)

set(LIBRARY_NAME "DocumentFiltersObjects")

set(SOURCES "src/DocumentFiltersObjects.cpp")

set(HEADERS include/DocumentFiltersObjects.h)

set(DEPENDENCIES DocumentFilters)

add_subdirectory(${CMAKE_CURRENT_LIST_DIR}/../c bindings)

# Create the libary for the C++11 bindings
add_library(${LIBRARY_NAME} STATIC ${SOURCES} ${HEADERS})
target_compile_features(${LIBRARY_NAME} PRIVATE cxx_std_11)
target_include_directories(${LIBRARY_NAME} PUBLIC include)
target_compile_definitions(${LIBRARY_NAME} PRIVATE _LARGEFILE64_SOURCE)
target_link_libraries(${LIBRARY_NAME} ${DEPENDENCIES})

# Create an alias for the library
add_library(DocumentFilters::Cpp11 ALIAS ${LIBRARY_NAME})

export(TARGETS ${LIBRARY_NAME} FILE ${LIBRARY_NAME}Config.cmake)
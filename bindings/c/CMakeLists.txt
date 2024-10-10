cmake_minimum_required(VERSION 3.16)

project(DocumentFilters
    VERSION 24.4.0
    DESCRIPTION "Document Filters Library - C"
    LANGUAGES CXX
)

set(DOCFILTERS_RELEASES_URL "" CACHE STRING "URL to download the Document Filters binaries from")
set(DOCFILTERS_VERSION "" CACHE STRING "Document Filters version")
set(DOCFILTERS_INSTALL_BINARIES OFF CACHE BOOL "Install the Document Filters binaries")
set(DOCFILTERS_INSTALL_ASSETS OFF CACHE BOOL "Install the Document Filters binaries")

set(LIBRARY_NAME "DocumentFilters")

set(HEADERS include/DocumentFilters.h)

set(DEPENDENCIES DocumentFilters)

# Create the libary
add_library(${LIBRARY_NAME} INTERFACE  ${HEADERS})
target_include_directories(${LIBRARY_NAME} INTERFACE include)
target_link_libraries(${LIBRARY_NAME} INTERFACE ${DEPENDENCIES})

if (WIN32)
	target_link_directories(${LIBRARY_NAME} INTERFACE ${CMAKE_CURRENT_LIST_DIR}/lib)
	if(${CMAKE_CXX_COMPILER_ARCHITECTURE_ID} MATCHES "X86")		
		target_link_libraries(${LIBRARY_NAME} INTERFACE "ISYS11df_x86")
	else()
		target_link_libraries(${LIBRARY_NAME} INTERFACE "ISYS11df_x64")
	endif()
# else()
# 	target_link_libraries(DocumentFiltersSamples PUBLIC "ISYS11df")
endif()

export(TARGETS DocumentFilters FILE DocumentFiltersConfig.cmake)

################################################################################

include(ExternalProject)

if (CMAKE_VERSION VERSION_GREATER_EQUAL "3.24.0")
    cmake_policy(SET CMP0135 NEW)
endif()

# Function to get the latest version from GitHub using its API
function(fetch_latest_version LATEST_VERSION_VAR)
    message(STATUS "Fetching the latest version from GitHub...")
    
    # GitHub API URL to get the latest release info (replace with your repo's details)
    set(LATEST_URL "https://api.github.com/repos/Hyland/DocumentFilters/releases/latest")
    
    file(DOWNLOAD ${LATEST_URL} ${CMAKE_BINARY_DIR}/latest_release.json
         TIMEOUT 60
         STATUS DOWNLOAD_STATUS
         LOG logFile)
    
    if(NOT DOWNLOAD_STATUS EQUAL 0)
        message(FATAL_ERROR "Failed to download latest release information from GitHub.")
    endif()

    # Parse the JSON to get the tag_name (which is the version)
    file(READ ${CMAKE_BINARY_DIR}/latest_release.json RELEASE_CONTENT)
    string(REGEX MATCH "\"tag_name\": \"([^\"]+)\"" _ ${RELEASE_CONTENT})
    set(${LATEST_VERSION_VAR} "${CMAKE_MATCH_1}" PARENT_SCOPE)
endfunction()

if("${DOCFILTERS_RELEASES_URL}" STREQUAL "")
    # If DOCFILTERS_VERSION is empty, get the latest version
    if(NOT DOCFILTERS_VERSION)
        fetch_latest_version(LATEST_VERSION)
        set(DOCFILTERS_VERSION ${LATEST_VERSION})
    endif()

    message(STATUS "Using version: ${DOCFILTERS_VERSION}")
    set(DOCFILTERS_RELEASES_URL "https://github.com/Hyland/DocumentFilters/releases/download/${DOCFILTERS_VERSION}")
endif()

message(STATUS "Downloading release binaries from: ${DOCFILTERS_RELEASES_URL}")

# Set the URL of the ZIP file you want to download
set(EXTERNAL_PROJECT_NAME DocumentFiltersBinaries)
set(EXTERNAL_PROJECT_PREFIX ${CMAKE_BINARY_DIR}/${EXTERNAL_PROJECT_NAME})

if("${DF_BINARIES}" STREQUAL "")
    if (CMAKE_SYSTEM_NAME MATCHES "Windows")
        set(EXTERNAL_PROJECT_URL ${DOCFILTERS_RELEASES_URL}/windows-intel-msvc-64.zip)
    elseif (CMAKE_SYSTEM_NAME MATCHES "Linux")
        if (CMAKE_SYSTEM_PROCESSOR MATCHES "aarch64")
            if (EXISTS "/etc/alpine-release")
                set(EXTERNAL_PROJECT_URL ${DOCFILTERS_RELEASES_URL}/linux-aarch64-clang-musl-64.zip)
            else()
                set(EXTERNAL_PROJECT_URL ${DOCFILTERS_RELEASES_URL}/linux-aarch64-gcc-64.zip)
            endif()
        elseif (CMAKE_SYSTEM_PROCESSOR MATCHES "ppc64le")
            set(EXTERNAL_PROJECT_URL ${DOCFILTERS_RELEASES_URL}/linux-ppc64le-gcc-64.zip)
        elseif (CMAKE_SYSTEM_PROCESSOR MATCHES "ppc64")
            set(EXTERNAL_PROJECT_URL ${DOCFILTERS_RELEASES_URL}/linux-ppc-gcc-64.zip)
        else()
            if (EXISTS "/etc/alpine-release")
                set(EXTERNAL_PROJECT_URL ${DOCFILTERS_RELEASES_URL}/linux-intel-clang-musl-64.zip)
            else()
                set(EXTERNAL_PROJECT_URL ${DOCFILTERS_RELEASES_URL}/linux-intel-gcc-64.zip)
            endif()
        endif()
    elseif (CMAKE_SYSTEM_NAME MATCHES "Darwin")
        if (CMAKE_SYSTEM_PROCESSOR MATCHES "arm64")
            set(EXTERNAL_PROJECT_URL ${DOCFILTERS_RELEASES_URL}/macos-arm64-clang-64.zip)
        else()
            set(EXTERNAL_PROJECT_URL ${DOCFILTERS_RELEASES_URL}/macos-intel-clang-64.zip)
        endif()
    else()
        message(FATAL_ERROR "Unsupported operating system: ${CMAKE_SYSTEM_NAME}")
    endif()

    # Configure and build the external project
    ExternalProject_Add(${EXTERNAL_PROJECT_NAME}_download
        PREFIX ${EXTERNAL_PROJECT_PREFIX}
        URL ${EXTERNAL_PROJECT_URL}
        DOWNLOAD_NAME "${EXTERNAL_PROJECT_NAME}.zip"
        UPDATE_COMMAND ""
        CONFIGURE_COMMAND ""
        BUILD_COMMAND ""
        INSTALL_COMMAND ""
    )

    # Define a custom target to execute after building the external project
    add_custom_target(${EXTERNAL_PROJECT_NAME}_unzip
        COMMAND ${CMAKE_COMMAND} -E tar xf ${EXTERNAL_PROJECT_PREFIX}/src/${EXTERNAL_PROJECT_NAME}.zip
        WORKING_DIRECTORY ${CMAKE_BINARY_DIR}
    )
    add_dependencies(${EXTERNAL_PROJECT_NAME}_unzip ${EXTERNAL_PROJECT_NAME}_download)

    # Define a custom target to copy the contents to the bin directory
    add_custom_target(${EXTERNAL_PROJECT_NAME}
        COMMAND ${CMAKE_COMMAND} -E copy_directory ${EXTERNAL_PROJECT_PREFIX}/src/${EXTERNAL_PROJECT_NAME}_download ${CMAKE_BINARY_DIR}/bin
        WORKING_DIRECTORY ${CMAKE_BINARY_DIR}
    )
    add_dependencies(${EXTERNAL_PROJECT_NAME} ${EXTERNAL_PROJECT_NAME}_unzip)

    # Install the extracted files (assumes the contents need to be installed to a specific location)
    if (DOCFILTERS_INSTALL_BINARIES)
        install(DIRECTORY ${EXTERNAL_PROJECT_PREFIX}/src/${EXTERNAL_PROJECT_NAME}_download/ DESTINATION ${CMAKE_INSTALL_PREFIX}/bin FILES_MATCHING PATTERN "*")  
    endif()

    # Configure and build the assets project (fonts, ocr data, etc.)
    set(EXTERNAL_PROJECT_ASSETS_NAME DocumentFiltersAssets)
    set(EXTERNAL_PROJECT_ASSETS_PREFIX ${CMAKE_BINARY_DIR}/${EXTERNAL_PROJECT_NAME})    
    set(EXTERNAL_PROJECT_ASSESTS_URL "${DOCFILTERS_RELEASES_URL}/assets.zip")
    ExternalProject_Add(${EXTERNAL_PROJECT_ASSETS_NAME}_download
        PREFIX ${EXTERNAL_PROJECT_ASSETS_PREFIX}
        URL ${EXTERNAL_PROJECT_ASSESTS_URL}
        UPDATE_COMMAND ""
        CONFIGURE_COMMAND ""
        BUILD_COMMAND ""
        INSTALL_COMMAND ""
    )

    # # Define a custom target to execute after building the external project
    add_custom_target(${EXTERNAL_PROJECT_ASSETS_NAME}_unzip
    set(EXTERNAL_PROJECT_ASSETS_PREFIX ${CMAKE_BINARY_DIR}/${EXTERNAL_PROJECT_NAME})    
        COMMAND ${CMAKE_COMMAND} -E tar xf ${EXTERNAL_PROJECT_ASSETS_PREFIX}/src/${T}.zip
        WORKING_DIRECTORY ${CMAKE_BINARY_DIR}
    )
    add_dependencies(${EXTERNAL_PROJECT_ASSETS_NAME}_unzip ${EXTERNAL_PROJECT_ASSETS_NAME}_download)

    # Define a custom target to copy the contents to the bin directory
    add_custom_target(${EXTERNAL_PROJECT_ASSETS_NAME}
        COMMAND ${CMAKE_COMMAND} -E copy_directory ${EXTERNAL_PROJECT_ASSETS_PREFIX}/src/${EXTERNAL_PROJECT_ASSETS_NAME}_download ${CMAKE_BINARY_DIR}/bin
        WORKING_DIRECTORY ${CMAKE_BINARY_DIR}
    )
    add_dependencies(${EXTERNAL_PROJECT_ASSETS_NAME} ${EXTERNAL_PROJECT_ASSETS_NAME}_unzip)

    if (DOCFILTERS_INSTALL_ASSETS)
        install(DIRECTORY ${EXTERNAL_PROJECT_ASSETS_PREFIX}/src/${EXTERNAL_PROJECT_ASSETS_NAME}_download/ DESTINATION ${CMAKE_INSTALL_PREFIX}/bin FILES_MATCHING PATTERN "*")  
    endif()

endif()

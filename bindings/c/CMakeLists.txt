cmake_minimum_required(VERSION 3.16)

project(DocumentFilters
    VERSION 25.3.0
    DESCRIPTION "Document Filters Library - C"
    LANGUAGES CXX
)

# Indicates if the Document Filters binaries should be downloaded
option(DOCFILTERS_DOWNLOAD_BINARIES "Download Document Filters binaries from GitHub" ON)

# Indicates if the Document Filters binaries should be installed during `cmake --install`
option(DOCFILTERS_INSTALL_BINARIES "Install the Document Filters binaries" ON)

# Indicates if the Document Filters assets should be installed during `cmake --install`
option(DOCFILTERS_INSTALL_ASSETS "Install the Document Filters binaries" ON)

# Location where the Document Filters binaries will be downloaded. If not set, the latest release will be downloaded.
set(DOCFILTERS_RELEASES_URL "" CACHE STRING "URL to download the Document Filters binaries from")

# Version of the Document Filters binaries to download. If not set, the latest release will be downloaded.
set(DOCFILTERS_VERSION "" CACHE STRING "Document Filters version")

# Location where the Document Filters binaries can be found one the system. 
set(DOCFILTERS_RUNTIME_BINARIES_DIRECTORY "" CACHE STRING "Directory where existing DocFilters release binaries are stored")

# Internal variables
set(DOCFILTERS_BINARIES_PROJECT DocumentFiltersBinaries)
set(DOCFILTERS_BINARIES_PROJECT_PREFIX ${CMAKE_BINARY_DIR}/${DOCFILTERS_BINARIES_PROJECT})

# Create the interface library for the C api
set(LIBRARY_NAME "DocumentFilters")
set(HEADERS include/DocumentFilters.h)

add_library(${LIBRARY_NAME} INTERFACE)
target_include_directories(${LIBRARY_NAME} INTERFACE include)
target_link_libraries(${LIBRARY_NAME} INTERFACE ISYS11df)

#  Create an alias for the library
add_library(DocumentFilters::DocumentFilters ALIAS DocumentFilters)
add_library(DocumentFilters::C ALIAS DocumentFilters)

if (WIN32)
	target_link_directories(${LIBRARY_NAME} INTERFACE ${CMAKE_CURRENT_LIST_DIR}/lib)
	if(${CMAKE_CXX_COMPILER_ARCHITECTURE_ID} MATCHES "X86")
        target_link_directories(${LIBRARY_NAME} INTERFACE "${CMAKE_CURRENT_LIST_DIR}/lib/x86")
	else()
        target_link_directories(${LIBRARY_NAME} INTERFACE "${CMAKE_CURRENT_LIST_DIR}/lib/x64")
	endif()
else()
    if (DOCFILTERS_DOWNLOAD_BINARIES)
	    target_link_directories(${LIBRARY_NAME} INTERFACE "${DOCFILTERS_BINARIES_PROJECT_PREFIX}/src/${DOCFILTERS_BINARIES_PROJECT}_download/")
    elseif ("${DOCFILTERS_RUNTIME_BINARIES_DIRECTORY}" STREQUAL "")
        target_link_directories(${LIBRARY_NAME} INTERFACE "${CMAKE_BINARY_DIR}")
    else()
        target_link_directories(${LIBRARY_NAME} INTERFACE "${DOCFILTERS_RUNTIME_BINARIES_DIRECTORY}")
    endif()
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
        message(FATAL_ERROR "Failed to download latest release information from GitHub. Status=${DOWNLOAD_STATUS}.")
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
    elseif (NOT DOCFILTERS_VERSION MATCHES "^v")
        # If the value in DOCFILTERS_VERSION does not start with v, add it
        set(DOCFILTERS_VERSION "v${DOCFILTERS_VERSION}")
    endif()

    message(STATUS "Using version: ${DOCFILTERS_VERSION}")
    set(DOCFILTERS_RELEASES_URL "https://github.com/Hyland/DocumentFilters/releases/download/${DOCFILTERS_VERSION}")
endif()

message(STATUS "Downloading release binaries from: ${DOCFILTERS_RELEASES_URL}")

# Set the URL of the ZIP file you want to download

if(DOCFILTERS_DOWNLOAD_BINARIES)
    if (CMAKE_SYSTEM_NAME MATCHES "Windows")
        set(DOCFILTERS_BINARIES_URL ${DOCFILTERS_RELEASES_URL}/windows-intel-msvc-64.zip)
    elseif (CMAKE_SYSTEM_NAME MATCHES "Linux")
        if (CMAKE_SYSTEM_PROCESSOR MATCHES "aarch64")
            if (EXISTS "/etc/alpine-release")
                set(DOCFILTERS_BINARIES_URL ${DOCFILTERS_RELEASES_URL}/linux-aarch64-clang-musl-64.zip)
            else()
                set(DOCFILTERS_BINARIES_URL ${DOCFILTERS_RELEASES_URL}/linux-aarch64-gcc-64.zip)
            endif()
        elseif (CMAKE_SYSTEM_PROCESSOR MATCHES "ppc64le")
            set(DOCFILTERS_BINARIES_URL ${DOCFILTERS_RELEASES_URL}/linux-ppc64le-gcc-64.zip)
        elseif (CMAKE_SYSTEM_PROCESSOR MATCHES "ppc64")
            set(DOCFILTERS_BINARIES_URL ${DOCFILTERS_RELEASES_URL}/linux-ppc-gcc-64.zip)
        else()
            if (EXISTS "/etc/alpine-release")
                set(DOCFILTERS_BINARIES_URL ${DOCFILTERS_RELEASES_URL}/linux-intel-clang-musl-64.zip)
            else()
                set(DOCFILTERS_BINARIES_URL ${DOCFILTERS_RELEASES_URL}/linux-intel-gcc-64.zip)
            endif()
        endif()
    elseif (CMAKE_SYSTEM_NAME MATCHES "Darwin")
        if (CMAKE_SYSTEM_PROCESSOR MATCHES "arm64")
            set(DOCFILTERS_BINARIES_URL ${DOCFILTERS_RELEASES_URL}/macos-arm64-clang-64.zip)
        else()
            set(DOCFILTERS_BINARIES_URL ${DOCFILTERS_RELEASES_URL}/macos-intel-clang-64.zip)
        endif()
    else()
        message(FATAL_ERROR "Unsupported operating system: ${CMAKE_SYSTEM_NAME}")
    endif()

    set(DOCFILTERS_BINARIES_OUTPUT_DIRECTORY ${CMAKE_RUNTIME_OUTPUT_DIRECTORY})
    if ("${DOCFILTERS_BINARIES_OUTPUT_DIRECTORY}" STREQUAL "")
        set(DOCFILTERS_BINARIES_OUTPUT_DIRECTORY ${CMAKE_BINARY_DIR})
    endif()

    # Configure and build the external project
    ExternalProject_Add(${DOCFILTERS_BINARIES_PROJECT}_download
        PREFIX ${DOCFILTERS_BINARIES_PROJECT_PREFIX}
        URL ${DOCFILTERS_BINARIES_URL}
        DOWNLOAD_NAME "${DOCFILTERS_BINARIES_PROJECT}.zip"
        UPDATE_COMMAND ""
        CONFIGURE_COMMAND ""
        BUILD_COMMAND ""
        INSTALL_COMMAND ""
    )

    # Define a custom target to unzip and copy the contents to the bin directory
    add_custom_target(${DOCFILTERS_BINARIES_PROJECT}
        COMMAND ${CMAKE_COMMAND} -E copy_directory ${DOCFILTERS_BINARIES_PROJECT_PREFIX}/src/${DOCFILTERS_BINARIES_PROJECT}_download ${DOCFILTERS_BINARIES_OUTPUT_DIRECTORY}
        WORKING_DIRECTORY ${CMAKE_BINARY_DIR}
    )
    add_dependencies(${DOCFILTERS_BINARIES_PROJECT} ${DOCFILTERS_BINARIES_PROJECT}_download)
    add_dependencies(DocumentFilters ${DOCFILTERS_BINARIES_PROJECT})

    # Install the extracted files (assumes the contents need to be installed to a specific location)
    if (DOCFILTERS_INSTALL_BINARIES)
        install(DIRECTORY ${DOCFILTERS_BINARIES_PROJECT_PREFIX}/src/${DOCFILTERS_BINARIES_PROJECT}_download/ DESTINATION ${CMAKE_INSTALL_PREFIX}/bin FILES_MATCHING PATTERN "*")
    endif()

    if (DOCFILTERS_INSTALL_ASSETS)
        # Configure and build the assets project (fonts, ocr data, etc.)
        set(DOCFILTERS_ASSETS_PROJECT DocumentFiltersAssets)
        set(DOCFILTERS_ASSETS_PROJECT_PREFIX ${CMAKE_BINARY_DIR}/${DOCFILTERS_BINARIES_PROJECT})
        set(DOCFILTERS_ASSETS_URL "${DOCFILTERS_RELEASES_URL}/assets.zip")
        ExternalProject_Add(${DOCFILTERS_ASSETS_PROJECT}_download
            PREFIX ${DOCFILTERS_ASSETS_PROJECT_PREFIX}
            URL ${DOCFILTERS_ASSETS_URL}
            UPDATE_COMMAND ""
            CONFIGURE_COMMAND ""
            BUILD_COMMAND ""
            INSTALL_COMMAND ""
        )

        # Define a custom target to unzip and copy the contents to the bin directory
        add_custom_target(${DOCFILTERS_ASSETS_PROJECT}
            COMMAND ${CMAKE_COMMAND} -E copy_directory ${DOCFILTERS_ASSETS_PROJECT_PREFIX}/src/${DOCFILTERS_ASSETS_PROJECT}_download ${DOCFILTERS_BINARIES_OUTPUT_DIRECTORY}
            WORKING_DIRECTORY ${CMAKE_BINARY_DIR}
        )
        add_dependencies(${DOCFILTERS_ASSETS_PROJECT} ${DOCFILTERS_ASSETS_PROJECT}_download)
        add_dependencies(DocumentFilters ${DOCFILTERS_ASSETS_PROJECT})

        install(DIRECTORY ${DOCFILTERS_ASSETS_PROJECT_PREFIX}/src/${DOCFILTERS_ASSETS_PROJECT}_download/ DESTINATION ${CMAKE_INSTALL_PREFIX}/bin FILES_MATCHING PATTERN "*")
    endif()

endif()

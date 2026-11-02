cmake_minimum_required(VERSION 3.16)

# ---------------------------------------------------------------------------
#  docfilters (C++17) --- the C++ port of DocFiltersCli/csharp/.
#
#  Builds standalone, the same way csharp/DocFiltersCli.csproj does: this
#  file never fetches a test framework, so `cmake --build` here never touches
#  the network for anything the shipped tool needs. The test suite is a
#  separate CMake project, tests/cpp17/, which links docfilters_lib and adds
#  Catch2 on its own --- test-only, exactly as xUnit is test-only for csharp/.
# ---------------------------------------------------------------------------

project(DocFiltersCli LANGUAGES CXX)

# The tool's version comes from DocFiltersCli/VERSION.txt, the same file the C#
# and Python ports read. Listed as a configure dependency so editing it
# re-runs CMake and the new value reaches the compiler.
set(DOCFILTERSCLI_VERSION_FILE ${CMAKE_CURRENT_LIST_DIR}/../VERSION.txt)
set_property(DIRECTORY APPEND PROPERTY CMAKE_CONFIGURE_DEPENDS ${DOCFILTERSCLI_VERSION_FILE})
file(STRINGS ${DOCFILTERSCLI_VERSION_FILE} DOCFILTERSCLI_VERSION LIMIT_COUNT 1)
string(STRIP "${DOCFILTERSCLI_VERSION}" DOCFILTERSCLI_VERSION)
if(NOT DOCFILTERSCLI_VERSION)
  message(FATAL_ERROR "${DOCFILTERSCLI_VERSION_FILE} is missing or empty")
endif()

set(CMAKE_RUNTIME_OUTPUT_DIRECTORY ${CMAKE_BINARY_DIR}/bin)
set(CMAKE_RUNTIME_OUTPUT_DIRECTORY_DEBUG ${CMAKE_BINARY_DIR}/bin)
set(CMAKE_RUNTIME_OUTPUT_DIRECTORY_RELEASE ${CMAKE_BINARY_DIR}/bin)

# The tool loads the Document Filters libraries from beside itself, wherever
# that is: the release archives are bin/ as built, and Document Filters ships
# the executable in its own platform packages. macOS has no $ORIGIN; its
# equivalent is @executable_path.
if(APPLE)
  set(DOCFILTERSCLI_RPATH "@executable_path")
else()
  set(DOCFILTERSCLI_RPATH "$ORIGIN")
endif()
set(CMAKE_INSTALL_RPATH "${DOCFILTERSCLI_RPATH}")

# Same CLI-parsing library the published C++17 samples already use (samples/cpp17/
# CMakeLists.txt), fetched the same way --- see DF-3104's no-new-dependencies
# constraint. No new dependency for the tool itself is introduced here.
include(FetchContent)
FetchContent_Declare(CLI11
  GIT_REPOSITORY https://github.com/CLIUtils/CLI11.git
  GIT_TAG        v2.4.2
)
FetchContent_MakeAvailable(CLI11)

add_subdirectory(${CMAKE_CURRENT_LIST_DIR}/../../bindings/cpp17 bindings)

# ---------------------------------------------------------------------------
#  docfilters_lib --- every stage except main(): Runtime, Configuration,
#  Planning, Discovery, Cli, Engine, Execution, Commands. Exists as its own
#  target so tests/cpp17/ can link the whole tool without pulling in argv
#  handling or the process exit code, the same seam ProjectReference gives
#  the C# test project against csharp/DocFiltersCli.csproj.
# ---------------------------------------------------------------------------
set(DOCFILTERSCLI_SOURCES
    src/Runtime/Strings.cpp
    src/Runtime/CliError.cpp
    src/Runtime/JsonWriter.cpp
    src/Runtime/Interruption.cpp
    src/Runtime/CaseProbe.cpp
    src/Runtime/SystemFileSystem.cpp
    src/Planning/FormatSpec.cpp
    src/Planning/FormatCatalog.cpp
    src/Planning/PageRange.cpp
    src/Planning/OutputNameTemplate.cpp
    src/Planning/NameMatcher.cpp
    src/Planning/RunPlan.cpp
    src/Configuration/SharedOptions.cpp
    src/Configuration/ValueParsers.cpp
    src/Configuration/OptionsValidator.cpp
    src/Discovery/InputEnumerator.cpp
    src/Planning/OutputPlanner.cpp
    src/Cli/ArgumentTable.cpp
    src/Cli/ContractDump.cpp
    src/Cli/CommandLineApp.cpp
    src/Commands/PlanFormat.cpp
    src/Commands/ConvertCommand.cpp
    src/Commands/ExtractCommand.cpp
    src/Commands/ListCommand.cpp
    src/Commands/IdentifyCommand.cpp
    src/Commands/MetaCommand.cpp
    src/Commands/FormatsCommand.cpp
    src/Commands/OptionsCommand.cpp
    src/Commands/HelpCommand.cpp
    src/Commands/VersionCommand.cpp
    src/Engine/EngineException.cpp
    src/Engine/DocumentFiltersEngine.cpp
    src/Execution/DocumentFiltersSession.cpp
    src/Execution/EngineLog.cpp
    src/Execution/PasswordProvider.cpp
    src/Execution/Diagnostics.cpp
    src/Execution/DebugInput.cpp
    src/Execution/ExtractorOpenSettings.cpp
    src/Execution/StructureReport.cpp
    src/Execution/MetadataReport.cpp
    src/Execution/CatalogReports.cpp
    src/Execution/TextConversion.cpp
    src/Execution/SubfileExtraction.cpp
    src/Execution/CanvasConversion.cpp
    src/Runtime/Viewer.cpp
    src/Runtime/SystemConsole.cpp
    src/Program.cpp
)

add_library(docfilters_lib STATIC ${DOCFILTERSCLI_SOURCES})
target_include_directories(docfilters_lib PUBLIC src)
target_compile_features(docfilters_lib PUBLIC cxx_std_17)
target_compile_definitions(docfilters_lib PRIVATE DOCFILTERSCLI_VERSION="${DOCFILTERSCLI_VERSION}")
target_link_libraries(docfilters_lib PUBLIC DocumentFilters::Cpp17 CLI11::CLI11)

# ---------------------------------------------------------------------------
#  docfilters --- the shipped executable.
# ---------------------------------------------------------------------------
add_executable(docfilters src/main.cpp)
target_link_libraries(docfilters PRIVATE docfilters_lib)

# The build tree is what gets shipped, so it carries the install rpath rather
# than CMake's build rpath, which would also name the absolute directory the
# libraries were linked from on the build machine.
set_target_properties(docfilters PROPERTIES BUILD_WITH_INSTALL_RPATH ON)

if(${CMAKE_VERSION} VERSION_GREATER_EQUAL "3.18")
  set_target_properties(CLI11 PROPERTIES FOLDER "Third-Party")
endif()

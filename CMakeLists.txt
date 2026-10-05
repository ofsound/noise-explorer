cmake_minimum_required(VERSION 3.25)
list(APPEND CMAKE_MODULE_PATH "${CMAKE_CURRENT_SOURCE_DIR}/cmake" "${CMAKE_CURRENT_SOURCE_DIR}/cmake_project")
include(PamplejuceVersion)
include(CPM)
include(JUCEDefaults)
if(APPLE AND NOT CMAKE_OSX_DEPLOYMENT_TARGET)
    set(CMAKE_OSX_DEPLOYMENT_TARGET "11.0" CACHE STRING "Minimum macOS version")
endif()
if(NOT CMAKE_BUILD_TYPE AND NOT CMAKE_CONFIGURATION_TYPES)
    set(CMAKE_BUILD_TYPE Debug CACHE STRING "Build configuration" FORCE)
endif()
set(PROJECT_NAME NoiseExplorer)
set(PRODUCT_NAME "Noise Explorer")
set(COMPANY_NAME ofsound)
set(BUNDLE_ID com.ofsound.noiseexplorer)
project(${PROJECT_NAME} VERSION ${CURRENT_VERSION})
include(PamplejuceLog)
include(PamplejuceFunctions)
add_subdirectory(JUCE)
set(FORMATS Standalone VST3)
if(APPLE)
    list(APPEND FORMATS AU)
endif()
option(NOISE_EXPLORER_COPY_PLUGINS "Install plugins in user plugin folders after building" ON)
juce_add_plugin(${PROJECT_NAME}
    COMPANY_NAME "${COMPANY_NAME}"
    BUNDLE_ID "${BUNDLE_ID}"
    ICON_BIG "${CMAKE_CURRENT_SOURCE_DIR}/packaging/icon.png"
    COPY_PLUGIN_AFTER_BUILD FALSE
    PLUGIN_MANUFACTURER_CODE Osnd
    PLUGIN_CODE Nexp
    FORMATS ${FORMATS}
    EDITOR_WANTS_KEYBOARD_FOCUS TRUE
    IS_SYNTH TRUE
    NEEDS_MIDI_INPUT TRUE
    NEEDS_MIDI_OUTPUT FALSE
    IS_MIDI_EFFECT FALSE
    VST3_CATEGORIES Instrument Synth
    AU_MAIN_TYPE kAudioUnitType_MusicDevice
    PRODUCT_NAME "${PRODUCT_NAME}")
if(APPLE)
    include(PamplejuceIcon)
    # Seal local development bundles after all resources are in place.
    # This is ad-hoc signing, not distribution signing or notarization.
    foreach(format IN LISTS FORMATS)
        add_custom_command(TARGET ${PROJECT_NAME}_${format} POST_BUILD
            COMMAND /usr/bin/codesign --force --deep --sign - "$<TARGET_BUNDLE_DIR:${PROJECT_NAME}_${format}>"
            VERBATIM)
    endforeach()
endif()
if(NOISE_EXPLORER_COPY_PLUGINS)
    juce_enable_copy_plugin_step(${PROJECT_NAME})
endif()
add_library(SharedCode INTERFACE)
target_compile_features(SharedCode INTERFACE cxx_std_23)
file(GLOB_RECURSE SourceFiles CONFIGURE_DEPENDS "source/*.cpp" "source/*.h")
target_sources(SharedCode INTERFACE ${SourceFiles})
target_compile_definitions(SharedCode INTERFACE
    JUCE_WEB_BROWSER=0 JUCE_USE_CURL=0 JUCE_VST3_CAN_REPLACE_VST2=0
    CMAKE_BUILD_TYPE="$<CONFIG>" VERSION="${CURRENT_VERSION}"
    PRODUCT_NAME_WITHOUT_VERSION="${PRODUCT_NAME}")
target_link_libraries(SharedCode INTERFACE
    juce_audio_utils juce_audio_processors juce_dsp juce_gui_extra
    juce::juce_recommended_config_flags juce::juce_recommended_warning_flags)
target_link_libraries(${PROJECT_NAME} PRIVATE SharedCode)
pamplejuce_add_tests()
file(GLOB_RECURSE OwnedTests CONFIGURE_DEPENDS "tests/*.cpp")
if(MSVC)
    set_source_files_properties(${SourceFiles} ${OwnedTests} PROPERTIES COMPILE_OPTIONS /WX)
else()
    set_source_files_properties(${SourceFiles} ${OwnedTests} PROPERTIES COMPILE_OPTIONS -Werror)
endif()
include(GitHubENV)

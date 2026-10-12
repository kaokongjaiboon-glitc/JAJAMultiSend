cmake_minimum_required(VERSION 3.22)
project(JAJAMultiSend VERSION 1.0.0)

set(CMAKE_CXX_STANDARD 17)

include(FetchContent)
FetchContent_Declare(JUCE
    GIT_REPOSITORY https://github.com/juce-framework/JUCE.git
    GIT_TAG 8.0.4
    GIT_SHALLOW TRUE)
FetchContent_MakeAvailable(JUCE)

juce_add_plugin(JAJAMultiSend
    COMPANY_NAME "JAJA Trading"
    PLUGIN_MANUFACTURER_CODE Jaja
    PLUGIN_CODE Jmsd
    FORMATS VST3
    PRODUCT_NAME "JAJA Multi Send"
    IS_SYNTH FALSE
    NEEDS_MIDI_INPUT FALSE
    NEEDS_MIDI_OUTPUT FALSE
    COPY_PLUGIN_AFTER_BUILD FALSE)

target_sources(JAJAMultiSend PRIVATE
    Source/PluginProcessor.cpp
    Source/PluginEditor.cpp)

target_compile_definitions(JAJAMultiSend PUBLIC
    JUCE_WEB_BROWSER=0
    JUCE_USE_CURL=0
    JUCE_VST3_CAN_REPLACE_VST2=0)

target_link_libraries(JAJAMultiSend
    PRIVATE
        juce::juce_audio_utils
    PUBLIC
        juce::juce_recommended_config_flags
        juce::juce_recommended_lto_flags
        juce::juce_recommended_warning_flags)

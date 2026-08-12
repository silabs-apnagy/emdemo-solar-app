# Define minimal required version of CMake.
cmake_minimum_required(VERSION "3.25")

# Project definition
project(
	MatterSolarPanelOverThread
	VERSION 1.0
	LANGUAGES C CXX ASM
)

# Include the definition of the slc_MatterSolarPanelOverThread target,
# which contains the content of the SLC project
include(MatterSolarPanelOverThread.cmake)

# Keep the generated source list aligned with the ZAP data model. These server
# components have no enabled endpoints and are removed from the .slcp project;
# filtering them here also makes the checked-in generated project buildable
# before Simplicity Studio regenerates it.
get_target_property(slc_sources slc SOURCES)
list(REMOVE_ITEM slc_sources
     "${CMAKE_SOURCE_DIR}/../src/LightingManager.cpp"
)
list(FILTER slc_sources EXCLUDE REGEX "/(fixed-label|localization-configuration|software-diagnostics|time-format-localization|user-label)-server/")
set_property(TARGET slc PROPERTY SOURCES "${slc_sources}")

# SDK 2026.6.1 requires the codegen provider implementation, but the upgraded
# generated project omitted the corresponding component sources.
foreach(slc_source IN LISTS slc_sources)
    if(slc_source MATCHES "^(.*)/third_party/matter_sdk/")
        set(MATTER_SDK_ROOT "${CMAKE_MATCH_1}/third_party/matter_sdk")
        break()
    endif()
endforeach()

set(codegen_provider_instance "${MATTER_SDK_ROOT}/src/data-model-providers/codegen/Instance.cpp")
list(FIND slc_sources "${codegen_provider_instance}" codegen_provider_index)
if(codegen_provider_index EQUAL -1)
    target_sources(slc PRIVATE
        "${MATTER_SDK_ROOT}/src/data-model-providers/codegen/ClusterIntegration.cpp"
        "${MATTER_SDK_ROOT}/src/data-model-providers/codegen/CodegenDataModelProvider.cpp"
        "${MATTER_SDK_ROOT}/src/data-model-providers/codegen/CodegenDataModelProvider_Read.cpp"
        "${MATTER_SDK_ROOT}/src/data-model-providers/codegen/CodegenDataModelProvider_Write.cpp"
        "${MATTER_SDK_ROOT}/src/data-model-providers/codegen/EmberAttributeDataBuffer.cpp"
        "${codegen_provider_instance}"
    )
endif()

add_executable(MatterSolarPanelOverThread
    # Add additional sources here
    "${CMAKE_SOURCE_DIR}/../src/ElectricalPowerMeasurementDelegate.cpp"
    "${CMAKE_SOURCE_DIR}/../src/PowerTopologyDelegate.cpp"
    "${CMAKE_SOURCE_DIR}/../src/SolarPowerManager.cpp"
)

target_include_directories(MatterSolarPanelOverThread PUBLIC
    # Add additional include paths here
)

target_compile_definitions(MatterSolarPanelOverThread PUBLIC
    # Add additional macros here
)

target_compile_options(MatterSolarPanelOverThread PUBLIC
    # Set additional compiler flags here
)

target_link_options(MatterSolarPanelOverThread PUBLIC
    # Set additional linker flags here
)

# Link with the content defined in the SLC project
target_link_libraries(MatterSolarPanelOverThread PRIVATE
    slc
)

# Include managed project content if available
include(MatterSolarPanelOverThread_project.cmake OPTIONAL RESULT_VARIABLE managed_project)
if(managed_project)
    message(STATUS "Using managed project content from ${managed_project}")
endif()

# Force the gcc linker command
set_target_properties(MatterSolarPanelOverThread PROPERTIES LINKER_LANGUAGE C)

# Create .bin, .hex and .s37 artifacts after building the project
add_custom_command(TARGET MatterSolarPanelOverThread
    POST_BUILD
    COMMAND ${CMAKE_OBJCOPY} ${OBJCOPY_SREC_CMD} "$<TARGET_FILE:MatterSolarPanelOverThread>" "$<TARGET_FILE_DIR:MatterSolarPanelOverThread>/$<TARGET_FILE_BASE_NAME:MatterSolarPanelOverThread>.s37"
    COMMAND ${CMAKE_OBJCOPY} ${OBJCOPY_IHEX_CMD} "$<TARGET_FILE:MatterSolarPanelOverThread>" "$<TARGET_FILE_DIR:MatterSolarPanelOverThread>/$<TARGET_FILE_BASE_NAME:MatterSolarPanelOverThread>.hex"
    COMMAND ${CMAKE_OBJCOPY} ${OBJCOPY_BIN_CMD}  "$<TARGET_FILE:MatterSolarPanelOverThread>" "$<TARGET_FILE_DIR:MatterSolarPanelOverThread>/$<TARGET_FILE_BASE_NAME:MatterSolarPanelOverThread>.bin" 
)

# Run post-build pipeline to perform additional post-processing
if(post_build_command)
add_custom_command(TARGET MatterSolarPanelOverThread
    POST_BUILD
    WORKING_DIRECTORY ${CMAKE_CURRENT_LIST_DIR}/..
    COMMAND ${post_build_command}
)
endif()

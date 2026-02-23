# Define minimal required version of CMake.
cmake_minimum_required(VERSION "3.25")

# Project definition
project(MatterSolarPanelSolution)

include(ExternalProject)

ExternalProject_Add(MatterSolarPanelOverThread
  SOURCE_DIR ${CMAKE_CURRENT_LIST_DIR}/../MatterSolarPanelOverThread/cmake_gcc
  BINARY_DIR ${CMAKE_CURRENT_LIST_DIR}/../MatterSolarPanelOverThread/cmake_gcc/build
  CMAKE_ARGS --preset project
  BUILD_COMMAND ${CMAKE_COMMAND} --build . --config $<CONFIG>
  INSTALL_COMMAND ""
  BUILD_ALWAYS true
  USES_TERMINAL_BUILD 1
  WORKING_DIRECTORY ${CMAKE_CURRENT_LIST_DIR}/../MatterSolarPanelOverThread/cmake_gcc
)
ExternalProject_Add(Matter-Bootloader_2
  SOURCE_DIR ${CMAKE_CURRENT_LIST_DIR}/../Matter-Bootloader_2/cmake_gcc
  BINARY_DIR ${CMAKE_CURRENT_LIST_DIR}/../Matter-Bootloader_2/cmake_gcc/build
  CMAKE_ARGS --preset project
  BUILD_COMMAND ${CMAKE_COMMAND} --build . --config $<CONFIG>
  INSTALL_COMMAND ""
  BUILD_ALWAYS true
  USES_TERMINAL_BUILD 1
  WORKING_DIRECTORY ${CMAKE_CURRENT_LIST_DIR}/../Matter-Bootloader_2/cmake_gcc
)

ExternalProject_Add_StepDependencies(Matter-Bootloader_2 build MatterSolarPanelOverThread)


set(post_build_command ${POST_BUILD_EXE} postbuild "./MatterSolarPanelSolution.slpb")


add_custom_target(postbuild ALL
    WORKING_DIRECTORY ${CMAKE_CURRENT_LIST_DIR}/..
    COMMAND ${post_build_command}
)

add_dependencies(postbuild MatterSolarPanelOverThread Matter-Bootloader_2)

# BEGIN_SIMPLICITY_STUDIO_METADATA=eJy1U11P2zAU/StWhCZ4ILcwaQ8lrNroHiIlBM2tBFKkyHUurTfHzmyHgND+OxdK044O7WmP9/qce8798GPE0/wqSy/S2U3FZ/NpWlS8yOaztLis8mL6LYvGUTK5bzS7Q+eVNedldBKPyoihkbZWZkmJL/wiTcto8rk0Sc6t7gIBPSOW8eN7rwiyCqEdA/R9H/cfY+uWcDoancB1nnG5wkYcK+ODMBKpMjHGxl6KBn0rJK4BmZUirPV3anmlxcLH0jbgPWykY5TWUSXyw1jiX7MvEcUbVG7rTuN3vEVHvaBnqqbiXsu4t+7ni3RciyDKCAbqNJ/irTLqmf8+/hVOhNb6sOiUrnnAdkjTwzZ5cEjuG2FqdEdswLMPvzobzg4ON+6PIBchoCP3wl0Jg3ro1ut2sYYnsC+WwN9NJK2zP1AGZmjQ1Mjb8gUtfLZyKGpaid5OH+aeLgFEa8TyAbhqWq2kCg88dLWycPepGsbxruW9h10x+IfF46/WBm0FDaw6/W/e3qgMJwB/3MD6wmDnxBLY/gAKo99P858xRg===END_SIMPLICITY_STUDIO_METADATA
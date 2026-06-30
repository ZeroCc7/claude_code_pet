cmake_minimum_required(VERSION 3.16)
project(pet_simulator LANGUAGES CXX)

set(CMAKE_CXX_STANDARD 17)
set(CMAKE_CXX_STANDARD_REQUIRED ON)

find_package(SDL2 REQUIRED)

set(FIRMWARE_DIR "${CMAKE_SOURCE_DIR}/../firmware/ai_pet"
    CACHE PATH "Path to firmware source directory")

set(SHIM_DIR "${CMAKE_SOURCE_DIR}/src/shims")

set(SIM_SOURCES
    src/main.cpp
    src/shims/Arduino.cpp
    src/shims/WString.cpp
    src/shims/Print.cpp
    src/shims/HardwareSerial.cpp
    src/shims/SPI.cpp
    src/shims/Adafruit_GFX.cpp
    src/shims/Adafruit_ST7735.cpp
    src/shims/GFXcanvas.cpp
    src/shims/U8g2_for_Adafruit_GFX.cpp
    src/shims/LittleFS.cpp
    src/shims/chinese_text_sim.cpp
    src/backend/sdl_renderer.cpp
    src/backend/input_manager.cpp
    src/overrides/display_device_sim.cpp
    src/overrides/button_scanner_sim.cpp
    src/overrides/save_store_sim.cpp
)

set(FIRMWARE_SOURCES
    ${FIRMWARE_DIR}/game_state.cpp
    ${FIRMWARE_DIR}/game_ui.cpp
    ${FIRMWARE_DIR}/game_app.cpp
    ${FIRMWARE_DIR}/pet_renderer.cpp
    ${FIRMWARE_DIR}/ai_event_protocol.cpp
)

add_executable(pet_simulator
    ${SIM_SOURCES}
    ${FIRMWARE_SOURCES}
)

target_include_directories(pet_simulator PRIVATE
    ${SHIM_DIR}
    ${CMAKE_SOURCE_DIR}/src
    ${CMAKE_SOURCE_DIR}/src/backend
    ${CMAKE_SOURCE_DIR}/src/overrides
    ${FIRMWARE_DIR}
)

target_compile_definitions(pet_simulator PRIVATE
    SIMULATOR_BUILD=1
)

if(TARGET SDL2::SDL2)
    target_link_libraries(pet_simulator PRIVATE SDL2::SDL2main SDL2::SDL2 SDL2::SDL2)
else()
    target_include_directories(pet_simulator PRIVATE ${SDL2_INCLUDE_DIRS})
    target_link_libraries(pet_simulator PRIVATE mingw32 ${SDL2_LIBRARIES} ${SDL2_LIBRARIES})
endif()

if(WIN32)
    find_file(SDL2_DLL "SDL2.dll"
        HINTS ${SDL2_LIBRARY_DIRS}
    )
    if(SDL2_DLL)
        add_custom_command(TARGET pet_simulator POST_BUILD
            COMMAND ${CMAKE_COMMAND} -E copy_if_different
                "${SDL2_DLL}" "$<TARGET_FILE_DIR:pet_simulator>"
        )
    endif()
endif()

set(
        IMGUI_SRC_FILES
        ${imgui_SOURCE_DIR}/imgui.cpp
        ${imgui_SOURCE_DIR}/imgui_draw.cpp
        ${imgui_SOURCE_DIR}/imgui_tables.cpp
        ${imgui_SOURCE_DIR}/imgui_widgets.cpp
        ${imgui_SOURCE_DIR}/imgui_demo.cpp
        ${imgui_SOURCE_DIR}/backends/imgui_impl_glfw.cpp
        ${imgui_SOURCE_DIR}/backends/imgui_impl_opengl3.cpp
)

set(
        DRAFT_SRC_FILES
        main.cpp
        rendering/Shader.cpp
        rendering/Camera.cpp
        rendering/Model.cpp
        rendering/Noise.cpp
)

set(CMAKE_RUNTIME_OUTPUT_DIRECTORY ${CMAKE_CURRENT_SOURCE_DIR}/../)

add_executable(draft ${DRAFT_SRC_FILES} ${IMGUI_SRC_FILES})
target_link_libraries(draft ${DRAFT_LIBS})
target_include_directories(draft PUBLIC ${imgui_SOURCE_DIR} ${stb_SOURCE_DIR})
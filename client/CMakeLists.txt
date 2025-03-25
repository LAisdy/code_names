# Клиентский проект
add_executable(CodeNamesClient CodeNamesCli.cpp)

# Линковка с SFML (требуется Graphics, Window, System)
target_link_libraries(CodeNamesClient PRIVATE 
    sfml-graphics-s 
    sfml-window-s 
    sfml-system-s
    sfml-network-s
    opengl32
    freetype
    winmm
    ws2_32
)

if (WIN32 AND MSVC)
    # Для Release: отключаем консоль
    set_target_properties(CodeNamesClient PROPERTIES
        LINK_FLAGS_RELEASE "/SUBSYSTEM:WINDOWS /ENTRY:mainCRTStartup"
    )
    
    # Для Debug: оставляем консоль
    set_target_properties(CodeNamesClient PROPERTIES
        LINK_FLAGS_DEBUG "/SUBSYSTEM:CONSOLE"
    )
endif()

# Для статической линковки
target_compile_definitions(CodeNamesClient PRIVATE SFML_STATIC)

# Путь к шрифтам
set(FONTS
    "${CMAKE_SOURCE_DIR}/res/cyr.OTF"
    "${CMAKE_SOURCE_DIR}/res/fontMenu.ttf"
)

# Копировать шрифты
foreach(FONT ${FONTS})
    add_custom_command(
        TARGET CodeNamesClient POST_BUILD
        COMMAND ${CMAKE_COMMAND} -E copy "${FONT}" "$<TARGET_FILE_DIR:CodeNamesClient>"
    )
endforeach()

# Предупреждения
if(MSVC)
    target_compile_options(CodeNamesClient PRIVATE /W4)
else()
    target_compile_options(CodeNamesClient PRIVATE -Wall -Wextra)
endif()

add_custom_command(TARGET CodeNamesClient POST_BUILD
    COMMAND ${CMAKE_COMMAND} -E copy_directory
    "${CMAKE_SOURCE_DIR}/libs/SFML/bin/"
    $<TARGET_FILE_DIR:CodeNamesClient>
)
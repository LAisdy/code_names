# Серверный проект
add_executable(CodeNamesServer CodeNamesServ.cpp)

# Линковка с SFML (требуется Network и System)

target_include_directories(CodeNamesServer PRIVATE
    "${CMAKE_SOURCE_DIR}/libs/SFML/include"
)

target_link_libraries(CodeNamesServer PRIVATE 
    sfml-network-s 
    sfml-system-s
    ws2_32
)

# Для статической линковки
target_compile_definitions(CodeNamesServer PRIVATE SFML_STATIC)

#Копируем базу слов 
if(EXISTS "${CMAKE_SOURCE_DIR}/words.txt")
    add_custom_command(TARGET CodeNamesServer POST_BUILD
        COMMAND ${CMAKE_COMMAND} -E copy_if_different
            "${CMAKE_SOURCE_DIR}/words.txt"
            $<TARGET_FILE_DIR:CodeNamesServer>
        COMMENT "Copying words.txt to server output directory"
    )
else()
    message(WARNING "words.txt not found in ${CMAKE_SOURCE_DIR}. Server may fail to start.")
endif()

# Предупреждения
if (MSVC)
    target_compile_options(CodeNamesServer PRIVATE /W4)
else()
    target_compile_options(CodeNamesServer PRIVATE -Wall -Wextra -Wpedantic)
endif()
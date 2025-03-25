# Серверный проект
add_executable(CodeNamesServer CodeNamesServ.cpp)

# Линковка с SFML (требуется Network и System)
target_link_libraries(CodeNamesServer PRIVATE 
    sfml-network-s 
    sfml-system-s
    ws2_32
)

# Для статической линковки
target_compile_definitions(CodeNamesServer PRIVATE SFML_STATIC)

# Предупреждения
if (MSVC)
    target_compile_options(CodeNamesServer PRIVATE /W4)
else()
    target_compile_options(CodeNamesServer PRIVATE -Wall -Wextra -Wpedantic)
endif()
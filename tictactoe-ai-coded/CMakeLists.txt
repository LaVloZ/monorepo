cmake_minimum_required(VERSION 3.16)
project(box_game C)

set(CMAKE_C_STANDARD 11)
set(CMAKE_C_STANDARD_REQUIRED ON)

find_package(raylib REQUIRED)

find_package(PkgConfig REQUIRED)
pkg_check_modules(CJSON REQUIRED libcjson)

add_executable(box-game
    src/main.c
    src/board.c
    src/game.c
    src/ai.c
)
target_link_libraries(box-game PRIVATE raylib)

enable_testing()
add_executable(box-game-tests
    tests/test_game.c
    src/game.c
    src/ai.c
    src/claude_parse.c
)
target_include_directories(box-game-tests PRIVATE src ${CJSON_INCLUDE_DIRS})
target_link_directories(box-game-tests PRIVATE ${CJSON_LIBRARY_DIRS})
target_link_libraries(box-game-tests PRIVATE ${CJSON_LIBRARIES})
add_test(NAME game_tests COMMAND box-game-tests)

cmake_minimum_required(VERSION 3.16)
project(box_game C)

set(CMAKE_C_STANDARD 11)
set(CMAKE_C_STANDARD_REQUIRED ON)

find_package(raylib REQUIRED)

add_executable(box-game
    src/main.c
    src/board.c
)
target_link_libraries(box-game PRIVATE raylib)

enable_testing()
add_executable(box-game-tests
    tests/test_game.c
    src/game.c
    src/ai.c
)
target_include_directories(box-game-tests PRIVATE src)
add_test(NAME game_tests COMMAND box-game-tests)

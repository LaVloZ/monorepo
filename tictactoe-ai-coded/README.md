# Tic Tac Toe

> 🇫🇷 *Lire en français : [README.fr.md](README.fr.md).*

A Tic-Tac-Toe game written in C with a [raylib](https://www.raylib.com/) graphical interface.
You play as **X**, the computer plays **O**, across three difficulty levels —
including a **Hard** level driven by Anthropic's Claude API.

## Features

- Graphical interface (3×3 grid, piece drop-in animation)
- Difficulty selection menu
- Three AI levels:
  - **Easy** — random moves
  - **Medium** — heuristic (win / block / center / corner)
  - **Hard** — move chosen by Claude (`claude-opus-4-8`) via the Anthropic API,
    falling back to the medium heuristic on network errors
- `Esc` to return to the menu at any time

## Requirements

- CMake ≥ 3.16 and a C11 compiler
- [raylib](https://www.raylib.com/)
- [libcurl](https://curl.se/libcurl/)
- [cJSON](https://github.com/DaveGamble/cJSON) (`libcjson`)
- pthreads (usually provided by the system)

On macOS (Homebrew):

```sh
brew install cmake raylib curl cjson
```

## Building

```sh
cmake -B build
cmake --build build
```

The executable is generated at `build/box-game`.

## Running the game

```sh
./build/box-game
```

For the **Hard** level, export your Anthropic API key before launching:

```sh
export ANTHROPIC_API_KEY="sk-ant-..."
./build/box-game
```

Without a valid key (or on network errors), the Hard level automatically
falls back to the Medium level heuristic.

## Tests

```sh
cmake --build build
ctest --test-dir build
```

The unit tests cover the game rules, the heuristic AI, and the parsing of
Claude's responses.

## Project structure

```
src/
  main.c          raylib game loop + state machine (menu / game)
  game.{c,h}      Tic-tac-toe rules (board, moves, win conditions)
  ai.{c,h}        Heuristic AI (easy / medium)
  claude.{c,h}    Claude API client (libcurl) + async wrapper (pthread)
  claude_parse.{c,h}  Prompt construction + JSON response parsing
  board.{c,h}     raylib rendering (grid, pieces, text, menu)
tests/
  test_game.c     Unit tests
docs/             Specs and implementation plans
```

## Architecture

The game is driven by a state machine (`AppState`: menu / game) and a
sub-machine for gameplay (`PlayPhase`: waiting, animation, AI wait, Claude
thinking). The call to the Claude API runs in a separate thread and is polled
without blocking the render loop (`ClaudeRequestStart` / `…Poll` / `…Free`),
keeping the interface smooth while Claude "thinks".

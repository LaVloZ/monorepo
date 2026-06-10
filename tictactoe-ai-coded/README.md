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

## Implementation workflow

The whole project was built with [Claude Code](https://claude.com/claude-code)
and the **Superpowers** skills, in small, verifiable increments. Every feature
went through the same three-step loop before a single line of production code
was written:

1. **Brainstorm & spec** — explore intent, constraints and design, captured as
   a spec document in `docs/` (`brainstorming` skill).
2. **Plan** — break the spec into an ordered, test-first plan
   (`writing-plans` skill).
3. **Build, test-first** — implement in tiny commits, tests before code,
   refactor as needed (`test-driven-development` skill).

The git history reflects this rhythm directly: it reads as a clean sequence of
`spec → plan → feat/test` triplets. Five milestones, each shippable on its own:

| # | Milestone | Incremental flow (read top-to-bottom in git) |
|---|-----------|----------------------------------------------|
| 1 | **Board rendering** | spec → plan → empty raylib window → 3×3 grid |
| 2 | **Game rules (TDD)** | spec → plan → CTest harness → place a move → reject illegal moves → win detection → draw detection → random AI → draw X/O + status bar → full game loop |
| 3 | **Difficulty levels** | spec → plan → `GameIsWinningMove` helper → refactor `AiChooseMove(Difficulty)` → medium heuristic (win / block / center / corner) → menu screen → menu/game state machine |
| 4 | **Piece animation** | spec → plan → partial-piece drawing → animated stroke + AI delay |
| 5 | **Hard level (Claude)** | spec → plan → prompt build + JSON parsing → async libcurl client (pthread) → Hard level + `PLAY_THINKING` phase |

Two things stand out for a reviewer:

- **Tests come first.** The rules engine (milestone 2) starts with the CTest
  harness, then grows one behavior per commit — you can watch the suite drive
  the design.
- **Each step is small and reversible.** No commit mixes a spec, a plan and a
  feature; every `feat:` is a single coherent behavior, so the history doubles
  as a step-by-step walkthrough of how the game was built.

> **A telling example — the difficulty menu (YAGNI in action).**
> While there was only one AI (the random "Easy" opponent), there was *no menu*
> at all — none was needed. The difficulty menu appeared in the very next
> commit *after* the medium heuristic was added (`feat: medium heuristic` →
> `feat: menu screen`): the moment a real choice existed, the UI to make that
> choice was built. We didn't speculate the menu up front; the second
> difficulty is what justified it.

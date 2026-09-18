# Dungeon Runner

A terminal-based dungeon-crawling game built as a multi-language system, combining a C game engine with a Python interface layer.

## Overview

Dungeon Runner is a text-based adventure game where the player navigates a dungeon, unlocks gated areas, and collects treasure to win. The project was built around a Model-View-Controller architecture to keep game logic, control flow, and rendering cleanly separated.

## Features

- **Core gameplay loop**: move through the dungeon, collect treasure, and reach the win condition
- **Lock-and-key mechanic**: certain areas are gated and require finding the right key to progress
- **Color-coded terminal rendering**: visual feedback for different game elements using the curses library
- **Persistent player profiles**: player state is saved and loaded via JSON serialization, so progress carries across sessions

## Architecture

The project follows an MVC pattern across two languages:

- **C**: core game engine logic
- **Python (Model)**: `Player` and related classes, implemented as direct wrappers around the C functions using `ctypes`
- **Python (Controller)**: `GameEngine`, which coordinates the model and view and manages game flow
- **Python (View)**: `GameUI`, which handles all rendering, display updates, and user input via the curses library

## Tech stack

C, Python, ctypes, curses, JSON

## Running the game

```bash
# from the python/ directory
python run_game.py
```

(Add any setup or build steps specific to your environment, e.g. compiling the C code first if needed.)

## Notes

This project was originally built as a course assignment for CIS*2750 at the University of Guelph. The required features (OOP/MVC design, JSON persistence, curses UI, game runner) were implemented in full, along with three extended features: colored rendering, the lock-and-key system, and the treasure collection win condition.





import curses
from ..models.game_engine import GameEngine
from ..bindings import Direction

class GameUI:
    def __init__(self, engine: GameEngine, profile: dict):
        # store engine, profile, and a message string for the message bar
        pass

    def run(self, stdscr) -> None:
        # store stdscr as instance variable
        # turn off cursor with curses.curs_set(0)
        # call _show_splash
        # call _game_loop
        # update profile dict with final stats from engine
        # call _show_quit_screen
        pass
import curses
from datetime import datetime, timezone
from ..models.game_engine import GameEngine
from ..bindings import Direction

class GameUI:
    def __init__(self, engine: GameEngine, profile: dict):
        # store engine, profile, and a message string for the message bar
        self._engine = engine
        self._profile = profile
        self._screen = None
        self._message = ""
        self._visited_rooms = set()
        self._victory = False

    def run(self, stdscr) -> None:
        # store stdscr as instance variable
        self._screen = stdscr
        # turn off cursor with curses.curs_set(0)
        curses.curs_set(0)
        # call _show_splash
        self._show_splash()
        # call _game_loop
        self._game_loop()
        # update profile dict with final stats from engine
        collected = self._engine.player.get_collected_count()
        self._profile["games_played"]+= 1
        self._profile["max_treasure_collected"]= max(collected, self._profile["max_treasure_collected"])
        self._profile["timestamp_last_played"] = datetime.now(timezone.utc).strftime("%Y-%m-%dT%H:%M:%SZ")
        self._profile["most_rooms_world_completed"] = max(len(self._visited_rooms), self._profile["most_rooms_world_completed"])
        # call _show_quit_screen
        self._show_quit_screen()

    def _show_splash(self) -> None:
        while True:
            self._screen.clear()
            screen_height, screen_width = self._screen.getmaxyx()
            if screen_height < 24 or screen_width < 80:
                self._screen.addstr(0, 0, "Terminal too small! Need 80 x 24")
                self._screen.refresh()
                self._screen.getch()
                continue  # loop back and check again
            # terminal is big enough, draw splash
            row = 0
            col = (screen_width - len("TREASURE RUNNER")) // 2
            self._screen.addstr(row, col, "TREASURE RUNNER")
            row += 1
            self._screen.addstr(row, 0, f"Player: {self._profile['player_name']}")
            row += 1
            self._screen.addstr(row, 0, f"Games Played: {self._profile['games_played']}")
            row += 1
            self._screen.addstr(row, 0, f"Max Treasure: {self._profile['max_treasure_collected']}")
            row += 1
            self._screen.addstr(row, 0, f"Rooms Completed: {self._profile['most_rooms_world_completed']}")
            row += 1
            self._screen.addstr(row, 0, f"Last Played: {self._profile['timestamp_last_played']}")
            row += 1
            col = max(0, screen_width - len("Press any key to continue..."))
            self._screen.addstr(row, col, "Press any key to continue...")
            self._screen.refresh()
            self._screen.getch()
            break  # splash shown and key pressed, exit loop

    def _game_loop(self):
        while True:
            try:
                self._draw()
                self._visited_rooms.add(self._engine.get_current_room_id())
            except curses.error:
                pass
            key = self._screen.getch()
            if not self._handle_input(key):
                break
            self._visited_rooms.add(self._engine.get_current_room_id())  # add after move too
            if self._check_victory():
                self._show_victory_screen()
                break

    def _draw(self) -> None:
        screen_height, screen_width = self._screen.getmaxyx()
        if screen_height < 24 or screen_width < 80:
            self._screen.clear()
            self._screen.addstr(0, 0, "Terminal too small! Need 80 x 24")
            self._screen.refresh()
            return
        self._screen.clear()
        row = 0
        self._screen.addstr(row, 0, self._message)
        row = row+1
        self._screen.addstr(row, 0, f"Room Number = {self._engine.get_current_room_id()}  Room Name = {self._engine.get_current_room_name()}")
        room_setup = self._engine.render_current_room()
        room_setup = room_setup.split("\n")
        row = row + 1
        for i, line in enumerate(room_setup):
            self._screen.addstr(row + i, 4, line)
        width, _ = self._engine.get_room_dimensions()
        legend = ["Game Elements:", "@ - player", "# - wall", "$ - gold", "x - exit",]
        for i, line in enumerate(legend):
            self._screen.addstr(row + i, 4 + width + 4, line)
        row = row + len(room_setup) + 1
        self._screen.addstr(row, 0, "Game Controls")
        row = row + 1
        self._screen.addstr(row, 0, "Controls: Arrows/WASD - move  > - portal  r - reset  q - quit")
        row = row + 2
        collected = self._engine.player.get_collected_count()
        self._screen.addstr(row, 0, f"Player: {self._profile['player_name']}  Treasure: {collected}  Room: {self._engine.get_current_room_id()}")
        row = row +1
        self._screen.addstr(row, 0, "TREASURE RUN")
        self._screen.addstr(row, screen_width- len("anagha.19.kulkarni@gmail.com"), "anagha.19.kulkarni@gmail.com")
        self._screen.refresh()

    def _handle_input(self, key) -> bool:
        if key in (curses.KEY_UP, ord('w')):
            # move north
            self._engine.move_player(Direction.NORTH)
            self._message = self._engine.last_message
        elif key in (curses.KEY_DOWN, ord('s')):
            # move south
            self._engine.move_player(Direction.SOUTH)
            self._message = self._engine.last_message
        elif key in (curses.KEY_LEFT, ord('a')):
            # move west
            self._engine.move_player(Direction.WEST)
            self._message = self._engine.last_message
        elif key in (curses.KEY_RIGHT, ord('d')):
            # move east
            self._engine.move_player(Direction.EAST)
            self._message = self._engine.last_message
        elif key == ord('>'):
            # use portal
            self._message = "Walk into a portal to use it."
        elif key == ord('r'):
            # reset
            self._engine.reset()
            self._message = self._engine.last_message
        elif key == ord('q'):
            return False
        return True

    def _show_quit_screen(self) -> None:
        self._screen.clear()
        _, screen_width = self._screen.getmaxyx()
        row = 0
        col = (screen_width - len("GAME OVER!!!")) // 2
        self._screen.addstr(row, col, "GAME OVER!!!")
        row = row + 1
        self._screen.addstr(row, 0, f"Player: {self._profile['player_name']}")
        row = row + 1
        self._screen.addstr(row, 0, f"Games Played: {self._profile['games_played']}")
        row = row + 1
        self._screen.addstr(row, 0, f"Max Treasure: {self._profile['max_treasure_collected']}")
        row = row + 1
        self._screen.addstr(row, 0, f"Rooms Completed: {self._profile['most_rooms_world_completed']}")
        row = row + 1
        self._screen.addstr(row, 0, f"Last Played: {self._profile['timestamp_last_played']}")
        row = row + 1
        col = max(0, screen_width - len("Press any key to exit..."))
        self._screen.addstr(row, col, "Press any key to exit...")
        self._screen.refresh()
        self._screen.getch()

    def _check_victory(self) -> bool:
        collected = self._engine.player.get_collected_count()
        total = self._engine.get_total_treasures()
        return collected >= total

    def _show_victory_screen(self) -> None:
        self._screen.clear()
        screen_height, screen_width = self._screen.getmaxyx()
        row = screen_height // 2 - 3
        col = (screen_width - len("YOU WIN!")) // 2
        self._screen.addstr(row, col, "YOU WIN!")
        row += 2
        collected = self._engine.player.get_collected_count()
        self._screen.addstr(row, 0, f"Treasures collected: {collected}")
        row += 1
        self._screen.addstr(row, 0, f"Rooms visited: {len(self._visited_rooms)}")
        row += 2
        col = max(0, screen_width - len("Press any key to continue..."))
        self._screen.addstr(row, col, "Press any key to exit...")
        self._screen.refresh()
        self._screen.getch()

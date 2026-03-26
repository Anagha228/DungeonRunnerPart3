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

    def run(self, stdscr) -> None:
        # store stdscr as instance variable
        self._screen = stdscr
        # turn off cursor with curses.curs_set(0)
        curses.curs_set(0)
        curses.start_color()
        curses.use_default_colors()
        curses.init_pair(1, curses.COLOR_GREEN,   curses.COLOR_BLACK)  # player
        curses.init_pair(2, curses.COLOR_YELLOW,  curses.COLOR_BLACK)  # treasure
        curses.init_pair(3, curses.COLOR_CYAN,    curses.COLOR_BLACK)  # portal
        curses.init_pair(4, curses.COLOR_RED,     curses.COLOR_BLACK)  # locked portal
        curses.init_pair(5, curses.COLOR_MAGENTA, curses.COLOR_BLACK)  # pushable
        curses.init_pair(6, curses.COLOR_BLUE,    curses.COLOR_BLACK)  # wall
        curses.init_pair(7, curses.COLOR_WHITE,   curses.COLOR_BLACK)  # switch off
        curses.init_pair(8, curses.COLOR_GREEN,   curses.COLOR_BLACK)  # switch on
        # call _show_splash
        self._show_splash()
        # call _game_loop
        self._game_loop()
        # update profile dict with final stats from engine
        collected = self._engine.player.get_collected_count()
        self._profile["games_played"]+= 1
        self._profile["max_treasure_collected"]= max(collected, self._profile["max_treasure_collected"])
        self._profile["timestamp_last_played"] = datetime.now(timezone.utc).strftime("%Y-%m-%dT  %H:%M:%SZ")
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
        self._room_setup(row, self._engine.render_current_room().split("\n"))
        width, _ = self._engine.get_room_dimensions()
        charset = self._engine.get_charset()
        legend = [
            "Game Elements:",
            f"{charset.player} - player",
            f"{charset.wall} - wall",
            f"{charset.treasure} - gold",
            f"{charset.portal} - portal",
            "L - locked portal",
            f"{charset.pushable} - pushable",
            f"{charset.switch_off} - switch (off)",
            f"{charset.switch_on} - switch (on)",
        ]
        for i, line in enumerate(legend):
            self._screen.addstr(row + i, 4 + width + 4, line)
        row = row + len(self._engine.render_current_room().split("\n"))
        self._screen.addstr(row, 0, "Game Controls")
        row = row + 1
        self._screen.addstr(row, 0, "Controls: | Arrows/WASD - move | > - portal | r - reset | q - quit")
        row = row + 2
        collected = self._engine.player.get_collected_count()
        self._screen.addstr(row, 0, f"Player: {self._profile['player_name']} | Gold Collected : {collected}/{self._engine.get_total_treasures()} | Room visiting: {self._engine.get_current_room_id()} | Rooms visited: {len(self._visited_rooms)}/{self._engine.get_room_count()}")
        row = row + 2
        self._screen.addstr(row, 0, "TREASURE RUN")
        self._screen.addstr(row, screen_width- len("anagha.19.kulkarni@gmail.com"), "anagha.19.kulkarni@gmail.com")
        self._message = ""
        self._screen.refresh()

    def _room_setup(self, row, room_lines):
        row = row+1
         # map characters to color pair numbers
        charset = self._engine.get_charset()
        color_map = {
            charset.player: (1, curses.A_BOLD),
            charset.treasure: (2, 0),
            charset.portal: (3, curses.A_BOLD),
            'L': (4, 0),
            charset.pushable: (5, 0),
            charset.wall: (6, 0),
            charset.switch_off: (7, 0),
            charset.switch_on: (8, 0),
        }
        for i, line in enumerate(room_lines):
            for j, char in enumerate(line):
                pair, bold = color_map.get(char, (0, 0))
                try:
                    self._screen.addch(row + i, 4 + j, char, curses.color_pair(pair) | bold if pair else curses.A_NORMAL)
                except curses.error:
                    pass

    def _handle_input(self, key) -> bool:
        direction_map = {
            curses.KEY_UP: Direction.NORTH,
            ord('w'): Direction.NORTH,
            curses.KEY_DOWN: Direction.SOUTH,
            ord('s'): Direction.SOUTH,
            curses.KEY_LEFT: Direction.WEST,
            ord('a'): Direction.WEST,
            curses.KEY_RIGHT: Direction.EAST,
            ord('d'): Direction.EAST,
        }
        if key in direction_map:
            before = self._engine.player.get_collected_count()
            self._engine.move_player(direction_map[key])
            if before < self._engine.player.get_collected_count():
                self._message = f"Picked up Gold!!! {self._engine.player.get_collected_count()}/{self._engine.get_total_treasures()} treasures"
            else:
                self._message = self._engine.last_message
        elif key == ord('>'):
            self._message = "Walk into a portal to use it."
        elif key == ord('r'):
            self._engine.reset()
            self._message = self._engine.last_message
        elif key in (ord('q'), ord('x')):
            return False
        return True


    def _show_quit_screen(self) -> None:
        try:
            self._screen.clear()
            _, screen_width = self._screen.getmaxyx()
            row = 0
            col = (screen_width - len("GAME OVER!!!")) // 2
            self._screen.addstr(row, col, "GAME OVER!!!")
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
            col = max(0, screen_width - len("Press any key to exit..."))
            self._screen.addstr(row, col, "Press any key to exit...")
        except curses.error:
            pass
        self._screen.refresh()
        self._screen.getch()

    def _check_victory(self) -> bool:
        collected = self._engine.player.get_collected_count()
        total = self._engine.get_total_treasures()
        return collected == total

    def _show_victory_screen(self) -> None:
        try:
            self._screen.clear()
            screen_height, screen_width = self._screen.getmaxyx()
            row = screen_height // 2 - 3
            col = (screen_width - len("YOU WIN!")) // 2
            self._screen.addstr(row, col, "YOU WIN!")
            row += 2
            collected = self._engine.player.get_collected_count()
            self._screen.addstr(row, 0, f"Treasures collected: {collected}/{self._engine.get_total_treasures()}")
            row += 1
            self._screen.addstr(row, 0, f"Rooms visited: {len(self._visited_rooms)}")
            row += 2
            col = max(0, screen_width - len("Press any key to continue..."))
            self._screen.addstr(row, col, "Press any key to exit...")
        except curses.error:
            pass
        self._screen.refresh()
        self._screen.getch()

import ctypes
from .player import Player
from ..bindings import Direction, lib, Status
from .exceptions import *

class GameEngine:
    def __init__(self, config_path: str):
        self._eng = ctypes.c_void_p()
        status = lib.game_engine_create(config_path.encode(), ctypes.byref(self._eng))
        if status != Status.OK:
            raise status_to_status_exception(status, message="")
        player_ptr = lib.game_engine_get_player(self._eng)
        if not player_ptr:
            raise RuntimeError("Failed to get player")
        self._player = Player(ptr=player_ptr)
        self._last_message = ""

    @property
    def player(self):
        return self._player
    @property
    def last_message(self)->str:
        return self._last_message

    def destroy(self) -> None:
        if self._eng:
            lib.game_engine_destroy(self._eng)
            self._eng = None

    def move_player(self, direction: Direction) -> None:
        self._last_message = ""
        status = lib.game_engine_move_player(self._eng, direction.value)
        if status == Status.ROOM_IMPASSABLE:
            self._last_message = "You can't go that way."
        elif status == Status.ROOM_NO_PORTAL:
            self._last_message = "Portal is locked! Push a block onto the switch (^)."
        elif status == Status.ROOM_NOT_FOUND:
            self._last_message = "Portal is locked! Push a block onto the switch (^)."
        elif status != Status.OK:
            raise status_to_status_exception(status, message="")
    def reset(self) -> None:
        self._last_message = "Game reset."
        status = lib.game_engine_reset(self._eng)
        if status != Status.OK:
            raise status_to_status_exception(status, message="")

    def render_current_room(self) -> str:
        s_str = ctypes.c_char_p()
        status = lib.game_engine_render_current_room(self._eng, ctypes.byref(s_str))
        if status != Status.OK:
            raise status_to_status_exception(status, message="")
        result = s_str.value.decode("utf-8")
        lib.game_engine_free_string(s_str)
        return result
    def get_room_count(self) -> int:
        x = ctypes.c_int()
        status = lib.game_engine_get_room_count(self._eng, ctypes.byref(x))
        if status != Status.OK:
            raise status_to_status_exception(status, message="")
        return x.value
    def get_room_dimensions(self) -> tuple[int, int]:
        x = ctypes.c_int()
        y = ctypes.c_int()
        status = lib.game_engine_get_room_dimensions(self._eng, ctypes.byref(x), ctypes.byref(y))
        if status != Status.OK:
            raise status_to_status_exception(status, message="")
        return (x.value, y.value)
    def get_room_ids(self) -> list[int]:
        r_id = ctypes.POINTER(ctypes.c_int)()
        count = ctypes.c_int()
        status = lib.game_engine_get_room_ids(self._eng, ctypes.byref(r_id), ctypes.byref(count))
        if status != Status.OK:
            raise status_to_status_exception(status, message="")
        r_list = []
        for i in range(count.value):
            r_list.append(r_id[i])
        lib.game_engine_free_string(r_id)
        return r_list
    def get_current_room_id(self) -> int:
        return self._player.get_room()
    def get_current_room_name(self) -> str:
        name = ctypes.c_char_p()
        status = lib.game_engine_get_current_room_name(self._eng, ctypes.byref(name))
        if status != Status.OK or name.value is None:
            return "Unknown"
        return name.value.decode("utf-8")
    def get_total_treasures(self) -> int:
        count = ctypes.c_int()
        status = lib.game_engine_get_total_treasure_count(self._eng, ctypes.byref(count))
        if status != Status.OK:
            raise status_to_status_exception(status, message="")
        return count.value

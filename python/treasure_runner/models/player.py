import ctypes
#import os
from ..bindings import Direction, lib
from ..bindings import Status
#from .exceptions import *


class Player:
    # set the instance variable _ptr to the opaque Player type defined in bindings.c
    def __init__(self, ptr):
        self._ptr = ptr
    # get the room using a function created in bindings.c
    def get_room(self) -> int:
        return lib.player_get_room(self._ptr)
    # get the player's position using a function created in bindings.c
    def get_position(self) -> tuple[int, int]:
        x = ctypes.c_int()
        y = ctypes.c_int()
        lib.player_get_position(self._ptr, ctypes.byref(x), ctypes.byref(y))
        return (x.value, y.value)
    def get_collected_count(self) -> int:
        return lib.player_get_collected_count(self._ptr)
    def has_collected_treasure(self, treasure_id: int) -> bool:
        return lib.player_has_collected_treasure(self._ptr, treasure_id)
    def get_collected_treasures(self) -> list[dict]:
        count = ctypes.c_int()
        treasures = lib.player_get_collected_treasures(self._ptr, ctypes.byref(count))
        t_list = []
        if count.value > 0:
            for i in range(count.value):
                t_struct = treasures[i]
                t_dict = {
                    "id": t_struct.id,
                    "name": t_struct.name.decode() if t_struct.name else None,
                    "starting_room_id": t_struct.starting_room_id,
                    "initial_x": t_struct.initial_x,
                    "initial_y": t_struct.initial_y,
                    "x": t_struct.x,
                    "y": t_struct.y,
                    "collected": t_struct.collected,
                }
                t_list.append(t_dict)
        return t_list

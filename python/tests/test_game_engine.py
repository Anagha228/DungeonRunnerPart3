import unittest
from treasure_runner.models.player import Player
from treasure_runner.models.game_engine import GameEngine
from treasure_runner.bindings import Direction, lib, Status
from treasure_runner.models.exceptions import GameEngineError

class TestGameEngine(unittest.TestCase):

    def setUp(self):
        self.engine = GameEngine("../assets/treasure_runner.ini")

    def test_engine_creation(self):
        self.assertIsNotNone(self.engine)

    def test_player_exists(self):
        self.assertIsNotNone(self.engine.player)

    def test_get_room_count(self):
        count = self.engine.get_room_count()
        self.assertIsInstance(count, int)
        self.assertGreater(count, 0)

    def test_get_room_dimensions(self):
        dims = self.engine.get_room_dimensions()
        self.assertIsInstance(dims, tuple)
        self.assertEqual(len(dims), 2)

    def test_render_current_room(self):
        room_str = self.engine.render_current_room()
        self.assertIsInstance(room_str, str)
        self.assertGreater(len(room_str), 0)

    def test_move_player_valid(self):
        # Just ensure it doesn't raise
        self.engine.move_player(Direction.EAST)

    def test_get_room_ids(self):
        ids = self.engine.get_room_ids()
        self.assertIsInstance(ids, list)
        self.assertTrue(all(isinstance(i, int) for i in ids))

    def test_reset(self):
        self.engine.move_player(Direction.EAST)
        self.engine.reset()
        # Just ensure no exception

    def test_destroy_twice_safe(self):
        self.engine.destroy()
        # Should not crash
        self.engine.destroy()

if __name__ == "__main__":
    unittest.main()
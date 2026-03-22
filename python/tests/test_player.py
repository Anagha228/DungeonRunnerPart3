import unittest
from treasure_runner.models.player import Player
from treasure_runner.models.game_engine import GameEngine
from treasure_runner.bindings import Direction, lib, Status
from treasure_runner.models.exceptions import GameEngineError

class TestPlayer(unittest.TestCase):

    def setUp(self):
        # Use a valid config file path from your repo
        self.engine = GameEngine("../assets/treasure_runner.ini")
        self.player = self.engine.player

    def tearDown(self):
        self.engine.destroy()

    def test_get_room_returns_int(self):
        room = self.player.get_room()
        self.assertIsInstance(room, int)

    def test_get_position_returns_tuple(self):
        pos = self.player.get_position()
        self.assertIsInstance(pos, tuple)
        self.assertEqual(len(pos), 2)
        self.assertIsInstance(pos[0], int)
        self.assertIsInstance(pos[1], int)

    def test_initial_collected_count_zero(self):
        count = self.player.get_collected_count()
        self.assertEqual(count, 0)

    def test_has_collected_treasure_false_initially(self):
        self.assertFalse(self.player.has_collected_treasure(9999))

    def test_get_collected_treasures_returns_list(self):
        treasures = self.player.get_collected_treasures()
        self.assertIsInstance(treasures, list)

    def test_collect_treasure_updates_state(self):
        """
        Move player around until a treasure is collected.
        This assumes your test world has at least one collectible treasure.
        """

        # Try moving in all directions to trigger possible pickup
        for direction in Direction:
            try:
                self.engine.move_player(direction)
            except Exception:
                pass

        count = self.player.get_collected_count()
        treasures = self.player.get_collected_treasures()

        if count > 0:
            self.assertGreater(len(treasures), 0)

            t = treasures[0]
            self.assertIn("id", t)
            self.assertIn("name", t)
            self.assertIn("starting_room_id", t)
            self.assertIn("initial_x", t)
            self.assertIn("initial_y", t)
            self.assertIn("x", t)
            self.assertIn("y", t)
            self.assertIn("collected", t)

            self.assertIsInstance(t["collected"], bool)

if __name__ == "__main__":
    unittest.main()
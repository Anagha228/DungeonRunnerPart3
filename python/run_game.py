import argparse
import curses 
import os
import sys
import json

sys.path.insert(0, os.path.dirname(__file__))

from treasure_runner.models.game_engine import GameEngine
from treasure_runner.ui.game_ui import GameUI

def create_profile(path: str) -> dict:
    if os.path.exists(path):
        with open(path, "r") as f:
            return json.load(f)
    name = input("No profile found. Enter your name: ").strip()
    while not name:
        name = input("Name cannot be empty. Enter your name: ").strip()
    return {
        "player_name": name,
        "games_played": 0,
        "max_treasure_collected": 0,
        "most_rooms_world_completed": 0,
        "timestamp_last_played": ""
    }
def save_profile(path, profile) -> None:
    with open(path, "w") as f:
        json.dump(profile, f, indent=2)
def main():
    parser = argparse.ArgumentParser(description = "Treasure runner")
    parser.add_argument("--config", required = True, help = "Path to .ini config file")
    parser.add_argument("--profile", required = True, help = "Path to player profile JSON" )
    args = parser.parse_args()

    profile = create_profile(args.profile)
    engine = GameEngine(args.config)
    ui = GameUI(engine, profile)

    curses.wrapper(ui.run)

    save_profile(args.profile, profile)
    engine.destroy()
if __name__ == "__main__":
    main()


    
#!/usr/bin/env python3
"""Deterministic system integration test runner for Treasure Runner."""

import os
import argparse
from treasure_runner.bindings import Direction
from treasure_runner.models.game_engine import GameEngine
from treasure_runner.models.exceptions import GameError, ImpassableError


def get_player_state(engine: GameEngine) -> str:
    player = engine.player
    room = player.get_room()
    x, y = player.get_position()
    collected = player.get_collected_count()
    return f"room={room}|x={x}|y={y}|collected={collected}"


def write_log(path, line):
    with open(path, "a") as f:
        f.write(line + "\n")


def determine_first_move(engine):
    directions = [Direction.SOUTH, Direction.WEST, Direction.NORTH, Direction.EAST]

    for d in directions:
        engine.reset()

        room = engine.player.get_room()
        pos = engine.player.get_position()

        try:
            engine.move_player(d)
        except ImpassableError:
            continue

        if room == engine.player.get_room() and pos != engine.player.get_position():
            return d
    # tries all directions and if they are blocked when we try to move
    # then moves to the next one 
    raise RuntimeError("No valid entry move")


def attempt_move(engine, direction):

    player = engine.player

    before_room = player.get_room()
    before_x, before_y = player.get_position()
    before_collected = player.get_collected_count()

    before_state = f"room={before_room}|x={before_x}|y={before_y}|collected={before_collected}"
    # recording state of player before move
    try:
        engine.move_player(direction)

        after_room = player.get_room()
        after_x, after_y = player.get_position()
        after_collected = player.get_collected_count()

        after_state = f"room={after_room}|x={after_x}|y={after_y}|collected={after_collected}"

        if (
            before_room == after_room and
            before_x == after_x and
            before_y == after_y and
            before_collected == after_collected
        ):
            result = "NO_PROGRESS"
        else:
            result = "OK"
        # we move and check if it did actually move and then return the appropriate values
    except ImpassableError:
        after_state = before_state
        after_collected = before_collected
        result = "BLOCKED"
    # raise appropriate exeptions
    except GameError:
        after_state = before_state
        after_collected = before_collected
        result = "BLOCKED"
    # track how many treasures collected
    delta = after_collected - before_collected

    return result, before_state, after_state, delta


def run_sweep(engine, log_path, phase_name, direction, step):
    seen = set()
    moves_in_sweep = 0
    #log each sweep
    write_log(log_path, f"SWEEP_START|phase={phase_name}|dir={direction.name}")

    while True:
        player = engine.player

        before_room = player.get_room()
        before_x, before_y = player.get_position()
        before_collected = player.get_collected_count()
        before_state = f"room={before_room}|x={before_x}|y={before_y}|collected={before_collected}"
        state_tuple = (before_room, before_x, before_y, before_collected)
        # do the same thing we did in attempt move check the before then move check
        # the after then check if they match if not, good! 
        try:
            engine.move_player(direction)

            after_room = player.get_room()
            after_x, after_y = player.get_position()
            after_collected = player.get_collected_count()
            after_state = f"room={after_room}|x={after_x}|y={after_y}|collected={after_collected}"
            delta = after_collected - before_collected

            step += 1

            # Check if NO state changed at all (position, room, AND collected)
            if (before_room == after_room and
                    before_x == after_x and
                    before_y == after_y and
                    before_collected == after_collected):
                write_log(
                    log_path,
                    f"MOVE|step={step}|phase={phase_name}|dir={direction.name}|result=NO_PROGRESS"
                    f"|before={before_state}|after={after_state}|delta_collected={delta}"
                )
                write_log(log_path, f"SWEEP_END|phase={phase_name}|reason=BLOCKED|moves={moves_in_sweep}")
                return step

            # Check for cycle AFTER confirming we actually moved
            if state_tuple in seen:
                write_log(log_path, f"SWEEP_END|phase={phase_name}|reason=CYCLE_DETECTED|moves={moves_in_sweep}")
                return step
            # If the runner encounters a state that already exists in seen, it concludes that
            # the system has entered a cycle. 
            # At that point the sweep terminates with the reason CYCLE_DETECTED. 
            seen.add(state_tuple)
            moves_in_sweep += 1

            # Successful move
            write_log(
                log_path,
                f"MOVE|step={step}|phase={phase_name}|dir={direction.name}|result=OK"
                f"|before={before_state}|after={after_state}|delta_collected={delta}"
            )

        except ImpassableError:
            step += 1
            write_log(
                log_path,
                f"MOVE|step={step}|phase={phase_name}|dir={direction.name}|result=BLOCKED"
                f"|before={before_state}|after={before_state}|delta_collected=0"
            )
            # Blocked move does NOT count toward moves_in_sweep
            write_log(log_path, f"SWEEP_END|phase={phase_name}|reason=BLOCKED|moves={moves_in_sweep}")
            return step

        except GameError:
            step += 1
            write_log(
                log_path,
                f"MOVE|step={step}|phase={phase_name}|dir={direction.name}|result=ERROR"
                f"|before={before_state}|after={before_state}|delta_collected=0"
            )
            write_log(log_path, f"SWEEP_END|phase={phase_name}|reason=ERROR|moves={moves_in_sweep}")
            return step


def parse_args():
    parser = argparse.ArgumentParser(description="Treasure Runner integration test logger")
    parser.add_argument(
        "--config",
        required=True,
        help="Path to generator config file",
    )
    parser.add_argument(
        "--log",
        required=True,
        help="Output log path",
    )
    return parser.parse_args()


def main():

    args = parse_args()

    config = os.path.abspath(args.config)
    log = os.path.abspath(args.log)
    #parsed now both contain the path to files 
    engine = GameEngine(config)
    engine.reset() #initalise everything and make sure nothing is collected or anything
    # now we have a fresh new game engine with everything reset 
    #and with the rooms and everthing in the config translated to a actual engine
    step = 0
    # logging start of run
    write_log(log, f"RUN_START|config={config}")
    #inital player state through helper function
    spawn_state = get_player_state(engine)
    write_log(log, f"STATE|step=0|phase=SPAWN|state={spawn_state}")

    try:
        #before doing anything we need to determine a valid entry direction 
        entry_dir = determine_first_move(engine)

        write_log(log, f"ENTRY|direction={entry_dir.name}")
        #once we found direction we rest and then using the attempt move we move in 
        #that direction
        engine.reset()

        result, before, after, delta = attempt_move(engine, entry_dir)

        step = 1
        #after entry move we log the move
        write_log(
            log,
            f"MOVE|step=1|phase=ENTRY|dir={entry_dir.name}|"
            f"result={result}|before={before}|after={after}|delta_collected={delta}",
        )

        if result == "ERROR":
            write_log(log, "TERMINATED: Initial Move Error")
            write_log(log, "RUN_END|steps=1|collected_total=0")
            return 1

    except RuntimeError:
        write_log(log, "TERMINATED: Initial Move Error")
        write_log(log, "RUN_END|steps=0|collected_total=0")
        return 1

    sweeps = [
        ("SWEEP_SOUTH", Direction.SOUTH),
        ("SWEEP_WEST", Direction.WEST),
        ("SWEEP_NORTH", Direction.NORTH),
        ("SWEEP_EAST", Direction.EAST),
    ]
    #call the sweep function and move in one direction until we either reach the end 
    #or termination conditions occurs
    for phase_name, direction in sweeps:
        step = run_sweep(engine, log, phase_name, direction, step)
    # then we log the final state, and the treasures collected and then end the main
    final_state = get_player_state(engine)
    write_log(log, f"STATE|step={step}|phase=FINAL|state={final_state}")

    collected = engine.player.get_collected_count()
    write_log(log, f"RUN_END|steps={step}|collected_total={collected}")

    return 0


if __name__ == "__main__":
    raise SystemExit(main())
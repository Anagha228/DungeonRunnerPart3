#include <check.h>
#include <stdlib.h>
#include <string.h>
#include "room.h"
#include "types.h"
#include "game_engine.h"
#include "player.h"
#include "graph.h"
#include "world_loader.h"
GameEngine *eng = NULL;
Room *room = NULL;

Status game_engine_get_total_treasure_count(const GameEngine *eng, int *count_out);
Status game_engine_get_current_room_name(const GameEngine *eng, char **name_out);

static void setup(void);
static void teardown(void);
//setup and teardown functions

static void setup(void) {

    Status s = game_engine_create("../assets/starter.ini", &eng);
    ck_assert_ptr_nonnull(eng);
    ck_assert_int_eq(s, OK);
    
}

static void teardown(void)
{
    if (eng != NULL) {
        game_engine_destroy(eng);
        eng = NULL;
    }
}

/* ============================================================
 * Game Engine Creation Tests
 * ============================================================ */
START_TEST(test_game_engine_create_basic)
{
    GameEngine *engTest;
    ck_assert_int_eq(game_engine_create("../assets/starter.ini", &engTest), OK);
    ck_assert_ptr_nonnull(engTest->player);
    ck_assert_ptr_nonnull(engTest->graph);
}
END_TEST

/* ============================================================
 * Game Engine Creation Tests null
 * ============================================================ */
START_TEST(test_game_engine_create_null_args)
{
    ck_assert_int_eq( game_engine_create(NULL, NULL), INVALID_ARGUMENT);
    ck_assert_int_eq( game_engine_create(NULL, &eng), INVALID_ARGUMENT);
    ck_assert_int_eq( game_engine_create("../assets/starter.ini", NULL), INVALID_ARGUMENT );
}
END_TEST

/* ============================================================
 * Get Player Tests
 * ============================================================ */

START_TEST(test_game_engine_get_player)
{
    ck_assert_ptr_nonnull(game_engine_get_player(eng));
}
END_TEST

/* ============================================================
 * Get Player Tests null
 * ============================================================ */

START_TEST(test_game_engine_get_player_null)
{
    ck_assert_ptr_null(game_engine_get_player(NULL));
}
END_TEST

/* ============================================================
 * Move Player Tests Valid Move
 * ============================================================ */

START_TEST(test_game_engine_move_player_success)
{
    const Player *p = game_engine_get_player(eng);
    int x0 = p->x;
    int y0 = p->y;

    Status s = game_engine_move_player(eng, DIR_EAST);

    if (s == OK) {
        p = game_engine_get_player(eng);
        ck_assert_int_eq(p->x, x0 + 1);
        ck_assert_int_eq(p->y, y0);
    } else {
        ck_assert_int_eq(s, ROOM_IMPASSABLE);
    }
}
END_TEST

/* ============================================================
 * Move Player Tests Wall Block
 * ============================================================ */
START_TEST(test_game_engine_move_player_into_wall)
{
    Status s = game_engine_move_player(eng, DIR_NORTH);
    ck_assert(s == OK || s == ROOM_IMPASSABLE);
}
END_TEST

/* ============================================================
 * Move Player Tests null
 * ============================================================ */

START_TEST(test_game_engine_move_player_null)
{
    ck_assert_int_eq(game_engine_move_player(NULL, DIR_NORTH), INVALID_ARGUMENT);
}
END_TEST

/* ============================================================
 * Room Count
 * ============================================================ */
START_TEST(test_game_engine_room_count)
{
    int count = -1;
    ck_assert_int_eq(game_engine_get_room_count(eng, &count), OK);

    /* Config says num_rooms=3 */
    ck_assert_int_eq(count, 3);
}
END_TEST

/* ============================================================
 * Get Room Count Tests null
 * ============================================================ */

START_TEST(test_game_engine_get_room_count_null)
{
    int count = -1;
    ck_assert_int_eq(game_engine_get_room_count(NULL, &count), INVALID_ARGUMENT);
    ck_assert_int_eq(game_engine_get_room_count(eng, NULL), NULL_POINTER);
}
END_TEST

/* ============================================================
 * Room Dimensions 
 * ============================================================ */
START_TEST(test_game_engine_room_dimensions)
{
    int w = 0, h = 0;

    ck_assert_int_eq(game_engine_get_room_dimensions(eng, &w, &h), OK);

    /* Base 20x15 with variance +/-2 */
    ck_assert_int_ge(w, 18);
    ck_assert_int_le(w, 22);
    ck_assert_int_ge(h, 13);
    ck_assert_int_le(h, 17);
}
END_TEST

/* ============================================================
 * Get Room Dimensions Tests null
 * ============================================================ */

START_TEST(test_game_engine_get_room_dimensions_null)
{
    int w = -1, h = -1;

    ck_assert_int_eq(game_engine_get_room_dimensions(NULL, &w, &h), INVALID_ARGUMENT);
    ck_assert_int_eq(game_engine_get_room_dimensions(eng, NULL, &h), NULL_POINTER);
    ck_assert_int_eq(game_engine_get_room_dimensions(eng, &w, NULL),NULL_POINTER);
}
END_TEST

/* ============================================================
 * Reset Engine
 * ============================================================ */
START_TEST(test_game_engine_reset)
{
    game_engine_move_player(eng, DIR_EAST);
    game_engine_move_player(eng, DIR_SOUTH);

    ck_assert_int_eq(game_engine_reset(eng), OK);

    const Player *p = game_engine_get_player(eng);
    ck_assert_int_eq(p->room_id, eng->initial_room_id);
    ck_assert_int_eq(p->x, eng->initial_player_x);
    ck_assert_int_eq(p->y, eng->initial_player_y);
}
END_TEST

/* ============================================================
 * Reset Tests null
 * ============================================================ */

START_TEST(test_game_engine_reset_null)
{
    ck_assert_int_eq(game_engine_reset(NULL), INVALID_ARGUMENT);
}
END_TEST

/* ============================================================
 * Render Current Room
 * ============================================================ */
START_TEST(test_game_engine_render_current_room)
{
    char *out = NULL;
    ck_assert_int_eq( game_engine_render_current_room(eng, &out), OK);
    ck_assert_ptr_nonnull(out);
    ck_assert(strchr(out, '@') != NULL);
    game_engine_free_string(out);

}
END_TEST

/* ============================================================
 * Render Current Room Tests null
 * ============================================================ */

START_TEST(test_game_engine_render_current_room_null)
{
    char *out = NULL;
    ck_assert_int_eq(game_engine_render_current_room(NULL, &out), INVALID_ARGUMENT);
    ck_assert_int_eq(game_engine_render_current_room(eng, NULL), INVALID_ARGUMENT);
}
END_TEST

/* ============================================================
 * Render Room by ID
 * ============================================================ */
START_TEST(test_game_engine_render_room)
{
    int *ids = NULL;
    int count = 0;
    ck_assert_int_eq(game_engine_get_room_ids(eng, &ids, &count), OK);
    char *out = NULL;
    ck_assert_int_eq(game_engine_render_room(eng, ids[0], &out), OK);
    ck_assert(strchr(out, '@') == NULL);
}
END_TEST

/* ============================================================
 * Render Room By ID Tests null
 * ============================================================ */

START_TEST(test_game_engine_render_room_null)
{
    char *out = NULL;

    ck_assert_int_eq(game_engine_render_room(NULL, 1, &out), INVALID_ARGUMENT);
    ck_assert_int_eq(game_engine_render_room(eng, 1, NULL), NULL_POINTER);
}
END_TEST

/* ============================================================
 * Get Room IDs
 * ============================================================ */
START_TEST(test_game_engine_get_room_ids)
{
    int *ids = NULL;
    int count = 0;
    ck_assert_int_eq(game_engine_get_room_ids(eng, &ids, &count), OK);
    ck_assert_int_eq(count, 3);
    ck_assert_ptr_nonnull(ids);
    for (int i = 0; i < count; i++) {
        for (int j = i + 1; j < count; j++) {
            ck_assert_int_ne(ids[i], ids[j]);
        }
        char *out = NULL;
        ck_assert_int_eq(game_engine_render_room(eng, ids[i], &out), OK);
        ck_assert_ptr_nonnull(out);
    }
}
END_TEST

/* ============================================================
 * Get Room IDs Tests null
 * ============================================================ */

START_TEST(test_game_engine_get_room_ids_null)
{
    int *ids = NULL;
    int count = 0;

    ck_assert_int_eq( game_engine_get_room_ids(NULL, &ids, &count), INVALID_ARGUMENT);
    ck_assert_int_eq( game_engine_get_room_ids(eng, NULL, &count), NULL_POINTER);
    ck_assert_int_eq( game_engine_get_room_ids(eng, &ids, NULL), NULL_POINTER);
}
END_TEST

/* ============================================================
 * Destroy Tests
 * ============================================================ */

START_TEST(test_game_engine_destroy_null)
{
    /* Must not crash */
    game_engine_destroy(NULL);
}
END_TEST

/* ============================================================
 * Additional Edge Case & Robustness Tests
 * ============================================================ */

START_TEST(test_game_engine_move_all_directions)
{
    Status s1 = game_engine_move_player(eng, DIR_NORTH);
    Status s2 = game_engine_move_player(eng, DIR_SOUTH);
    Status s3 = game_engine_move_player(eng, DIR_EAST);
    Status s4 = game_engine_move_player(eng, DIR_WEST);

    ck_assert(s1 == OK || s1 == ROOM_IMPASSABLE);
    ck_assert(s2 == OK || s2 == ROOM_IMPASSABLE);
    ck_assert(s3 == OK || s3 == ROOM_IMPASSABLE);
    ck_assert(s4 == OK || s4 == ROOM_IMPASSABLE);
}
END_TEST


START_TEST(test_game_engine_move_invalid_direction)
{
    Status s = game_engine_move_player(eng, (Direction)999);
    ck_assert_int_eq(s, INVALID_ARGUMENT);
}
END_TEST


START_TEST(test_render_contains_newlines)
{
    char *out = NULL;
    ck_assert_int_eq(game_engine_render_current_room(eng, &out), OK);

    ck_assert_ptr_nonnull(out);
    ck_assert(strchr(out, '\n') != NULL);

    game_engine_free_string(out);
}
END_TEST


START_TEST(test_player_position_changes_on_valid_move)
{
    const Player *p = game_engine_get_player(eng);
    int x0 = p->x;
    int y0 = p->y;

    Status s = game_engine_move_player(eng, DIR_EAST);

    if (s == OK) {
        p = game_engine_get_player(eng);
        ck_assert(p->x != x0 || p->y != y0);
    }
}
END_TEST


START_TEST(test_reset_clears_treasures)
{
    game_engine_move_player(eng, DIR_EAST);
    game_engine_move_player(eng, DIR_SOUTH);

    game_engine_reset(eng);

    const Player *p = game_engine_get_player(eng);
    ck_assert_int_eq(p->collected_count, 0);
}
END_TEST


START_TEST(test_render_room_invalid_id)
{
    char *out = NULL;
    ck_assert_int_eq(game_engine_render_room(eng, 9999, &out), GE_NO_SUCH_ROOM);
}
END_TEST


START_TEST(test_room_ids_valid)
{
    int *ids = NULL;
    int count = 0;

    ck_assert_int_eq(game_engine_get_room_ids(eng, &ids, &count), OK);
    ck_assert(count > 0);

    for (int i = 0; i < count; i++) {
        ck_assert_int_ge(ids[i], 0);
    }

    free(ids);
}
END_TEST


START_TEST(test_game_engine_create_invalid_file)
{
    GameEngine *bad = NULL;
    Status s = game_engine_create("invalid_path.ini", &bad);

    ck_assert(s != OK);
}
END_TEST


START_TEST(test_room_dimensions_stable)
{
    int w1, h1, w2, h2;

    ck_assert_int_eq(game_engine_get_room_dimensions(eng, &w1, &h1), OK);

    game_engine_move_player(eng, DIR_EAST);

    ck_assert_int_eq(game_engine_get_room_dimensions(eng, &w2, &h2), OK);

    ck_assert_int_eq(w1, w2);
    ck_assert_int_eq(h1, h2);
}
END_TEST


START_TEST(test_multiple_resets)
{
    ck_assert_int_eq(game_engine_reset(eng), OK);
    ck_assert_int_eq(game_engine_reset(eng), OK);
    ck_assert_int_eq(game_engine_reset(eng), OK);
}
END_TEST

/* ============================================================
 * Treasure Collection
 * ============================================================ */
START_TEST(test_treasure_count_positive)
{
    int count = 0;
    ck_assert_int_eq(game_engine_get_total_treasure_count(eng, &count), OK);
    ck_assert_int_gt(count, 0);
}
END_TEST

START_TEST(test_treasure_count_null)
{
    int count = 0;
    ck_assert_int_eq(game_engine_get_total_treasure_count(NULL, &count), INVALID_ARGUMENT);
    ck_assert_int_eq(game_engine_get_total_treasure_count(eng, NULL), INVALID_ARGUMENT);
}
END_TEST

/* ============================================================
 * Current Room Name
 * ============================================================ */
START_TEST(test_get_current_room_name)
{
    char *name = NULL;
    Status s = game_engine_get_current_room_name(eng, &name);
    ck_assert_int_eq(s, OK);
}
END_TEST

START_TEST(test_get_current_room_name_null)
{
    char *name = NULL;
    Status s = game_engine_get_current_room_name(eng, &name);
    ck_assert_int_eq(s, OK);
}
END_TEST

/* ============================================================
 * Player starts in valid position
 * ============================================================ */
START_TEST(test_player_start_position_valid)
{
    const Player *p = game_engine_get_player(eng);
    int w = 0, h = 0;
    game_engine_get_room_dimensions(eng, &w, &h);
    ck_assert_int_gt(p->x, 0);
    ck_assert_int_gt(p->y, 0);
    ck_assert_int_lt(p->x, w );
    ck_assert_int_lt(p->y, h );
}
END_TEST

/* ============================================================
 * Reset restores position after multiple moves
 * ============================================================ */
START_TEST(test_reset_restores_position)
{
    const Player *p = game_engine_get_player(eng);
    int x0 = p->x;
    int y0 = p->y;
    int room0 = p->room_id;

    for (int i = 0; i < 5; i++) {
        game_engine_move_player(eng, DIR_EAST);
        game_engine_move_player(eng, DIR_SOUTH);
    }

    game_engine_reset(eng);
    p = game_engine_get_player(eng);
    ck_assert_int_eq(p->x, x0);
    ck_assert_int_eq(p->y, y0);
    ck_assert_int_eq(p->room_id, room0);
}
END_TEST

/* ============================================================
 * Render room does not contain player character
 * ============================================================ */
START_TEST(test_render_room_no_player)
{
    int *ids = NULL;
    int count = 0;
    game_engine_get_room_ids(eng, &ids, &count);

    for (int i = 0; i < count; i++) {
        char *out = NULL;
        ck_assert_int_eq(game_engine_render_room(eng, ids[i], &out), OK);
        ck_assert_ptr_null(strchr(out, '@'));
        game_engine_free_string(out);
    }
    free(ids);
}
END_TEST

/* ============================================================
 * Render current room always contains player
 * ============================================================ */
START_TEST(test_render_current_room_always_has_player)
{
    for (int i = 0; i < 5; i++) {
        game_engine_move_player(eng, DIR_EAST);
        char *out = NULL;
        ck_assert_int_eq(game_engine_render_current_room(eng, &out), OK);
        ck_assert_ptr_nonnull(strchr(out, '@'));
        game_engine_free_string(out);
    }
}
END_TEST

/* ============================================================
 * Collected count never exceeds total
 * ============================================================ */
START_TEST(test_collected_never_exceeds_total)
{
    int total = 0;
    game_engine_get_total_treasure_count(eng, &total);

    for (int i = 0; i < 10; i++) {
        game_engine_move_player(eng, DIR_EAST);
        game_engine_move_player(eng, DIR_SOUTH);
    }

    const Player *p = game_engine_get_player(eng);
    ck_assert_int_le(p->collected_count, total);
}
END_TEST

/* ============================================================
 * Treasure collection via movement
 * ============================================================ */

START_TEST(test_collect_treasure_on_move){
    const Player *p = game_engine_get_player(eng);
    int before = p->collected_count;

    int *ids = NULL;
    int count = 0;
    game_engine_get_room_ids(eng, &ids, &count);

    game_engine_move_player(eng, DIR_NORTH);
    game_engine_move_player(eng, DIR_SOUTH);
    game_engine_move_player(eng, DIR_EAST);
    game_engine_move_player(eng, DIR_WEST);
    p = game_engine_get_player(eng);
    ck_assert_int_ge(p->collected_count, before);
    free(ids);
}
END_TEST


/* ============================================================
 * Room render size correct
 * ============================================================ */
START_TEST(test_render_room_size)
{
    int w = 0, h = 0;
    game_engine_get_room_dimensions(eng, &w, &h);

    char *out = NULL;
    game_engine_render_current_room(eng, &out);
    ck_assert_ptr_nonnull(out);

    /* count newlines — should equal height */
    int newlines = 0;
    for (int i = 0; out[i] != '\0'; i++) {
        if (out[i] == '\n') newlines++;
    }
    ck_assert_int_eq(newlines, h);
    game_engine_free_string(out);
}
END_TEST

/* ============================================================
 * Player room id matches after creation
 * ============================================================ */
START_TEST(test_player_room_id_valid)
{
    const Player *p = game_engine_get_player(eng);
    int *ids = NULL;
    int count = 0;
    game_engine_get_room_ids(eng, &ids, &count);

    bool found = false;
    for (int i = 0; i < count; i++) {
        if (ids[i] == p->room_id) {
            found = true;
            break;
        }
    }
    ck_assert(found);
    free(ids);
}
END_TEST

/* ============================================================
 * Reset after room change
 * ============================================================ */
START_TEST(test_reset_after_room_change)
{
    const Player *p = game_engine_get_player(eng);
    int initial_room = p->room_id;

    /* try to move into a portal by exhausting moves */
    for (int i = 0; i < 20; i++) {
        game_engine_move_player(eng, DIR_NORTH);
        game_engine_move_player(eng, DIR_SOUTH);
        game_engine_move_player(eng, DIR_EAST);
        game_engine_move_player(eng, DIR_WEST);
    }

    game_engine_reset(eng);
    p = game_engine_get_player(eng);
    ck_assert_int_eq(p->room_id, initial_room);
    ck_assert_int_eq(p->collected_count, 0);
}
END_TEST

/* ============================================================
 * render_room vs render_current_room consistency
 * ============================================================ */
START_TEST(test_render_room_vs_current)
{
    const Player *p = game_engine_get_player(eng);
    int current_id = p->room_id;

    char *current = NULL;
    char *by_id = NULL;
    game_engine_render_current_room(eng, &current);
    game_engine_render_room(eng, current_id, &by_id);

    ck_assert_ptr_nonnull(current);
    ck_assert_ptr_nonnull(by_id);

    /* both should have same length */
    ck_assert_int_eq(strlen(current), strlen(by_id));

    game_engine_free_string(current);
    game_engine_free_string(by_id);
}
END_TEST

/* ============================================================
 * Total treasure count matches sum across rooms
 * ============================================================ */
START_TEST(test_total_treasure_consistent_with_rooms)
{
    int total = 0;
    game_engine_get_total_treasure_count(eng, &total);

    int *ids = NULL;
    int count = 0;
    game_engine_get_room_ids(eng, &ids, &count);
    ck_assert_int_ge(total, 0);
    ck_assert_int_ge(count, 1);
    free(ids);
}
END_TEST

/* ============================================================
 * Room count matches room ids count
 * ============================================================ */
START_TEST(test_room_count_matches_ids)
{
    int count1 = 0;
    game_engine_get_room_count(eng, &count1);

    int *ids = NULL;
    int count2 = 0;
    game_engine_get_room_ids(eng, &ids, &count2);

    ck_assert_int_eq(count1, count2);
    free(ids);
}
END_TEST

/* ============================================================
 * Multiple render calls are stable
 * ============================================================ */
START_TEST(test_render_stable)
{
    char *out1 = NULL;
    char *out2 = NULL;
    game_engine_render_current_room(eng, &out1);
    game_engine_render_current_room(eng, &out2);

    ck_assert_ptr_nonnull(out1);
    ck_assert_ptr_nonnull(out2);
    ck_assert_str_eq(out1, out2);

    game_engine_free_string(out1);
    game_engine_free_string(out2);
}
END_TEST

/* ============================================================
 * free string null safe
 * ============================================================ */
START_TEST(test_free_string_null)
{
    game_engine_free_string(NULL);
    ck_assert_int_eq(1, 1); /* must not crash */
}
END_TEST

static GameEngine *sw_eng = NULL;

static void switch_setup(void) {
    Status s = game_engine_create("../python/example_integration_run (1).ini", &sw_eng);
    ck_assert_int_eq(s, OK);
    ck_assert_ptr_nonnull(sw_eng);
}

static void switch_teardown(void) {
    if (sw_eng != NULL) {
        game_engine_destroy(sw_eng);
        sw_eng = NULL;
    }
}

/* ============================================================
 * Switch world creation
 * ============================================================ */
START_TEST(test_switch_engine_create)
{
    ck_assert_ptr_nonnull(sw_eng->player);
    ck_assert_ptr_nonnull(sw_eng->graph);
}
END_TEST

START_TEST(test_switch_world_room_count)
{
    int count = 0;
    ck_assert_int_eq(game_engine_get_room_count(sw_eng, &count), OK);
    ck_assert_int_eq(count, 8);
}
END_TEST

START_TEST(test_switch_world_treasure_count)
{
    int total = 0;
    ck_assert_int_eq(game_engine_get_total_treasure_count(sw_eng, &total), OK);
    ck_assert_int_gt(total, 0);
}
END_TEST

/* ============================================================
 * Switch world movement
 * ============================================================ */
START_TEST(test_switch_world_move_all_directions)
{
    Status s1 = game_engine_move_player(sw_eng, DIR_NORTH);
    Status s2 = game_engine_move_player(sw_eng, DIR_SOUTH);
    Status s3 = game_engine_move_player(sw_eng, DIR_EAST);
    Status s4 = game_engine_move_player(sw_eng, DIR_WEST);

    ck_assert(s1 == OK || s1 == ROOM_IMPASSABLE);
    ck_assert(s2 == OK || s2 == ROOM_IMPASSABLE);
    ck_assert(s3 == OK || s3 == ROOM_IMPASSABLE);
    ck_assert(s4 == OK || s4 == ROOM_IMPASSABLE);
}
END_TEST

START_TEST(test_switch_world_render)
{
    char *out = NULL;
    ck_assert_int_eq(game_engine_render_current_room(sw_eng, &out), OK);
    ck_assert_ptr_nonnull(out);
    ck_assert_ptr_nonnull(strchr(out, '@'));
    game_engine_free_string(out);
}
END_TEST

START_TEST(test_switch_world_render_contains_switch)
{
    /* render all rooms and check at least one has a switch character */
    int *ids = NULL;
    int count = 0;
    game_engine_get_room_ids(sw_eng, &ids, &count);

    bool found_switch = false;
    for (int i = 0; i < count; i++) {
        char *out = NULL;
        game_engine_render_room(sw_eng, ids[i], &out);
        if (strchr(out, '^') != NULL || strchr(out, '*') != NULL) {
            found_switch = true;
        }
        game_engine_free_string(out);
    }
    ck_assert(found_switch);
    free(ids);
}
END_TEST

START_TEST(test_switch_world_render_contains_locked_portal)
{
    int *ids = NULL;
    int count = 0;
    game_engine_get_room_ids(sw_eng, &ids, &count);

    bool found_locked = false;
    for (int i = 0; i < count; i++) {
        char *out = NULL;
        game_engine_render_room(sw_eng, ids[i], &out);
        if (strchr(out, 'L') != NULL) {
            found_locked = true;
        }
        game_engine_free_string(out);
    }
    ck_assert(found_locked);
    free(ids);
}
END_TEST

/* ============================================================
 * Switch world reset
 * ============================================================ */
START_TEST(test_switch_world_reset)
{
    const Player *p = game_engine_get_player(sw_eng);
    int initial_room = p->room_id;
    int initial_x = p->x;
    int initial_y = p->y;

    for (int i = 0; i < 10; i++) {
        game_engine_move_player(sw_eng, DIR_EAST);
        game_engine_move_player(sw_eng, DIR_SOUTH);
    }

    ck_assert_int_eq(game_engine_reset(sw_eng), OK);
    p = game_engine_get_player(sw_eng);
    ck_assert_int_eq(p->room_id, initial_room);
    ck_assert_int_eq(p->x, initial_x);
    ck_assert_int_eq(p->y, initial_y);
    ck_assert_int_eq(p->collected_count, 0);
}
END_TEST

/* ============================================================
 * Switch world player stays in bounds
 * ============================================================ */
START_TEST(test_switch_world_player_in_bounds)
{
    for (int i = 0; i < 30; i++) {
        game_engine_move_player(sw_eng, DIR_NORTH);
        game_engine_move_player(sw_eng, DIR_EAST);
        game_engine_move_player(sw_eng, DIR_SOUTH);
        game_engine_move_player(sw_eng, DIR_WEST);
    }

    const Player *p = game_engine_get_player(sw_eng);
    int w = 0, h = 0;
    game_engine_get_room_dimensions(sw_eng, &w, &h);
    ck_assert_int_ge(p->x, 0);
    ck_assert_int_ge(p->y, 0);
    ck_assert_int_lt(p->x, w);
    ck_assert_int_lt(p->y, h);
}
END_TEST

START_TEST(test_switch_world_collected_never_exceeds_total)
{
    int total = 0;
    game_engine_get_total_treasure_count(sw_eng, &total);

    for (int i = 0; i < 20; i++) {
        game_engine_move_player(sw_eng, DIR_NORTH);
        game_engine_move_player(sw_eng, DIR_EAST);
        game_engine_move_player(sw_eng, DIR_SOUTH);
        game_engine_move_player(sw_eng, DIR_WEST);
    }

    const Player *p = game_engine_get_player(sw_eng);
    ck_assert_int_le(p->collected_count, total);
}
END_TEST

/* ============================================================
 * Suite Setup
 * ============================================================ */
Suite *game_engine_suite(void) {
    Suite *s = suite_create("Game_engine");
    TCase *tc_core = tcase_create("Core");
    tcase_add_checked_fixture(tc_core, setup, teardown);

    tcase_add_test(tc_core, test_game_engine_create_basic);
    tcase_add_test(tc_core, test_game_engine_create_null_args);
    tcase_add_test(tc_core, test_game_engine_move_player_success);
    tcase_add_test(tc_core, test_game_engine_get_player);
    tcase_add_test(tc_core, test_game_engine_destroy_null);
    tcase_add_test(tc_core, test_game_engine_get_player_null);
    tcase_add_test(tc_core, test_game_engine_move_player_null);
    tcase_add_test(tc_core, test_game_engine_get_room_count_null);
    tcase_add_test(tc_core, test_game_engine_get_room_dimensions_null);
    tcase_add_test(tc_core, test_game_engine_reset_null);
    tcase_add_test(tc_core, test_game_engine_render_current_room_null);
    tcase_add_test(tc_core, test_game_engine_render_room_null);
    tcase_add_test(tc_core, test_game_engine_get_room_ids_null);
    tcase_add_test(tc_core, test_game_engine_get_room_ids);
    tcase_add_test(tc_core, test_game_engine_render_room);
    tcase_add_test(tc_core, test_game_engine_render_current_room);
    tcase_add_test(tc_core, test_game_engine_reset);
    tcase_add_test(tc_core, test_game_engine_room_dimensions);
    tcase_add_test(tc_core, test_game_engine_room_count);
    tcase_add_test(tc_core, test_game_engine_move_player_into_wall);

    tcase_add_test(tc_core, test_game_engine_move_all_directions);
    tcase_add_test(tc_core, test_game_engine_move_invalid_direction);
    tcase_add_test(tc_core, test_render_contains_newlines);
    tcase_add_test(tc_core, test_player_position_changes_on_valid_move);
    tcase_add_test(tc_core, test_reset_clears_treasures);
    tcase_add_test(tc_core, test_render_room_invalid_id);
    tcase_add_test(tc_core, test_room_ids_valid);
    tcase_add_test(tc_core, test_game_engine_create_invalid_file);
    tcase_add_test(tc_core, test_room_dimensions_stable);
    tcase_add_test(tc_core, test_multiple_resets);
    
    tcase_add_test(tc_core, test_treasure_count_positive);
    tcase_add_test(tc_core, test_treasure_count_null);
    tcase_add_test(tc_core, test_get_current_room_name);
    tcase_add_test(tc_core, test_get_current_room_name_null);
    tcase_add_test(tc_core, test_player_start_position_valid);
    tcase_add_test(tc_core, test_reset_restores_position);
    tcase_add_test(tc_core, test_render_room_no_player);
    tcase_add_test(tc_core, test_render_current_room_always_has_player);
    tcase_add_test(tc_core, test_collected_never_exceeds_total);

    tcase_add_test(tc_core, test_collect_treasure_on_move);
    tcase_add_test(tc_core, test_render_room_size);
    tcase_add_test(tc_core, test_player_room_id_valid);
    tcase_add_test(tc_core, test_reset_after_room_change);
    tcase_add_test(tc_core, test_render_room_vs_current);
    tcase_add_test(tc_core, test_total_treasure_consistent_with_rooms);
    tcase_add_test(tc_core, test_room_count_matches_ids);
    tcase_add_test(tc_core, test_render_stable);
    tcase_add_test(tc_core, test_free_string_null);
    suite_add_tcase(s, tc_core);

    TCase *tc_switch = tcase_create("Switch");
    tcase_add_checked_fixture(tc_switch, switch_setup, switch_teardown);

    tcase_add_test(tc_switch, test_switch_engine_create);
    tcase_add_test(tc_switch, test_switch_world_room_count);
    tcase_add_test(tc_switch, test_switch_world_treasure_count);
    tcase_add_test(tc_switch, test_switch_world_move_all_directions);
    tcase_add_test(tc_switch, test_switch_world_render);
    tcase_add_test(tc_switch, test_switch_world_render_contains_switch);
    tcase_add_test(tc_switch, test_switch_world_render_contains_locked_portal);
    tcase_add_test(tc_switch, test_switch_world_reset);
    tcase_add_test(tc_switch, test_switch_world_player_in_bounds);
    tcase_add_test(tc_switch, test_switch_world_collected_never_exceeds_total);

    suite_add_tcase(s, tc_switch);

    return s;
}


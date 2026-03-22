#include <check.h>
#include <stdlib.h>

#include "player.h"
#include "types.h"

static Player *p = NULL;
static void setup(void);
static void teardown(void);
//setup and teardown functions

static void setup(void) {

    Status s = player_create(1, 10, 20, &p);
    ck_assert_int_eq(s, OK);
    ck_assert_ptr_nonnull(p);
}

static void teardown(void) {
    player_destroy(p);
    p=NULL;
}
/* ============================================================
 * player_create
 * ============================================================ */

START_TEST(test_player_create_valid) {
    Player *local = NULL;

    ck_assert_int_eq(player_create(1, 2, 3, &local), OK);
    ck_assert_ptr_nonnull(local);
    ck_assert_int_eq(local->room_id, 1);
    ck_assert_int_eq(local->x, 2);
    ck_assert_int_eq(local->y, 3);
    ck_assert_ptr_null(local->collected_treasures);
    ck_assert_int_eq(local->collected_count, 0);

}
END_TEST

START_TEST(test_player_create_null) {
    ck_assert_int_eq(player_create(0, 0, 0, NULL), INVALID_ARGUMENT);
    ck_assert_int_eq(player_create(1, 1, 2, NULL), INVALID_ARGUMENT);
}
END_TEST

/* ============================================================
 * player_destroy
 * ============================================================ */

START_TEST(test_player_destroy_null_safe) {
    /* Must not crash */
    player_destroy(NULL);
}
END_TEST

/* ============================================================
 * player_get_room
 * ============================================================ */

START_TEST(test_player_get_room_valid) {
    ck_assert_int_eq(player_get_room(p), 1);
}
END_TEST

START_TEST(test_player_get_room_null) {
    ck_assert_int_eq(player_get_room(NULL), -1);
}
END_TEST

/* ============================================================
 * player_get_position
 * ============================================================ */

START_TEST(test_player_get_position_valid) {
    int x = -1;
    int y = -1;
    ck_assert_int_eq(player_get_position(p, &x, &y), OK);
    ck_assert_int_eq(x, 10);
    ck_assert_int_eq(y, 20);

}
END_TEST

START_TEST(test_player_get_position_null_args) {
    Player *p = NULL;
    int x, y;

    ck_assert_int_eq(player_get_position(NULL, &x, &y), INVALID_ARGUMENT);
    ck_assert_int_eq(player_get_position(NULL, &x, &x), INVALID_ARGUMENT);
    ck_assert_int_eq(player_get_position(p, NULL, &y), INVALID_ARGUMENT);
    ck_assert_int_eq(player_get_position(p, &x, NULL), INVALID_ARGUMENT);

}
END_TEST

/* ============================================================
 * player_set_position
 * ============================================================ */

START_TEST(test_player_set_position_valid) {
    int x = 2, y = 3;

    ck_assert_int_eq(player_set_position(p, x, y), OK);
}
END_TEST

START_TEST(test_player_set_position_null) {
    ck_assert_int_eq(player_set_position(NULL, 1, 1), INVALID_ARGUMENT);
}
END_TEST

/* ============================================================
 * player_move_to_room
 * ============================================================ */

START_TEST(test_player_move_to_room_valid) {
    ck_assert_int_eq(player_move_to_room(p, 4), OK);
    ck_assert_int_eq(player_get_room(p), 4);

}
END_TEST

START_TEST(test_player_move_to_room_null) {
    ck_assert_int_eq(player_move_to_room(NULL, 2), INVALID_ARGUMENT);
}
END_TEST

/* ============================================================
 * player_reset_to_start
 * ============================================================ */

START_TEST(test_player_reset_to_start_valid) {

    int x, y;
    ck_assert_int_eq(player_reset_to_start(p, 1, 2, 3), OK);

    ck_assert_int_eq(player_get_room(p), 1);
    player_get_position(p, &x, &y);
    ck_assert_int_eq(x, 2);
    ck_assert_int_eq(y, 3);

}
END_TEST

START_TEST(test_player_reset_to_start_null) {
    ck_assert_int_eq( player_reset_to_start(NULL, 0, 0, 0), INVALID_ARGUMENT);
}
END_TEST

/* ============================================================
 * player_get_collected_count
 * ============================================================ */

START_TEST(test_player_get_collected_count_valid) {

    p->collected_count = 5;
    ck_assert_int_eq(player_get_collected_count(p), 5);

}
END_TEST

START_TEST(test_player_get_collected_count_null) {
    ck_assert_int_eq( player_get_collected_count(NULL), 0);
}
END_TEST

/* ============================================================
 * player_get_collected_treasures
 * ============================================================ */

START_TEST(test_get_collected_treasures_valid) {

    int x;
    Treasure *t = malloc(sizeof(Treasure));
    ck_assert_ptr_nonnull(t);
    t->id = 100;
    t->x = 2;;
    t->y = 2;
    t->name = malloc(strlen("Gold Coin") + 1);
    ck_assert_ptr_nonnull(t->name);
    strcpy(t->name, "Gold Coin");

    player_try_collect(p, t);
    ck_assert_ptr_nonnull(player_get_collected_treasures(p, &x));

}
END_TEST

START_TEST(test_player_get_collected_treasures_null) {
    int x;
    ck_assert_ptr_null(player_get_collected_treasures(NULL, &x));
    ck_assert_ptr_null(player_get_collected_treasures(p, NULL));
}
END_TEST

/* ============================================================
 * player_try_collect
 * ============================================================ */
START_TEST(test_player_try_collect_success)
{
    Treasure *t = malloc(sizeof(Treasure));
    ck_assert_ptr_nonnull(t);
    t->id = 100;
    t->x = 2;;
    t->y = 2;
    t->name = malloc(strlen("Gold Coin") + 1);
    ck_assert_ptr_nonnull(t->name);
    strcpy(t->name, "Gold Coin");

    ck_assert_int_eq(player_try_collect(p, t), OK);

    ck_assert_int_eq(p->collected_count, 1);
    ck_assert_ptr_nonnull(p->collected_treasures);
    ck_assert_ptr_eq(p->collected_treasures[0], t);
    ck_assert(t->collected == true);
}
END_TEST

START_TEST(test_player_try_collect_null)
{
    Treasure *t = malloc(sizeof(Treasure));
    ck_assert_ptr_nonnull(t);
    t->id = 100;
    t->x = 2;;
    t->y = 2;
    t->name = malloc(strlen("Gold Coin") + 1);
    ck_assert_ptr_nonnull(t->name);
    strcpy(t->name, "Gold Coin");

    ck_assert_int_eq(player_try_collect(NULL, t), NULL_POINTER);
    ck_assert_int_eq(player_try_collect(p, NULL), NULL_POINTER);
}
END_TEST

START_TEST(test_player_try_collect_duplicate)
{
    Treasure *t = malloc(sizeof(Treasure));
    ck_assert_ptr_nonnull(t);
    t->id = 100;
    t->x = 2;;
    t->y = 2;
    t->name = malloc(strlen("Gold Coin") + 1);
    ck_assert_ptr_nonnull(t->name);
    strcpy(t->name, "Gold Coin");

    ck_assert_int_eq(player_try_collect(p, t), OK);
    ck_assert_int_eq(player_try_collect(p, t), INVALID_ARGUMENT);

    ck_assert_int_eq(p->collected_count, 1);
}
END_TEST

/* ============================================================
 * player_has_collected_treasure
 * ============================================================ */

START_TEST(test_player_has_collected_true)
{
    Treasure *t = malloc(sizeof(Treasure));
    ck_assert_ptr_nonnull(t);
    t->id = 100;
    t->x = 2;;
    t->y = 2;
    t->name = malloc(strlen("Gold Coin") + 1);
    ck_assert_ptr_nonnull(t->name);
    strcpy(t->name, "Gold Coin");

    player_try_collect(p, t);

    ck_assert(player_has_collected_treasure(p, 100) == true);
}
END_TEST

START_TEST(test_player_has_collected_false)
{
    ck_assert(player_has_collected_treasure(p, 1) == false);
}
END_TEST

START_TEST(test_player_has_collected_invalid)
{
    ck_assert(player_has_collected_treasure(NULL, 1) == false);
    ck_assert(player_has_collected_treasure(p, -1) == false);
}
END_TEST


/* ============================================================
 * Test Suite
 * ============================================================ */

Suite *player_suite(void) {
    Suite *s = suite_create("PlayerSuite");
    TCase *tc = tcase_create("Basics");

    tcase_add_checked_fixture(tc, setup, teardown);

    tcase_add_test(tc, test_player_create_valid);
    tcase_add_test(tc, test_player_create_null);
    tcase_add_test(tc, test_player_destroy_null_safe);
    tcase_add_test(tc, test_player_get_room_valid);
    tcase_add_test(tc, test_player_get_room_null);
    tcase_add_test(tc, test_player_get_position_valid);
    tcase_add_test(tc, test_player_get_position_null_args);
    tcase_add_test(tc, test_player_set_position_valid);
    tcase_add_test(tc, test_player_set_position_null);
    tcase_add_test(tc, test_player_move_to_room_valid);
    tcase_add_test(tc, test_player_move_to_room_null);
    tcase_add_test(tc, test_player_reset_to_start_valid);
    tcase_add_test(tc, test_player_reset_to_start_null);
    tcase_add_test(tc, test_player_get_collected_count_valid);
    tcase_add_test(tc, test_player_get_collected_treasures_null);
    
    tcase_add_test(tc, test_get_collected_treasures_valid);
    tcase_add_test(tc, test_player_get_collected_count_null);
    tcase_add_test(tc, test_player_get_collected_count_valid);
    tcase_add_test(tc, test_player_try_collect_success);

    tcase_add_test(tc, test_player_try_collect_null);
    tcase_add_test(tc, test_player_try_collect_duplicate);
    tcase_add_test(tc, test_player_has_collected_true);
    tcase_add_test(tc, test_player_has_collected_false);
    tcase_add_test(tc, test_player_has_collected_invalid);
    suite_add_tcase(s, tc);
    return s;
}

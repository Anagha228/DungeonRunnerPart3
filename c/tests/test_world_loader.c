#include <check.h>
#include <stdlib.h>

#include "world_loader.h"
#include "graph.h"
#include "types.h"

static Graph *graph = NULL;
static Room  *first_room = NULL;
static int    num_rooms = 0;
static Charset charset;

/* ============================================================
 * Setup and Teardown
 * ============================================================ */

static void setup(void)
{
    Status s = loader_load_world("../assets/starter.ini", &graph, &first_room, &num_rooms, &charset);

    ck_assert_int_eq(s, OK);
    ck_assert_ptr_nonnull(graph);
    ck_assert_ptr_nonnull(first_room);
    ck_assert_int_gt(num_rooms, 0);
}

static void teardown(void)
{
    graph_destroy(graph);
    graph = NULL;
    first_room = NULL;
    num_rooms = 0;
}

/* ============================================================
 * loader_load_world – Basic Success Tests
 * ============================================================ */

START_TEST(test_loader_graph_created)
{
    ck_assert_ptr_nonnull(graph);
}
END_TEST


START_TEST(test_loader_first_room_set)
{
    ck_assert_ptr_nonnull(first_room);
}
END_TEST


START_TEST(test_loader_room_count_positive)
{
    ck_assert_int_gt(num_rooms, 0);
}
END_TEST


START_TEST(test_loader_charset_copied)
{
    ck_assert_int_ne(charset.wall, 0);
    ck_assert_int_ne(charset.floor, 0);
    ck_assert_int_ne(charset.portal, 0);
    ck_assert_int_ne(charset.treasure, 0);
    ck_assert_int_ne(charset.player, 0);
    ck_assert_int_ne(charset.pushable, 0);
}
END_TEST


/* ============================================================
 * loader_load_world – Error Handling (no fixture)
 * ============================================================ */

START_TEST(test_loader_null_arguments){
    ck_assert_int_eq(loader_load_world(NULL, &graph, &first_room, &num_rooms, &charset), INVALID_ARGUMENT);

    ck_assert_int_eq(loader_load_world("../assets/starter.ini", NULL, &first_room, &num_rooms, &charset), INVALID_ARGUMENT);

    ck_assert_int_eq(loader_load_world("../assets/starter.ini", &graph, NULL, &num_rooms, &charset), INVALID_ARGUMENT);

    ck_assert_int_eq(loader_load_world("../assets/starter.ini", &graph, &first_room, NULL, &charset), INVALID_ARGUMENT);

    ck_assert_int_eq(loader_load_world("../assets/starter.ini", &graph, &first_room, &num_rooms, NULL), INVALID_ARGUMENT);
}
END_TEST


START_TEST(test_loader_invalid_config)
{
    ck_assert_int_eq(loader_load_world("does_not_exist.ini", &graph, &first_room, &num_rooms, &charset), WL_ERR_CONFIG);
}
END_TEST

START_TEST(test_loader_multiple_calls)
{
    Status s1 = loader_load_world("../assets/starter.ini",
                                  &graph,
                                  &first_room,
                                  &num_rooms,
                                  &charset);

    ck_assert_int_eq(s1, OK);
    graph_destroy(graph);

    Status s2 = loader_load_world("../assets/starter.ini",
                                  &graph,
                                  &first_room,
                                  &num_rooms,
                                  &charset);

    ck_assert_int_eq(s2, OK);
    ck_assert_ptr_nonnull(graph);

    graph_destroy(graph);
    graph = NULL;
}

/* ============================================================
 * Test Suite
 * ============================================================ */

Suite *world_loader_suite(void)
{
    Suite *s = suite_create("WorldLoaderSuite");
    TCase *tc = tcase_create("Loader");
    tcase_add_checked_fixture(tc, setup, teardown);
    tcase_add_test(tc, test_loader_multiple_calls);
    tcase_add_test(tc, test_loader_graph_created);
    tcase_add_test(tc, test_loader_first_room_set);
    tcase_add_test(tc, test_loader_room_count_positive);
    tcase_add_test(tc, test_loader_charset_copied);
    tcase_add_test(tc, test_loader_null_arguments);
    tcase_add_test(tc, test_loader_invalid_config);

    suite_add_tcase(s, tc);
    return s;
}

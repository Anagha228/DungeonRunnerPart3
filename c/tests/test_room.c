#include <check.h>
#include <stdlib.h>
#include <string.h>
#include "room.h"
#include "types.h"
Room *r =NULL;
static void setup(void);
static void teardown(void);
//setup and teardown functions

static void setup(void) {

    r = room_create(1, "Test Room", 3, 3);
    ck_assert_ptr_nonnull(r);
    ck_assert_int_eq(r->id, 1);
    ck_assert_str_eq(r->name, "Test Room");
    ck_assert_int_eq(r->width, 3);
    ck_assert_int_eq(r->height, 3);
    
}

static void teardown(void) {
    room_destroy(r);
    r=NULL;
}

/* ============================================================
 * Room Creation Tests
 * ============================================================ */
START_TEST(test_room_create_basic)
{
    Room *local = room_create(1, "Test Room", 5, 5);
    ck_assert_ptr_nonnull(local);
    ck_assert_int_eq(local->id, 1);
    ck_assert_str_eq(local->name, "Test Room");
    ck_assert_int_eq(local->width, 5);
    ck_assert_int_eq(local->height, 5);
    ck_assert_ptr_null(local->floor_grid);
    ck_assert_ptr_null(local->portals);
    ck_assert_int_eq(local->portal_count, 0);
    ck_assert_ptr_null(local->treasures);
    ck_assert_int_eq(local->treasure_count, 0);
    ck_assert_ptr_null(local->pushables);
    ck_assert_int_eq(local->pushable_count, 0);
    room_destroy(local);
}
END_TEST

/* ============================================================
 * Room Creation Tests 0
 * ============================================================ */
START_TEST(test_room_create_0)
{
    Room *local = room_create(1, "Test Room", 0, 0);
    ck_assert_ptr_nonnull(local);
    ck_assert_int_eq(local->id, 1);
    ck_assert_str_eq(local->name, "Test Room");
    ck_assert_int_eq(local->width, 1);
    ck_assert_int_eq(local->height, 1);
    ck_assert_ptr_null(local->floor_grid);
    ck_assert_ptr_null(local->portals);
    ck_assert_int_eq(local->portal_count, 0);
    ck_assert_ptr_null(local->treasures);
    ck_assert_int_eq(local->treasure_count, 0);
    room_destroy(local);
}
END_TEST

/* ============================================================
 * Room Creation Tests negative
 * ============================================================ */
START_TEST(test_room_create_negative)
{
    Room *local = room_create(1, "Test Room",-19, -19);
    ck_assert_ptr_nonnull(local);
    ck_assert_int_eq(local->id, 1);
    ck_assert_str_eq(local->name, "Test Room");
    ck_assert_int_eq(local->width, 1);
    ck_assert_int_eq(local->height, 1);
    ck_assert_ptr_null(local->floor_grid);
    ck_assert_ptr_null(local->portals);
    ck_assert_int_eq(local->portal_count, 0);
    ck_assert_ptr_null(local->treasures);
    ck_assert_int_eq(local->treasure_count, 0);
    room_destroy(local);
}
END_TEST

/* ============================================================
 * Room Creation Tests empty name
 * ============================================================ */
START_TEST(test_room_create_empty_name)
{
    Room *local = room_create(3, "", 1, 1);
    ck_assert_ptr_nonnull(local);

    ck_assert_ptr_nonnull(local->name);
    ck_assert_str_eq(local->name, "");
    room_destroy(local);
}
END_TEST

/* ============================================================
 * Room Creation Tests null name
 * ============================================================ */
START_TEST(test_room_create_null_name)
{
    Room *local = room_create(3, NULL, 1, 1);
    ck_assert_ptr_nonnull(local);
    ck_assert_int_eq(local->id, 3);
    ck_assert_ptr_null(local->name);
    ck_assert_int_eq(local->width, 1);
    ck_assert_int_eq(local->height, 1);
    ck_assert_ptr_null(local->floor_grid);
    ck_assert_ptr_null(local->portals);
    ck_assert_int_eq(local->portal_count, 0);
    ck_assert_ptr_null(local->treasures);
    ck_assert_int_eq(local->treasure_count, 0);
    room_destroy(local);
    
}
END_TEST

/* ============================================================
 * Room get width Tests
 * ============================================================ */
START_TEST(test_room_get_width)
{

    ck_assert_int_eq(room_get_width(r), 3);   
}
END_TEST

/* ============================================================
 * Room get width Tests
 * ============================================================ */
START_TEST(test_room_get_width_null)
{

    ck_assert_int_eq(room_get_width(NULL), 0);   
}
END_TEST

/* ============================================================
 * Room get height Tests
 * ============================================================ */
START_TEST(test_room_get_height)
{

    ck_assert_int_eq(room_get_height(r), 3);   
}
END_TEST

/* ============================================================
 * Room get height Tests null
 * ============================================================ */
START_TEST(test_room_get_height_null)
{

    ck_assert_int_eq(room_get_height(NULL), 0);   
}
END_TEST


/* ============================================================
 * Floor Grid Tests
 * ============================================================ */
START_TEST(test_room_set_floor_grid)
{
    bool *grid = malloc(9 * sizeof(bool));
    for (int i = 0; i < 9; i++) {
        if(i%2==0){
            grid[i]=true;
        }else{
            grid[i]=false;
        }
    }
    ck_assert_int_eq(room_set_floor_grid(r, grid), OK);
    for (int i = 0; i < 9; i++) {
        ck_assert_int_eq(r->floor_grid[i], grid[i]);
    }
}
END_TEST

/* ============================================================
 * Floor Grid Tests null arguments
 * ============================================================ */
START_TEST(test_room_set_floor_grid_null)
{
    bool *grid = malloc(9 * sizeof(bool));
    for (int i = 0; i < 9; i++) {
        if(i%2==0){
            grid[i]=true;
        }else{
            grid[i]=false;
        }
    }
    ck_assert_int_eq(room_set_floor_grid(r, NULL), OK);
    ck_assert_ptr_eq(r->floor_grid, NULL);
    ck_assert_int_eq(room_set_floor_grid(NULL, NULL), INVALID_ARGUMENT);
    ck_assert_int_eq(room_set_floor_grid(NULL, grid), INVALID_ARGUMENT);
    free(grid);
}
END_TEST

/* ============================================================
 * Portal Tests 
 * ============================================================ */
START_TEST(test_room_set_portals_and_get)
{
    Portal *portals = malloc(1 * sizeof(Portal));
    ck_assert_ptr_nonnull(portals);
    portals[0].x = 1;
    portals[0].y = 1;
    portals[0].target_room_id = 200;
    portals[0].name = malloc(strlen("Portal1") + 1);
    ck_assert_ptr_nonnull(portals[0].name);
    strcpy(portals[0].name, "Portal1");
    ck_assert_int_eq(room_set_portals(r, portals, 1), OK);
    ck_assert_int_eq(room_get_portal_destination(r, 1, 1), 200);
    ck_assert_int_eq(room_get_portal_destination(r, 0, 0), -1);
}
END_TEST

/* ============================================================
 * Portal Tests invalid arguments
 * ============================================================ */
START_TEST(test_room_set_portals_invalid_args)
{
    Portal *portals = malloc(sizeof(Portal));

    ck_assert_int_eq(room_set_portals(NULL, portals, 1), INVALID_ARGUMENT);
    ck_assert_int_eq(room_set_portals(r, NULL, 1), INVALID_ARGUMENT);

    /* portal_count == 0 allows portals == NULL */
    ck_assert_int_eq(room_set_portals(r, NULL, 0), OK);

    free(portals);
}
END_TEST

/* ============================================================
 * Treasure Tests
 * ============================================================ */
START_TEST(test_room_set_treasure_and_get)
{
    Treasure *t = malloc(sizeof(Treasure));
    ck_assert_ptr_nonnull(t);
    t->id = 100;
    t->x = 2;;
    t->y = 2;
    t->name = malloc(strlen("Gold Coin") + 1);
    ck_assert_ptr_nonnull(t->name);
    strcpy(t->name, "Gold Coin");
    ck_assert_int_eq(room_set_treasures(r, t, 1), OK);
    ck_assert_int_eq(room_get_treasure_at(r, 2, 2), 100);
    ck_assert_int_eq(room_get_treasure_at(r, 0, 0), -1);
}
END_TEST

/* ============================================================
 * Portal Tests invalid arguments
 * ============================================================ */
START_TEST(test_room_set_Treasures_invalid_args)
{
    Treasure *treasures = malloc(sizeof(Treasure));

    ck_assert_int_eq(room_set_treasures(NULL, treasures, 1), INVALID_ARGUMENT);
    ck_assert_int_eq(room_set_treasures(r, NULL, 1), INVALID_ARGUMENT);

    /* treasure_count == 0 allows treasures == NULL */
    ck_assert_int_eq(room_set_treasures(r, NULL, 0), OK);
    free(treasures);

}
END_TEST

/* ============================================================
 * Treasure Placement Tests
 * ============================================================ */
START_TEST(test_room_place_treasure_and_get)
{
    Treasure *t = malloc(sizeof(Treasure));
    ck_assert_ptr_nonnull(t);

    t->id = 100;
    t->x = 2;
    t->y = 2;
    t->name = malloc(strlen("Gold") + 1);
    ck_assert_ptr_nonnull(t->name);
    strcpy(t->name, "Gold");

    ck_assert_int_eq(room_place_treasure(r, t), OK);

    ck_assert_int_eq(room_get_treasure_at(r, 2, 2), 100);
    ck_assert_int_eq(r->treasure_count, 1);
    ck_assert_str_eq(r->treasures[0].name, "Gold");
    ck_assert_ptr_ne(r->treasures[0].name, t->name);

    ck_assert_int_eq(room_get_treasure_at(r, 2, 2), 100);
    ck_assert_int_eq(room_get_treasure_at(r, 0, 0), -1);
    
}
END_TEST

/* ============================================================
 * Treasure Placement Tests null arguments
 * ============================================================ */
START_TEST(test_room_place_treasure_null_args)
{
    Treasure *t = malloc(sizeof(Treasure));
    ck_assert_ptr_nonnull(t);

    t->id = 100;
    t->x = 2;
    t->y = 2;
    t->name = malloc(strlen("X") + 1);
    ck_assert_ptr_nonnull(t->name);
    strcpy(t->name, "X");

    ck_assert_int_eq(room_place_treasure(NULL, t), INVALID_ARGUMENT);
    ck_assert_int_eq(room_place_treasure(r, NULL), INVALID_ARGUMENT);
    ck_assert_int_eq(room_get_treasure_at(NULL, 2, 2), -1);

}
END_TEST

/* ============================================================
 * Portal Tests portal destination retrieval
 * ============================================================ */
START_TEST(test_room_get_portal_destination)
{
    Portal *p = malloc(sizeof(Portal));
    p[0].x = 1;
    p[0].y = 2;
    p[0].target_room_id = 99;
    p[0].name = strdup("Exit");

    ck_assert_int_eq(room_set_portals(r, p, 1), OK);

    ck_assert_int_eq(room_get_portal_destination(r, 1, 2), 99);
    ck_assert_int_eq(room_get_portal_destination(r, 0, 0), -1);
    ck_assert_int_eq(room_get_portal_destination(NULL, 1, 2), -1);
}
END_TEST

/* ============================================================
 * Walkability Tests
 * ============================================================ */
START_TEST(test_room_is_walkable)
{
    bool *grid = malloc(9 * sizeof(bool));
    for (int i = 0; i < 9; i++) {
        if(i%2==0){
            grid[i]=true;
        }else{
            grid[i]=false;
        }
    }
    room_set_floor_grid(r, grid);
    ck_assert(room_is_walkable(r, 0, 0));
    ck_assert(!room_is_walkable(r, 0, 1)); // grid[4] == 0
    ck_assert(!room_is_walkable(r, -1, 0));
    ck_assert(!room_is_walkable(r, 3, 3));

}
END_TEST

/* ============================================================
 * Walkability Tests null room
 * ============================================================ */
START_TEST(test_room_is_walkable_null)
{
    ck_assert(!room_is_walkable(NULL, 0, 0));
}
END_TEST

/* ============================================================
 * Walkability Tests implicit walls
 * ============================================================ */
START_TEST(test_room_is_walkable_implicit_walls)
{
    ck_assert(!room_is_walkable(r, 0, 0));       
    ck_assert(room_is_walkable(r, 1, 1));        
}
END_TEST

/* ============================================================
 * Start Position Tests no portal
 * ============================================================ */
START_TEST(test_room_get_start_position)
{

    int x, y;
    ck_assert_int_eq(room_get_start_position(r, &x, &y), OK);
    ck_assert_int_ge(x, 1);
    ck_assert_int_ge(y, 1);
}
END_TEST

/* ============================================================
 * Start Position Tests with portal
 * ============================================================ */
START_TEST(test_room_get_start_position_portal)
{
    Portal *p = malloc(sizeof(Portal));
    p[0].x = 1;
    p[0].y = 0;
    p[0].target_room_id = 10;
    p[0].name = strdup("P");

    room_set_portals(r, p, 1);

    int x, y;
    ck_assert_int_eq(room_get_start_position(r, &x, &y), OK);
    ck_assert_int_eq(x, 1);
    ck_assert_int_eq(y, 0);
}
END_TEST

/* ============================================================
 * Start Position Tests null arguments
 * ============================================================ */
START_TEST(test_room_get_start_position_null)
{
    int x, y;
    ck_assert_int_eq(room_get_start_position(NULL, &x, &y), INVALID_ARGUMENT);
    ck_assert_int_eq(room_get_start_position(r, NULL, &y), INVALID_ARGUMENT);
    ck_assert_int_eq(room_get_start_position(r, &x, NULL), INVALID_ARGUMENT);
}
END_TEST

/* ============================================================
 * Tile Classification Tests treasure
 * ============================================================ */
START_TEST(test_room_classify_tile_treasure)
{
    Treasure *t = malloc(sizeof(Treasure));

    t->id = 7;
    t->x = 2;
    t->y = 2;
    t->name = strdup("Gold");
    int id = -1;
    room_set_treasures(r, t, 1);

    ck_assert_int_eq(room_classify_tile(r, 2, 2, &id), ROOM_TILE_TREASURE);
    ck_assert_int_eq(id, 7);
}
END_TEST

/* ============================================================
 * Tile Classification Tests Portal
 * ============================================================ */
START_TEST(test_room_classify_tile_Portal)
{
    Portal *p = malloc(sizeof(Portal));

    p->x = 2;
    p->y = 2;
    p->target_room_id = 7;
    p->name = strdup("Exit");
    room_set_portals(r, p, 1);

    int id = -1;
    ck_assert_int_eq(room_classify_tile(r, 2, 2, &id), ROOM_TILE_PORTAL);
    ck_assert_int_eq(id, 7);
}
END_TEST

/* ============================================================
 * Tile Classification Tests wall and floor
 * ============================================================ */
START_TEST(test_room_classify_tile_wall_and_floor)
{
    ck_assert_int_eq(room_classify_tile(r, 0, 0, NULL), ROOM_TILE_WALL);
    ck_assert_int_eq(room_classify_tile(r, 1, 1, NULL), ROOM_TILE_FLOOR);
}
END_TEST

/* ============================================================
 * Rendering Tests
 * ============================================================ */
START_TEST(test_room_render)
{
    Charset cs = {
        .floor = '.',
        .wall = '#',
        .treasure = '$',
        .portal = 'O'
    };
    char buffer[9];
    ck_assert_int_eq(room_render(r, &cs, buffer, 3, 3), OK);
    // Check corners are walls
    ck_assert(buffer[0] == '#');
    ck_assert(buffer[2] == '#');
    ck_assert(buffer[6] == '#');
    ck_assert(buffer[8] == '#');
    // Check center is floor
    ck_assert(buffer[4] == '.');
}
END_TEST

/* ============================================================
 * Rendering Tests custom grid
 * ============================================================ */
START_TEST(test_room_render_custom_grid)
{
    Charset cs = {
        .floor = '.',
        .wall = '#',
        .treasure = '$',
        .portal = 'O'
    };
    char buffer[9];
    bool *grid = malloc(9 * sizeof(bool));
    for (int i = 0; i < 9; i++) {
        if(i%2==0){
            grid[i]=true;
        }else{
            grid[i]=false;
        }
    }
    room_set_floor_grid(r, grid);

    Portal *p = malloc(sizeof(Portal));
    p->x = 2;
    p->y = 2;
    p->target_room_id = 7;
    p->name = strdup("Exit");
    room_set_portals(r, p, 1);

    Treasure *t = malloc(sizeof(Treasure));

    t->id = 7;
    t->x = 1;
    t->y = 0;
    t->name = strdup("Gold");
    room_set_treasures(r, t, 1);

    ck_assert_int_eq(room_render(r, &cs, buffer, 3, 3), OK);
    ck_assert_int_eq(buffer[0], '.');
    ck_assert_int_eq(buffer[1], '$'); // treasure
    ck_assert_int_eq(buffer[2], '.');

    ck_assert_int_eq(buffer[3], '#');
    ck_assert_int_eq(buffer[4], '.');
    ck_assert_int_eq(buffer[5], '#');

    ck_assert_int_eq(buffer[6], '.');
    ck_assert_int_eq(buffer[7], '#');
    ck_assert_int_eq(buffer[8], 'O'); // portal
}
END_TEST

/* ============================================================
 * Rendering Tests custom grid overlap
 * ============================================================ */
START_TEST(test_room_render_custom_grid_overlap)
{
    Charset cs = {
        .floor = '.',
        .wall = '#',
        .treasure = '$',
        .portal = 'O'
    };
    char buffer[9];
    bool *grid = malloc(9 * sizeof(bool));
    for (int i = 0; i < 9; i++) {
        if(i%2==0){
            grid[i]=true;
        }else{
            grid[i]=false;
        }
    }
    room_set_floor_grid(r, grid);

    Portal *p = malloc(sizeof(Portal));

    p->x = 2;
    p->y = 2;
    p->target_room_id = 7;
    p->name = strdup("Exit");
    room_set_portals(r, p, 1);

    Treasure *t = malloc(sizeof(Treasure));
    t->id = 7;
    t->x = 2;
    t->y = 2;
    t->name = strdup("Gold"); 
    room_set_treasures(r, t, 1);

    ck_assert_int_eq(room_render(r, &cs, buffer, 3, 3), OK);
    ck_assert_int_eq(buffer[0], '.'); 
    ck_assert_int_eq(buffer[1], '#'); 
    ck_assert_int_eq(buffer[2], '.'); 

    ck_assert_int_eq(buffer[3], '#'); 
    ck_assert_int_eq(buffer[4], '.'); 
    ck_assert_int_eq(buffer[5], '#'); 

    ck_assert_int_eq(buffer[6], '.'); 
    ck_assert_int_eq(buffer[7], '#'); 

    // The last tile [2,2] is where portal and treasure are
    // Portals render on top of treasures
    ck_assert_int_eq(buffer[8], 'O'); 
}
END_TEST

/* ============================================================
 * room_get_id
 * ============================================================ */
START_TEST(test_room_get_id_valid)
{
    Room r;
    r.id = 42;

    ck_assert_int_eq(room_get_id(&r), 42);
}
END_TEST

START_TEST(test_room_get_id_null)
{
    ck_assert_int_eq(room_get_id(NULL), -1);
}
END_TEST

/* ============================================================
 * room_pick_up_treasure
 * ============================================================ */
START_TEST(test_room_pick_up_success)
{
    Room r;
    Treasure t;
    Treasure *out = NULL;

    t.id = 5;
    t.collected = false;

    r.treasure_count = 1;
    r.treasures = &t;

    Status s = room_pick_up_treasure(&r, 5, &out);

    ck_assert_int_eq(s, OK);
    ck_assert_ptr_eq(out, &t);
    ck_assert_int_eq(t.collected, true);
}
END_TEST

START_TEST(test_room_pick_up_not_found)
{
    Room r;
    Treasure t;
    Treasure *out = NULL;

    t.id = 1;
    t.collected = false;

    r.treasure_count = 1;
    r.treasures = &t;

    Status s = room_pick_up_treasure(&r, 99, &out);

    ck_assert_int_eq(s, ROOM_NOT_FOUND);
}
END_TEST

START_TEST(test_room_pick_up_already_collected)
{
    Room r;
    Treasure t;
    Treasure *out = NULL;

    t.id = 3;
    t.collected = true;

    r.treasure_count = 1;
    r.treasures = &t;

    Status s = room_pick_up_treasure(&r, 3, &out);

    ck_assert_int_eq(s, INVALID_ARGUMENT);
}
END_TEST

START_TEST(test_room_pick_up_invalid_args)
{
    Treasure *out = NULL;

    ck_assert_int_eq(room_pick_up_treasure(NULL, 1, &out), INVALID_ARGUMENT);
    ck_assert_int_eq(room_pick_up_treasure(NULL, -1, NULL), INVALID_ARGUMENT);
}
END_TEST

/* ============================================================
 * destroy_treasure
 * ============================================================ */
START_TEST(test_destroy_treasure_null)
{
    destroy_treasure(NULL);
    ck_assert_int_eq(1, 1); /* should not crash */
}
END_TEST

START_TEST(test_destroy_treasure_valid)
{
    Treasure *t = malloc(sizeof(Treasure));
    t->name = malloc(10);
    strcpy(t->name, "Gold");

    destroy_treasure(t);

    ck_assert_int_eq(1, 1); /* if no crash, success */
}
END_TEST

/* ============================================================
 * room_has_pushable_at
 * ============================================================ */
START_TEST(test_room_has_pushable_true)
{
    Room r;
    Pushable p;

    p.x = 2;
    p.y = 3;

    r.pushable_count = 1;
    r.pushables = &p;

    int idx = -1;

    bool result = room_has_pushable_at(&r, 2, 3, &idx);

    ck_assert_int_eq(result, true);
    ck_assert_int_eq(idx, 0);
}
END_TEST

START_TEST(test_room_has_pushable_false)
{
    Room r;
    Pushable p;

    p.x = 1;
    p.y = 1;

    r.pushable_count = 1;
    r.pushables = &p;

    bool result = room_has_pushable_at(&r, 9, 9, NULL);

    ck_assert_int_eq(result, false);
}
END_TEST

START_TEST(test_room_has_pushable_null_room)
{
    ck_assert_int_eq(room_has_pushable_at(NULL, 0, 0, NULL), false);
}
END_TEST

/* ============================================================
 * room_try_push
 * ============================================================ */
START_TEST(test_room_try_push_success)
{
    int size = r->width * r->height;
    bool *grid = malloc(size * sizeof(bool));
    for (int i = 0; i < size; i++){
        grid[i] = true;
    }
    room_set_floor_grid(r, grid);

    Pushable *pushables = malloc(sizeof(Pushable));
    pushables[0].id = 0;
    pushables[0].x = 1;
    pushables[0].y = 1;

    r->pushable_count = 1;
    r->pushables = pushables;

    Status s = room_try_push(r, 0, DIR_WEST);

    ck_assert_int_eq(s, OK);
    ck_assert_int_eq(r->pushables[0].x, 0);
    ck_assert_int_eq(r->pushables[0].y, 1);
}
END_TEST

START_TEST(test_room_try_push_invalid_args)
{
    Room r;
    r.pushable_count = 0;

    ck_assert_int_eq(room_try_push(NULL, 0, DIR_NORTH), INVALID_ARGUMENT);
    ck_assert_int_eq(room_try_push(&r, -1, DIR_NORTH), INVALID_ARGUMENT);
}
END_TEST

START_TEST(test_room_try_push_blocked)
{
    Room r;
    Pushable p;

    p.id = 0;
    p.x = 0;
    p.y = 0;

    r.pushable_count = 1;
    r.pushables = &p;

    /* assume pushing west is not walkable */
    Status s = room_try_push(&r, 0, DIR_WEST);

    ck_assert_int_eq(s, ROOM_IMPASSABLE);
}
END_TEST

/* ============================================================
 * Suite Setup
 * ============================================================ */
Suite *room_suite(void) {
    Suite *s = suite_create("Room");
    TCase *tc_core = tcase_create("Core");
    tcase_add_checked_fixture(tc_core, setup, teardown);

    tcase_add_test(tc_core, test_room_create_basic);
    tcase_add_test(tc_core, test_room_create_0);
    tcase_add_test(tc_core, test_room_create_negative);
    tcase_add_test(tc_core, test_room_create_empty_name);
    tcase_add_test(tc_core, test_room_create_null_name);
    tcase_add_test(tc_core, test_room_get_width);
    tcase_add_test(tc_core, test_room_get_width_null);
    tcase_add_test(tc_core, test_room_get_height);
    tcase_add_test(tc_core, test_room_get_height_null);
    tcase_add_test(tc_core, test_room_set_floor_grid);
    tcase_add_test(tc_core, test_room_set_floor_grid_null);
    tcase_add_test(tc_core, test_room_set_portals_and_get);
    tcase_add_test(tc_core, test_room_set_portals_invalid_args);
    tcase_add_test(tc_core, test_room_set_treasure_and_get);
    tcase_add_test(tc_core, test_room_set_Treasures_invalid_args);
    tcase_add_test(tc_core, test_room_place_treasure_null_args);
    tcase_add_test(tc_core, test_room_place_treasure_and_get);
    tcase_add_test(tc_core, test_room_get_portal_destination);
    tcase_add_test(tc_core, test_room_is_walkable);
    tcase_add_test(tc_core, test_room_is_walkable_null);
    tcase_add_test(tc_core, test_room_is_walkable_implicit_walls);
    tcase_add_test(tc_core, test_room_get_start_position);
    tcase_add_test(tc_core, test_room_get_start_position_portal);
    tcase_add_test(tc_core, test_room_get_start_position_null);
    tcase_add_test(tc_core, test_room_classify_tile_treasure);
    tcase_add_test(tc_core, test_room_classify_tile_Portal);
    tcase_add_test(tc_core, test_room_classify_tile_wall_and_floor);
    tcase_add_test(tc_core, test_room_render);
    tcase_add_test(tc_core, test_room_render_custom_grid);
    tcase_add_test(tc_core, test_room_render_custom_grid_overlap);
    tcase_add_test(tc_core, test_room_try_push_blocked);
    tcase_add_test(tc_core, test_room_try_push_invalid_args);
    tcase_add_test(tc_core, test_room_try_push_success);
    tcase_add_test(tc_core, test_room_has_pushable_null_room);
    tcase_add_test(tc_core, test_room_has_pushable_false);
    tcase_add_test(tc_core, test_room_has_pushable_true);
    tcase_add_test(tc_core, test_destroy_treasure_valid);
    tcase_add_test(tc_core, test_destroy_treasure_null);
    tcase_add_test(tc_core, test_room_pick_up_invalid_args);
    tcase_add_test(tc_core, test_room_pick_up_already_collected);
    tcase_add_test(tc_core, test_room_pick_up_not_found);
    tcase_add_test(tc_core, test_room_pick_up_success);
    tcase_add_test(tc_core, test_room_get_id_null);
    tcase_add_test(tc_core, test_room_get_id_valid);
    suite_add_tcase(s, tc_core);
    return s;
}


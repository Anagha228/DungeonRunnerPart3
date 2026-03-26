#include "types.h"   /* Direction, Status */
#include "game_engine.h"
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include "player.h"
#include "graph.h"
#include "room.h"
#include "world_loader.h"

typedef struct Graph Graph;
typedef struct Player Player;

static bool try_collect_at(Room *room, Player *p, int x, int y);
static Room *find_portal_target(GameEngine *eng, Room *room, int x, int y);
Status game_engine_get_current_room_name(const GameEngine *eng, char **name_out);
static bool is_switch_active(Room *room, int switch_id);
static Status get_entry_position(const Room *r, int *x_out, int *y_out);
Status game_engine_get_charset(GameEngine *eng, Charset *out);
static Status handle_push(Room *room, Player *p, int nx, int ny, Direction dir);
static Status handle_portal(GameEngine *eng, Player *p, Room *current_room, int nx, int ny);
Status game_engine_get_total_treasure_count(const GameEngine *eng, int *count_out);

/* ============================================================
 * Creation & Destruction
 * ============================================================ */

Status game_engine_create(const char *config_file_path, GameEngine **engine_out){
    if(config_file_path == NULL || engine_out == NULL){
        return INVALID_ARGUMENT;
    }
    GameEngine *engine = calloc (1, sizeof(GameEngine));
    if(engine == NULL){
        return NO_MEMORY;
    }
    Room *first_room = NULL;
    Status s = loader_load_world(config_file_path, &engine->graph, &first_room, &engine->room_count, &engine->charset);
    if (s != OK || first_room == NULL){
        game_engine_destroy(engine);
        return WL_ERR_DATAGEN;
    }

    engine->player = calloc(1, sizeof(Player));
    if(engine->player == NULL){
        game_engine_destroy(engine);
        return NO_MEMORY;
    }

    engine->player->room_id = first_room->id;
    engine->initial_room_id = first_room->id;
    if(room_get_start_position(first_room, &engine->initial_player_x, &engine->initial_player_y)!=OK){
        return NO_MEMORY;
    }
    engine->player->x = engine->initial_player_x;
    engine->player->y = engine->initial_player_y;

    *engine_out = engine;
    return OK;
}

void game_engine_destroy(GameEngine *eng){
    if (eng == NULL) return;
    if (eng->player) {
        free(eng->player->collected_treasures);
        eng->player->collected_treasures = NULL;
        free(eng->player);
        eng->player = NULL;
    }
    if (eng->graph) {
        graph_destroy(eng->graph);
        eng->graph = NULL;
    }
    free(eng);
}

/* ============================================================
 * Player Access
 * ============================================================ */

const Player *game_engine_get_player(const GameEngine *eng){
    if (eng == NULL || eng->player == NULL){
        return NULL;
    }
    return eng->player;
}

/* ============================================================
 * Switch helper
 * ============================================================ */

/*
 * Check if a switch is active.
 * A switch is active if:
 *   - A pushable is on it, OR
 *   - The player is standing on it (consumed case)
 * Searches by switch ID, not array index.
 */
static bool is_switch_active(Room *room, int switch_id) {
    if (room == NULL) return false;

    /* find the switch by ID */
    for (int i = 0; i < room->switch_count; i++) {
        if (room->switches[i].id == switch_id) {
            int sx = room->switches[i].x;
            int sy = room->switches[i].y;

            /* check if pushable is on the switch */
            for (int j = 0; j < room->pushable_count; j++) {
                if (room->pushables[j].x == sx && room->pushables[j].y == sy) {
                    return true;
                }
            }

            return false;
        }
    }
    return false;
}

/* ============================================================
 * Player Movement & Interaction
 * ============================================================ */

Status game_engine_move_player(GameEngine *eng, Direction dir){
    if (eng == NULL) {
        return INVALID_ARGUMENT;
    }

    Player *p = eng->player;
    if (p == NULL) {
        return INTERNAL_ERROR;
    }

    int current_room_id = player_get_room(p);
    Room key = {0};
    key.id = current_room_id;

    Room *current_room = (Room *)graph_get_payload(eng->graph, &key);
    if (current_room == NULL) {
        return GE_NO_SUCH_ROOM;
    }

    int nx = 0;
    int ny = 0;
    Status s = player_get_position(p, &nx, &ny);
    if (s != OK) {
        return INTERNAL_ERROR;
    }

    /* Compute next position */
    switch (dir) {
        case DIR_NORTH: ny--; break;
        case DIR_SOUTH: ny++; break;
        case DIR_WEST:  nx--; break;
        case DIR_EAST:  nx++; break;
        default: return INVALID_ARGUMENT;
    }
    /* Step 1: collect treasure at destination */
    if (try_collect_at(current_room, p, nx, ny)) {
        return OK;
    }
    /* Step 2: try to push a pushable */
    s = handle_push(current_room, p, nx, ny, dir);
    if (s == OK){
        return OK;
    }
    if (s == ROOM_IMPASSABLE) {
        return ROOM_IMPASSABLE;
    }
    /* Step 3: check if destination is a locked portal BEFORE walkability check
     * Portal tiles on walls are not walkable, so we must check portals first */
    s = handle_portal(eng, p, current_room, nx, ny);
    if (s == OK){
        return OK;
    }
    /* Step 4: check walkability for non-portal tiles */
    if (!room_is_walkable(current_room, nx, ny)) {
        return ROOM_IMPASSABLE;
    }
    /* Step 5: normal move */
    player_set_position(p, nx, ny);
    return OK;
}

static bool try_collect_at(Room *room, Player *p, int x, int y) {
    for (int i = 0; i < room->treasure_count; i++) {
        Treasure *t = &room->treasures[i];
        if (t->x == x && t->y == y && !t->collected) {
            player_try_collect(p, t);
            return true;
        }
    }
    return false;
}

static Room *find_portal_target(GameEngine *eng, Room *room, int x, int y) {
    for (int i = 0; i < room->portal_count; i++) {
        Portal *portal = &room->portals[i];
        if (portal->x == x && portal->y == y) {
            if (portal->gated && room->switch_count > 0) {
                int sid = portal->required_switch_id;
                if (!is_switch_active(room, sid)) {
                    return NULL;
                }
            }
            Room key = {0};
            key.id = portal->target_room_id;
            return (Room *)graph_get_payload(eng->graph, &key);
        }
    }
    return NULL;
}

static Status get_entry_position(const Room *r, int *x_out, int *y_out) {
    if (room_get_start_position(r, x_out, y_out) != OK) {
        return ROOM_NOT_FOUND;
    }
    /* check 4 neighbours of the portal position for a walkable interior tile */
    int dx[] = {0, 0, -1, 1};
    int dy[] = {-1, 1, 0, 0};
    for (int d = 0; d < 4; d++) {
        int cx = *x_out + dx[d];
        int cy = *y_out + dy[d];
        if (cx > 0 && cy > 0 && cx < r->width - 1 && cy < r->height - 1
            && room_is_walkable(r, cx, cy)
            && room_get_portal_destination(r, cx, cy) == -1) {
            *x_out = cx;
            *y_out = cy;
            return OK;
        }
    }
    /* no valid neighbour found, fall back to any interior walkable tile */
    for (int row = 1; row < r->height - 1; row++) {
        for (int col = 1; col < r->width - 1; col++) {
            if (room_is_walkable(r, col, row)
                && room_get_portal_destination(r, col, row) == -1) {
                *x_out = col;
                *y_out = row;
                return OK;
            }
        }
    }

    return ROOM_NOT_FOUND;
}

static Status handle_push(Room *current_room, Player *p, int nx, int ny, Direction dir) {
    int pushable_idx = -1;
    if (!room_has_pushable_at(current_room, nx, ny, &pushable_idx)) {
        return GE_NO_SUCH_ROOM;
    }
    /* Check if pushable is on a switch */
    bool on_switch = false;
    for (int i = 0; i < current_room->switch_count; i++) {
        if (current_room->switches[i].x == nx &&
            current_room->switches[i].y == ny) {
            on_switch = true;
            break;
        }
    }
    if (on_switch) {
        // Treat like empty tile
        player_set_position(p, nx, ny);
        return OK;
    }
    if (room_try_push(current_room, pushable_idx, dir) != OK) {
        return ROOM_IMPASSABLE;
    }
    player_set_position(p, nx, ny);
    return OK;
}

static Status handle_portal(GameEngine *eng, Player *p, Room *current_room, int nx, int ny) {
    Room *target_room = find_portal_target(eng, current_room, nx, ny);
    if (target_room != NULL) {
        Status s = player_move_to_room(p, target_room->id);
        if (s != OK) return INTERNAL_ERROR;
        int entry_x = 1;
        int entry_y = 1;
        get_entry_position(target_room, &entry_x, &entry_y);
        if (room_get_portal_destination(target_room, entry_x, entry_y) != -1) {
            for (int row = 1; row < target_room->height - 1; row++) {
                for (int col = 1; col < target_room->width - 1; col++) {
                    if (room_is_walkable(target_room, col, row) &&
                        room_get_portal_destination(target_room, col, row) == -1) {
                        entry_x = col;
                        entry_y = row;
                        goto found;
                    }
                }
            }
            found:;
        }
        player_set_position(p, entry_x, entry_y);
        return OK;
    }
    return ROOM_IMPASSABLE; 
}

Status game_engine_get_room_count(const GameEngine *eng, int *count_out){
    if (eng == NULL ){
        return INVALID_ARGUMENT;    
    }
    if(count_out == NULL ){
        return NULL_POINTER;
    }

    const void * const *payloads_out = NULL;
    if(graph_get_all_payloads(eng->graph, &payloads_out, count_out) != GRAPH_STATUS_OK){
        return INVALID_ARGUMENT;
    }
    return OK;
}

Status game_engine_get_room_dimensions(const GameEngine *eng, int *width_out, int *height_out){
    if (eng == NULL ){
        return INVALID_ARGUMENT;    
    }
    if(width_out == NULL || height_out == NULL){
        return NULL_POINTER;
    }
    if(eng->player == NULL){
        return INTERNAL_ERROR;
    }
    const Room *room = graph_get_payload(eng->graph, &eng->player->room_id);
    if (room == NULL){
        return GE_NO_SUCH_ROOM;
    }
    *width_out = room->width;
    *height_out = room->height;
    return OK;
}

/* ============================================================
 * Resetting / Restarting
 * ============================================================ */

Status game_engine_reset(GameEngine *eng){
    if (eng == NULL) return INVALID_ARGUMENT;
    if (eng->player == NULL) return INTERNAL_ERROR;

    eng->player->room_id = eng->initial_room_id;
    eng->player->x = eng->initial_player_x;
    eng->player->y = eng->initial_player_y;
    free(eng->player->collected_treasures);
    eng->player->collected_treasures = NULL;
    eng->player->collected_count = 0;

    const void * const *all_rooms = NULL;
    int total_rooms = 0;
    graph_get_all_payloads(eng->graph, &all_rooms, &total_rooms);

    for (int i = 0; i < total_rooms; i++) {
        Room *r = (Room *)all_rooms[i];
        for (int j = 0; j < r->treasure_count; j++) {
            r->treasures[j].collected = false;
        }
        for (int j = 0; j < r->pushable_count; j++) {
            r->pushables[j].x = r->pushables[j].initial_x;
            r->pushables[j].y = r->pushables[j].initial_y;
        }
    }

    return OK;
}

/* ============================================================
 * Room Rendering
 * ============================================================ */

Status game_engine_render_current_room(const GameEngine *eng, char **str_out){
    if (eng == NULL ){
        return INVALID_ARGUMENT;    
    }
    if (eng->player == NULL || eng->graph ==NULL) {
        return INTERNAL_ERROR;
    }
    if(str_out == NULL ){
        return INVALID_ARGUMENT;
    }
    const Room *room = graph_get_payload(eng->graph, &eng->player->room_id);
    if (room == NULL){
        return INTERNAL_ERROR;
    }

    int buffer_size = room->width * room->height;
    char *buffer = malloc(buffer_size * sizeof(char)); 
    if (buffer == NULL){
        return NO_MEMORY;
    }

    Status s = room_render(room, &eng->charset, buffer, room->width, room->height);
    if (s != OK){
        free(buffer);
        return INTERNAL_ERROR;
    }
    int out_size = room->width * room->height + room->height + 1;
    char *str_buffer = malloc(out_size * sizeof(char));
    if (str_buffer == NULL){
        free(buffer);
        return NO_MEMORY;
    }
    int linefeed = 0; 
    for(int i = 0; i < room->height; i++){
        for(int j = 0; j < room->width; j++){
            str_buffer[linefeed] = buffer[i*room->width+j];
            if(j == eng->player->x && i == eng->player->y){
                str_buffer[linefeed] = eng->charset.player;
            }
            linefeed++;
        }
        str_buffer[linefeed++] = '\n';
    }
    str_buffer[linefeed] = '\0';

    free(buffer);
    *str_out = str_buffer;
    return OK;
}

Status game_engine_render_room(const GameEngine *eng, int room_id, char **str_out){
    if (eng == NULL ){
        return INVALID_ARGUMENT;    
    }
    if (eng->graph == NULL) {
        return INTERNAL_ERROR;
    }
    if(str_out == NULL ){
        return NULL_POINTER;
    }
    const Room *room = graph_get_payload(eng->graph, &room_id);
    if (room == NULL){
        return GE_NO_SUCH_ROOM;
    }

    int buffer_size = room->width * room->height;
    char *buffer = malloc(buffer_size * sizeof(char)); 
    if (buffer == NULL){
        return NO_MEMORY;
    }

    Status s = room_render(room, &eng->charset, buffer, room->width, room->height);
    if (s != OK){
        free(buffer);
        return INTERNAL_ERROR;
    }
    int out_size = room->width * room->height + room->height + 1;
    char *str_buffer = malloc(out_size * sizeof(char));
    if (str_buffer == NULL){
        free(buffer);
        return NO_MEMORY;
    }
    int linefeed = 0; 
    for(int i = 0; i < room->height; i++){
        for(int j = 0; j < room->width; j++){
            str_buffer[linefeed] = buffer[i*room->width+j];
            linefeed++;
        }
        str_buffer[linefeed++] = '\n';
    }
    str_buffer[linefeed] = '\0';
    free(buffer);
    *str_out = str_buffer;
    return OK;
}

Status game_engine_get_room_ids(const GameEngine *eng, int **ids_out, int *count_out){
    if (eng == NULL ){
        return INVALID_ARGUMENT;    
    }
    if(ids_out == NULL || count_out == NULL){
        return NULL_POINTER;
    }
    if (eng->graph == NULL){
        return INTERNAL_ERROR;
    }
    const void * const *payloads_out = NULL;
    int room_count = 0;
    if(graph_get_all_payloads(eng->graph, &payloads_out, &room_count) != GRAPH_STATUS_OK){
        return INTERNAL_ERROR;
    }

    int *ids = malloc (room_count * sizeof(int));
    if(ids == NULL){
        return NO_MEMORY;
    }

    for(int i = 0; i < room_count; i++){
        const Room *room = (const Room *)payloads_out[i];
        ids[i] = room->id;
    }

    *ids_out = ids;
    *count_out = room_count;

    return OK;
}

/* ============================================================
 * Memory Utilities
 * ============================================================ */

void game_engine_free_string(void *ptr){
    free(ptr);
}

/* ============================================================
 * Added for A3
 * ============================================================ */

Status game_engine_get_current_room_name(const GameEngine *eng, char **name_out) {
    if (eng == NULL || name_out == NULL) return INVALID_ARGUMENT;
    Room key = {0};
    key.id = eng->player->room_id;
    Room *room = (Room *)graph_get_payload(eng->graph, &key);
    if (room == NULL) return GE_NO_SUCH_ROOM;
    *name_out = room->name;
    return OK;
}

Status game_engine_get_total_treasure_count(const GameEngine *eng, int *count_out) {
    if (eng == NULL || count_out == NULL){
        return INVALID_ARGUMENT;
    }
    const void * const *all_rooms = NULL;
    int total_rooms = 0;
    int total = 0;
    graph_get_all_payloads(eng->graph, &all_rooms, &total_rooms);
    for (int i = 0; i < total_rooms; i++) {
        Room *r = (Room *)all_rooms[i];
        total += r->treasure_count;
    }
    *count_out = total;
    return OK;
}

Status game_engine_get_charset(GameEngine *eng, Charset *out) {
    if (!eng || !out) {
        return NULL_POINTER;
    }
    out->player = eng->charset.player;
    out->wall = eng->charset.wall;
    out->treasure = eng->charset.treasure;
    out->portal = eng->charset.portal;
    out->pushable = eng->charset.pushable;
    out->switch_off = eng->charset.switch_off;
    out->switch_on = eng->charset.switch_on;

    return OK;
}
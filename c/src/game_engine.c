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
/* ============================================================
 * GameEngine - Main Game Controller
 *
 * The GameEngine orchestrates the entire game state:
 *   • Owns the connectivity Graph (world topology)
 *   • Indirectly owns all Rooms via the Graph payloads
 *   • Owns the Player entity
 *   • Owns the Charset used for rendering
 *   • Implements player movement, interaction, and room transitions
 *
 * All internal state is opaque to Python.
 * Only this public API is exposed via ctypes in later assignments.
 *
 * The Graph and Room structures are never exposed directly.
 * ============================================================ */

/* ============================================================
 * Creation & Destruction
 * ============================================================ */

/*
 * game_engine_create
 * ------------------
 * Build the entire game state by:
 *   1. Loading the world using the world loader
 *   2. Creating a connectivity graph of rooms
 *   3. Loading the rendering charset
 *   4. Creating and placing the player in the starting room
 *
 * Parameters:
 *   config_path:
 *     Path to the world configuration file (must not be NULL).
 *
 *   engine_out:
 *     On success, receives an owning pointer to a newly created GameEngine.
 *
 * Returns:
 *   OK on success
 *   INVALID_ARGUMENT if inputs are NULL
 *   WL_ERR_DATAGEN if world loading fails
 *   NO_MEMORY on allocation failure
 *
 * Postconditions:
 *   On success, caller owns the engine and must call game_engine_destroy().
 */
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

/*
 * game_engine_destroy
 * -------------------
 * Destroy the game engine and all owned state.
 *
 * This frees:
 *   • The player
 *   • The connectivity graph
 *   • All rooms owned by the graph
 */
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

/*
 * Retrieve the current player.
 *
 * The returned pointer is owned by the engine and remains valid
 * for the lifetime of the engine.
 *
 * Returns:
 *   Pointer to the player on success
 *   NULL if eng is NULL
 */
const Player *game_engine_get_player(const GameEngine *eng){
    if (eng == NULL || eng->player == NULL){
        return NULL;
    }
    return eng->player;
}

/* ============================================================
 * Player Movement & Interaction
 * ============================================================ */

/*
 * Attempt to move the player in the given direction.
 *
 * Movement logic:
 *   - Walls block movement (returns ROOM_IMPASSABLE)
 *   - Floor tiles allow movement
 *   - Treasures allow movement (visual only in A1, will be collected in A2)
 *   - Portals trigger room transitions to connected rooms
 *
 * Returns:
 *   OK on success
 *   INVALID_ARGUMENT if inputs are invalid
 *   ROOM_IMPASSABLE if movement is blocked by a wall
 *   GE_NO_SUCH_ROOM if a referenced room does not exist
 *   INTERNAL_ERROR on invariant failure
 */
Status game_engine_move_player(GameEngine *eng, Direction dir){
    // checks valid engine
    if (eng == NULL) {
        return INVALID_ARGUMENT;
    }

    // checks valid player
    Player *p = eng->player;
    if (p == NULL) {
        return INTERNAL_ERROR;
    }

    // Get current room ID and Room pointer
    int current_room_id = player_get_room(p);
    Room key = {0};
    key.id = current_room_id;

    // if room doesn't exist then return GE_NO_SUCH_ROOM
    Room *current_room = (Room *)graph_get_payload(eng->graph, &key);
    if (current_room == NULL) {
        return GE_NO_SUCH_ROOM;
    }

    // Get player's current coordinates
    int nx = 0;
    int ny = 0;
    Status s = player_get_position(p, &nx, &ny);
    if (s != OK) {
        return INTERNAL_ERROR;
    }

    // Compute next position based on direction
    switch (dir) {
        case DIR_NORTH: ny--; break;
        case DIR_SOUTH: ny++; break;
        case DIR_WEST:  nx--; break;
        case DIR_EAST:  nx++; break;
        default: return INVALID_ARGUMENT;
    }

    // If there is an uncollected treasure at the destination, 
    // collect it without moving.
    if (try_collect_at(current_room, p, nx, ny)) {
        return OK;
    }

    // If a pushable is at the destination, try to push it and step in.
    int pushable_idx = -1;
    if (room_has_pushable_at(current_room, nx, ny, &pushable_idx)) {
        if (room_try_push(current_room, pushable_idx, dir) != OK) {
            return ROOM_IMPASSABLE;
        }
        player_set_position(p, nx, ny);
        return OK;
    }

    // Check if destination is walkable (floor grid + pushables; treasure already 
    // handled above)
    if (!room_is_walkable(current_room, nx, ny)) {
        return ROOM_IMPASSABLE;
    }

    // Check for portal at destination
    Room *target_room = find_portal_target(eng, current_room, nx, ny);
    if (target_room != NULL) {
        s = player_move_to_room(p, target_room->id);
        if (s != OK) {
            return INTERNAL_ERROR;
        }
        int entry_x = 1;
        int entry_y = 1;
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
        found:
        player_set_position(p, entry_x, entry_y);
        return OK;
    }

    // No portal — move within current room
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
            Room key = {0};
            key.id = portal->target_room_id;
            return (Room *)graph_get_payload(eng->graph, &key);
        }
    }
    return NULL;
}

/* ============================================================
 * Metadata Queries
 * ============================================================ */

/*
 * Retrieve the total number of rooms in the world.
 *
 * Returns:
 *   OK on success (count_out is set)
 *   INVALID_ARGUMENT if eng is NULL
 *   NULL_POINTER if count_out is NULL
 */
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

/*
 * Retrieve the width and height of the player's current room.
 *
 * Returns:
 *   OK on success (width_out and height_out are set)
 *   INVALID_ARGUMENT if eng is NULL
 *   NULL_POINTER if width_out or height_out are NULL
 *   INTERNAL_ERROR if player or room is invalid
 *   GE_NO_SUCH_ROOM if the current room cannot be found
 */
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

/*
 * Reset the game to its initial state.
 *
 * Effects:
 *   • Player returns to starting room and position
 *
 * Returns:
 *   OK on success
 *   INVALID_ARGUMENT if eng is NULL
 *   INTERNAL_ERROR if reset cannot complete
 */
Status game_engine_reset(GameEngine *eng){
    if (eng == NULL) return INVALID_ARGUMENT;
    if (eng->player == NULL) return INTERNAL_ERROR;

    /* Reset player */
    eng->player->room_id = eng->initial_room_id;
    eng->player->x = eng->initial_player_x;
    eng->player->y = eng->initial_player_y;
    free(eng->player->collected_treasures);
    eng->player->collected_treasures = NULL;
    eng->player->collected_count = 0;

    /* Reset all rooms */
    const void * const *all_rooms = NULL;
    int total_rooms = 0;
    graph_get_all_payloads(eng->graph, &all_rooms, &total_rooms);

    for (int i = 0; i < total_rooms; i++) {
        Room *r = (Room *)all_rooms[i];
        /* Reset treasures */
        for (int j = 0; j < r->treasure_count; j++) {
            r->treasures[j].collected = false;
        }
        /* Reset pushables using their stored initial positions */
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

/*
 * Render the player's current room as a string.
 *
 * The output includes:
 *   • Walls and floor
 *   • Portals
 *   • Treasures (visual only - not collected in A1)
 *   • Player character
 *
 * The output is formatted as a multi-line string with linefeeds after
 * each row, suitable for printing to the console.
 *
 * Parameters:
 *   eng:
 *     The game engine.
 *
 *   str_out:
 *     On success, receives a newly allocated string.
 *
 * Returns:
 *   OK on success
 *   INVALID_ARGUMENT if inputs are invalid
 *   INTERNAL_ERROR on rendering failure
 *
 * Ownership:
 *   The caller must free() the returned string.
 */
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
    // Now, convert buffer to multi-line string with linefeeds
    int out_size = room->width * room->height + room->height + 1;
    char *str_buffer = malloc(out_size * sizeof(char));
    if (str_buffer == NULL){
        free(buffer);
        return NO_MEMORY;
    }
    int linefeed =0; 
    for(int i=0; i<room->height; i++){
        for(int j=0; j<room->width; j++){
            str_buffer[linefeed] = buffer[i*room->width+j];
            //overlay player
            if(j==eng->player->x && i==eng->player->y){
                str_buffer[linefeed]=eng->charset.player;
            }
            linefeed++;
        }
        str_buffer[linefeed++]= '\n';
    }
    str_buffer[linefeed]='\0';

    free(buffer);
    *str_out = str_buffer;
    return OK;
}

/*
 * Render a room by ID into a newly allocated string (no player overlay).
 *
 * The output includes:
 *   - Walls and floor
 *   - Portals
 *   - Treasures
 *   - Does NOT include player character
 *
 * The output is formatted as a multi-line string with linefeeds after
 * each row, suitable for printing to the console.
 *
 * Returns:
 *   OK on success (str_out receives newly allocated string)
 *   INVALID_ARGUMENT if eng is NULL
 *   NULL_POINTER if str_out is NULL
 *   GE_NO_SUCH_ROOM if room_id is invalid
 *
 * Ownership:
 *   The caller must free() the returned string.
 */
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
    //convert buffer to multi-line string with linefeeds
    int out_size = room->width * room->height + room->height + 1;
    char *str_buffer = malloc(out_size * sizeof(char));
    if (str_buffer == NULL){
        free(buffer);
        return NO_MEMORY;
    }
    int linefeed =0; 
    for(int i=0; i<room->height; i++){
        for(int j=0; j<room->width; j++){
            str_buffer[linefeed] = buffer[i*room->width+j];
            linefeed++;
        }
        str_buffer[linefeed++]= '\n';
    }
    str_buffer[linefeed]='\0';
    free(buffer);
    *str_out = str_buffer;
    return OK;
}

/*
 * Retrieve all room IDs in the loaded world.
 *
 * Returns:
 *   OK on success (ids_out and count_out are set)
 *   INVALID_ARGUMENT if eng is NULL
 *   NULL_POINTER if ids_out or count_out are NULL
 *   INTERNAL_ERROR if graph is invalid
 *   NO_MEMORY on allocation failure
 *
 * Ownership:
 *   The caller owns the returned array and must free() it.
 */
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

    for(int i=0; i< room_count; i++){
        const Room *room = (const Room *)payloads_out[i];
        ids[i] = room->id;
    }

    *ids_out = ids;
    *count_out = room_count;

    return OK;
}

// **********************************************************                                
// ********************** ADDED FOR A2 **********************
// **********************************************************

/* ============================================================
 * Memory Utilities
 * ============================================================ */

/*
 * Free a heap buffer allocated by the game engine (e.g., render string).
 *
 * This is required for C/Python interoperability in A2 and A3
 *
 */
void game_engine_free_string(void *ptr){
    free(ptr);
}

// **********************************************************                                
// ********************** ADDED FOR A3 **********************
// **********************************************************

Status game_engine_get_current_room_name(const GameEngine *eng, char **name_out) {
    if (eng == NULL || name_out == NULL) return INVALID_ARGUMENT;
    Room key = {0};
    key.id = eng->player->room_id;
    Room *room = (Room *)graph_get_payload(eng->graph, &key);
    if (room == NULL) return GE_NO_SUCH_ROOM;
    *name_out = room->name;
    return OK;
}

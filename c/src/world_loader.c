#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include "types.h"   /* Room, Charset */
#include "world_loader.h"
#include "datagen.h"
#include "graph.h"
#include "room.h"
typedef struct Graph Graph;

/* --------------------------------------------------
 * Local helpers 
 * -------------------------------------------------- */

static int room_compare_world(const void *a, const void *b)
{
    const Room *ra = a;
    const Room *rb = b;
    return ra->id - rb->id;
}

static void room_destroy_world(void *payload)
{
    room_destroy((Room *)payload);
}

static Status copy_floor_grid(Room *new_room, const DG_Room *room){
    if (room->floor_grid != NULL) {
        size_t count = (size_t)room->width * (size_t)room->height;
        
        new_room->floor_grid = malloc(count * sizeof(bool));
        if (!new_room->floor_grid) {
            return NO_MEMORY;
        }
        memcpy(new_room->floor_grid, room->floor_grid, count * sizeof(bool));
    }
    return OK;
}

static Status copy_portals(Room *new_room, const DG_Room *room){
    if (room->portal_count > 0 && room->portals != NULL) {
        new_room->portals = malloc(room->portal_count * sizeof(Portal));
        if (!new_room->portals) {
            return NO_MEMORY;
        }
        for (int i = 0; i < room->portal_count; i++) {
            new_room->portals[i].id = room->portals[i].id;
            new_room->portals[i].x  = room->portals[i].x;
            new_room->portals[i].y  = room->portals[i].y;
            new_room->portals[i].target_room_id = room->portals[i].neighbor_id;
            new_room->portals[i].name = NULL;
        }
        new_room->portal_count = room->portal_count;
    }
    return OK;
}

static Status copy_treasures(Room *new_room, const DG_Room *room){
    if (room->treasure_count > 0 && room->treasures != NULL) {
        new_room->treasures = malloc(room->treasure_count * sizeof(Treasure));
        if (!new_room->treasures) {
            return NO_MEMORY;
        }
        for (int i = 0; i < room->treasure_count; i++) {
            new_room->treasures[i].id = room->treasures[i].global_id;
            new_room->treasures[i].x  = room->treasures[i].x;
            new_room->treasures[i].y  = room->treasures[i].y;
            new_room->treasures[i].name = NULL;
            new_room->treasures[i].initial_x = room->treasures[i].x;
            new_room->treasures[i].initial_y = room->treasures[i].y;
            new_room->treasures[i].starting_room_id = new_room->id;
            new_room->treasures[i].collected = false;
        }
        new_room->treasure_count = room->treasure_count;
    }
    return OK;
}
static Status copy_pushables(Room *new_room, const DG_Room *room){
    if (room->pushable_count > 0 && room->pushables != NULL) {
        new_room->pushables = malloc(room->pushable_count * sizeof(Pushable));
        if (!new_room->pushables) {
            return NO_MEMORY;
        }
        for (int i = 0; i < room->pushable_count; i++) {
            new_room->pushables[i].id = room->pushables[i].id;
            new_room->pushables[i].x  = room->pushables[i].x;
            new_room->pushables[i].y  = room->pushables[i].y;
            new_room->pushables[i].initial_x = room->pushables[i].x;
            new_room->pushables[i].initial_y = room->pushables[i].y;
            new_room->pushables[i].name = NULL;
        }
        new_room->pushable_count = room->pushable_count;
    }
    return OK;
}        
static Status connect_rooms(Room **rooms, size_t room_count, Graph **graph){
    for (size_t i = 0; i < room_count; i++) {
        Room *room = rooms[i];

        for (int por = 0; por < room->portal_count; por++) {
            Portal *portal = &room->portals[por];

            /* Linear search for target room */
            for (size_t j = 0; j < room_count; j++) {
                if (rooms[j]->id == portal->target_room_id) {
                    graph_connect(*graph, room, rooms[j]);
                    break;
                }
            }
        }
    }
    return OK;
}
/* ============================================================
 * World Loader
 *
 * Responsibilities:
 *   • Consume datagen library output (generator-owned memory)
 *   • Deep-copy room data into student-owned Room structs
 *   • Build a connectivity Graph from room neighbor relationships
 *   • Cache the Charset provided by datagen
 *
 * The loader bridges the gap between datagen's temporary output
 * and the student's persistent data structures.
 *
 * IMPORTANT:
 *   All datagen pointers are temporary.
 *   The loader performs deep copies so that room data remains
 *   valid after datagen is shut down.
 * ============================================================ */

/* ============================================================
 * loader_load_world
 * ============================================================
 *
 * Load a world by:
 *   1. Passing the config file to datagen
 *   2. Iterating through datagen output
 *   3. Deep-copying datagen data to add to Room structs
 *   4. Building a connectivity graph of Room structs and returning it to the game engine
 *   5. Passing the charset back to the game engine
 *
 * Parameters:
 *   config_file:
 *     Path to the datagen configuration file.
 *
 * Outputs:
 *   graph_out:
 *     On success, receives a pointer to a newly created Graph.
 *
 *   first_room_out:
 *     On success, receives a pointer to the first room inserted
 *     into the graph.
 *
 *   num_rooms_out:
 *     On success, receives the total number of rooms loaded.
 *
 *   charset_out:
 *     On success, receives the loaded charset.
 *
 * Returns:
 *   OK on success
 *   WL_ERR_CONFIG if the config path is invalid
 *   WL_ERR_DATAGEN if datagen fails
 *   NO_MEMORY on allocation failure
 *
 * Ownership:
 *   - The caller owns the returned Graph.
 *   - All Room structs are owned by the Graph as payloads.
 *   - Destroying the Graph frees all Rooms.
 */
Status loader_load_world(const char *config_file, Graph **graph_out, Room **first_room_out, int  *num_rooms_out, Charset *charset_out){
    if (config_file == NULL || graph_out == NULL || first_room_out == NULL || num_rooms_out == NULL || charset_out == NULL) {
        return WL_ERR_CONFIG;
    }
    if (fopen(config_file, "r")==NULL){
        return WL_ERR_CONFIG;
    }
    int status = start_datagen(config_file);
    if (status != 0) {
        return WL_ERR_DATAGEN;
    }
    Graph *graph =NULL;
    GraphStatus g = graph_create(room_compare_world, room_destroy_world, &graph);
    if (g != GRAPH_STATUS_OK) {
        stop_datagen();
        return NO_MEMORY;
    }
    *graph_out = graph;
    *num_rooms_out = 0;
    *first_room_out = NULL;


    Room **rooms = NULL;
    size_t room_count = 0;

    // Iterate through datagen rooms
    while (has_more_rooms()) {
        DG_Room room = get_next_room();

        Room *new_room = room_create(room.id, NULL, room.width, room.height);
        if (!new_room) {
            free(rooms);
            graph_destroy(graph);
            stop_datagen();
            return NO_MEMORY;
        }
 
        if (graph_insert(graph, new_room) != GRAPH_STATUS_OK) {
            free(rooms);
            graph_destroy(graph);
            stop_datagen();
            return NO_MEMORY;
        }

        Room **tmp = realloc(rooms, (room_count + 1) * sizeof(Room *));
        if (!tmp) {
            free(rooms);
            graph_destroy(graph);
            stop_datagen();
            return NO_MEMORY;
        }
        rooms = tmp;
        rooms[room_count++] = new_room;

        if (*first_room_out == NULL) {
            *first_room_out = new_room;
        }
        (*num_rooms_out)++;

        // Deep copy floor grid, portals, and treasures
        if (copy_floor_grid(new_room, &room)!= OK || copy_portals(new_room, &room)!= OK || copy_treasures(new_room, &room)!= OK || copy_pushables(new_room, &room)!= OK){
            free(rooms);
            graph_destroy(graph);
            stop_datagen();
            return NO_MEMORY;
        }

    }

    //connect rooms based on portals
    if (connect_rooms(rooms, room_count, &graph) != OK){
        free(rooms);
        graph_destroy(graph);
        stop_datagen();
        return NO_MEMORY;
    }

    const DG_Charset *dg_charset = dg_get_charset();
    if (!dg_charset) {
        free(rooms);
        graph_destroy(graph);
        stop_datagen();
        return WL_ERR_DATAGEN;
    }

    charset_out->wall     = dg_charset->wall;
    charset_out->floor    = dg_charset->floor;
    charset_out->portal   = dg_charset->portal;
    charset_out->treasure = dg_charset->treasure;
    charset_out->player   = dg_charset->player;
    charset_out->pushable = dg_charset->pushable;
    
    free(rooms);
    stop_datagen();
    return OK;
}


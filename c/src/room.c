#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <stdbool.h>
#include "types.h"
#include "room.h"
#include "datagen.h"
//helper
static RoomTileType classify_pushable(const Room *r, int x, int y, int *out_id);

/* ============================================================
 * Creation
 * ============================================================ */

Room *room_create(int id, const char *name, int width, int height){
    
    Room *new_room = malloc(sizeof(Room));
    
    if (new_room == NULL) {
        return NULL;
    }
    
    new_room->id = id;
    new_room->name = NULL;

    if(name != NULL){
        new_room->name = malloc (strlen(name)+1);
        if(new_room->name == NULL){
            free(new_room);
            return NULL;
        }
        strcpy(new_room->name, name);
    }
    if(width <= 0 || height <= 0){
        new_room->width = 1;
        new_room->height = 1;
    }else{
        new_room->width = width;
        new_room->height = height;
    }
    
    new_room->floor_grid = NULL;
    new_room->portals = NULL;            
    new_room->portal_count = 0;
    new_room->treasures = NULL;        
    new_room->treasure_count = 0;
    new_room->pushables = NULL;        
    new_room->pushable_count = 0;
    new_room->switches = NULL;
    new_room->switch_count = 0;
    return new_room;
}

/* ============================================================
 * Basic Queries
 * ============================================================ */

int room_get_width(const Room *r){
    if(r == NULL){
        return  0;
    }
    return r->width;
}

int room_get_height(const Room *r){
    if(r == NULL){
        return  0;
    }
    return r->height;
}

/* ============================================================
 * Room Data Setters
 * ============================================================ */

Status room_set_floor_grid(Room *r, bool *floor_grid){
    if(r == NULL){
        return  INVALID_ARGUMENT;
    }
    if(r->floor_grid != NULL){
        free(r->floor_grid);
        r->floor_grid = NULL;
    }
    r->floor_grid = floor_grid;  
    return  OK;
}

Status room_set_portals(Room *r, Portal *portals, int portal_count){
    if (r == NULL || (portal_count > 0 && portals == NULL)){
        return INVALID_ARGUMENT;
    }
    if (r->portals != NULL){
        for(int i=0; i<(r->portal_count); i++){
            free(r->portals[i].name);
        }
        free(r->portals);
        r->portals = NULL;
    }
    r->portal_count = portal_count;
    r->portals = portals;
    return  OK;
}

Status room_set_treasures(Room *r, Treasure *treasures, int treasure_count){
    if (r == NULL || (treasure_count > 0 && treasures == NULL)){
        return INVALID_ARGUMENT;
    }
    if(r->treasures != NULL){
        for(int i=0; i<(r->treasure_count); i++){
            free(r->treasures[i].name);
        }
        free(r->treasures);
        r->treasures = NULL;
        r->treasure_count = 0; 
    }
    if (treasure_count == 0){
        return OK;
    }
    r->treasures = treasures;
    r->treasure_count = treasure_count;
    return OK;
}

/* ============================================================
 * Treasure Management
 * ============================================================ */

Status room_place_treasure(Room *r, const Treasure *treasure){
    if (r == NULL ||  treasure == NULL){
        return INVALID_ARGUMENT;
    }
    Treasure *temp = realloc (r->treasures, (r->treasure_count+1)*sizeof(Treasure));
    if (temp == NULL) {
        return NO_MEMORY;
    }
    r->treasures = temp;
    r->treasures[r->treasure_count].id = treasure->id;
    r->treasures[r->treasure_count].x = treasure->x;
    r->treasures[r->treasure_count].y = treasure->y;
    r->treasures[r->treasure_count].name = strdup(treasure->name);
    if (r->treasures[r->treasure_count].name == NULL) return NO_MEMORY;
    r->treasure_count = r->treasure_count+1;
    r->treasures[r->treasure_count].collected = false;
    return OK;
}

int room_get_treasure_at(const Room *r, int x, int y){
    if (r == NULL || r->treasures == NULL) {
        return -1;
    }
    for (int i = 0; i < r->treasure_count; i++) {
        if (r->treasures[i].x == x && r->treasures[i].y == y && r->treasures[i].collected == false) {
            return r->treasures[i].id;
        }
    }
    return -1;
}

/* ============================================================
 * Portals
 * ============================================================ */

int room_get_portal_destination(const Room *r, int x, int y){
    if (r == NULL || r->portals == NULL) {
        return -1;
    }
    for (int i = 0; i < r->portal_count; i++) {
        if (r->portals[i].x == x && r->portals[i].y == y) {
            return r->portals[i].target_room_id;
        }
    }
    return -1;
}

/* ============================================================
 * Walkability & Tile Classification
 * ============================================================ */

bool room_is_walkable(const Room *r, int x, int y){
    if (r == NULL || x>=r->width || y>=r->height || x<0 ||y<0) {
        return false;
    }
    /* Switch tiles are always walkable so pushables can be pushed onto them
     * and the player can walk through an active switch to reach the portal */
    for (int i = 0; i < r->switch_count; i++) {
        if (r->switches[i].x == x && r->switches[i].y == y) {
            return true;
        }
    }
    for (int i = 0; i < r->portal_count; i++) {
        if (r->portals[i].x == x && r->portals[i].y == y) {
            if (!r->portals[i].gated) {
                return true;
            }

            int sid = r->portals[i].required_switch_id;

            for (int j = 0; j < r->switch_count; j++) {
                if (r->switches[j].id == sid) {
                    int sx = r->switches[j].x;
                    int sy = r->switches[j].y;

                    for (int k = 0; k < r->pushable_count; k++) {
                        if (r->pushables[k].x == sx && r->pushables[k].y == sy) {
                            return true; // unlocked
                        }
                    }
                }
            }

            return false; // locked portal
        }
    }
    for (int i = 0; i < r->pushable_count; i++) {
        if (r->pushables[i].x == x && r->pushables[i].y == y) {
            return false;
        }
    }
    if(r->floor_grid != NULL){
        return r->floor_grid[y * r->width + x];
    }
    if (r->floor_grid == NULL) {
        return !(x == 0 || y == 0 || x == r->width - 1 || y == r->height - 1);
    }
    return false;
}

static RoomTileType classify_pushable(const Room *r, int x, int y, int *out_id){
    for (int i = 0; i < r->pushable_count; i++) {
        if (r->pushables[i].x == x && r->pushables[i].y == y) {
            if (out_id) *out_id = i;
            return ROOM_TILE_PUSHABLE;
        }
    }
    return ROOM_TILE_INVALID;
}

RoomTileType room_classify_tile(const Room *r, int x, int y, int *out_id){
    if(r == NULL || x < 0 || y < 0 || x >= r->width || y >= r->height){
        return ROOM_TILE_INVALID;
    }
    RoomTileType type = classify_pushable(r, x, y, out_id);
    if (type != ROOM_TILE_INVALID) return type;
    
    for (int i = 0; i < r->treasure_count; i++) {
        if (r->treasures[i].x == x && r->treasures[i].y == y && !r->treasures[i].collected) {
            if (out_id != NULL){
                *out_id = r->treasures[i].id;
            }
            return ROOM_TILE_TREASURE;
        }
    }
    for (int i = 0; i < r->portal_count; i++) {
        if (r->portals[i].x == x && r->portals[i].y == y) {
            if (out_id != NULL){
                *out_id = r->portals[i].target_room_id;
            }
            return ROOM_TILE_PORTAL;
        }
    }
    if (r->floor_grid == NULL) {
        if (x == 0 || y == 0 || x == r->width-1 || y == r->height-1){
            return ROOM_TILE_WALL;
        }
        return ROOM_TILE_FLOOR;
    }
    if(r->floor_grid != NULL && r->floor_grid[y * r->width + x]){
        return ROOM_TILE_FLOOR;
    }
    return ROOM_TILE_WALL;
}

/* ============================================================
 * Rendering
 * ============================================================ */

Status room_render(const Room *r, const Charset *charset, char *buffer, int buffer_width, int buffer_height){
    if(r == NULL || charset == NULL || buffer == NULL ){
        return INVALID_ARGUMENT;
    }
    if(buffer_width != r->width || buffer_height != r->height){
        return INVALID_ARGUMENT;
    }

    /* Layer 1: floor/wall base */
    int cell_count = r->width * r->height;
    for (int i = 0; i < cell_count; i++) {
        int row = i / r->width;
        int col = i % r->width;
        if (r->floor_grid == NULL) {
            if(col==0 || row==0 || col == r->width-1 || row == r->height-1){
                buffer[i] = charset->wall;
            }else{
                buffer[i] = charset->floor;
            }
        } else {
            if(r->floor_grid[i]){
                buffer[i] = charset->floor;
            }else{
                buffer[i] = charset->wall;
            }
        }
    }

    /* Layer 2: switches — active if a pushable is sitting on them */
    for (int i = 0; i < r->switch_count; i++) {
        int x = r->switches[i].x;
        int y = r->switches[i].y;
        if (x >= 0 && y >= 0 && x < r->width && y < r->height) {
            bool active = false;
            for (int j = 0; j < r->pushable_count; j++) {
                if (r->pushables[j].x == x && r->pushables[j].y == y) {
                    active = true;
                    break;
                }
            }
            buffer[y * r->width + x] = active
                ? charset->switch_on
                : charset->switch_off;
        }
    }

    /* Layer 3: treasures */
    for (int i = 0; i < r->treasure_count; i++) {
        if (r->treasures[i].x >= 0 && r->treasures[i].y >= 0 && 
            r->treasures[i].x < r->width && r->treasures[i].y < r->height
            && !r->treasures[i].collected) {
            buffer[r->treasures[i].y * r->width + r->treasures[i].x] = charset->treasure;
        }
    }
    
    /* Layer 4: portals — locked portals show as switch_off, open as portal */
    for (int i = 0; i < r->portal_count; i++) {
        if (r->portals[i].x >= 0 && r->portals[i].y >= 0 && 
            r->portals[i].x < r->width && r->portals[i].y < r->height) {
            if (r->portals[i].gated) {
                int sid = r->portals[i].required_switch_id;
                bool unlocked = false;
                /* search for switch by ID, then check if pushable is on it */
                for (int j = 0; j < r->switch_count; j++) {
                    if (r->switches[j].id == sid) {
                        int sx = r->switches[j].x;
                        int sy = r->switches[j].y;
                        for (int k = 0; k < r->pushable_count; k++) {
                            if (r->pushables[k].x == sx && r->pushables[k].y == sy) {
                                unlocked = true;
                                break;
                            }
                        }
                        break;
                    }
                }
                buffer[r->portals[i].y * r->width + r->portals[i].x] =
                    unlocked ? charset->portal : 'L';
            } else {
                buffer[r->portals[i].y * r->width + r->portals[i].x] = charset->portal;
            }
        }
    }

    /* Layer 5: pushables — skip if sitting on a switch (consumed case) */
    for (int i = 0; i < r->pushable_count; i++) {
        int x = r->pushables[i].x;
        int y = r->pushables[i].y;
        if (x < 0 || y < 0 || x >= r->width || y >= r->height) continue;
        bool on_switch = false;
        for (int j = 0; j < r->switch_count; j++) {
            if (r->switches[j].x == x && r->switches[j].y == y) {
                on_switch = true;
                break;
            }
        }
        if (!on_switch) {
            buffer[y * r->width + x] = charset->pushable;
        }
    }


    return OK;
}

/* ============================================================
 * Entry Position
 * ============================================================ */

Status room_get_start_position(const Room *r, int *x_out, int *y_out){
    if(r == NULL || x_out == NULL || y_out == NULL){
        return INVALID_ARGUMENT;
    }

    /* Preference 1: first portal location */
    if (r->portal_count > 0) {
        *x_out = r->portals[0].x;
        *y_out = r->portals[0].y;
        return OK;
    }

    /* Preference 2: any interior walkable tile */
    for (int row = 1; row < r->height - 1; row++) {
        for (int col = 1; col < r->width - 1; col++) {
            if (room_is_walkable(r, col, row)) {
                *x_out = col;
                *y_out = row;
                return OK;
            }
        }
    }

    return ROOM_NOT_FOUND;
}

/* ============================================================
 * Destruction
 * ============================================================ */

void room_destroy(Room *r){
    if (r == NULL){
        return;
    }
    free(r->name);
    free(r->floor_grid);
    if (r->portals != NULL) {
        for (int i = 0; i < r->portal_count; i++){ 
            free(r->portals[i].name);              
        }
        free(r->portals);                           
    }
    if (r->treasures != NULL) {
        for (int i = 0; i < r->treasure_count; i++){ 
            free(r->treasures[i].name);              
        }
        free(r->treasures);                          
    }
    if (r->pushables != NULL) {
        for (int i = 0; i < r->pushable_count; i++){ 
            free(r->pushables[i].name);              
        }
        free(r->pushables);                          
    }
    if (r->switches != NULL) {
        free(r->switches);
    }
    free(r);
}

// **********************************************************                                
// ********************** ADDED FOR A2 **********************
// **********************************************************

int room_get_id(const Room *r){
    if(r==NULL){
        return -1; 
    }
    return r->id;
}

Status room_pick_up_treasure(Room *r, int treasure_id, Treasure **treasure_out){
    if(r == NULL || treasure_id < 0 || treasure_out == NULL){
        return INVALID_ARGUMENT;
    }
    for(int i = 0; i < r->treasure_count; i++){
        if(r->treasures[i].id == treasure_id){
            if(r->treasures[i].collected){
                return INVALID_ARGUMENT;
            }
            r->treasures[i].collected = true;
            *treasure_out = &r->treasures[i];
            return OK;
        }
    }
    return ROOM_NOT_FOUND;
}

void destroy_treasure(Treasure *t){
    if(t==NULL){
        return;
    }
    free(t->name);
    free(t);
}

/* ============================================================
 * Pushables
 * ============================================================ */

bool room_has_pushable_at(const Room *r, int x, int y, int *pushable_idx_out){
    if(r==NULL){
        return false;
    }
    for(int i=0; i<r->pushable_count; i++){
        if(r->pushables[i].x == x && r->pushables[i].y == y){
            if (pushable_idx_out != NULL) {
                *pushable_idx_out = i;
            }
            return true;
        }
    }
    return false;
}

Status room_try_push(Room *r, int pushable_idx, Direction dir){
    if (r==NULL||pushable_idx<0||pushable_idx>=r->pushable_count){
        return INVALID_ARGUMENT;
    }
    if (dir != DIR_NORTH && dir != DIR_SOUTH && dir != DIR_EAST && dir != DIR_WEST){
        return INVALID_ARGUMENT;
    }
    int the_one = 0; 
    int new_x = 0;
    int new_y = 0;
    for(int i=0; i<r->pushable_count; i++){
        if(r->pushables[i].id == pushable_idx){
            the_one = i;
            new_x = r->pushables[i].x;
            new_y = r->pushables[i].y;
            if(dir == DIR_NORTH){
                new_y -= 1;
            }else if(dir == DIR_SOUTH){
                new_y += 1;
            }else if(dir == DIR_EAST){
                new_x += 1;
            }else if(dir == DIR_WEST){
                new_x -= 1;
            }else{
                return INVALID_ARGUMENT;
            }
        }
    }
    if(room_is_walkable(r, new_x, new_y)){
        r->pushables[the_one].x = new_x;
        r->pushables[the_one].y = new_y;
        /* A3: if pushable lands on a switch, it stays there (consumed).
         * The pushable remains at the switch coordinates so is_switch_active
         * will find it and unlock the linked portal. The pushable is not
         * rendered because room_render skips pushables that are on a switch. */
        return OK;
    }    
    return ROOM_IMPASSABLE;
}
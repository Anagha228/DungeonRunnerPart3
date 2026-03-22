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

/*
 * Create a new Room with the given identity and dimensions.
 * - You need to allocate the Room and the name. 
 * - The room name must be a copy of the name argument.
 * - All the other pointers must be set to NULL. They will be initialized 
 *   using the relevant setter functions, which are described below
 *
 * Parameters:
 *   id:
 *     Unique room ID.
 *   name:
 *     Room name (room owns the memory).
 *   width, height:
 *     Room dimensions
 *
 * Preconditions:
 *   No preconditions. name may be NULL.
 *
 * Postconditions:
 *   On success, returns a newly allocated Room with a correctly set name.
 *   All other pointers in the Room must be initialized to NULL.
 *
 * Returns:
 *   Newly allocated Room on success
 *   NULL on allocation failure
 */
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
    
    new_room->floor_grid = NULL;           /* width * height */
    new_room->portals = NULL;            
    new_room->portal_count = 0;
    new_room->treasures = NULL;        
    new_room->treasure_count = 0;
    new_room->pushables = NULL;        
    new_room->pushable_count = 0;
    return new_room;
}

/* ============================================================
 * Basic Queries
 * ============================================================ */

/*
 * Retrieve the room width in tiles.
 *
 * Returns:
 *   width on success
 *   0 if r is NULL
 */
int room_get_width(const Room *r){
    if(r == NULL){
        return  0;
    }
    return r->width;
}

/*
 * Retrieve the room height in tiles.
 *
 * Returns:
 *   height on success
 *   0 if r is NULL
 */
int room_get_height(const Room *r){
    if(r == NULL){
        return  0;
    }
    return r->height;
}

/* ============================================================
 * Room Data Setters
 * ============================================================ */

/*
 * Set the floor grid (ownership transfers to the room). 
 *
 * Parameters:
 *   floor_grid:
 *     Array of width * height booleans (true = floor).
 *     Pass NULL to indicate implicit boundary walls.
 *
 * Preconditions:
 *   r must not be NULL.
 *   If floor_grid is non-NULL, it must be width * height entries.
 *
 * Postconditions:
 *   On success, ownership of floor_grid transfers to the room.
 *   If the floor grid in the room was previously initialized:
 *   - It must be overwritten by the new grid.
 *   - The old grid must be freed.
 *
 * Returns:
 *   OK on success
 *   INVALID_ARGUMENT if room is NULL
 */
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

/*
 * Set portals (ownership transfers to the room).
 *
 * Preconditions:
 *   r must not be NULL.
 *   If portal_count > 0, portals must not be NULL.
 *
 * Postconditions:
 *   On success, ownership of portals transfers to the room.
 *   If the portal array in the room was previously initialized:
 *   - It must be overwritten by the new portal array.
 *   - The new portal count must be correct.
 *   - The old portal array must be freed, along with old portal names.
 *
 * Returns:
 *   OK on success
 *   INVALID_ARGUMENT if room is NULL or parameters are inconsistent
 */
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

/*
 * Set treasures (ownership transfers to the room).
 *
 * Preconditions:
 *   r must not be NULL.
 *   If treasure_count > 0, treasures must not be NULL.
 *
 * Postconditions:
 *   On success, ownership of treasures transfers to the room.
 *   If the treasure array in the room was previously initialized:
 *   - It must be overwritten by the new treasure array.
 *   - The new treasure count must be correct.
 *   - The old treasure array must be freed, along with old treasure names.
 *
 * Returns:
 *   OK on success
 *   INVALID_ARGUMENT if room is NULL or parameters are inconsistent
 */
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
/*
 * Add a treasure into the room (ownership transfers to the room).
 *
 * Preconditions:
 *   r and treasure must not be NULL.
 *
 * Postconditions:
 *   On success, ownership of the new treasure transfers to the room.
 *   The treasures array is expanded to accommodate the new treasure.
 *   The existing treasures in the room must not be affected by the addition.
 *
 * Returns:
 *   OK on success
 *   INVALID_ARGUMENT if room or treasure is NULL
 *   NO_MEMORY on allocation failure
 */
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

/*
 * Check if a treasure exists at a given position.
 *
 * Returns:
 *   treasure ID if found
 *   -1 if no treasure exists or room is NULL
 */
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

/*
 * Check if a portal exists at (x,y).
 *
 * Returns:
 *   destination room ID if found
 *   -1 if no portal exists or room is NULL
 */
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

/*
 * Determine whether a tile is walkable.
 *
 * A tile is not walkable if:
 *   • Out of bounds
 *   • Wall (including interior walls)
 *
 * Returns:
 *   true if walkable
 *   false otherwise or if room is NULL
 */
bool room_is_walkable(const Room *r, int x, int y){
    if (r == NULL || x>=r->width || y>=r->height || x<0 ||y<0) {
        return false;
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

//helper
static RoomTileType classify_pushable(const Room *r, int x, int y, int *out_id){
    for (int i = 0; i < r->pushable_count; i++) {
        if (r->pushables[i].x == x &&
            r->pushables[i].y == y) {

            if (out_id) *out_id = i;
            return ROOM_TILE_PUSHABLE;
        }
    }
    return ROOM_TILE_INVALID;
}


/*
 * Classify a tile and optionally return an associated ID.
 *
 * If out_id is non-NULL and the tile contains:
 *   • treasure: out_id receives treasure ID
 *   • portal: out_id receives destination room ID
 *
 * Returns:
 *   ROOM_TILE_INVALID for out-of-bounds or if room is NULL
 *   ROOM_TILE_WALL for walls
 *   ROOM_TILE_FLOOR for empty walkable tiles
 *   ROOM_TILE_TREASURE or ROOM_TILE_PORTAL as appropriate
 */
RoomTileType room_classify_tile(const Room *r, int x, int y, int *out_id){
    if(r == NULL || x < 0 || y < 0 || x >= r->width || y >= r->height){
        return ROOM_TILE_INVALID;
    }

    RoomTileType type =classify_pushable(r, x, y, out_id);
    if (type != ROOM_TILE_INVALID) return type;
    
    for (int i = 0; i < r->treasure_count; i++) {
        if (r->treasures[i].x == x && r->treasures[i].y == y && !r->treasures[i].collected) {
            if (out_id !=NULL){
                *out_id = r->treasures[i].id;
            }
            return ROOM_TILE_TREASURE;
        }
    }

    for (int i = 0; i < r->portal_count; i++) {
        if (r->portals[i].x == x && r->portals[i].y == y) {
            if (out_id !=NULL){
                *out_id = r->portals[i].target_room_id;
            }
            return ROOM_TILE_PORTAL;
        }
    }

    if (r->floor_grid == NULL) {
    // implicit walls
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
/*
 * Render the room into a pre-allocated buffer.
 *
 * Produces a "framebuffer" - a flat character array with NO linefeeds.
 * This allows easy overlay of dynamic entities (like the player) by the
 * game engine before formatting the final display string.
 *
 * Rendering is layered:
 *   1. Base layer: Use floor_grid to render walls and floors
 *      - floor_grid[i] == true  → render charset->floor
 *      - floor_grid[i] == false → render charset->wall
 *      - If floor_grid is NULL, assume perimeter walls with open interior
 *   2. Overlay treasures at their positions (if not collected)
 *   3. Overlay portals at their positions
 *
 * The buffer uses simple 2D-to-1D indexing: buffer[y * width + x].
 * Does NOT render the player - that's done by the game engine.
 *
 * Parameters:
 *   r:
 *     The room to render.
 *   charset:
 *     Character set for rendering.
 *   buffer:
 *     Pre-allocated buffer of exactly width * height bytes (no linefeeds).
 *   buffer_width, buffer_height:
 *     Must match room dimensions.
 *
 * Returns:
 *   OK on success
 *   INVALID_ARGUMENT if room, charset, or buffer is NULL
 *   INVALID_ARGUMENT if buffer dimensions don't match room dimensions
 */
Status room_render(const Room *r, const Charset *charset, char *buffer, int buffer_width, int buffer_height){
    if(r == NULL || charset == NULL || buffer == NULL ){
        return INVALID_ARGUMENT;
    }
    if(buffer_width != r->width || buffer_height != r->height){
        return INVALID_ARGUMENT;
    }
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
    
    for (int i = 0; i < r->treasure_count; i++) {
        if (r->treasures[i].x >= 0 && r->treasures[i].y >= 0 && 
            r->treasures[i].x < r->width && r->treasures[i].y < r->height
            && !r->treasures[i].collected) {
                buffer[r->treasures[i].y * r->width + r->treasures[i].x] = charset->treasure;
        }
    }

    for (int i = 0; i < r->portal_count; i++) {
        if (r->portals[i].x >= 0 && r->portals[i].y >= 0 && 
            r->portals[i].x < r->width && r->portals[i].y < r->height) {
            buffer[r->portals[i].y * r->width + r->portals[i].x] = charset->portal;
        }
    }
    return OK;
}


/* ============================================================
 * Entry Position
 * ============================================================ */

/*
 * Determine a valid starting position in the room.
 *
 * Preference:
 *   1. First portal location
 *   2. Any interior walkable tile
 *
 * Returns:
 *   OK on success (x_out and y_out are set)
 *   INVALID_ARGUMENT if room or output pointers are NULL
 *   ROOM_NOT_FOUND if no valid starting position exists
 */
Status room_get_start_position(const Room *r, int *x_out, int *y_out){
    if(r == NULL || x_out == NULL || y_out == NULL){
        return INVALID_ARGUMENT;
    }
    if (r->portals != NULL) {
        for (int i = 0; i < r->portal_count; i++) {
            *x_out = r->portals[i].x;
            *y_out = r->portals[i].y;
            return OK;
        }
    }
    for (int col = 1; col < r->width - 1; col++) {
        for (int row = 1; row < r->height - 1; row++) {
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
 * Destruction & Debugging
 * ============================================================ */

/*
 * Free all memory owned by the room.
 *
 * Must be safe to call on NULL.
 */
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
    
    // Free treasures
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
    
    free(r);
}

// **********************************************************                                
// ********************** ADDED FOR A2 **********************
// **********************************************************

/* ============================================================
 * Additional Room and Treasure functions
 * ============================================================ */


/*
 * Retrieve the Unique room ID.
 *
 * Returns:
 *   ID on success
 *   -1 if r is NULL
 */

int room_get_id(const Room *r){
    if(r==NULL){
        return -1; 
    }
    return r->id;
}

/*
 * Remove a treasure from the room by ID.
 *
 * Preconditions:
 *   r and treasure_out are not NULL 
 *   treasure_id is not negative
 *
 * Parameters:
 *   treasure_out: receives a pointer to the room-owned Treasure on success
 *
 * Returns:
 *   OK on success (treasure_out is set)
 *   INVALID_ARGUMENT if r or treasure_out is NULL, or treasure already collected
 *   ROOM_NOT_FOUND if treasure_id does not exist in the room
 *
 * Postconditions:
 *   If the treasure was found, its .collected status was set to true
 *   No other Room data has been modified in any way
 *
 * Ownership:
 *   The room retains ownership of the Treasure. Callers must NOT free it.
 */
Status room_pick_up_treasure(Room *r, int treasure_id, Treasure **treasure_out){
    if(r == NULL || treasure_id < 0 || treasure_out == NULL){
        return INVALID_ARGUMENT;
    }

    for(int i = 0; i < r->treasure_count; i++){
        if(r->treasures[i].id == treasure_id){

            if(r->treasures[i].collected){
                return INVALID_ARGUMENT;
            }

            r->treasures[i].collected = true;   // <-- missing line
            *treasure_out = &r->treasures[i];

            return OK;
        }
    }

    return ROOM_NOT_FOUND;
}


/*
 * Free a heap-allocated Treasure (if one was ever created).
 *
 * Note:
 *   This is not room-prefixed because treasure lifetimes may need to be
 *   managed outside the Room API.
 *   Room-owned treasures should NOT be freed with this function.
 *   This function might be useful in A3.
 */
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

/*
 * Check whether a pushable exists at (x,y).
 *
 * Preconditions:
 *   r is not NULL 
 *
 * Returns:
 *   true if a pushable exists at the specified coordinates
 *   false if it does not, or on invalid arguments
 *
 * Postconditions:
 *   If pushable_idx_out is non-NULL, it receives the pushable index used 
 *   internally by the room.
 */
bool room_has_pushable_at(const Room *r, int x, int y, int *pushable_idx_out){
    if(r==NULL){
        return false;
    }
    for(int i=0; i<r->pushable_count; i++){
        if(r->pushables[i].x== x && r->pushables[i].y == y){
            if (pushable_idx_out != NULL) {
                *pushable_idx_out = i;
            }
            return true;
        }
    }
    return false;
}

/*
 * Attempt to push a pushable in the given direction.
 *
 * Preconditions:
 *   r is not NULL 
 *   pushable_idx is not negative and is less than r->pushable_count
 *   dir is a valid member of the enum type Direction
 *
 * Returns:
 *   OK on success
 *   ROOM_IMPASSABLE if blocked
 *   INVALID_ARGUMENT if arguments are invalid
 *
 * Postconditions:
 *   If push was possible, the pushable's x and y coordinates in r->pushables
 *   have been correctly updated - i.e. the obstacle was pushed
 */
Status room_try_push(Room *r, int pushable_idx, Direction dir){
    if (r==NULL||pushable_idx<0||pushable_idx>=r->pushable_count){
        return INVALID_ARGUMENT;
    }
    if (dir != DIR_NORTH && dir != DIR_SOUTH && dir != DIR_EAST && dir != DIR_WEST){
        return INVALID_ARGUMENT;
    }
    int the_one =0; 
    int new_x =0;
    int new_y =0;
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
        return OK;
    }    
    
    return ROOM_IMPASSABLE;
}




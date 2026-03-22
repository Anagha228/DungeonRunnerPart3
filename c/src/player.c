#include <stdlib.h>
#include <stdio.h>
#include "types.h"   /* Status, Direction, Treasure */
#include "player.h"

/* ============================================================
 * Lifecycle
 * ============================================================ */
/*
 * Create a new player at the given room and position.
 *
 * Preconditions:
 *   player_out must not be NULL.
 *
 * Returns:
 *   OK on success (player_out set)
 *   INVALID_ARGUMENT if player_out is NULL
 *   NO_MEMORY on allocation failure
 */
Status player_create(int initial_room_id, int initial_x, int initial_y, Player **player_out){
    if(player_out == NULL){
        return INVALID_ARGUMENT;
    }

    Player *player = malloc(sizeof(Player));
    if (player == NULL) {
        return NO_MEMORY;
    }

    player->room_id = initial_room_id;
    player->x = initial_x;
    player->y = initial_y;
    player->collected_count = 0;
    player->collected_treasures = NULL;

    *player_out = player;

    return OK;
}

/*
 * Destroy the player and free all owned memory.
 *
 * Must be safe to call on NULL.
 */
void player_destroy(Player *p){
    if(p!=NULL){
        free(p->collected_treasures);
        p->collected_treasures = NULL;
        free(p);
        p = NULL; 
    }
}

/* ============================================================
 * Position and Room State
 * ============================================================ */
/*
 * Get the ID of the room the player is currently in.
 *
 * Returns:
 *   Room ID on success
 *   -1 if p is NULL
 */
int player_get_room(const Player *p){
    if(p == NULL){
        return -1;
    }
    return p->room_id;
}

/*
 * Get the player's current position.
 *
 * Returns:
 *   OK on success (x_out and y_out are set)
 *   INVALID_ARGUMENT if any pointer is NULL
 */
Status player_get_position(const Player *p, int *x_out, int *y_out){
    if(p==NULL || x_out==NULL || y_out==NULL){
        return INVALID_ARGUMENT;
    }
    
    *x_out = p->x;
    *y_out = p->y;
    return OK;
}

/*
 * Set the player's position within the current room (no validation).
 *
 * Returns:
 *   OK on success
 *   INVALID_ARGUMENT if player is NULL
 */
Status player_set_position(Player *p, int x, int y){
    if(p == NULL){
        return INVALID_ARGUMENT;
    }

    p->x = x; 
    p->y = y;
    return OK;
}

/*
 * Transition the player to a different room (position unchanged).
 *
 * Returns:
 *   OK on success
 *   INVALID_ARGUMENT if player is NULL
 */
Status player_move_to_room(Player *p, int new_room_id){
    if(p == NULL){
        return INVALID_ARGUMENT;
    }

    p->room_id = new_room_id;
    return OK;
}

 
/* ============================================================
 * Reset
 * ============================================================ */

/*
 * Reset the player to an initial state:
 *   • Resets room ID and position
 *
 * Returns:
 *   OK on success
 *   INVALID_ARGUMENT if player is NULL
 */
Status player_reset_to_start(Player *p, int starting_room_id, int start_x, int start_y){
    if(p == NULL){
        return INVALID_ARGUMENT;
    }

    p->room_id = starting_room_id;
    p->x = start_x;
    p->y = start_y;
    for (int i = 0; i < p->collected_count; i++) {
        p->collected_treasures[i] = NULL;
    }
    p->collected_treasures =NULL;
    p->collected_count = 0;
    
    return OK;
}

// **********************************************************                                
// ********************** ADDED FOR A2 **********************
// **********************************************************


/* ============================================================
 * Treasure Collection
 * ============================================================ */

/*
 * Attempt to collect a treasure and borrow its pointer.
 *
 * Preconditions:
 *   p and treasure are not NULL
 *   treasure has not been collected: treasure->collected is false
 *   and this treasure is not in the p->collected_treasures array
 *
 * Returns:
 *   OK on success
 *   NULL_POINTER if p or treasure is NULL
 *   INVALID_ARGUMENT if treasure already collected
 *   NO_MEMORY on allocation failure
 *
 * Postconditions:
 *   p->collected_treasures has been expanded to accommodate the new treasure
 *   treasure ptr has been correctly added to p->collected_treasures array
 *   and array length was updated
 *   treasure->collected status has been correctly updated
 *
 * Ownership:
 *   The room retains ownership of the Treasure. Player must NOT free it.
 */
Status player_try_collect(Player *p, Treasure *treasure){
    if(p==NULL||treasure==NULL){
        return NULL_POINTER;
    }
    bool isthere = false;
    for(int i=0; i<p->collected_count; i++){
        if(p->collected_treasures[i]==treasure ){
            isthere=true;
        } 
    }
    if(isthere==true || treasure->collected == true){
        return INVALID_ARGUMENT;
    }

    p->collected_count++;
    Treasure **temp = realloc (p->collected_treasures, p->collected_count*sizeof(Treasure*));
    if (temp == NULL){
        p->collected_count--;
        return NO_MEMORY;        
    }
    p->collected_treasures = temp; 
    p->collected_treasures[p->collected_count-1]= treasure;
    treasure->collected = true;
    return OK;
}

/* Check whether the player has collected a given treasure ID. 
 *
 * Preconditions:
 *   p is not NULL 
 *   treasure_id is not negative
 *
 * Returns:
 *   true is treasure has been collected
 *   false otherwise, or on invalid arguments
 */
bool player_has_collected_treasure(const Player *p, int treasure_id){
    if(p==NULL||treasure_id<0||p->collected_treasures==NULL||p->collected_count==0){
        return false;
    }
    for(int i=0; i<p->collected_count; i++){
        if(p->collected_treasures[i]->id==treasure_id){
            return true;
        }
    }
    return false;
}

/* Return the number of collected treasures.
 *
 * Preconditions:
 *   p is not NULL 
 *
 * Returns:
 *   the number of collected treasures
 *   0 if p is NULL
 */
int player_get_collected_count(const Player *p){
    if (p==NULL){
        return 0;
    }
    return p->collected_count;
}

/*
 * Retrieve the array of collected treasures.
 *
 * Preconditions:
 *   p and count_out are not NULL
 *
 * Returns:
 *   Pointer to internal array of collected treasures (read-only)
 *   NULL if p or count_out is NULL
 *
 * Ownership:
 *   Caller must NOT free or modify returned pointers.
 *
 * Postconditions:
 *   p has not been modified
 *   count_out has been set to the correct number of treasures
 */
const Treasure * const * player_get_collected_treasures(const Player *p, int *count_out){
    if(p==NULL || count_out==NULL){
        return NULL;
    }
    *count_out = p->collected_count;
    if(p->collected_count == 0){
        return NULL;
    }
    return (const Treasure * const *)p->collected_treasures;
}







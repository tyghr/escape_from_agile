#ifndef EFA_GAME_H
#define EFA_GAME_H

#define WINDOW_WIDTH  800
#define WINDOW_HEIGHT 600
#define MENU_SKIP 0.1
#define MS_PER_MOVE 50
#define FIELD_SIZE 64
#define FIELDS_X (WINDOW_WIDTH/FIELD_SIZE + 1)
#define FIELDS_Y (WINDOW_HEIGHT/FIELD_SIZE + 1)

#define WINDOW_TITLE "Escape from Agile"

#define FLOOR_TEXTURE "./media/Room_Builder_free_48x48.png"
#define INTERIOR_TEXTURE "./media/Interiors_free_48x48.png"

// #define DRAW_COLLISION_RECT

#define OBJECT_COLLISION_RATIO 1.0

#define PLAYER_TEXTURE "./media/player.png"
#define PLAYER_SRC_SIZE 100
#define PLAYER_INITIAL_SPEED 8.0
#define PLAYER_COLLISION_RATIO 0.9
#define PLAYER_INITIAL_HEALTH 100

#define MANAGER_TEXTURE "./media/manager.png"
#define MANAGER_SRC_SIZE 32
#define MANAGER_INITIAL_SPEED 4.0
#define MANAGER_COLLISION_RATIO 1.0
#define MANAGER_SPEED_INCREASE_PERIOD 20
#define MANAGER_SPEED_INCREASE_RATIO 0.1;

#define MANAGERS_COUNT 10
#define MANAGERS_APPEAR_PERIOD 100

enum {
    FIELD_TYPE_GRAYED = 01,
    FIELD_TYPE_DESK   = 02
};

typedef struct roomField {
    uint8_t flags;
    int weight;
	int prevX;
    int prevY;
} roomField;

typedef struct tileset {
    SDL_Texture *floorTexture;
    SDL_Texture *interiorTexture;
    roomField *fields;
} tileset;

typedef struct person {
    SDL_Texture *texture;
    float posX;
    float posY;
    float dirX;
    float dirY;
    float prevDirX;
    float prevDirY;
    float speed;
    float collisionRatio;
    uint8_t moveCounter;
} person;

// funcs

int initNewManager(void);
void randomManagerPosition(person *m);
int calcMovement(void);

uint8_t getFieldTypeByID(int x, int y);
uint8_t getFieldTypeByCoord(float x, float y);
void calcPathRoutes(void);

#endif  // EFA_GAME_H

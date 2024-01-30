#define _GNU_SOURCE

#include <stdio.h>
#include <stdlib.h>
#include <stdbool.h>
#include <time.h>
// #include <stdint.h>
// #include <stdarg.h>
#include <string.h>
// #include <math.h>
// #include <assert.h>
// #include <limits.h>

#include <SDL2/SDL.h>
#include <SDL2/SDL_image.h>
#include <SDL2/SDL_ttf.h>

#define NK_INCLUDE_FIXED_TYPES
#define NK_INCLUDE_STANDARD_IO
#define NK_INCLUDE_STANDARD_VARARGS
#define NK_INCLUDE_DEFAULT_ALLOCATOR
#define NK_INCLUDE_VERTEX_BUFFER_OUTPUT
#define NK_INCLUDE_FONT_BAKING
#define NK_INCLUDE_DEFAULT_FONT
#define NK_IMPLEMENTATION
#define NK_SDL_RENDERER_IMPLEMENTATION
#include "nuklear.h"
#include "nuklear_sdl_renderer.h"

#include "game.h"

/* Platform */
static SDL_Window *window;
static SDL_Renderer *renderer;
TTF_Font* drawFont;

/* GUI */
static struct nk_context *ctx;
const struct nk_colorf bg = {.r = 0.10f, .g = 0.15f, .b = 0.20f, .a = 1.0f};
static bool gui_menu_pause = true;

// globals
tileset room;
person player;
person *managers[MANAGERS_COUNT];
Uint64 prevMoveTime;
uint64_t safetyDist;
uint64_t codingProgress = 0;
uint64_t mentalHealth = PLAYER_INITIAL_HEALTH;

int initPlatform(void) {
    /* SDL setup */
    SDL_SetHint(SDL_HINT_VIDEO_HIGHDPI_DISABLED, "0");
    SDL_Init(SDL_INIT_VIDEO);

    int img_flags = IMG_INIT_PNG;
    if ( !( IMG_Init(img_flags) & img_flags ) ) {
        SDL_Log("Can't init image: %s", IMG_GetError());
        return -1;
    }

	if ( TTF_Init() < 0 ) {
        SDL_Log("Can't init ttf: %s", TTF_GetError());
        return -1;
    }
    drawFont = TTF_OpenFont("media/PixelEmulator.ttf", 24);
    if ( !drawFont ) {
        SDL_Log("Error loading font: %s", TTF_GetError());
    }

    window = SDL_CreateWindow(WINDOW_TITLE,
        SDL_WINDOWPOS_CENTERED, SDL_WINDOWPOS_CENTERED,
        WINDOW_WIDTH, WINDOW_HEIGHT, SDL_WINDOW_SHOWN | SDL_WINDOW_ALLOW_HIGHDPI);
    if (window == NULL) {
        SDL_Log("Error SDL_CreateWindow %s", SDL_GetError());
        return -1;
    }

    int flags = 0;
    flags |= SDL_RENDERER_ACCELERATED;
    flags |= SDL_RENDERER_PRESENTVSYNC;

    renderer = SDL_CreateRenderer(window, -1, flags);

    if (renderer == NULL) {
        SDL_Log("Error SDL_CreateRenderer %s", SDL_GetError());
        return -1;
    }

    float font_scale = 1;

    /* scale the renderer output for High-DPI displays */
    {
        int render_w, render_h;
        int window_w, window_h;
        float scale_x, scale_y;
        SDL_GetRendererOutputSize(renderer, &render_w, &render_h);
        SDL_GetWindowSize(window, &window_w, &window_h);
        scale_x = (float)(render_w) / (float)(window_w);
        scale_y = (float)(render_h) / (float)(window_h);
        SDL_RenderSetScale(renderer, scale_x, scale_y);
        font_scale = scale_y;
    }

    /* GUI */
    ctx = nk_sdl_init(window, renderer);
    /* Load Fonts: if none of these are loaded a default font will be used  */
    /* Load Cursor: if you uncomment cursor loading please hide the cursor */
    {
        struct nk_font_atlas *atlas;
        struct nk_font_config config = nk_font_config(0);
        struct nk_font *font;

        /* set up the font atlas and add desired font; note that font sizes are multiplied by font_scale to produce better results at higher DPIs */
        nk_sdl_font_stash_begin(&atlas);
        font = nk_font_atlas_add_default(atlas, 14 * font_scale, &config);
        nk_sdl_font_stash_end();

        /* this hack makes the font appear to be scaled down to the desired size and is only necessary when font_scale > 1 */
        font->handle.height /= font_scale;
        /*nk_style_load_all_cursors(ctx, atlas->cursors);*/
        nk_style_set_font(ctx, &font->handle);
    }

    return 0;
}

void randomizeRoomFields(void) {
    int i, ix, iy;
    int roomType = rand()%5;

    for (i = 0; i < FIELDS_X * FIELDS_Y; ++i) {
        room.fields[i].flags = 0;
        if (rand()%2) {
            room.fields[i].flags |= FIELD_TYPE_GRAYED | FIELD_TYPE_DESK;
        }
        room.fields[i].flags |= (roomType<<2);

        ix = i % FIELDS_X;
        iy = i / FIELDS_X;

        const int walk_areas = 2;

        // делаем главные проходы (убираем лишние столы)
        if (ix % (FIELDS_X / walk_areas) == 0 || (FIELDS_X - ix) < 3) {
            room.fields[i].flags &= ~(FIELD_TYPE_DESK);
        }
        if (iy % (FIELDS_Y / walk_areas) == 0 || (FIELDS_Y - iy) < 3) {
            room.fields[i].flags &= ~(FIELD_TYPE_DESK);
        }
    }
}

uint8_t getFieldTypeByID(int iX, int iY) {
    return room.fields[iY*FIELDS_X + iX].flags;
}

uint8_t getFieldTypeByCoord(float x, float y) {
    int iX = x/FIELD_SIZE;
    int iY = y/FIELD_SIZE;
    return room.fields[iY*FIELDS_X + iX].flags;
}

int initNewManager(void) {
    int i = 0;
    for (; i < MANAGERS_COUNT; ++i) {
        if (managers[i] == NULL) {
            break;
        }
    }
    if (i >= MANAGERS_COUNT) {
        return -1;
    }

    managers[i] = (person*)malloc(sizeof(person));
    managers[i]->texture = IMG_LoadTexture(renderer, MANAGER_TEXTURE);
    randomManagerPosition(managers[i]);
    managers[i]->moveCounter = 0;
    managers[i]->speed = MANAGER_INITIAL_SPEED;
    managers[i]->collisionRatio = MANAGER_COLLISION_RATIO;

    return 0;
}

int initWorld(void) {
    room.floorTexture = IMG_LoadTexture(renderer, FLOOR_TEXTURE);
    room.interiorTexture = IMG_LoadTexture(renderer, INTERIOR_TEXTURE);
    room.fields = (roomField *) malloc(FIELDS_X * FIELDS_Y * sizeof(roomField));
    randomizeRoomFields();

    player.texture = IMG_LoadTexture(renderer, PLAYER_TEXTURE);
    player.posX = FIELDS_X / 2 * FIELD_SIZE;
    player.posY = FIELDS_Y / 2 * FIELD_SIZE;
    player.moveCounter = 0;
    player.speed = PLAYER_INITIAL_SPEED;
    player.collisionRatio = PLAYER_COLLISION_RATIO;

    for (int i = 0; i < MANAGERS_COUNT; ++i) {
        managers[i] = NULL;
    }

    int err = initNewManager();
    return err;
}

void drawRoom (void) {
    const int roomSrcFieldSize = 48;
    const int roomDstFieldScale = 2;

    for (float x = 0; x <= WINDOW_WIDTH; x += FIELD_SIZE/roomDstFieldScale) {
        for (float y = 0; y <= WINDOW_HEIGHT; y += FIELD_SIZE/roomDstFieldScale) {
            SDL_Rect floorSrcRect = {0, 0, roomSrcFieldSize, roomSrcFieldSize};
            uint8_t f = getFieldTypeByCoord(x,y);

            int grayed = (f & FIELD_TYPE_GRAYED) ? 1 : 0;
            floorSrcRect.x = (12 + grayed) * roomSrcFieldSize;
            floorSrcRect.y = (6 + (f>>2)*2) * roomSrcFieldSize;

            SDL_FRect floorDstRect = {x, y, FIELD_SIZE/roomDstFieldScale - 1, FIELD_SIZE/roomDstFieldScale - 1};
            SDL_RenderCopyF(renderer, room.floorTexture, &floorSrcRect, &floorDstRect);
        }
    }

    for (float x = 0; x <= WINDOW_WIDTH; x += FIELD_SIZE) {
        for (float y = 0; y <= WINDOW_HEIGHT; y += FIELD_SIZE) {
            if (getFieldTypeByCoord(x,y) & FIELD_TYPE_DESK) {
                SDL_Rect objectSrcRect = {7*roomSrcFieldSize, 40*roomSrcFieldSize, 48*3, 48*3};
                SDL_FRect objectDstRect = {x, y, FIELD_SIZE - 1, FIELD_SIZE - 1};
                SDL_RenderCopyF(renderer, room.interiorTexture, &objectSrcRect, &objectDstRect);

                #ifdef DRAW_COLLISION_RECT
                    float oCollRatio = OBJECT_COLLISION_RATIO;
                    if (oCollRatio > 1.0) oCollRatio = 1.0;
                    float collFix = ((1-oCollRatio) * (float)FIELD_SIZE)/2;
                    SDL_FRect objectCollRect = {x + collFix, y + collFix, FIELD_SIZE - 1 - collFix, FIELD_SIZE - 1 - collFix};
                    SDL_SetRenderDrawColor(renderer, 0, 200, 0, 255);
                    SDL_RenderDrawRectF(renderer, &objectCollRect);
                #endif
            }
        }
    }

    #ifdef DRAW_COLLISION_RECT
        SDL_Color textColor = {0, 200, 200, 200};
        SDL_Surface* surfaceMessage;
        SDL_Texture* message;
        SDL_FRect messageRect;
        char msg[20] = "0:0";
        for (int i = 0; i < FIELDS_X * FIELDS_Y; ++i) {
            int ix = i % FIELDS_X;
            int iy = i / FIELDS_X;

            int space = 10;
            messageRect.x = ix*FIELD_SIZE + space;
            messageRect.y = iy*FIELD_SIZE + space;
            messageRect.w = FIELD_SIZE - 1 - space*2;
            messageRect.h =  FIELD_SIZE - 1 - space*2;

            sprintf(msg, "%d:%d", room.fields[i].prevX, room.fields[i].prevY);

            surfaceMessage = TTF_RenderText_Solid(drawFont, msg, textColor);
            message = SDL_CreateTextureFromSurface(renderer, surfaceMessage);
            SDL_RenderCopyF(renderer, message, NULL, &messageRect);
        }
        SDL_DestroyTexture(message);
        SDL_FreeSurface(surfaceMessage);
    #endif
}

void drawPersons(void) {
    SDL_FRect playerDstRect = {player.posX, player.posY, FIELD_SIZE - 1, FIELD_SIZE - 1};
    SDL_Rect playerSrcRect = {0, 0, PLAYER_SRC_SIZE, PLAYER_SRC_SIZE};
    if (mentalHealth == 0) {
        playerSrcRect.x = 4 * PLAYER_SRC_SIZE;
        playerSrcRect.y = 4 * PLAYER_SRC_SIZE;
    }
    SDL_RenderCopyF(renderer, player.texture, &playerSrcRect, &playerDstRect);

    for (int i = 0; i < MANAGERS_COUNT; ++i) {
        if (managers[i] != NULL) {
            SDL_FRect managerDstRect = {managers[i]->posX, managers[i]->posY, FIELD_SIZE - 1, FIELD_SIZE - 1};
            float srcRectPosX = managers[i]->moveCounter % 8 * MANAGER_SRC_SIZE;
            float srcRectPosY = 8 * MANAGER_SRC_SIZE;
            SDL_Rect managerSrcRect = {srcRectPosX, srcRectPosY, MANAGER_SRC_SIZE, MANAGER_SRC_SIZE};
            SDL_RenderCopyF(renderer, managers[i]->texture, &managerSrcRect, &managerDstRect);
        }
    }

    #ifdef DRAW_COLLISION_RECT
        float pCollRatio = PLAYER_COLLISION_RATIO;
        if (pCollRatio > 1.0) pCollRatio = 1.0;
        float collFix = ((1-pCollRatio) * (float)FIELD_SIZE)/2;
        SDL_FRect playerCollRect = {player.posX + collFix, player.posY + collFix, FIELD_SIZE - 1 - collFix, FIELD_SIZE - 1 - collFix};
        SDL_SetRenderDrawColor(renderer, 0, 200, 0, 255);
        SDL_RenderDrawRectF(renderer, &playerCollRect);
    #endif

    // бар индикатора близости
    SDL_Color sdTextColor = {255, 50, 50, 255};
    SDL_FRect sdMessageRect = {0, 0, 128, 24};
    char sdMsg[20] = {0};
    sprintf(sdMsg, " DIST: %04ld ", safetyDist);
    SDL_Surface* sdSurfaceMessage = TTF_RenderText_Solid(drawFont, sdMsg, sdTextColor);
    SDL_Texture* sdMessage = SDL_CreateTextureFromSurface(renderer, sdSurfaceMessage);
    SDL_SetRenderDrawColor(renderer, 0, 0, 0, 100);
    SDL_RenderFillRectF(renderer, &sdMessageRect);
    SDL_RenderCopyF(renderer, sdMessage, NULL, &sdMessageRect);
    SDL_DestroyTexture(sdMessage);
    SDL_FreeSurface(sdSurfaceMessage);

    // бар с процессом кодинга (зависит от близости)
    SDL_Color cpTextColor = {50, 50, 255, 255};
    SDL_FRect cpMessageRect = {WINDOW_WIDTH/2 - 64, 0, 128, 24};
    char cpMsg[20] = {0};
    sprintf(cpMsg, " PROG: %04ld ", codingProgress);
    SDL_Surface* cpSurfaceMessage = TTF_RenderText_Solid(drawFont, cpMsg, cpTextColor);
    SDL_Texture* cpMessage = SDL_CreateTextureFromSurface(renderer, cpSurfaceMessage);
    SDL_SetRenderDrawColor(renderer, 0, 0, 0, 100);
    SDL_RenderFillRectF(renderer, &cpMessageRect);
    SDL_RenderCopyF(renderer, cpMessage, NULL, &cpMessageRect);
    SDL_DestroyTexture(cpMessage);
    SDL_FreeSurface(cpSurfaceMessage);

    // бар с энергией (уменьшается при касании, со временем восстанавливаться)
    SDL_Color mhTextColor = {50, 255, 50, 255};
    SDL_FRect mhMessageRect = {WINDOW_WIDTH - 128, 0, 128, 24};
    char mhMsg[20] = {0};
    sprintf(mhMsg, " HEAL: %04ld ", mentalHealth);
    SDL_Surface* mhSurfaceMessage = TTF_RenderText_Solid(drawFont, mhMsg, mhTextColor);
    SDL_Texture* mhMessage = SDL_CreateTextureFromSurface(renderer, mhSurfaceMessage);
    SDL_SetRenderDrawColor(renderer, 0, 0, 0, 100);
    SDL_RenderFillRectF(renderer, &mhMessageRect);
    SDL_RenderCopyF(renderer, mhMessage, NULL, &mhMessageRect);
    SDL_DestroyTexture(mhMessage);
    SDL_FreeSurface(mhSurfaceMessage);
}

int drawGui(void) {
    int exitState = 0;

    /* GUI */
    if (nk_begin(ctx, "Menu pause", nk_rect(WINDOW_WIDTH * MENU_SKIP, WINDOW_HEIGHT * MENU_SKIP, WINDOW_WIDTH * (1 - 2 * MENU_SKIP), WINDOW_HEIGHT * (1 - 2 * MENU_SKIP)),
        NK_WINDOW_BORDER|NK_WINDOW_MOVABLE|
        NK_WINDOW_TITLE))
    {
        // nk_layout_row_dynamic(ctx, 20, 1);
        // nk_label(ctx, "background:", NK_TEXT_LEFT);
        // nk_layout_row_dynamic(ctx, 25, 1);
        // if (nk_combo_begin_color(ctx, nk_rgb_cf(bg), nk_vec2(nk_widget_width(ctx),400))) {
        // 	nk_layout_row_dynamic(ctx, 120, 1);
        // 	bg = nk_color_picker(ctx, bg, NK_RGBA);
        // 	nk_layout_row_dynamic(ctx, 25, 1);
        // 	bg.r = nk_propertyf(ctx, "#R:", 0, bg.r, 1.0f, 0.01f,0.005f);
        // 	bg.g = nk_propertyf(ctx, "#G:", 0, bg.g, 1.0f, 0.01f,0.005f);
        // 	bg.b = nk_propertyf(ctx, "#B:", 0, bg.b, 1.0f, 0.01f,0.005f);
        // 	bg.a = nk_propertyf(ctx, "#A:", 0, bg.a, 1.0f, 0.01f,0.005f);
        // 	nk_combo_end(ctx);
        // }

        /* custom widget pixel width */
        // nk_layout_row_begin(ctx, NK_DYNAMIC, 30, 3);
        // {
        //     nk_layout_row_push(ctx, 0.3);
        //     nk_label(ctx, "Manager speed:", NK_TEXT_RIGHT);
        //     nk_layout_row_push(ctx, 0.4);
        //     nk_slider_int(ctx, 1, &(manager.speed), 20, 1);
        // }
        // nk_layout_row_end(ctx);

        /* fixed widget pixel width */
        nk_layout_row_dynamic(ctx, 30, 1);
        if (nk_button_label(ctx, "New Game")) {
            randomizeRoomFields();

            player.posX = WINDOW_WIDTH / 2;
            player.posY = WINDOW_HEIGHT / 2;

            for (int i = 1; i < MANAGERS_COUNT; ++i) {
                if (managers[i] != NULL) {
                    SDL_DestroyTexture(managers[i]->texture);
                    free(managers[i]);
                    managers[i] = NULL;
                }
            }
            randomManagerPosition(managers[0]);

            gui_menu_pause = false;
        }

        nk_layout_row_dynamic(ctx, 30, 1);
        if (nk_button_label(ctx, "Exit")) {
            exitState = 1;
        }

    }
    nk_end(ctx);

    return exitState;
}

int processInputEvents(void) {
    /* Input */
    SDL_Event event;

    nk_input_begin(ctx);

    while (SDL_PollEvent(&event)) {
        switch (event.type) {
        case SDL_QUIT:
            return 1;
            break;
        case SDL_KEYDOWN:
            switch (event.key.keysym.sym) {
            case SDLK_ESCAPE:
                gui_menu_pause = !gui_menu_pause;
                break;
            };
            break;
        }

        nk_sdl_handle_event(&event);
    }

    const Uint8 *keys = SDL_GetKeyboardState(NULL);
    player.dirX = 0;
    player.dirY = 0;
    if (mentalHealth > 0) {
        if (keys[SDL_SCANCODE_LEFT])
            player.dirX--;
        if (keys[SDL_SCANCODE_RIGHT])
            player.dirX++;
        if (keys[SDL_SCANCODE_UP])
            player.dirY--;
        if (keys[SDL_SCANCODE_DOWN])
            player.dirY++;
    }

    nk_input_end(ctx);

    return 0;
}

void calcFieldWeight(int curX, int curY, int weight) {
    if (curX > 0) {
        if ((room.fields[curY * FIELDS_X + (curX - 1)].flags & FIELD_TYPE_DESK) == 0 &&
            room.fields[curY * FIELDS_X + (curX - 1)].weight > weight + 1
        ) {
            room.fields[curY * FIELDS_X + (curX - 1)].prevX = curX;
            room.fields[curY * FIELDS_X + (curX - 1)].prevY = curY;
            room.fields[curY * FIELDS_X + (curX - 1)].weight = weight + 1;
            calcFieldWeight(curX - 1, curY, weight + 1);
        }
    }
    if (curX + 1 < FIELDS_X) {
        if ((room.fields[curY * FIELDS_X + (curX + 1)].flags & FIELD_TYPE_DESK) == 0 &&
            room.fields[curY * FIELDS_X + (curX + 1)].weight > weight + 1
        ) {
            room.fields[curY * FIELDS_X + (curX + 1)].prevX = curX;
            room.fields[curY * FIELDS_X + (curX + 1)].prevY = curY;
            room.fields[curY * FIELDS_X + (curX + 1)].weight = weight + 1;
            calcFieldWeight(curX + 1, curY, weight + 1);
        }
    }
    if (curY > 0) {
        if ((room.fields[(curY - 1) * FIELDS_X + curX].flags & FIELD_TYPE_DESK) == 0 &&
            room.fields[(curY - 1) * FIELDS_X + curX].weight > weight + 1
        ) {
            room.fields[(curY - 1) * FIELDS_X + curX].prevX = curX;
            room.fields[(curY - 1) * FIELDS_X + curX].prevY = curY;
            room.fields[(curY - 1) * FIELDS_X + curX].weight = weight + 1;
            calcFieldWeight(curX, curY - 1, weight + 1);
        }
    }
    if (curY + 1 < FIELDS_Y) {
        if ((room.fields[(curY + 1) * FIELDS_X + curX].flags & FIELD_TYPE_DESK) == 0 &&
            room.fields[(curY + 1) * FIELDS_X + curX].weight > weight + 1
        ) {
            room.fields[(curY + 1) * FIELDS_X + curX].prevX = curX;
            room.fields[(curY + 1) * FIELDS_X + curX].prevY = curY;
            room.fields[(curY + 1) * FIELDS_X + curX].weight = weight + 1;
            calcFieldWeight(curX, curY + 1, weight + 1);
        }
    }
}

void calcPathRoutes(void) {
    static int calculatedPosX = -1;
    static int calculatedPosY = -1;

    int curFieldXCentered = (player.posX + (FIELD_SIZE-1)/2) / FIELD_SIZE;
    int curFieldYCentered = (player.posY + (FIELD_SIZE-1)/2) / FIELD_SIZE;

    if (calculatedPosX != curFieldXCentered || calculatedPosY != curFieldYCentered) {
        for (int i = 0; i < FIELDS_X * FIELDS_Y; ++i) {
            room.fields[i].weight = INT8_MAX;
            room.fields[i].prevX = -1;
            room.fields[i].prevY = -1;
        }
        room.fields[curFieldYCentered * FIELDS_X + curFieldXCentered].weight = 0;
        calcFieldWeight(curFieldXCentered, curFieldYCentered, 0);
        calculatedPosX = curFieldXCentered;
        calculatedPosY = curFieldYCentered;
    }
}

int main() {
    srand(time(0));

    int err = initPlatform();
    if (err != 0) {
        exit(err);
    }

    err = initWorld();
    if (err != 0) {
        exit(err);
    }

    prevMoveTime = SDL_GetTicks64();

    bool quit = false;

    while (!quit) {
        if ((quit = processInputEvents()) != false) {
            goto cleanup;
        }

		if (gui_menu_pause) {
			quit = drawGui();
        } else {
            calcMovement();
        }

        SDL_SetRenderDrawColor(renderer, bg.r * 255, bg.g * 255, bg.b * 255, bg.a * 255);
        SDL_RenderClear(renderer);

        drawRoom();
        drawPersons();

        nk_sdl_render(NK_ANTI_ALIASING_ON);

        SDL_RenderPresent(renderer);
    }

cleanup:
    nk_sdl_shutdown();
    SDL_DestroyTexture(player.texture);
    for (int i = 0; i < MANAGERS_COUNT; ++i) {
        if (managers[i] != NULL) {
            SDL_DestroyTexture(managers[i]->texture);
            free(managers[i]);
        }
    }
    SDL_DestroyRenderer(renderer);
    SDL_DestroyWindow(window);
    SDL_Quit();
    IMG_Quit();
    TTF_Quit();

    free(room.fields);

    return EXIT_SUCCESS;
}

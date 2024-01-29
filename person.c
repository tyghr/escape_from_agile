#define _GNU_SOURCE

#include <stdio.h>
#include <stdlib.h>
#include <stdbool.h>
#include <time.h>
#include <string.h>

#include <SDL2/SDL.h>

#include "game.h"

extern tileset room;
extern person player;
extern person *managers[MANAGERS_COUNT];
extern Uint64 prevMoveTime;
extern uint64_t safetyDist;
extern uint64_t codingProgress;
extern uint64_t mentalHealth;

// bool calcCollision(person *p, float x2, float y2);
// float calcPersonCollSize(person *p, int dir);
// bool calcPersonMovement(person *p);

void randomManagerPosition(person *m) {
    switch (rand()%4) {
    case 0:
        m->posX = (rand() % (FIELDS_X -1)) * FIELD_SIZE;
        m->posY = 0;
        break;
    case 1:
        m->posX = 0;
        m->posY = (rand() % (FIELDS_Y -1)) * FIELD_SIZE;
        break;
    case 2:
        m->posX = (rand() % (FIELDS_X -1)) * FIELD_SIZE;
        m->posY = (WINDOW_HEIGHT/FIELD_SIZE - 1) * FIELD_SIZE;
        break;
    case 3:
        m->posX = (WINDOW_WIDTH/FIELD_SIZE - 1) * FIELD_SIZE;
        m->posY = (rand() % (FIELDS_Y -1)) * FIELD_SIZE;
        break;
    }
}

bool calcPPCollision(person *p1, person *p2) {
    if(p1->posX + FIELD_SIZE-1 < p2->posX || p1->posX > p2->posX + FIELD_SIZE-1) return false;
    if(p1->posY + FIELD_SIZE-1 < p2->posY || p1->posY > p2->posY + FIELD_SIZE-1) return false;
    return true;
}

// float calcPersonCollSize(person *p, int dir) {
//     return (float)FIELD_SIZE/2 * (1.0 + (float)dir * p->collisionRatio);
// }

bool calcPersonMovement(person *p) {
    if (p->dirX) p->dirX /= fabs(p->dirX);
    if (p->dirY) p->dirY /= fabs(p->dirY);
    float personDirRatio = (p->dirX && p->dirY) ? 0.7 : 1.0 ;

    float pCollRatio = p->collisionRatio;
    if (pCollRatio > 1.0) pCollRatio = 1.0;
    float pCollFix = ((1-pCollRatio) * (float)FIELD_SIZE)/2;

    // float oCollRatio = OBJECT_COLLISION_RATIO;
    // if (oCollRatio > 1.0) oCollRatio = 1.0;
    // float oCollFix = ((1-oCollRatio) * (float)FIELD_SIZE)/2;

    float prevRectPosX = p->posX;
    float prevRectPosY = p->posY;
    float newRectPosX = (float)p->posX + (float)p->dirX * personDirRatio * (float)p->speed;
    float newRectPosY = (float)p->posY + (float)p->dirY * personDirRatio * (float)p->speed;

    int prevX1 = (int)(prevRectPosX + pCollFix) / FIELD_SIZE;
    int prevX2 = (int)(prevRectPosX + FIELD_SIZE-1 - pCollFix) / FIELD_SIZE;
    int prevY1 = (int)(prevRectPosY + pCollFix) / FIELD_SIZE;
    int prevY2 = (int)(prevRectPosY + FIELD_SIZE-1 - pCollFix) / FIELD_SIZE;

    int newX1 = (int)(newRectPosX + pCollFix) / FIELD_SIZE;
    int newX2 = (int)(newRectPosX + FIELD_SIZE-1 - pCollFix) / FIELD_SIZE;
    int newY1 = (int)(newRectPosY + pCollFix) / FIELD_SIZE;
    int newY2 = (int)(newRectPosY + FIELD_SIZE-1 - pCollFix) / FIELD_SIZE;

    int p1Coll = getFieldTypeByCoord(newRectPosX + pCollFix, newRectPosY + pCollFix) & (FIELD_TYPE_DESK);
    int p2Coll = getFieldTypeByCoord(newRectPosX + FIELD_SIZE-1 - pCollFix, newRectPosY + pCollFix) & (FIELD_TYPE_DESK);
    int p3Coll = getFieldTypeByCoord(newRectPosX + pCollFix, newRectPosY + FIELD_SIZE-1 - pCollFix) & (FIELD_TYPE_DESK);
    int p4Coll = getFieldTypeByCoord(newRectPosX + FIELD_SIZE-1 - pCollFix, newRectPosY + FIELD_SIZE-1 - pCollFix) & (FIELD_TYPE_DESK);

    if (p1Coll) {
        if (getFieldTypeByID(prevX1, prevY1-1) & (FIELD_TYPE_DESK) && (newY1 < prevY1)) {
            newRectPosY = (float)prevY1 * FIELD_SIZE - pCollFix;
        }
        if (getFieldTypeByID(prevX1-1, prevY1) & (FIELD_TYPE_DESK) && (newX1 < prevX1)) {
            newRectPosX = (float)prevX1 * FIELD_SIZE - pCollFix;
        }
        if (getFieldTypeByID(prevX1-1, prevY1-1) & (FIELD_TYPE_DESK) && (newX1 < prevX1) && (newY1 < prevY1)) {
            newRectPosX = (float)prevX1 * FIELD_SIZE - pCollFix;
            newRectPosY = (float)prevY1 * FIELD_SIZE - pCollFix;
        }
        if (getFieldTypeByID(prevX1-1, prevY1+1) & (FIELD_TYPE_DESK) && (newX1 < prevX1) && (newY1 > prevY1)) {
            newRectPosX = (float)(prevX1) * FIELD_SIZE - pCollFix;
            newRectPosY = (float)(prevY1 + 1) * FIELD_SIZE - pCollFix;
        }
        if (getFieldTypeByID(prevX1+1, prevY1-1) & (FIELD_TYPE_DESK) && (newX1 > prevX1) && (newY1 < prevY1)) {
            newRectPosX = (float)(prevX1 + 1) * FIELD_SIZE - pCollFix;
            newRectPosY = (float)(prevY1) * FIELD_SIZE - pCollFix;
        }
    }
    if (p2Coll) {
        if (getFieldTypeByID(prevX2, prevY1-1) & (FIELD_TYPE_DESK) && (newY1 < prevY1)) {
            newRectPosY = (float)prevY1 * FIELD_SIZE - pCollFix;
        }
        if (getFieldTypeByID(prevX2+1, prevY1) & (FIELD_TYPE_DESK) && (newX2 > prevX2)) {
            newRectPosX = (float)prevX2 * FIELD_SIZE + pCollFix;
        }
        if (getFieldTypeByID(prevX2+1, prevY1-1) & (FIELD_TYPE_DESK) && (newX2 > prevX2) && (newY1 < prevY1)) {
            newRectPosX = (float)prevX2 * FIELD_SIZE + pCollFix;
            newRectPosY = (float)prevY1 * FIELD_SIZE - pCollFix;
        }
        if (getFieldTypeByID(prevX2-1, prevY1-1) & (FIELD_TYPE_DESK) && (newX2 < prevX2) && (newY1 < prevY1)) {
            newRectPosX = (float)(prevX2-1) * FIELD_SIZE + pCollFix;
            newRectPosY = (float)prevY1 * FIELD_SIZE - pCollFix;
        }
        if (getFieldTypeByID(prevX2+1, prevY1+1) & (FIELD_TYPE_DESK) && (newX2 > prevX2) && (newY1 > prevY1)) {
            newRectPosX = (float)prevX2 * FIELD_SIZE + pCollFix;
            newRectPosY = (float)(prevY1+1) * FIELD_SIZE - pCollFix;
        }
    }
    if (p3Coll) {
        if (getFieldTypeByID(prevX1, prevY2+1) & (FIELD_TYPE_DESK) && (newY2 > prevY2)) {
            newRectPosY = (float)prevY2 * FIELD_SIZE + pCollFix;
        }
        if (getFieldTypeByID(prevX1-1, prevY2) & (FIELD_TYPE_DESK) && (newX1 < prevX1)) {
            newRectPosX = (float)prevX1 * FIELD_SIZE - pCollFix;
        }
        if (getFieldTypeByID(prevX1-1, prevY2+1) & (FIELD_TYPE_DESK) && (newX1 < prevX1) && (newY2 > prevY2)) {
            newRectPosX = (float)prevX1 * FIELD_SIZE - pCollFix;
            newRectPosY = (float)prevY2 * FIELD_SIZE + pCollFix;
        }
        if (getFieldTypeByID(prevX1+1, prevY2+1) & (FIELD_TYPE_DESK) && (newX1 > prevX1) && (newY2 > prevY2)) {
            newRectPosX = (float)(prevX1+1) * FIELD_SIZE - pCollFix;
            newRectPosY = (float)prevY2 * FIELD_SIZE + pCollFix;
        }
        if (getFieldTypeByID(prevX1-1, prevY2-1) & (FIELD_TYPE_DESK) && (newX1 < prevX1) && (newY2 < prevY2)) {
            newRectPosX = (float)prevX1 * FIELD_SIZE - pCollFix;
            newRectPosY = (float)(prevY2-1) * FIELD_SIZE + pCollFix;
        }
    }
    if (p4Coll) {
        if (getFieldTypeByID(prevX2, prevY2+1) & (FIELD_TYPE_DESK) && (newY2 > prevY2)) {
            newRectPosY = (float)prevY2 * FIELD_SIZE + pCollFix;
        }
        if (getFieldTypeByID(prevX2+1, prevY2) & (FIELD_TYPE_DESK) && (newX2 > prevX2)) {
            newRectPosX = (float)prevX2 * FIELD_SIZE + pCollFix;
        }
        if (getFieldTypeByID(prevX2+1, prevY2+1) & (FIELD_TYPE_DESK) && (newX2 > prevX2) && (newY2 > prevY2)) {
            newRectPosX = (float)prevX2 * FIELD_SIZE + pCollFix;
            newRectPosY = (float)prevY2 * FIELD_SIZE + pCollFix;
        }
        if (getFieldTypeByID(prevX2-1, prevY2+1) & (FIELD_TYPE_DESK) && (newX2 < prevX2) && (newY2 > prevY2)) {
            newRectPosX = (float)(prevX2-1) * FIELD_SIZE + pCollFix;
            newRectPosY = (float)prevY2 * FIELD_SIZE + pCollFix;
        }
        if (getFieldTypeByID(prevX2+1, prevY2-1) & (FIELD_TYPE_DESK) && (newX2 > prevX2) && (newY2 < prevY2)) {
            newRectPosX = (float)prevX2 * FIELD_SIZE + pCollFix;
            newRectPosY = (float)(prevY2-1) * FIELD_SIZE + pCollFix;
        }
    }

    if (newRectPosX + pCollFix < 0) newRectPosX = - pCollFix;
    if (newRectPosX > WINDOW_WIDTH - FIELD_SIZE + pCollFix) newRectPosX = WINDOW_WIDTH - FIELD_SIZE + pCollFix;
    if (newRectPosY + pCollFix < 0) newRectPosY =  - pCollFix;
    if (newRectPosY > WINDOW_HEIGHT - FIELD_SIZE + pCollFix) newRectPosY = WINDOW_HEIGHT - FIELD_SIZE + pCollFix;

    p->posX = newRectPosX;
    p->posY = newRectPosY;

    return newRectPosX == prevRectPosX && newRectPosY == prevRectPosY;
}

int calcMovement(void) {
    /* calc movement */

    calcPersonMovement(&player);

    Uint64 currentTime = SDL_GetTicks64();
    if ((currentTime - prevMoveTime) >= MS_PER_MOVE) {
        prevMoveTime = currentTime;

        calcPathRoutes();

        player.moveCounter++;
        if (player.moveCounter % MANAGERS_APPEAR_PERIOD == 0) {
            initNewManager();
        }

        float fSafetyDist2 = WINDOW_WIDTH*WINDOW_WIDTH + WINDOW_HEIGHT*WINDOW_WIDTH;

        for (int i = 0; i < MANAGERS_COUNT; ++i) {
            if (managers[i] != NULL) {
                if (calcPPCollision(&player,managers[i])) {
                    managers[i]->dirX = 0;
                    managers[i]->dirY = 0;
                } else {
                    float curX = managers[i]->posX;
                    float curY = managers[i]->posY;

                    int curFieldX1 = (int)(curX) / FIELD_SIZE;
                    int curFieldX2 = (int)(curX + FIELD_SIZE-1) / FIELD_SIZE;
                    int curFieldY1 = (int)(curY) / FIELD_SIZE;
                    int curFieldY2 = (int)(curY + FIELD_SIZE-1) / FIELD_SIZE;

                    int curFieldXCentered = (managers[i]->posX + (FIELD_SIZE-1)/2) / FIELD_SIZE;
                    int curFieldYCentered = (managers[i]->posY + (FIELD_SIZE-1)/2) / FIELD_SIZE;

                    if (managers[i]->dirX == 0 && managers[i]->dirY == 0) {
                        managers[i]->dirX = room.fields[curFieldYCentered * FIELDS_X + curFieldXCentered].prevX - curFieldXCentered;
                        managers[i]->dirY = room.fields[curFieldYCentered * FIELDS_X + curFieldXCentered].prevY - curFieldYCentered;
                    }
                    if (managers[i]->dirX < 0) {
                        managers[i]->dirX = room.fields[curFieldYCentered * FIELDS_X + curFieldX2].prevX - curFieldX2;
                    } else if (managers[i]->dirX > 0) {
                        managers[i]->dirX = room.fields[curFieldYCentered * FIELDS_X + curFieldX1].prevX - curFieldX1;
                    } else if (managers[i]->dirY < 0) {
                        managers[i]->dirY = room.fields[curFieldY2 * FIELDS_X + curFieldXCentered].prevY - curFieldY2;
                    } else if (managers[i]->dirY > 0) {
                        managers[i]->dirY = room.fields[curFieldY1 * FIELDS_X + curFieldXCentered].prevY - curFieldY1;
                    }

                    calcPersonMovement(managers[i]);
                }

                float d2 = (managers[i]->posX - player.posX)*(managers[i]->posX - player.posX) + (managers[i]->posY - player.posY)*(managers[i]->posY - player.posY);
                if (d2 < fSafetyDist2)
                    fSafetyDist2 = d2;

                managers[i]->moveCounter++;
                if (managers[i]->moveCounter % MANAGER_SPEED_INCREASE_PERIOD == 0)
                    managers[i]->speed += MANAGER_SPEED_INCREASE_RATIO;
            }
        }

        safetyDist = (int)sqrt(fSafetyDist2);
        if (!player.dirX && !player.dirY) {
            // бар с процессом кодинга (зависит от близости)
            int sF;
            if ((sF = safetyDist/FIELD_SIZE) < 4) {
                codingProgress += 10/4 * sF;
            } else {
                codingProgress += 10;
                if (mentalHealth < 100)
                    mentalHealth++;
            }
        }
        if (safetyDist < FIELD_SIZE*1.5) {
            // бар с энергией (уменьшается при касании, со временем восстанавливаться)
            if (mentalHealth > 0)
                mentalHealth--;
        }
    }

    return 0;
}

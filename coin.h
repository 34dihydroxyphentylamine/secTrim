#pragma once
#ifndef COIN_H
#define COIN_H

#include <SDL.h>
#include <vector>   
#include <string>   

bool initCoinSystem(SDL_Renderer* renderer);

void draw_Coin(int x, int y, float scale, SDL_Renderer* renderer, Uint32 currentTime);

int getCoinRenderedWidth(float scale);

int getCoinRenderedHeight(float scale);

void closeCoinSystem();

#endif
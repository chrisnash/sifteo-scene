/*
 * public.h
 *
 *  Created on: Apr 24, 2013
 *      Author: chrisnash
 */

#ifndef PUBLIC_H_
#define PUBLIC_H_

#ifndef PUBLIC
#define PUBLIC extern
#endif

#include <sifteo.h>
#include <scene.h>

// type definitions
const uint8_t TYPE_BOARD = 0;
const uint8_t TYPE_BALL = 1;
const uint8_t TYPE_BAT = 2;
const uint8_t TYPE_SCORE = 3;
const uint8_t TYPE_SPLASH = 4;

const uint8_t MODE_MAIN	= 0;
const uint8_t MODE_SPLASH = 1;

// physics parameters
const float ballRadius = 2.0;
const float batWidth = 4.0;
const float batLength = 16.0;
const float ballVelocity = 2.0;
const float wallWidth = 4.0;
const float batVelocity = 1.0;
const float batCloseness = 4.0;
const float vision = 100.0;

const uint8_t SERVE_TIME = 120;
const uint8_t SPLASH_TIME = 240;
const uint8_t MAX_SCORE = 9;

const int SCORE_1_X = 12;
const int SCORE_2_X = 1;
const int SCORE_Y = 1;
const int SCORE_XS = 3;
const int SCORE_YS = 5;

// element group pointers (2 elements per group)
PUBLIC Scene::Element *pBall;
PUBLIC Scene::Element *pBats;
PUBLIC Scene::Element *pScores;
PUBLIC Scene::Element *pSplashes;

// RNG for the entire game
PUBLIC Sifteo::Random rng;

#endif /* PUBLIC_H_ */

/*
 * wheel_speed.c
 *
 *  Created on: Apr 17, 2024
 *      Author: leoja
 */


#include "wheel_speed.h"

void WheelSpeed_Init(WheelSpeed_t* ws)
{
	ws->last_count = 0;
	ws->count = 0;
	ws->last_tick = HAL_GetTick();
}

uint32_t WheelSpeed_GetCPS(WheelSpeed_t* ws)
{
	uint32_t tick = HAL_GetTick();

	uint32_t cps = 1000 * (ws->count - ws->last_count) / (tick - ws->last_tick);

	ws->last_count = ws->count;
	ws->last_tick = tick;

	return cps;
}

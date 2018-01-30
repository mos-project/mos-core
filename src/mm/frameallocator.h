/*
 * mm.h
 *
 *  Created on: 28 jan. 2018
 *      Author: micke
 */

#pragma once

#include "basic_types.h"

void FrameInit(void* memstart, u32 memsize);
void* FrameAlloc(u32 size);
bool FrameFree(void* ptr);
void FrameCleanup(u32& remainingAllocations);

#pragma once 
#include <stdio.h>
#include <stdint.h>
#include <stdbool.h>
#include <string.h>

typedef struct {
    uint16_t ail;
    uint16_t elv;
    uint8_t  thr;
    uint16_t    bat;
} DataFrameData;

extern DataFrameData lastDFrame;

void dataTask();
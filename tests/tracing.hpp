#pragma once

#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <string.h>

#define TRACE(fmt, ...) printf(fmt, ##__VA_ARGS__)
#define TRACE_ERROR(fmt, ...) fprintf(stderr, "Error: " fmt, ##__VA_ARGS__)

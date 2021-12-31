#pragma once
#define INTERNALSCOPE static
#define APP_CLAMP(value, min, max) (value < min) ? min : (value > max) ? max : value;
#include <assert.h>
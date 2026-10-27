#pragma once
#include "configs.h"
#ifdef MARAUDER_M5PAPER
#include "M5PaperPower.h"
namespace epaper = m5paper;
#elif defined(MARAUDER_RETERMINAL_STICKY)
#include "StickyPower.h"
namespace epaper = sticky;
#endif

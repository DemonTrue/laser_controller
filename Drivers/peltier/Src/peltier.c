#include "peltier.h"
#include "common.h"

void peltier_init(peltier_t* peltier)
{
	peltier->power = START_PELTIER_POWER;
}


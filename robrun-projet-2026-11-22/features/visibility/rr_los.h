/* rr_los.h - Lifecycle and three-state line-of-sight query interface. */
#ifndef RR_LOS_H
#define RR_LOS_H
#include "game/rr_game.h"
#include "rr_los_geom.h"
#ifdef __cplusplus
extern "C" {
#endif
/* Reader-owned background worker; returns UNKNOWN while snapshots are stale. */
int rr_los_start(void);
void rr_los_stop(void);
int rr_los_check(RR_V3 start, RR_V3 end);
#ifdef __cplusplus
}
#endif
#endif

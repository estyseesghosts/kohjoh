#ifndef KJ_DIAGNOSTICS_DUMP_H
#define KJ_DIAGNOSTICS_DUMP_H

#include "runtime/kj_bridge.h"

#include <stdbool.h>

bool kj_dump_frame(const char *path, const kj_frame_input *in);

#endif

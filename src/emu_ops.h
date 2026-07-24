#ifndef EMU_OPS_H
#define EMU_OPS_H

#include "emu.h"
#include "decoder.h"

int emu_execute(CPU *cpu, jd_decoded_inst_t *inst, uint64_t next_rip);

#endif // EMU_OPS_H

#ifndef FORMATTER_H
#define FORMATTER_H

#include "decoder.h"
#include <stdint.h>
#include <stddef.h>

void format_instruction(const jd_decoded_inst_t *inst, uint64_t address, char *out_str, size_t out_size);

#endif // FORMATTER_H

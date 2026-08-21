#ifndef EDGE_POINT_UTIL_H
#define EDGE_POINT_UTIL_H

#include <stdint.h>

int edge_dblink_parse(const char *hex_str, uint32_t *out_id);
uint8_t edge_dblink_channel(uint32_t id);

#endif /* EDGE_POINT_UTIL_H */

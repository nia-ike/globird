#ifndef EDGE_POINT_WRITE_H
#define EDGE_POINT_WRITE_H

#include <stdint.h>

int edge_point_is_virtual_channel(uint8_t chnl_id);
int edge_point_read(uint32_t id, double *val, int ctrl_only);
int edge_point_write(uint32_t id, double val, int is_ctrl);

#endif /* EDGE_POINT_WRITE_H */

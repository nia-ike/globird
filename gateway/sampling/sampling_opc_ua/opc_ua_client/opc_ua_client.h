#ifndef __OPC_UA_CLIENT_H__
#define __OPC_UA_CLIENT_H__

#include <open62541/client.h>
#include <stdint.h>
#include <stdbool.h>

#include "opc_ua_cfg.h"

UA_NodeId opc_ua_nodeid_from_cfg(uint16_t ns, int32_t n, const char *s);
bool opc_ua_nodeid_is_valid(uint16_t ns, int32_t n, const char *s);

UA_Client *opc_ua_client_create(void);
void opc_ua_client_destroy(UA_Client *client);

UA_StatusCode opc_ua_client_connect(UA_Client *client,
                                  const char *endpoint_url,
                                  const char *username,
                                  const char *password);

void opc_ua_client_disconnect(UA_Client *client);

UA_StatusCode opc_ua_client_read_double(UA_Client *client,
                                        uint16_t ns, int32_t n, const char *s,
                                        double *value);

UA_StatusCode opc_ua_client_write_value(UA_Client *client,
                                        uint16_t ns, int32_t n, const char *s,
                                        uint8_t datattr, double value);

#endif

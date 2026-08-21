#ifndef __OPC_UA_VALUE_H__
#define __OPC_UA_VALUE_H__

#include <open62541/types.h>
#include <stdbool.h>

bool opc_ua_variant_to_double(const UA_Variant *variant, double *out);
bool opc_ua_double_to_variant(double value, uint8_t datattr, UA_Variant *variant);

#endif

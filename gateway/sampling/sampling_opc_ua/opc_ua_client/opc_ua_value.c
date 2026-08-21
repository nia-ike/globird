#include "opc_ua_value.h"

#include <open62541/client.h>
#include <string.h>

/*********************************************************************************************************
** Function name:       opc_ua_variant_to_double
** Descriptions:        将 OPC UA Variant 标量转换为 double，供实时库写入
** input parameters:    variant  OPC UA 读取结果
** output parameters:   out      转换后的浮点值
** Returned value:      true 转换成功，false 类型不支持
*********************************************************************************************************/
bool opc_ua_variant_to_double(const UA_Variant *variant, double *out)
{
    UA_Byte kind;

    if (variant == NULL || out == NULL || variant->data == NULL || variant->type == NULL)
        return false;

    kind = variant->type->typeKind;

    switch (kind) {
    case UA_DATATYPEKIND_BOOLEAN:
        *out = *(UA_Boolean *)variant->data ? 1.0 : 0.0;
        return true;
    case UA_DATATYPEKIND_SBYTE:
        *out = (double)*(UA_SByte *)variant->data;
        return true;
    case UA_DATATYPEKIND_BYTE:
        *out = (double)*(UA_Byte *)variant->data;
        return true;
    case UA_DATATYPEKIND_INT16:
        *out = (double)*(UA_Int16 *)variant->data;
        return true;
    case UA_DATATYPEKIND_UINT16:
        *out = (double)*(UA_UInt16 *)variant->data;
        return true;
    case UA_DATATYPEKIND_INT32:
        *out = (double)*(UA_Int32 *)variant->data;
        return true;
    case UA_DATATYPEKIND_UINT32:
        *out = (double)*(UA_UInt32 *)variant->data;
        return true;
    case UA_DATATYPEKIND_INT64:
        *out = (double)*(UA_Int64 *)variant->data;
        return true;
    case UA_DATATYPEKIND_UINT64:
        *out = (double)*(UA_UInt64 *)variant->data;
        return true;
    case UA_DATATYPEKIND_FLOAT:
        *out = (double)*(UA_Float *)variant->data;
        return true;
    case UA_DATATYPEKIND_DOUBLE:
        *out = *(UA_Double *)variant->data;
        return true;
    default:
        return false;
    }
}

/*********************************************************************************************************
** Function name:       opc_ua_double_to_variant
** Descriptions:        将 double 按五遥类型封装为 OPC UA Variant，用于写节点
** input parameters:    value    待写入数值
**                      datattr  五遥类型（3=DO 布尔，其它按 double）
** output parameters:   variant  输出 Variant（调用方负责 UA_Variant_clear）
** Returned value:      true 成功，false 失败
*********************************************************************************************************/
bool opc_ua_double_to_variant(double value, uint8_t datattr, UA_Variant *variant)
{
    UA_Variant_init(variant);

    if (datattr == 3) {
        UA_Boolean bVal = (value != 0.0) ? true : false;
        return UA_Variant_setScalarCopy(variant, &bVal, &UA_TYPES[UA_TYPES_BOOLEAN]) == UA_STATUSCODE_GOOD;
    }

    {
        UA_Double dVal = value;
        return UA_Variant_setScalarCopy(variant, &dVal, &UA_TYPES[UA_TYPES_DOUBLE]) == UA_STATUSCODE_GOOD;
    }
}

#include "iec101-utils.h"
#include <stdlib.h>

uint8_t CheckSum8(uint8_t *pBuff, int nLen)
{// 计算8位校验和
    uint8_t byReturn = 0x00;

    for (int i = 0; i < nLen; i++)
    {
        byReturn += pBuff[i];
    }

    return (uint8_t)byReturn;
}

int json_get_int(const cJSON * const pObject, const char * const key, int defaultVal, bool *succeed)
{
    if (pObject == NULL)
    {
        *succeed = false;
        return defaultVal;
    }

    cJSON *pItem = cJSON_GetObjectItem(pObject, key);
    if(pItem == NULL)
    {
        *succeed = false;
        return defaultVal;
    }
    
    *succeed = true;
    if (cJSON_IsNumber(pItem)) return pItem->valueint;
    else if (cJSON_IsString(pItem)) return atoi(pItem->valuestring);
    else if (cJSON_IsTrue(pItem)) return 1;
    else if (cJSON_IsFalse(pItem)) return 0;
    
    *succeed = false;
    return defaultVal;
}

float json_get_float(const cJSON * const pObject, const char * const key, float defaultVal, bool *succeed)
{
    if (pObject == NULL)
    {
        *succeed = false;
        return defaultVal;
    }

    cJSON *pItem = cJSON_GetObjectItem(pObject, key);
    if(pItem == NULL)
    {
        *succeed = false;
        return defaultVal;
    }
    
    *succeed = true;
    if (cJSON_IsNumber(pItem)) return (float)pItem->valuedouble;
    else if (cJSON_IsString(pItem)) return (float)atof(pItem->valuestring);
    else if (cJSON_IsTrue(pItem)) return 1;
    else if (cJSON_IsFalse(pItem)) return 0;
    
    *succeed = false;
    return false;
}

double json_get_double(const cJSON * const pObject, const char * const key, double defaultVal, bool *succeed)
{
    if (pObject == NULL)
    {
        *succeed = false;
        return defaultVal;
    }

    cJSON *pItem = cJSON_GetObjectItem(pObject, key);
    if(pItem == NULL)
    {
        *succeed = false;
        return defaultVal;
    }
    
    *succeed = true;
    if (cJSON_IsNumber(pItem)) return pItem->valuedouble;
    else if (cJSON_IsString(pItem)) return atof(pItem->valuestring);
    else if (cJSON_IsTrue(pItem)) return 1;
    else if (cJSON_IsFalse(pItem)) return 0;
    
    *succeed = false;
    return false;
}

bool json_get_bool(const cJSON * const pObject, const char * const key, bool defaultVal, bool *succeed)
{
    if (pObject == NULL)
    {
        *succeed = false;
        return defaultVal;
    }

    cJSON *pItem = cJSON_GetObjectItem(pObject, key);
    if(pItem == NULL)
    {
        *succeed = false;
        return defaultVal;
    }
    
    *succeed = true;
    if (cJSON_IsNumber(pItem)) return pItem->valueint != 0;
    else if (cJSON_IsTrue(pItem)) return true;
    else if (cJSON_IsFalse(pItem)) return false;
    else if (cJSON_IsString(pItem))
    {
        if (pItem->valuestring[0] == '1') return true;
        if (pItem->valuestring[0] == '0') return false;
    }
    
    *succeed = false;
    return false;
}

#include <stdio.h>
#include <string.h>
#include<unistd.h>
#include <stdlib.h>
#include <sys/types.h> 
#include <unistd.h> 
#include <sys/stat.h>  //stat
#include <stdbool.h>
#include <time.h>
#include <sys/time.h>
#include "mb_cfg.h"

static struct system_info
{
    uint32_t    unix_time;
    uint16_t    timezone;
};

struct system_info   systeminfo;

#define  TIMEZONE_TZ_PATH  "  /etc/TZ"

typedef enum {
    UTC_0_00 = 0,
    UTC_1_00  ,
    UTC_2_00  ,
    UTC_3_00  ,
    UTC_3_30  ,
    UTC_4_00  ,
    UTC_4_30  ,
    UTC_5_00  ,
    UTC_5_30  ,
    UTC_5_45  ,
    UTC_6_00  ,
    UTC_6_30  ,
    UTC_7_00  ,
    UTC_8_00  ,
    UTC_8_30  ,
    UTC_8_45  ,
    UTC_9_00  ,
    UTC_9_30  ,
    UTC_10_00 ,
    UTC_10_30 ,
    UTC_11_00 ,
    UTC_12_00 ,
    UTC_12_45 ,
    UTC_13_00 ,
    UTC_13_45 ,
    UTC_14_00 ,
    UTC_N1_00 ,
    UTC_N2_00 ,
    UTC_N2_30 ,
    UTC_N3_00 ,
    UTC_N3_30 ,
    UTC_N4_00 ,
    UTC_N5_00 ,
    UTC_N6_00 ,
    UTC_N7_00 ,
    UTC_N8_00 ,
    UTC_N9_00 ,
    UTC_N9_30 ,
    UTC_N10_00,
    UTC_N11_00
}timezone_en;

typedef struct timezone_unit{
    char     timezone_path[32];
    char     timezone_content[16];
    uint16_t timezone;
} timezone_unit_t;

static  timezone_unit_t timezone_unit_table[] = { 
    {
        .timezone = UTC_0_00,
        .timezone_path = "UTC+0-00",
        .timezone_content = "UTC-0"
    },
    {
        .timezone = UTC_1_00,
        .timezone_path = "UTC+1-00",
        .timezone_content = "UTC-1"
    },
    {
        .timezone = UTC_2_00,
        .timezone_path = "UTC+2-00",
        .timezone_content = "UTC-2"
    },
    {
        .timezone = UTC_3_00,
        .timezone_path = "UTC+3-00",
        .timezone_content = "UTC-3"
    },
    {
        .timezone = UTC_3_30,
        .timezone_path = "UTC+3-30",
        .timezone_content = "UTC-3:30"
    },
    {
        .timezone = UTC_4_00,
        .timezone_path = "UTC+4-00",
        .timezone_content = "UTC-4"
    },
    {
        .timezone = UTC_4_30,
        .timezone_path = "UTC+4-30",
        .timezone_content = "UTC-4:30"
    },
    {
        .timezone = UTC_5_00,
        .timezone_path = "UTC+5-00",
        .timezone_content = "UTC-5" 
    },
    {
        .timezone = UTC_5_30,
        .timezone_path = "UTC+5-30",
        .timezone_content = "UTC-5:30"
    },
    {
        .timezone = UTC_5_45,
        .timezone_path = "UTC+5-45",
        .timezone_content = "UTC-5:45"
    },
    {
        .timezone = UTC_6_00,
        .timezone_path = "UTC+6-00",
        .timezone_content = "UTC-6" 
    },
    {
        .timezone = UTC_6_30,
        .timezone_path = "UTC+6-30",
        .timezone_content = "UTC-6:30"
    },
    {
        .timezone = UTC_7_00,
        .timezone_path = "UTC+7-00",
        .timezone_content = "UTC-7"
    },
    {
        .timezone = UTC_8_00,
        .timezone_path = "UTC+8-00",
        .timezone_content = "UTC-8"
    },
    {
        .timezone = UTC_8_30,
        .timezone_path = "UTC+8-30",
        .timezone_content = "UTC-8:30"
    },
    {
        .timezone = UTC_8_45,
        .timezone_path = "UTC+8-45",
        .timezone_content = "UTC-8:45"
    },
    {
        .timezone = UTC_9_00,
        .timezone_path = "UTC+9-00",
        .timezone_content = "UTC-9"
    },
    {
        .timezone = UTC_9_30,
        .timezone_path = "UTC+09-30",
        .timezone_content = "UTC-9:30"
    },
    {
        .timezone = UTC_10_00,
        .timezone_path = "UTC+10-00",
        .timezone_content = "UTC-10"
    },
    {
        .timezone = UTC_10_30,
        .timezone_path = "UTC+10-30",
        .timezone_content = "UTC-10:30"
    },
    {
        .timezone = UTC_11_00,
        .timezone_path = "UTC+11-00",
        .timezone_content = "UTC-11"
    },
    {
        .timezone = UTC_12_00,
        .timezone_path = "UTC+12-00",
        .timezone_content = "UTC-12"
    },
    {
        .timezone = UTC_12_45,
        .timezone_path = "UTC+12-45",
        .timezone_content = "UTC-12:45"
    },
    {
        .timezone = UTC_13_00,
        .timezone_path = "UTC+13-00",
        .timezone_content = "UTC-13"
    },
    {
        .timezone = UTC_13_45,
        .timezone_path = "UTC+13-45",
        .timezone_content = "UTC-13:45"
    },
    {
        .timezone = UTC_14_00,
        .timezone_path = "UTC+14-00",
        .timezone_content = "UTC-14"
    },
    {
        .timezone = UTC_N1_00,
        .timezone_path = "UTC-1-00",
        .timezone_content = "UTC+1"
    },
    {
        .timezone = UTC_N2_00,
        .timezone_path = "UTC-2-00",
        .timezone_content = "UTC+2"
    },
    {
        .timezone = UTC_N2_30,
        .timezone_path = "UTC-2-30",
        .timezone_content = "UTC+2:30"
    },
    {
        .timezone = UTC_N3_00,
        .timezone_path = "UTC-3-00",
        .timezone_content = "UTC+3"
    },
    {
        .timezone = UTC_N3_30,
        .timezone_path = "UTC-3-30",
        .timezone_content = "UTC+3:30"
    },
    {
        .timezone = UTC_N4_00,
        .timezone_path = "UTC-4-00",
        .timezone_content = "UTC+4"
    },
    {
        .timezone = UTC_N5_00,
        .timezone_path = "UTC-5-00",
        .timezone_content = "UTC+5"
    },
    {
        .timezone = UTC_N6_00,
        .timezone_path = "UTC-6-00",
        .timezone_content = "UTC+6"
    },
    {
        .timezone = UTC_N7_00,
        .timezone_path = "UTC-7-00",
        .timezone_content = "UTC+7"
    },
    {
        .timezone = UTC_N8_00,
        .timezone_path = "UTC-8-00",
        .timezone_content = "UTC+8"
    },
    {
        .timezone = UTC_N9_00,
        .timezone_path = "UTC-9-00",
        .timezone_content = "UTC+9"
    },
    {
        .timezone = UTC_N9_30,
        .timezone_path = "UTC-9-30",
        .timezone_content = "UTC+9:30"
    },
    {
        .timezone = UTC_N10_00,
        .timezone_path = "UTC-10-00",
        .timezone_content = "UTC+10"
    },
    {
        .timezone = UTC_N11_00,
        .timezone_path = "UTC-11-00",
        .timezone_content = "UTC+11"
    },
};



/*********************************************************************************************************
** Function name:       mb_get_timezone_content
** Descriptions:        update unix 
** input parameters:    NONE
** output parameters:   NONE
** Returned value:      
*********************************************************************************************************/
static int mb_get_timezone_content(const char *path ,char *character, int len)
{
    FILE * fp = NULL;
    int n = 0;

    fp = fopen(path, "r");
    if(NULL == fp)
    {
        return -1;
    }

    n = fread(character, 1, len-1, fp);
    if(n <= 0)
    {
        goto __err_close;
    }

    character[n] = '\0';

__err_close:
    fclose(fp);

    return n;    
}


/*********************************************************************************************************
** Function name:       mb_execute_cmd
** Descriptions:        NONE 
** input parameters:    NONE
** output parameters:   NONE
** Returned value:      
*********************************************************************************************************/
static int mb_execute_cmd(const char *cmd, char *result,int len)
{
    FILE *ptr;
    memset((void*)result,0,len);
    if((ptr=popen(cmd, "r"))!=NULL)
    {
        fgets(result, len, ptr);
        result[len] = 0;
        pclose(ptr);
        ptr = NULL;
        return 0;
    }
    else
    {
        return -1;
    }
}



/*********************************************************************************************************
** Function name:       mb_get_timezone
** Descriptions:        NONE 
** input parameters:    NONE
** output parameters:   NONE
** Returned value:      
*********************************************************************************************************/
int  mb_get_timezone(uint16_t *timezone)
{
    int ret = -1;
    char timezone_content[16];

    /*get timezone content */
    mb_get_timezone_content("/etc/TZ",timezone_content,sizeof(timezone_content));

    for (size_t i = 0; i < sizeof(timezone_unit_table) / sizeof(timezone_unit_table[0]); i++)
    {
        if (strstr(timezone_content, timezone_unit_table[i].timezone_content))
        {
            *timezone = timezone_unit_table[i].timezone;
            return 0;
        }
    }
    /*default UTC-8*/
    *timezone = 13;
    return ret;
}



/*********************************************************************************************************
** Function name:       mb_set_timezone
** Descriptions:        update unix 
** input parameters:    NONE
** output parameters:   NONE
** Returned value:      
*********************************************************************************************************/
int  mb_set_timezone(uint16_t timezone)
{
    int ret = -1;
    char target[128];
    char timezone_path[12];
    char result[64];
    int  counter = 0;
    struct stat lstat;

    if(stat("/usr/share/zoneinfo/UTC", &lstat) < 0)
    {
        return ret;
    }

    /* check timezone valid */
    counter = sizeof(timezone_unit_table) / sizeof(timezone_unit_table[0]);
    if(timezone >= counter)
    {
        return ret;
    }

    for (size_t i = 0; i < counter; i++)
    {
        if (timezone == timezone_unit_table[i].timezone)
        {
            /*get timezone file name */
            snprintf(timezone_path, sizeof(timezone_path), "%s", timezone_unit_table[i].timezone_path);
            ret = 0;
            break;
        }
    }
    if(0 == ret)
    {
        snprintf(target, sizeof(target), "%s%s%s", "ln -sf /usr/share/zoneinfo/UTC/",timezone_path,TIMEZONE_TZ_PATH); 
        ret = mb_execute_cmd(target,result,sizeof(result));
        if(0 == ret)
        {
            /* update current timezone */
            systeminfo.timezone = timezone;
        }

        snprintf(target, sizeof(target), "jq '.timezone=%d' /userdata/config/misc/ntp.json > /tmp/ntp.json && mv /tmp/ntp.json /userdata/config/misc/ntp.json && sync", timezone); 
        ret = mb_execute_cmd(target, result, sizeof(result));
    }

    return ret;
}



/*********************************************************************************************************
** Function name:       mb_config_getUnitTime
** Descriptions:        update unix 
** input parameters:    NONE
** output parameters:   NONE
** Returned value:      
*********************************************************************************************************/
void mb_config_getUnitTime(void)
{
    systeminfo.unix_time = time(NULL);
}


/*********************************************************************************************************
** Function name:       mb_systeminfo_pointer_get
** Descriptions:        NONE 
** input parameters:    NONE
** output parameters:   NONE
** Returned value:      
*********************************************************************************************************/
struct system_info *mb_systeminfo_pointer_get(void)
{    
    return (&systeminfo);
}
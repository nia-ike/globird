/*************************************************
  Copyright (C), 2003-2014, pilot Co., Ltd.
  File name:      PublicDefines.h
  Author:       wangcq    Version: 1.0       Date: 2016-02-19
  Description:    ����ͷ�ļ���
  Others:
  History:         // ��ʷ�޸ļ�¼
      wangcq    2016/02/19     1.0     build this moudle
*************************************************/

#include "PublicDefines.h"
#include <errno.h>
#include <string.h>

void * pVmem = NULL;

//���Ŀ¼
void delete_path(const char* path)
{
    char next_path[1024];
    int check = 0;
    DIR *dir;
    struct dirent *entry;
    struct stat  buf;
    if(lstat(path,&buf) < 0)
    {
        return;
    }
    if(!S_ISDIR(buf.st_mode))
    {
        unlink(path);
        return;
    }
    dir = opendir(path);
    if (dir == NULL)
    {
       return;
    }
    while ((entry = readdir(dir)) != NULL)
    {
        if(strcmp(entry->d_name ,"." ) ==0 || strcmp(entry->d_name ,".." ) ==0 )
            continue;
        if (entry->d_type == DT_DIR)
        {
            sprintf(next_path,"%s/%s",path,entry->d_name);
            delete_path(next_path);
        }
        else
        {
            sprintf(next_path,"%s/%s",path,entry->d_name);
            if(unlink(next_path)<0)
            {
                return;
            }
        }
    }
    remove(path);
    closedir(dir);
    if(check == 1)
        return;
    return;
}

int32_t executeCMD(const char *cmd, char *result,int len)
{
    FILE *ptr;
    int32_t ret = -1;

    // Check input parameters
    if (!cmd || !result || len <= 0) {
        return -1;
    }

    // Clear the result buffer
    memset((void*)result, 0, len);
    
    if ((ptr = popen(cmd, "r")) != NULL) {
        // Read command output, ensuring we don't overflow the buffer
        if (fgets(result, len, ptr) != NULL) {
            // Ensure null termination
            result[len - 1] = '\0';
            ret = 0;  // Success
        }
        
        pclose(ptr);
        ptr = NULL;
    }

    return ret;
}


unsigned long get_file_size(const char *path)
{
    unsigned long filesize = 0;
    struct stat statbuff;
    if(stat(path, &statbuff) != -1)
    {
        filesize = statbuff.st_size;
    }
    else
    {
        filesize = 0;
    }
    return filesize;
}

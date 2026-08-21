/*
* Copyright (c) 2013,珠海派诺电子有限公司
* All rights reserved.
* 
* 文件名称：db_cfg.c
* 文件标识：
* 描    述：
* 
* 当前版本：V1.0
* 作    者：chenzy
* 更改说明：
* 完成日期：20131108
*
* 修改记录 
*  作者       时间         版本      修改描述
*  chenzy   2020.6.09   V1.00      创建
*/

#include <stdio.h>
#include <string.h>
#include <ctype.h>
#include <stdlib.h>
#include <sys/types.h> 
#include <unistd.h> 
#include <sys/stat.h>  //stat
#include <fcntl.h>
#include <stdlib.h>

/*************************************************************************/
/** Function Name:                                                     **/    
/** Purpose:                                                           **/    
/** Params:            None.                                           **/    
/** Return:            None.                                           **/    
/** Notice:            None.                                           **/    
/*************************************************************************/
int db_file_copy(const char *src_file, const char *des_file)
{
    int n;
    int fd_src, fd_des;
    unsigned char buf[1024];

    fd_src = open(src_file, O_RDWR|O_CREAT, S_IRWXU);//打开要拷贝的文件
    if(fd_src < 0)
        return -1;

    fd_des = open(des_file, O_RDWR|O_CREAT|O_EXCL, S_IRWXU);//打开要拷贝的文件
    if(fd_des < 0)
        goto __close_src_fd;

    do 
    {
        n = read(fd_src, buf, sizeof(buf));
        if(n > 0)
            write(fd_des, buf, n);
    }while(n > 0);
    
    close(fd_des);
    close(fd_src);
    return 0;

__close_src_fd:
    close(fd_src);

    return -1;
}
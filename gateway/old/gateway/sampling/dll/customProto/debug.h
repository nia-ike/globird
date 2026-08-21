#ifndef _DEBUG_H_
#define _DEBUG_H_

#include <stdio.h>

    

/* debug enable */
#define DEBUG_ENABLE        1



#define DEBUG_OFF           0
#define DEBUG_ON            1

#define DEBUG_LEVEL_1       (1 << 0)
#define DEBUG_LEVEL_2       (1 << 1)


#if DEBUG_ENABLE
#define DEBUG_PRINT(debug, _format, ...)            do { \
                                                        if (debug & DEBUG_ON) { \
                                                            if(debug & DEBUG_LEVEL_2)\
                                                                printf("\nfile:%s function:%s line:%d\n", __FILE__, __FUNCTION__, __LINE__);\
                                                            printf (_format,##__VA_ARGS__);\
                                                            printf("\n"); \
                                                        } \
                                                    }while(0)
    
#define DEBUG_HEX(debug, addr, n)                   do { \
                                                        int i = 0, j = 0, k = 0; \
                                                        if ((debug & DEBUG_ON) && (n > 0)) { \
                                                            for(i=0; i<(n/16); i++)\
                                                            {\
                                                                for(j=0; j<16; j++) \
                                                                { \
                                                                    printf("%02x ", *(addr + i * 16 + j)); \
                                                                } \
                                                                printf("\n"); \
                                                            }\
                                                            for(k=0; k<(n%16); k++)\
                                                            {\
                                                                printf("%02x ", *(addr + i * 16 + k)); \
                                                            }\
                                                            printf("\n"); \
                                                        } \
                                                    }while(0)

#else
#define DEBUG_PRINT(debug, _format, ...)
#define DEBUG_HEX(debug, addr, n)
#endif

#define DEBUG_INFO_PACK(dest,src,n)         do{ \
                                                char tmp[200];\
                                                memset(tmp,0,sizeof(tmp));\
                                                int len = strlen(dest);\
                                                int i = 0,j = 0,k = 0,l=0;\
                                                if (n > 0)\
                                                {\
                                                    for(i=0; i<(n/32); i++)\
                                                    {\
                                                        for(j = 0; j < 32 ; j++)\
                                                        {\
                                                            sprintf(tmp,"%02x  ", *(src + i * 32 + j));\
                                                            strcat(dest,tmp);\
                                                        }\
                                                        strcat(dest,"\r\n");\
                                                        for(l = 0;l < len; l++)\
                                                        {\
                                                            strcat(dest," ");\
                                                        }\
                                                    }\
                                                    for(k=0; k<(n%32); k++)\
                                                    {\
                                                        sprintf(tmp,"%02x  ", *(src + i * 32 + k));\
                                                        strcat(dest,tmp);\
                                                    }\
                                                    strcat(dest,"\r\n");\
                                                }\
                                             }while(0)

#endif  // #ifndef _DEBUG_H_


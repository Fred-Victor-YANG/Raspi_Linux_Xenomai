#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <sys/types.h>
#include <sys/stat.h>
#include <fcntl.h>
#include <unistd.h>

#define PY_SSIZE_T_CLEAN
//#include <Python.h>

int main(void)
{
	char buf[128];
	char *resp="Message Received!";
	int fd, ret, len;
    char *etat_recu="rien";

    while (1) {
        
        fd = open( "/dev/rtdm/rtdm_driver_0", O_RDWR );
        if (fd < 0) {
            printf("open error\n");
        }
            

        /* Get the next message from realtime device. */
        ret = read(fd, buf, sizeof(buf));
        
        if (ret <= 0) {
            printf("read error\n");
        }
        
        else {
            if (buf == "bouton_appuye") {
                printf("==> Received from Realtime %d bytes : %.*s\n", strlen(buf), strlen(buf), buf);
                
                len = strlen(resp);
                ret = write(fd, resp, len);
                if (ret <= 0) {
                    printf("write error\n");
                }
                else {
                    len = strlen(resp);
                    printf("%s: sent %d bytes, \"%.*s\"\n",  __FUNCTION__, len, len, resp);
                }
                
            }
            
            else if (buf == "-1"){
                printf("pas de msg");
            }
            
            else if (buf == "bouton_libre") {
                printf ("bouton libre");
            }
            
            else {
                printf ("buf = %s \n", buf);
            }
            
            
        }
		

        /* Echo the message back to realtime device. */
//        len = strlen(resp);
//
//        ret = write(fd, resp, len);
//        if (ret <= 0)
//            printf("write error\n");
//        else
//            printf("%s: sent %d bytes, \"%.*s\"\n",  __FUNCTION__, len, len, resp);
        
        sleep (0.1);//100ns
        }
    
}


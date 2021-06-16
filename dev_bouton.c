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
    char etat_recu="rien";

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
            //etat_recu = buf;
            if (etat_recu == "bouton_appuye") {
                printf("==> Received from Realtime %d bytes : %.*s\n", strlen(buf), strlen(buf), buf);
                
            }
            
            else if (etat_recu == "-1"){
                printf("pas de msg");
            }
            
            else if (etat_recu == "bouton_libre") {
                printf ("bouton libre");
            }
            
            else {
                printf ("received = %s \n", buf);
            }
            
            
        }
		

        /* Echo the message back to realtime device. */
        resp = buf;
        len = strlen(resp);
        
        ret = write(fd, resp, len);
        if (ret <= 0)
            printf("write error\n");
        else
            printf("%s: sent %d bytes, status of button reveived = \"%.*s\n",  __FUNCTION__, len, len, resp);
        
        sleep (1);//1ms
        }
    
}


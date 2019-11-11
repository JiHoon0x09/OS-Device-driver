#include <stdio.h>
#include <fcntl.h>

#include <linux/input.h>

char *key_string[5] = {"ENTER","UP","LEFT","DOWN","RIGHT"};	

int main(void)
{
	int fd;
	int i;
	int len = 0;
	char key_value;
	struct input_event event_buf[3];

	printf("\nStarting keyscan test\n");

	//fd = open("/dev/rebis_keyscan", O_RDONLY, 0);
	fd = open("/dev/event_key", O_RDONLY, 0);
	printf("fd = %d  \n", fd);

	if(fd < 0) {
		printf("keyscan device open error\n");
		exit(0);
	}

	while(1) {
		//printf("press keypad: \n");

		while(1) {

			//len = read(fd, &key_value, 1);
			len = read(fd, event_buf, (sizeof(struct input_event)*3) );
			//printf("len = %d \n", len);

			if(len > 0)	break;
		}

		//if(key_value != 0) {
			//printf("%s(%d)\n", key_string[key_value-1], key_value);

			for( i=0; i<(len/sizeof(struct input_event)); i++ )
                {
				//printf("type = %d \n", event_buf[i].type);
                    switch( event_buf[i].type )
                    {
                    case EV_SYN:
                        printf("---------------------------------------\n");
                        break;
                    	
                    case EV_KEY:
                        printf("Button code %d", event_buf[i].code);
                        switch (event_buf[i].value)
                        {
                        case 1:
                            printf(": pressed\n");
                            break;
                        case 0:
                            printf(": released\n");
                            break;
                        default:
                            printf("Unknown: type %d, code %d, value %d",
                                  event_buf[i].type, 
                                  event_buf[i].code, 
                                  event_buf[i].value);
                            break;
                        }
                        break;
					default:
                        printf("Unknown: type %d, code %d, value %d\n",
                               event_buf[i].type, 
                               event_buf[i].code, 
                               event_buf[i].value);
            
                        break;
					} //switch
				} //for
		//}	//if key_value

	} //first while(1)

	close(fd);
} //main



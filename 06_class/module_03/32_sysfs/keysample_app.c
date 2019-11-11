#include <stdio.h>
#include <fcntl.h>

char *key_string[10] = {"SW2","","SW4","","SW6","","SW8","","SW10",""};	

int main(void)
{
	int fd;
	int len = 0;
	char key_value;

	printf("\nStarting keyscan test\n");

	fd = open("/dev/m2_keyscan", O_RDONLY, 0);
	//fd = open("/dev/input/event0", O_RDONLY, 0);
	if(fd < 0) {
		printf("keyscan device open error\n");
		exit(0);
	}

	while(1) {
		printf("press keypad: \n");

		while(1) {

			len = read(fd, &key_value, 1);

			if(len > 0)	break;
		}

		if(key_value != 0) {
			printf("%s(%d)\n", key_string[key_value-2], key_value);
	
		}

	}

	close(fd);
}

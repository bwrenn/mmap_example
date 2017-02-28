#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <fcntl.h>
#include <unistd.h>
#include <sys/mman.h>
#include <time.h>
#include <stdint.h>

#define MY_MMAP_LEN (0x25000)
#define PAGE_SIZE 4096

static char *exec_name;

static void
mem_dump (const uint8_t *pkt, uint32_t len, int full)
{
	const uint8_t *pkt_seg = pkt;
	int i;

	if (!full)
	{
		printf("[%s] 0x%lX [ ", exec_name, (unsigned long)(pkt));
		for (i = 0; i < 8; i++)
		{
			printf("0x%02X ", pkt[i]);
		}
		printf("... ");
		for (i = len - 8; i < len; i++)
		{
			printf("0x%02X ", pkt[i]);
		}
		printf("]\n");

		return;
	}

	for (i = 0; i < len; i++)
	{
		if (0 == i % 16)
		{
			printf("\n[%s] 0x%lX:", exec_name, (unsigned long)(pkt_seg));
		}

		printf(" 0x%02X", *pkt_seg);

		pkt_seg++;
	}
	printf("\n");
}

int main ( int argc, char **argv )
{
  int fd;
  char *address = NULL;
  time_t t = time(NULL);
  char *sbuff;
  int i;

  exec_name = argv[0];
  sbuff = (char*) calloc(MY_MMAP_LEN,sizeof(char));

  fd = open("/dev/expdev", O_RDWR);
  if(fd < 0)
  {
    perror("Open call failed");
    return -1;
  }

  address = mmap( NULL,
                  MY_MMAP_LEN,
                  PROT_READ|PROT_WRITE,
                  MAP_SHARED,
                  fd,
                  0);
  if (address == MAP_FAILED)
  {
    perror("mmap operation failed");
    return -1;
  }

  //printf("%s: first userspace read\n",tbuff);
  /*
  memcpy(sbuff, address,80);
  printf("Initial message: %s\n", sbuff);
  memcpy(sbuff, address+0xf00,80);
  printf("Initial message: %s\n", sbuff);
  memcpy(sbuff, address+0xf000,80);
  printf("Initial message: %s\n", sbuff);
  */
  /*
  for(i=0; i<MY_MMAP_LEN; i++)
  {
    printf("%16p: %c\n",address+i, (char)*(address+i));
  }
  */

  usleep(5000);
  for (i = 0; i < (MY_MMAP_LEN / PAGE_SIZE); i++)
  {
	  mem_dump(address + (i * PAGE_SIZE), PAGE_SIZE, 0);
  }

  if (munmap(address, MY_MMAP_LEN) == -1)
  {
	  perror("Error un-mmapping the file");
  }
  close(fd);
  return 0;
}

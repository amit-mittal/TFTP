CC=gcc

myclient: tftp_c.c myserver
	$(CC) tftp_c.c -o tftp_c

myserver: tftp_s.c
	$(CC) tftp_s.c -o tftp_s
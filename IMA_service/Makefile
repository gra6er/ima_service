CC=gcc
CFLAGS=-c -Wall
DBUS_CFLAGS=-I/usr/include/dbus-1.0 -I/usr/lib/x86_64-linux-gnu/dbus-1.0/include
DBUS_LIBS=-ldbus-1

service: ima_client.o err_proc.o dbus_deamon.o dbus_func.o ima_server.o 
	$(CC) ima_client.o err_proc.o -o ima_client $(DBUS_CFLAGS) $(DBUS_LIBS)
	$(CC) dbus_deamon.o dbus_func.o -o dbus_deamon $(DBUS_CFLAGS) $(DBUS_LIBS) 	

ima_client.o: ima_client.c
	$(CC) $(CFLAGS) -o ima_client.o ima_client.c $(DBUS_CFLAGS) $(DBUS_LIBS)

err_proc.o: err_proc.c
	$(CC) $(CFLAGS) -o err_proc.o err_proc.c

dbus_deamon.o: dbus_deamon.c
	$(CC) $(CFLAGS) -o dbus_deamon.o dbus_deamon.c $(DBUS_CFLAGS) $(DBUS_LIBS)

dbus_func.o: dbus_func.c
	$(CC) $(CFLAGS) -o dbus_func.o dbus_func.c $(DBUS_CFLAGS) $(DBUS_LIBS)

ima_server.o: ima_server.c err_proc.c
	$(CC) -pthread ima_server.c err_proc.c -o ima_server

clean:
	rm -rf *.o service
#ifndef DBUS_FUNC_H
#define DBUS_FUNC_H

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <syslog.h>
#include <fcntl.h>
#include <sys/stat.h>
#include <sys/wait.h> 
#include <dbus/dbus.h>

#define BUF_SIZE 256

void check_and_abort(DBusError *error);

DBusHandlerResult messages_callback(DBusConnection *connection, DBusMessage *message, void *user_data);

void respond_to_introspect(DBusConnection *connection, DBusMessage *request);

void respond_to_status(DBusConnection *connection, DBusMessage *request);

void respond_to_addFile(DBusConnection *connection, DBusMessage *request);

void respond_to_rmFile(DBusConnection *connection, DBusMessage *request);

void dbus_listen(void);

int audit_add_file(char* filename);

int audit_search_file(char* filename, int* str_num);

void str_trim(char* str, int length);

int verify_file(char* filename);

#endif
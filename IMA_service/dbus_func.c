#include "dbus_func.h"

const char *const INTERFACE_NAME = "org.app.imaDbus";
const char *const SERVER_BUS_NAME = "org.app.imaServer";
const char *const CLIENT_BUS_NAME = "org.app.imaClient";
const char *const SERVER_OBJECT_PATH_NAME = "/org/app/imaServObj";
const char *const METHOD1_NAME = "status";
const char *const METHOD2_NAME = "addFile";
const char *const METHOD3_NAME = "rmFile";

static char ima_resp[BUF_SIZE*4];


void dbus_listen(void)
{
	DBusConnection *connection;
    DBusError error;
    DBusObjectPathVTable vtable;
 
    dbus_error_init(&error);
    connection = dbus_bus_get(DBUS_BUS_SESSION, &error);
    check_and_abort(&error);
 
    dbus_bus_request_name(connection, SERVER_BUS_NAME, 0, &error);
    check_and_abort(&error);
 
    vtable.message_function = messages_callback;
    vtable.unregister_function = NULL;

    dbus_connection_try_register_object_path(connection,
                         SERVER_OBJECT_PATH_NAME,
                         &vtable,
                         NULL,
                         &error);
    check_and_abort(&error);
 
    while(1) 
    {
        dbus_connection_read_write_dispatch(connection, 1000);
    }
}

DBusHandlerResult messages_callback(DBusConnection *connection, DBusMessage *message, void *user_data) 
{
    const char *interface_name = dbus_message_get_interface(message);
    const char *member_name = dbus_message_get_member(message);
     
    if (0==strcmp("org.freedesktop.DBus.Introspectable", interface_name) &&
        0==strcmp("Introspect", member_name)) 
    {
        respond_to_introspect(connection, message);
        return DBUS_HANDLER_RESULT_HANDLED;
    } 
    else if(0==strcmp(INTERFACE_NAME, interface_name) &&
            0==strcmp(METHOD1_NAME, member_name)) 
    {
        respond_to_status(connection, message);
        return DBUS_HANDLER_RESULT_HANDLED;
    } 
    else if(0==strcmp(INTERFACE_NAME, interface_name) &&
            0==strcmp(METHOD2_NAME, member_name)) 
    {     
        respond_to_addFile(connection, message);
        return DBUS_HANDLER_RESULT_HANDLED;
    } 
    else if(0==strcmp(INTERFACE_NAME, interface_name) &&
            0==strcmp(METHOD3_NAME, member_name)) 
    {     
        respond_to_rmFile(connection, message);
        return DBUS_HANDLER_RESULT_HANDLED;
    } 
    else 
    {
        return DBUS_HANDLER_RESULT_NOT_YET_HANDLED;
    }
}

void respond_to_introspect(DBusConnection *connection, DBusMessage *request) 
{
    DBusMessage *reply;

    const char *introspection_data =
        " <!DOCTYPE node PUBLIC \"-//freedesktop//DTD D-BUS Object Introspection 1.0//EN\" "
        "\"http://www.freedesktop.org/standards/dbus/1.0/introspect.dtd\">"
        " <!-- dbus-sharp 0.8.1 -->"
        " <node>"
        "   <interface name=\"org.freedesktop.DBus.Introspectable\">"
        "     <method name=\"Introspect\">"
        "       <arg name=\"data\" direction=\"out\" type=\"s\" />"
        "     </method>"
        "   </interface>"
        "   <interface name=\"org.app.imaDbus\">"
        "     <method name=\"status\">"
        "       <arg name=\"output\" direction=\"out\" type=\"s\" />"
        "     </method>"
        "     <method name=\"addFile\">"
        "       <arg name=\"filename\" direction=\"in\" type=\"s\" />"
        "       <arg name=\"status\" direction=\"out\" type=\"s\" />"
        "     </method>"
        "     <method name=\"rmFile\">"
        "       <arg name=\"filename\" direction=\"in\" type=\"s\" />"
        "       <arg name=\"status\" direction=\"out\" type=\"s\" />"
        "     </method>"
        "   </interface>"
        " </node>";

    reply = dbus_message_new_method_return(request);
    dbus_message_append_args(reply,
                 DBUS_TYPE_STRING, &introspection_data,
                 DBUS_TYPE_INVALID);
    dbus_connection_send(connection, reply, NULL);
    dbus_message_unref(reply);
}

void respond_to_status(DBusConnection *connection, DBusMessage *request) 
{
    DBusMessage *reply;
    DBusError error;
    char* p = ima_resp;

    int st = init_audit();

    switch(st)
    {
        case -3:
            sprintf(ima_resp, "No file added to audit");
        case -2:
            bzero(ima_resp, sizeof(ima_resp));
            sprintf(ima_resp, "ERROR Verify error");
        case -1:
            bzero(ima_resp, sizeof(ima_resp));
            sprintf(ima_resp, "ERROR Audit file read error");
    }
 
    dbus_error_init(&error);
 
    reply = dbus_message_new_method_return(request);
    dbus_message_append_args(reply,
                 DBUS_TYPE_STRING, &p,
                 DBUS_TYPE_INVALID);
    dbus_connection_send(connection, reply, NULL);
    dbus_message_unref(reply);
    bzero(ima_resp, sizeof(ima_resp));
}

void respond_to_addFile(DBusConnection *connection, DBusMessage *request)
{
	DBusMessage *reply;
    DBusError error;
    char* filename;
    char* p = ima_resp;

    dbus_error_init(&error);
 
    dbus_message_get_args(request, &error,
                  DBUS_TYPE_STRING, &filename,
                  DBUS_TYPE_INVALID);


    if (dbus_error_is_set(&error)) {
        reply = dbus_message_new_error(request, "wrong_arguments", "Illegal arguments to addFile");
        dbus_connection_send(connection, reply, NULL);
        dbus_message_unref(reply);
        return;
    }

    int res = audit_add_file(filename);
 
    switch(res)
    {
        case 0:
            sprintf(ima_resp, "File %s added.", filename);
            break;
        case 1:
            sprintf(ima_resp, "ERROR (ima_sign). Possibly this file does not exist.");
            break;
        case -1:
            sprintf(ima_resp, "ERROR Can't open audit file.");
            break;
        case -2:
            sprintf(ima_resp, "ERROR (fork).");
            break;
        case -3:
            sprintf(ima_resp, "File %s already in audit list.", filename);
            break;
        case -4:
            sprintf(ima_resp, "ERROR Can't write in audit file");
            break;
    }

    reply = dbus_message_new_method_return(request);
    dbus_message_append_args(reply,
                 DBUS_TYPE_STRING, &p,
                 DBUS_TYPE_INVALID);
    dbus_connection_send(connection, reply, NULL);
    dbus_message_unref(reply);
    bzero(ima_resp, sizeof(ima_resp));
}

void respond_to_rmFile(DBusConnection *connection, DBusMessage *request)
{
	DBusMessage *reply;
    DBusError error;
    char* filename;
    const char* resp = "FILE REMOVED.";
 
    dbus_error_init(&error);
 
    dbus_message_get_args(request, &error,
                  DBUS_TYPE_STRING, &filename,
                  DBUS_TYPE_INVALID);
    
    if (dbus_error_is_set(&error)) {
        reply = dbus_message_new_error(request, "wrong_arguments", "Illegal arguments to rmFile");
        dbus_connection_send(connection, reply, NULL);
        dbus_message_unref(reply);
        return;
    }
 

    reply = dbus_message_new_method_return(request);
    dbus_message_append_args(reply,
                 DBUS_TYPE_STRING, &resp,
                 DBUS_TYPE_INVALID);
    dbus_connection_send(connection, reply, NULL);
    dbus_message_unref(reply);
    bzero(ima_resp, sizeof(ima_resp));
}

int audit_add_file(char* filename)
{
    pid_t pid;
    int ch_status;
    int line = 0;
    str_trim(filename, BUF_SIZE);

    int audit_fd = open("audit", O_RDWR|O_CREAT|O_APPEND, S_IREAD|S_IWRITE);
    if(audit_fd == -1) return -1;

    pid = fork();

    switch(pid)
    {
        case 0:
            execl("/usr/bin/evmctl", "/usr/bin/evmctl", "ima_sign", filename, NULL);
        case -1:
            return -2;
        default:
            break;
    }

    wait(&ch_status);
    
    if(ch_status == 0)
    {
        if(audit_search_file(filename, &line) == 0) // File already in audit list 
            return -3;

        int res = write(audit_fd, filename, strlen(filename));
        if(res == -1) return -4;
        write(audit_fd, "\n", 1);

        return 0;
    }else
        return 1;
}

int audit_search_file(char* filename, int* str_num)
{
    int i = 0;
    char fname_buf[BUF_SIZE];
    char* pstr;

    str_trim(filename, BUF_SIZE);

    FILE* audit = fopen("audit", "r");
    if(audit == NULL) return -1;

    while(1)
    {
        pstr = fgets(fname_buf, BUF_SIZE, audit);

        if(pstr == NULL)
        {
            if(feof(audit) != 0) break;
            else return -2;
        }

        i++;
        str_trim(fname_buf, BUF_SIZE);
        if(strcmp(filename, fname_buf) == 0)
        {
            *str_num = i;
            return 0;
        }

    }
    return 1;
}

int init_audit(void)
{
    char fname_buf[BUF_SIZE];
    char* pstr;
    int st;

    FILE* audit = fopen("audit", "r");
    if(audit == NULL) return -3;

    while(1)
    {
        pstr = fgets(fname_buf, BUF_SIZE, audit);

        if(pstr == NULL)
        {
            if(feof(audit) != 0)
            {
                break;
            }
            else
            {
                return -1;
            }
        }

       st = verify_file(fname_buf);
       if(st == -1) return -2;
    }

    return 0;
}

int verify_file(char* filename)
{
    pid_t pid;
    int ch_status;

    str_trim(filename, BUF_SIZE);

    pid = fork();

    switch(pid)
    {
        case 0:
            execl("/usr/bin/evmctl", "/usr/bin/evmctl", "ima_verify", filename, NULL);
        case -1:
            return -1;
        default:
            break;
    }

    wait(&ch_status);    
    if(ch_status == 0)
        sprintf(ima_resp, "%s[OK]: %s\n", ima_resp, filename);              
    else
        sprintf(ima_resp, "%s[ALERT]: %s\n", ima_resp, filename);               

    return 0;
}



void check_and_abort(DBusError *error) 
{
    if (dbus_error_is_set(error)) {
        syslog(LOG_LOCAL0|LOG_INFO,"[ERROR] %s", error->message);
        abort();
    }
}

void str_trim(char* str, int length)
{
    for(int i =0; i < length; i++)
    {
        if(str[i] == '\n')
        {
            str[i] = '\0';
            break;
        }
    }
}
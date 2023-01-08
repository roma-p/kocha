#include "logger.h"
#include "platform/platform.h"

#include <stdio.h>
#include <string.h>
#include <stdarg.h>

b8 log_init(){
    //TODO : create log file
    return TRUE;
}

void log_stop(){
    //TODO
}


void log_output(log_level level, const char* message, ...){
    const char * level_strings[6] = {
    "[FATAL]: ",
    "[ERROR]: ",
    "[WARN]:  ", 
    "[INFO]:  ", 
    "[DEBUG]: ", 
    "[TRACE]: ", 
    }; 

    const i32 msg_length = 32000;
    b8 is_error = level < LOG_LEVEL_WARN;

    char out_message[msg_length];
    char out_message_tmp[msg_length];
    memset(out_message, 0, sizeof(out_message));
    memset(out_message, 0, sizeof(out_message_tmp));

    __builtin_va_list p_arg;
    va_start(p_arg, message);
    vsnprintf(out_message_tmp, msg_length, message, p_arg);
    va_end(p_arg);

    sprintf(out_message, "%s%s\n", level_strings[level], out_message_tmp);
    
    if (is_error) {
    platform_console_write_error(out_message, level);
    } else {
    platform_console_write(out_message, level);
    }
}


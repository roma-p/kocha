#include "logger.h"

#include <stdio.h>
#include <string.h>
#include <stdarg.h>

b8 log_init(){
    //TODO : create log file
}

void log_stop(){
    //TODO
}


void log_output(log_level level, const char* message, ...){
    const char * level_strings[6] = {
	"[FATAL]: ", "[ERROR]: ",  "[WARN]: ", 
	"[INFO]: ", "[DEBUG]: ", "[TRACE]: ", 
    }; 
    //b8 is_error = level < 2;
    char out_message[32000];
    char out_message_tmp[32000];
    memset(out_message, 0, sizeof(out_message));
    memset(out_message, 0, sizeof(out_message_tmp));

    __builtin_va_list p_arg;
    va_start(p_arg, message);
    vsnprintf(out_message_tmp, 32000, message, p_arg);
    va_end(p_arg);

    sprintf(out_message, "%s%s\n", level_strings[level], out_message_tmp);

    printf("%s", out_message);
}


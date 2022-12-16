#include"assert.h"
#include"logger.h"

void report_assertion_failure(const char* expression, const char* message, const char * file, i32 line){
    LOG_FATAL("Assertion failure %s : %s : %d : %s", expression, file, line, message);
}

#include <core/logger.h>
#include <core/assert.h>

int main(void) {
    LOG_FATAL("C'est la merde %f", 3.14f);
    LOG_ERROR("C'est la merde %d", 4);
    LOG_WARN("C'est la merde %d", 4);
    LOG_INFO("C'est la merde %d", 4);
    LOG_DEBUG("C'est la merde %d", 4);
    LOG_TRACE("C'est la merde %d", 4);
    
    KASSERT(1==0);
    //KASSERT_MSG(1==0, "????");

    return 0;
}

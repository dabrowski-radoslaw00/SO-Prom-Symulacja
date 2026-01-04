#include "utils/logger.h"

int main() {
    logger_init();
    log_message("Test message 1");
    log_message("Test message 2");
    logger_cleanup();

    return 0;
}
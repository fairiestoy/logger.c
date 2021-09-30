#include "../src/logger.h"

int main(int argc,char *argv[argc]) {
  if(logger_factory_console(LOGGER_WARNING) <= 0 || logger_is_initialized() == false) {
    fprintf(stderr,"Could not initialize logger\n");
    return -1;
  }
  logger_debug("Initialized Logger");
  logger_emergency("A test emergency message");
  logger_warning("A visible message");
  logger_toggle(false);
  logger_error("This should not be visible");
  return 0;
}

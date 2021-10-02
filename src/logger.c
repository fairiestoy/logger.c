/*
A logging library that is aimed at being flexible for most
output targets and formats.

There are two kinds of logging types:
- General Log Message
    This is a traditional Log message of the form [timestamp] [log level] [file] [linenumber] [message]
- Arbitrary Data Message
    This is to be used for printing stack traces or long, arbitrary data
*/

#include "logger.h"

typedef struct {
  int log_level;
  void *output_object;
  logger_push_log output_function;
  logger_transform transform_function;
  bool is_active;
} logging_context;

/*
Static context structure. This in turn limits this module
to single-context operation.

An alternative extension Idea would be to allow for callbacks
based on Module/File.

Rough Idea: On startup, create one global context, assign general-use
callbacks for transformation/output.

After that, assign callbacks with a filename, so that the log entries
coming in from "logger.c" will be assigned to different callbacks.
*/
static logging_context Logger = {0};

/*
Factory Functions or default behaviour, for example
output to the console or a simple .txt file.

Can be skipped or reviewed for a sample implementation
*/
static char *
logger_factory_console_transform(time_t const timestamp,int const log_level,char const * const file, int const filenumber,char * message) {
  if(log_level < LOGGER_EMERGENCY || log_level > LOGGER_DEBUG || message == (void*)0 || file == (void*)0) {
    fprintf(stderr,"Could not transform message, invalid parameters");
    return (void*)0;
  }
  char tmp_buffer[LOGGER_MESSAGE_BUFFER] = {0};
  char const log_level_mapping[][10] = {
    "EMERGENCY",
    "ALERT",
    "CRITICAL",
    "ERROR",
    "WARNING",
    "NOTICE",
    "INFO",
    "DEBUG"
  };
  int const offset = strftime(tmp_buffer,LOGGER_MESSAGE_BUFFER,"%c",localtime(&timestamp));
  if(offset < 1) {
    fprintf(stderr,"Could not write time to buffer\n");
    return (void*)0;
  }
  if(snprintf(tmp_buffer + offset,LOGGER_MESSAGE_BUFFER," %-10s %s:%d - %s\n",log_level_mapping[log_level],file,filenumber,message) < 1) {
    fprintf(stderr,"Could not amalgamate Message - sprintf returned 0 bytes");
    return (void*)0;
  }
  memset(message,0,LOGGER_MESSAGE_BUFFER);
  memcpy(message,&tmp_buffer,LOGGER_MESSAGE_BUFFER);
  return message;
}

static int
logger_factory_console_output(void const * const custom_object,char const * const message) {
  /* Using this setup to remove compiler warning about unused variables */
  fprintf((FILE *)custom_object,message);
  return 1;
}

extern int
logger_factory_console(int log_level) {
  return logger_setup_context(log_level,stdout,logger_factory_console_output,logger_factory_console_transform,true);
}

static FILE *
logger_factory_file_file = (void*)0;

static void
logger_factory_file_exit(void) {
  if(logger_factory_file_file != (void*)0) {
    fflush(logger_factory_file_file);
    fclose(logger_factory_file_file);
  }
}

extern int
logger_factory_file(int log_level,char const * const file_path) {
  if(file_path == (void*)0 || log_level < LOGGER_EMERGENCY || log_level > LOGGER_DEBUG) {
    return -1;
  }
  if(logger_factory_file_file != (void*)0) {
    fflush(logger_factory_file_file);
    fclose(logger_factory_file_file);
  }
  logger_factory_file_file = fopen(file_path,"w");
  if(logger_factory_file_file == (void*)0) {
    perror("Could not open File for factory setup");
    return -2;
  }
  if(atexit(logger_factory_file_exit) != 0) {
    fprintf(stderr,"Could not setup atexit handler\n");
    fclose(logger_factory_file_file);
    return -3;
  }
  return logger_setup_context(log_level,logger_factory_file_file,logger_factory_console_output,logger_factory_console_transform,true);
}

static char *
logger_factory_csv_transform(time_t const timestamp,int const log_level,char const * const file, int const filenumber,char * message) {
  if(log_level < LOGGER_EMERGENCY || log_level > LOGGER_DEBUG || message == (void*)0 || file == (void*)0) {
    fprintf(stderr,"Could not transform message, invalid parameters");
    return (void*)0;
  }
  char tmp_buffer[LOGGER_MESSAGE_BUFFER] = {0};
  char const log_level_mapping[][10] = {
    "emergency",
    "alert",
    "critical",
    "error",
    "warning",
    "notice",
    "info",
    "debug"
  };
  if(snprintf(tmp_buffer,LOGGER_MESSAGE_BUFFER,"%ld,%s,%s,%i,%s\n",timestamp,log_level_mapping[log_level],file,filenumber,message) < 1) {
    fprintf(stderr,"Could not amalgamate Message - sprintf returned 0 bytes");
    return (void*)0;
  }
  memset(message,0,LOGGER_MESSAGE_BUFFER);
  memcpy(message,&tmp_buffer,LOGGER_MESSAGE_BUFFER);
  return message;
}

extern int
logger_factory_csv(int log_level,char const * const file_path) {
  int ret_code = logger_factory_file(log_level,file_path);
  if(ret_code <= 0) {return ret_code;}
  logger_set_transform(logger_factory_csv_transform);
  fprintf(logger_factory_file_file,"timestamp,priority,filename,linenumber,message\n");
  return ret_code;
}
/*
Parameters:
-----------
log_level
  Value between LOGGER_EMERGENCY - LOGGER_DEBUG
  Is set so that the most critical level, is the lowest value and vice versa

file
  A constant string provided by the macros (preprocessor __FILE__ value)

linenumber
  A constant number provided by the macros (preprocessor __LINE__ value)

message
  Format string that fits the standard conversion specifiers

...
  Variadic parameters - used in the format string "message"

Return Values:
--------------
None

Description:
------------
This function is being called by the macros logger_emergency() ... logger_debug()
and prepares the message buffer, concatenates the variadic parameters into the
format string and pushes it to the Logger.transform_function and Logger.output_function
*/
extern void
logger_log(int log_level,char const * const file,int linenumber,char const * const message, ...) {
  if(log_level < 0 || log_level > 7 || message == (void*)0 || file == (void*)0) {
    fprintf(stderr,"Could not log message - empty or outside log levels\n");
    return;
  }
  if(log_level > Logger.log_level || Logger.is_active == false) {return;}
  char message_buffer[LOGGER_MESSAGE_BUFFER] = {0};
  va_list parameter_list;
  va_start(parameter_list,message);
  if(vsnprintf(message_buffer,LOGGER_MESSAGE_BUFFER,message,parameter_list) < 1) {
    fprintf(stderr,"Could not log message - pre-formatting returned 0 bytes\n");
    return;
  }
  va_end(parameter_list);
  if(Logger.output_function(Logger.output_object,Logger.transform_function(time((void*)0),log_level,file,linenumber,message_buffer)) < 1) {
    fprintf(stderr,"Could not log message - push function failed\n");
    return;
  }
}

/*
Parameters:
-----------
log_level
  Value between LOGGER_EMERGENCY - LOGGER_DEBUG
  Is set so that the most critical level, is the lowest value and vice versa
  Determines with what starting log level we push messages to the
  output routine

output_data
  Any kind of User Data that can be used to push messages to. For example
  a buffer, file stream, network connection, custom struct for databases.

output_function
  Used to push messages to the output medium
  Signature: int fname(void const * const custom_object,char const * const message)

transform_function
  Used to prepare a buffer used in the output routine. This buffer must be the
  final message (in regards to format). Size is controlled via LOGGER_MESSAGE_BUFFER
  Signature: char * fname(time_t const timestamp,int const log_level,char const * const file, int const filenumber,char * message)

is_active
  Start setting if the Logging is currently turned on. If set to "false",
  all messages stop being transformed + pushed

Return Value:
-------------
value <= 0 = ERROR
value > 0 = SUCCESS

Description:
------------
Initializes the static structure in this module. Performs a few checks
and sets initial values.

ToDo: Maybe add a memory check if everything checks out?
*/
extern int
logger_setup_context(int log_level,
                    void *output_data,
                    logger_push_log output_function,
                    logger_transform transform_function,
                    bool is_active) {
  if(log_level < 0 || log_level > 7) {
    return 0;
  } else if(output_function == (void*)0 || transform_function == (void*)0) {
    return -1;
  }
  Logger.log_level = log_level;
  Logger.output_object = output_data;
  Logger.output_function = output_function;
  Logger.transform_function = transform_function;
  Logger.is_active = is_active;
  return 1;
}

/*
Parameters:
-----------
new_output
  The new output function to be used from this point forward.

Return Value:
-------------
value <= 0 = ERROR
value > 0 = SUCCESS

Description:
------------
Sets the new output function for pushing messages to the destination medium
*/
extern int
logger_set_output_callback(logger_push_log new_output) {
  if(Logger.output_function == (void*)0 || new_output == (void*)0) {return 0;}
  Logger.output_function = new_output;
  return 1;
}

/*
Parameters:
-----------
log_level
  The new Log level in the Range LOGGER_EMERGENCY -> LOGGER_DEBUG

Return Value:
-------------
value <= 0 = ERROR
value > 0 = SUCCESS

Description:
------------
Sets the new output log level for pushing messages to the destination medium
*/
extern int
logger_set_loglevel(int log_level) {
  if(log_level < 0 || Logger.log_level == log_level) {return 0;}
  Logger.log_level = log_level;
  return 1;
}

/*
Parameters:
-----------
new_transform
  The new transform function to be used from this point forward.

Return Value:
-------------
value <= 0 = ERROR
value > 0 = SUCCESS

Description:
------------
Sets the new transform function for modifying messages to the output function
*/
extern int
logger_set_transform(logger_transform new_transform) {
  if(Logger.output_function == (void*)0 || new_transform == (void*)0) {return 0;}
  Logger.transform_function = new_transform;
  return 1;
}

/*
Parameters:
-----------
set_active
  true = messages are pushed / false = messages are silent

Return Value:
-------------
None

Description:
------------
Determines, if messages are silent or should be pushed
*/
extern void
logger_toggle(bool set_active) {
  if(set_active == Logger.is_active) {return;}
  Logger.is_active = set_active;
}

/*
Parameters:
-----------
None

Return Value:
-------------
bool
  true = messages are pushed / false = messages are silent

Description:
------------

*/
extern bool
logger_get_status(void) {
  return Logger.is_active;
}

/*
Parameters:
-----------
None

Return Value:
-------------
bool
  true = struct seems initialized / false = struct appears to be invalid

Description:
------------

*/
extern bool
logger_is_initialized(void) {
  if(Logger.transform_function != (void*)0 && Logger.output_function != (void*)0 && Logger.log_level >= LOGGER_EMERGENCY && Logger.log_level <= LOGGER_DEBUG) {
    return true;
  }
  return false;
}

#ifdef LOGGERTESTSUITE

static void
tests_preinit_check(void **state) {
  (void*)state;
  assert_true(sizeof(logging_context) > 0);
  char const * const check_pointer = (char const * const)&Logger;
  for(size_t index = 0;index < sizeof(logging_context);index++) {
    assert_true(*check_pointer == 0);
  }
}

static char tests_output_simple[LOGGER_MESSAGE_BUFFER] = {0};

static int
tests_init_output(void const * const custom_object,char const * const message) {
  snprintf((char *)custom_object,LOGGER_MESSAGE_BUFFER,"%s",message);
  return 1;
}

static char *
tests_init_transform(time_t const timestamp,int const log_level,char const * const file, int const filenumber,char * message) {
  assert_true(timestamp > 0);
  assert_true(log_level >= LOGGER_EMERGENCY);
  assert_true(log_level <= LOGGER_DEBUG);
  assert_true(file != (void*)0);
  assert_true(filenumber > -1);
  assert_true(message != (void*)0);
  char tmp_buffer[LOGGER_MESSAGE_BUFFER] = {0};
  char const log_level_mapping[][10] = {
    "EMERGENCY",
    "ALERT",
    "CRITICAL",
    "ERROR",
    "WARNING",
    "NOTICE",
    "INFO",
    "DEBUG"
  };
  time_t fixed_time = 1633035745;
  int const offset = strftime(tmp_buffer,LOGGER_MESSAGE_BUFFER,"%c",localtime(&fixed_time));
  if(offset < 1) {
    fprintf(stderr,"Could not write time to buffer\n");
    return (void*)0;
  }
  if(snprintf(tmp_buffer + offset,LOGGER_MESSAGE_BUFFER," %-10s %s:%d - %s\n",log_level_mapping[log_level],file,filenumber,message) < 1) {
    fprintf(stderr,"Could not amalgamate Message - sprintf returned 0 bytes");
    return (void*)0;
  }
  memset(message,0,LOGGER_MESSAGE_BUFFER);
  memcpy(message,&tmp_buffer,LOGGER_MESSAGE_BUFFER);
  return message;
}

static void
tests_init_check(void **state) {
  assert_true(logger_setup_context(-1,(void*)0,tests_init_output,tests_init_transform,true) < 1);
  assert_true(logger_setup_context(LOGGER_DEBUG,(void*)0,(void*)0,tests_init_transform,true) < 1);
  assert_true(logger_setup_context(LOGGER_DEBUG,(void*)0,tests_init_output,(void*)0,true) < 1);
  assert_true(logger_setup_context(LOGGER_DEBUG,tests_output_simple,tests_init_output,tests_init_transform,true) > 0);
  assert_true(logger_get_status() == true);
  logger_toggle(false);
  assert_true(logger_get_status() == false);
  logger_toggle(true);
  assert_true(logger_get_status() == true);
}

static void
tests_simpleoutputs_check(void **state) {
  logger_info("This is one Test");
  assert_string_equal(tests_output_simple,"Thu Sep 30 23:02:25 2021 INFO       ./src/logger.c:456 - This is one Test\n");
  memset(tests_output_simple,0,LOGGER_MESSAGE_BUFFER);
  logger_debug("This is a parameter test: %s","parameter");
  assert_string_equal(tests_output_simple,"Thu Sep 30 23:02:25 2021 DEBUG      ./src/logger.c:459 - This is a parameter test: parameter\n");
  memset(tests_output_simple,0,LOGGER_MESSAGE_BUFFER);
  logger_debug("Another slightly longer message 000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000...");
  assert_string_equal(tests_output_simple,"Thu Sep 30 23:02:25 2021 DEBUG      ./src/logger.c:462 - Another slightly longer message 000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000...\n");
  logger_toggle(false);
  logger_debug("A string that will disappear");
  assert_string_equal(tests_output_simple,"Thu Sep 30 23:02:25 2021 DEBUG      ./src/logger.c:462 - Another slightly longer message 000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000...\n");
  logger_toggle(true);
  memset(tests_output_simple,0,LOGGER_MESSAGE_BUFFER);
}

static void
tests_simplefile_check(void **state) {
  assert_true(logger_factory_file(LOGGER_DEBUG,"./logger_tests_long_filename.txt") > 0);
  assert_true(logger_set_transform(tests_init_transform) == 1);
  logger_info("this is one exact test output");
  logger_debug("Another line in the file %s","lsdkfjlsdkfjsldfkjsldfkjsdflksjdflkjsdfklj");
  fflush(logger_factory_file_file);
  fclose(logger_factory_file_file);
  logger_factory_file_file = (void*)0;
  remove("./logger_tests_long_filename.txt");
}

static void
tests_simplecsv_check(void **state) {
  assert_true(logger_factory_csv(LOGGER_DEBUG,"./logger_tests_long_filename.csv") > 0);
  logger_info("this is the first test");
  logger_debug("about to finish the test: %s","02034020342394827394");
  for(size_t index = 0;index < 200;index++) {
    switch(index % (LOGGER_DEBUG + 1)) {
      case LOGGER_EMERGENCY:
        logger_emergency("a message that comes in from the loop");
        break;
      case LOGGER_ALERT:
        logger_alert("a message that comes in from the loop");
        break;
      case LOGGER_CRITICAL:
        logger_critical("a message that comes in from the loop");
        break;
      case LOGGER_ERROR:
        logger_error("a message that comes in from the loop");
        break;
      case LOGGER_WARNING:
        logger_warning("a message that comes in from the loop");
        break;
      case LOGGER_NOTICE:
        logger_notice("a message that comes in from the loop");
        break;
      case LOGGER_INFO:
        logger_info("a message that comes in from the loop");
        break;
      case LOGGER_DEBUG:
        logger_debug("a message that comes in from the loop");
        break;
    }
    sleep(1);
  }
  fflush(logger_factory_file_file);
  fclose(logger_factory_file_file);
  logger_factory_file_file = (void*)0;
}

int main(int argc,char *argv[argc]) {
  printf("Starting Test Suite ...\n");
  struct CMUnitTest const tests[] = {
    cmocka_unit_test(tests_preinit_check),
    cmocka_unit_test(tests_init_check),
    cmocka_unit_test(tests_simpleoutputs_check),
    cmocka_unit_test(tests_simplefile_check),
    cmocka_unit_test(tests_simplecsv_check),
  };
  return cmocka_run_group_tests(tests,(void*)0,(void*)0);
}

#endif /* Test Suite */

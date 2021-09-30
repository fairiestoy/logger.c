# Logger
A simple logging library in C for my personal use in general. It aims
to be flexible in regards to output formats and media.

To showcase this, a few default behaviours will be added with their
factory setup functions. This allows for quick-start logging without
having to supply/implement own functions.

The library has been inspired by [log.c by rxi](https://github.com/rxi/log.c),
however I wanted to implement something similar in my own flavor to
learn about some basic concepts.

## Usage
Both files should be dropped into any project without any dependencies (at
this point in time, that is). In case this is to be used in
conjunction with syslog, there are 8 function-like macros available:

```c
logger_emergency(...)
logger_alert(...)
logger_critical(...)
logger_error(...)
logger_warning(...)
logger_notice(...)
logger_info(...)
logger_debug(...)
```

To start the library, a setup function must be called. For Example:
```c
if(logger_setup_context(LOGGER_DEBUG,(void*)0,tests_init_output,tests_init_transform,true) <= 0)) {
  fprintf(stderr,"Could not initialize logging library\n");
  return;
}
```

A simpler version would be to use the provided factory functions:
```c
if(logger_factory_console(LOGGER_WARNING) <= 0)) {
  fprintf(stderr,"Could not initialize logging library\n");
  return;
}
```
Any call to a log_level above warning will be ignored. Please note, that in
this library, the most significant Log level has the lowest value. E.g.:
LOGGER_EMERGENCY = 0
LOGGER_DEBUG = 7

With this factory-function setup, the following call will result in:
```c
logger_info("This is one Test");
"Thu Sep 30 23:02:25 2021 INFO       logger.c:389 - This is one Test\n"
```

Messages can also be silenced in runtime.
```c
if(logger_factory_console(LOGGER_DEBUG) <= 0)) {
  fprintf(stderr,"Could not initialize logging library\n");
  return;
}
logger_debug("This message is visible");
logger_toggle(false);
logger_debug("This message is hidden / never pushed");
logger_toggle(true);
logger_debug("This message is visible");
```

The same applies to transform and output functions:
```c
if(logger_factory_console(LOGGER_DEBUG) <= 0)) {
  fprintf(stderr,"Could not initialize logging library\n");
  return;
}
logger_debug("This message is pushed with original output");
if(logger_set_output_callback(new_output_function) <= 0) {
  logger_error("Could not set new output callback");
  return;
}
logger_debug("This message will be pushed with new output");
```

Please refer to [logger.c](src/logger.c) for any additional information. The
functions should be self-explanatory with their comments.

## Some explanation
The one thing not necessarily clear from the [logger.c](src/logger.c) file
is, what the transform function is used for.
The transform function takes all possible arguments available and transforms
these into one buffer. This buffer is then pushed to the output function.

This should (in theory) allow users to replace either the transform or
output function to individually switch the output medium (from console to file
for example) or the string format (from simple text to csv).

-> Logger Macro(message) --- Message Buffer ---> Transform function --- Same Buffer ---> Output function

Check [console factory](src/logger.c#L44) for a sample implementation

## Thoughts
### Why is there no thread locking?
The idea was, to leave thread-handling to the output function with its
custom data object. If necessary, this can contain any locking and the
application can use its own equivalent without having to modify this
library if it used any existing locking.

### Why are there no color options?
The same thought on this - the basic implementation should only cover very
basic transform/output functions. It might add a factory version for this
at one point.

### Will examples follow?
With additional factory functions, I want to create more examples. This
includes:
- Simple Text File output
- Simple Network logging (pushing messages via network socket)
- Simple CSV Output
- Simple Binary/Mapped output

## License
Please refer to [License](license)

/*This is the first modle test to register ProcessMaker*/

#ifndef PHP_REGISTERPM_H
#define PHP_REGISTERPM_H 1

#define REGISTERPM_RELEASE 1

#define PHP_REGISTERPM_VERSION "0.0.1"
#define PHP_REGISTERPM_EXTNAME "registerPM"

#define REGISTERPM_MAX_MEMORY_STACK 48

typedef struct _register_memory_entry {
  int pointer;
  zval **addresses[REGISTERPM_MAX_MEMORY_STACK];
  struct _register_memory_entry *prev;
  struct _register_memory_entry *next;
} registerpm_memory_entry;

ZEND_BEGIN_MODULE_GLOBALS(registerPM)
  registerpm_memory_entry *start_memory;
  registerpm_memory_entry *active_memory;
  zend_bool   pm_enable;
#ifndef REGISTERPM_RELEASE
  int registerpm_stack_stats;
  int registerpm_number_grows;
#endif
ZEND_END_MODULE_GLOBALS(registerPM)//here: inside declare zend_registerPM_globals struct

ZEND_EXTERN_MODULE_GLOBALS(registerPM)

#ifdef ZTS
  #define REGISTERPM_GLOBAL(v) TSRMG(registerPM_globals_id, zend_registerPM_globals *, v)
#else
  #define REGISTERPM_GLOBAL(v) (registerPM_globals.v)
#endif

extern zend_module_entry registerpm_module_entry;
#define phpext_registerpm_ptr &registerpm_module_entry

#endif

#if PHP_VERSION_ID >= 50400
  #define REGISTERPM_INIT_FUNCS(class_functions) static const zend_function_entry class_functions[] =
#else
  #define REGISTERPM_INIT_FUNCS(class_functions) static const function_entry class_functions[] =
#endif

#ifndef PHP_FE_END
  #define PHP_FE_END { NULL, NULL, NULL, 0, 0 }
#endif

/** Define FASTCALL */
#if defined(__GNUC__) && ZEND_GCC_VERSION >= 3004 && defined(__i386__)
# define REGISTERPM_FASTCALL __attribute__((fastcall))
#elif defined(_MSC_VER) && defined(_M_IX86)
# define REGISTERPM_FASTCALL __fastcall
#else
# define REGISTERPM_FASTCALL
#endif


/**

New BSD License

*/

#include <stdio.h>

#ifdef HAVE_CONFIG_H
#include "config.h"
#endif

#include "php.h"
#include "php_registerPM.h"
#include "registerPM.h"

#include "main/php_main.h"
#include "main/php_streams.h"
#include "ext/standard/php_string.h"
#include "ext/standard/php_smart_str.h"
#include "ext/pdo/php_pdo_driver.h"
#include "ext/standard/php_filestat.h"

#include "Zend/zend_ini.h"
#include "Zend/zend_API.h"
#include "Zend/zend_operators.h"
#include "Zend/zend_exceptions.h"
#include "Zend/zend_interfaces.h"
#include "Zend/zend_execute.h"

#define PHQL_EQUALS                          1
#define PHQL_NOTEQUALS                       2
#define PHQL_LESS                            3
#define PHQL_GREATER                         4
#define PHQL_GREATEREQUAL                    5
#define PHQL_LESSEQUAL                       6
#define PHQL_AND                             7
#define PHQL_OR                              8
#define PHQL_LIKE                            9
#define PHQL_DIVIDE                         10
#define PHQL_TIMES                          11
#define PHQL_MOD                            12
#define PHQL_PLUS                           13
#define PHQL_MINUS                          14
#define PHQL_IS                             15
#define PHQL_IN                             16
#define PHQL_NOT                            17
#define PHQL_SELECT                         18
#define PHQL_FROM                           19
#define PHQL_COMMA                          20
#define PHQL_IDENTIFIER                     21
#define PHQL_DOT                            22
#define PHQL_AS                             23
#define PHQL_JOIN                           24
#define PHQL_ON                             25
#define PHQL_INNER                          26
#define PHQL_CROSS                          27
#define PHQL_LEFT                           28
#define PHQL_OUTER                          29
#define PHQL_RIGHT                          30
#define PHQL_FULL                           31
#define PHQL_INSERT                         32
#define PHQL_INTO                           33
#define PHQL_VALUES                         34
#define PHQL_BRACKET_OPEN                   35
#define PHQL_BRACKET_CLOSE                  36
#define PHQL_UPDATE                         37
#define PHQL_SET                            38
#define PHQL_DELETE                         39
#define PHQL_WHERE                          40
#define PHQL_ORDER                          41
#define PHQL_BY                             42
#define PHQL_ASC                            43
#define PHQL_DESC                           44
#define PHQL_INTEGER                        45
#define PHQL_GROUP                          46
#define PHQL_HAVING                         47
#define PHQL_LIMIT                          48
#define PHQL_OFFSET                         49
#define PHQL_NULL                           50
#define PHQL_STRING                         51
#define PHQL_DOUBLE                         52
#define PHQL_NPLACEHOLDER                   53
#define PHQL_SPLACEHOLDER                   54




#define PHQL_SCANNER_RETCODE_EOF -1
#define PHQL_SCANNER_RETCODE_ERR -2
#define PHQL_SCANNER_RETCODE_IMPOSSIBLE -3

#define PHQL_T_IGNORE 257

/* Literals & Identifiers */
#define PHQL_T_INTEGER 258
#define PHQL_T_DOUBLE 259
#define PHQL_T_STRING 260
#define PHQL_T_IDENTIFIER 265

/* Operators */
#define PHQL_T_ADD '+'
#define PHQL_T_SUB '-'
#define PHQL_T_MUL '*'
#define PHQL_T_DIV '/'
#define PHQL_T_MOD '%'
#define PHQL_T_AND 266
#define PHQL_T_OR 267
#define PHQL_T_LIKE 268

#define PHQL_T_DOT '.'
#define PHQL_T_COMMA 269

#define PHQL_T_EQUALS '='
#define PHQL_T_NOTEQUALS 270
#define PHQL_T_NOT '!'
#define PHQL_T_LESS '<'
#define PHQL_T_LESSEQUAL 271
#define PHQL_T_GREATER '>'
#define PHQL_T_GREATEREQUAL 272

#define PHQL_T_BRACKET_OPEN '('
#define PHQL_T_BRACKET_CLOSE ')'

/** Placeholders */
#define PHQL_T_NPLACEHOLDER 273
#define PHQL_T_SPLACEHOLDER 274

/** Reserved words */
#define PHQL_T_UPDATE 300
#define PHQL_T_SET 301
#define PHQL_T_WHERE 302
#define PHQL_T_DELETE 303
#define PHQL_T_FROM 304
#define PHQL_T_AS 305
#define PHQL_T_INSERT 306
#define PHQL_T_INTO 307
#define PHQL_T_VALUES 308
#define PHQL_T_SELECT 309
#define PHQL_T_ORDER 310
#define PHQL_T_BY 311
#define PHQL_T_LIMIT 312
#define PHQL_T_GROUP 313
#define PHQL_T_HAVING 314
#define PHQL_T_IN 315
#define PHQL_T_ON 316
#define PHQL_T_INNER 317
#define PHQL_T_JOIN 318
#define PHQL_T_LEFT 319
#define PHQL_T_RIGHT 320
#define PHQL_T_IS 321
#define PHQL_T_NULL 322
#define PHQL_T_NOTIN 323
#define PHQL_T_CROSS 324
#define PHQL_T_FULL 325
#define PHQL_T_OUTER 326
#define PHQL_T_ASC 327
#define PHQL_T_DESC 328
#define PHQL_T_OFFSET 329

/** Special Tokens */
#define PHQL_T_FCALL 350
#define PHQL_T_NLIKE 351
#define PHQL_T_ALL 352
#define PHQL_T_DOMAINALL 353
#define PHQL_T_EXPR 354
#define PHQL_T_QUALIFIED 355
#define PHQL_T_ENCLOSED 356

#define PHQL_T_INNERJOIN 360
#define PHQL_T_LEFTJOIN 361
#define PHQL_T_RIGHTJOIN 362
#define PHQL_T_CROSSJOIN 363
#define PHQL_T_FULLOUTER 364
#define PHQL_T_ISNULL 365
#define PHQL_T_ISNOTNULL 366
#define PHQL_T_MINUS 367

/* list of tokens and their names */
typedef struct _phql_token_names
{
    unsigned int code;
    char *name;
} phql_token_names;

/* active token state */
typedef struct _phql_scanner_state {
  int active_token;
	char* start;
	char* end;
} phql_scanner_state;

/* extra information tokens */
typedef struct _phql_scanner_token {
	int opcode;
	char *value;
	int len;
} phql_scanner_token;

int phql_get_token(phql_scanner_state *s, phql_scanner_token *token);

const phql_token_names phql_tokens[];




typedef struct _phql_parser_token {
	int opcode;
	char *token;
	int token_len;
	int free_flag;
} phql_parser_token;

typedef struct _phql_parser_status {
	int status;
	zval *ret;
	phql_scanner_state *scanner_state;
	char *syntax_error;
	zend_uint syntax_error_len;
} phql_parser_status;

#define PHQL_PARSING_OK 1
#define PHQL_PARSING_FAILED 0

int phql_parse_phql(zval *result, zval *phql TSRMLS_DC);
int phql_internal_parse_phql(zval **result, char *phql, zval **error_msg TSRMLS_DC);


#define PHVOLT_COMMA                           1
#define PHVOLT_SBRACKET_OPEN                   2
#define PHVOLT_RANGE                           3
#define PHVOLT_IS                              4
#define PHVOLT_EQUALS                          5
#define PHVOLT_NOTEQUALS                       6
#define PHVOLT_LESS                            7
#define PHVOLT_GREATER                         8
#define PHVOLT_GREATEREQUAL                    9
#define PHVOLT_LESSEQUAL                      10
#define PHVOLT_IDENTICAL                      11
#define PHVOLT_NOTIDENTICAL                   12
#define PHVOLT_AND                            13
#define PHVOLT_OR                             14
#define PHVOLT_PIPE                           15
#define PHVOLT_DIVIDE                         16
#define PHVOLT_TIMES                          17
#define PHVOLT_MOD                            18
#define PHVOLT_PLUS                           19
#define PHVOLT_MINUS                          20
#define PHVOLT_CONCAT                         21
#define PHVOLT_NOT                            22
#define PHVOLT_DOT                            23
#define PHVOLT_OPEN_DELIMITER                 24
#define PHVOLT_IF                             25
#define PHVOLT_CLOSE_DELIMITER                26
#define PHVOLT_ENDIF                          27
#define PHVOLT_ELSE                           28
#define PHVOLT_FOR                            29
#define PHVOLT_IN                             30
#define PHVOLT_ENDFOR                         31
#define PHVOLT_SET                            32
#define PHVOLT_ASSIGN                         33
#define PHVOLT_OPEN_EDELIMITER                34
#define PHVOLT_CLOSE_EDELIMITER               35
#define PHVOLT_BLOCK                          36
#define PHVOLT_IDENTIFIER                     37
#define PHVOLT_ENDBLOCK                       38
#define PHVOLT_EXTENDS                        39
#define PHVOLT_STRING                         40
#define PHVOLT_INCLUDE                        41
#define PHVOLT_RAW_FRAGMENT                   42
#define PHVOLT_DEFINED                        43
#define PHVOLT_BRACKET_OPEN                   44
#define PHVOLT_BRACKET_CLOSE                  45
#define PHVOLT_SBRACKET_CLOSE                 46
#define PHVOLT_DOUBLECOLON                    47
#define PHVOLT_INTEGER                        48
#define PHVOLT_DOUBLE                         49
#define PHVOLT_NULL                           50
#define PHVOLT_FALSE                          51
#define PHVOLT_TRUE                           52




#define PHVOLT_RAW_BUFFER_SIZE 256

#define PHVOLT_SCANNER_RETCODE_EOF -1
#define PHVOLT_SCANNER_RETCODE_ERR -2
#define PHVOLT_SCANNER_RETCODE_IMPOSSIBLE -3

/** Modes */
#define PHVOLT_MODE_RAW 0
#define PHVOLT_MODE_CODE 1
#define PHVOLT_MODE_COMMENT 2

#define PHVOLT_T_IGNORE 257

/* Literals & Identifiers */
#define PHVOLT_T_INTEGER 258
#define PHVOLT_T_DOUBLE 259
#define PHVOLT_T_STRING 260
#define PHVOLT_T_NULL 261
#define PHVOLT_T_FALSE 262
#define PHVOLT_T_TRUE 263
#define PHVOLT_T_IDENTIFIER 265

/* Operators */
#define PHVOLT_T_ADD '+'
#define PHVOLT_T_SUB '-'
#define PHVOLT_T_MUL '*'
#define PHVOLT_T_DIV '/'
#define PHVOLT_T_MOD '%'
#define PHVOLT_T_AND 266
#define PHVOLT_T_OR 267
#define PHVOLT_T_CONCAT '~'
#define PHVOLT_T_PIPE '|'

#define PHVOLT_T_DOT '.'
#define PHVOLT_T_COMMA 269

#define PHVOLT_T_NOT '!'
#define PHVOLT_T_LESS '<'
#define PHVOLT_T_LESSEQUAL 270
#define PHVOLT_T_GREATER '>'
#define PHVOLT_T_GREATEREQUAL 271
#define PHVOLT_T_EQUALS 272
#define PHVOLT_T_NOTEQUALS 273
#define PHVOLT_T_IDENTICAL 274
#define PHVOLT_T_NOTIDENTICAL 275
#define PHVOLT_T_RANGE 276
#define PHVOLT_T_ASSIGN '='
#define PHVOLT_T_DOUBLECOLON 277

#define PHVOLT_T_BRACKET_OPEN '('
#define PHVOLT_T_BRACKET_CLOSE ')'
#define PHVOLT_T_SBRACKET_OPEN '['
#define PHVOLT_T_SBRACKET_CLOSE ']'

/** Reserved words */
#define PHVOLT_T_IF 300
#define PHVOLT_T_ELSE 301
#define PHVOLT_T_ENDIF 303
#define PHVOLT_T_FOR 304
#define PHVOLT_T_ENDFOR 305
#define PHVOLT_T_SET 306
#define PHVOLT_T_BLOCK 307
#define PHVOLT_T_ENDBLOCK 308
#define PHVOLT_T_IN 309
#define PHVOLT_T_EXTENDS 310
#define PHVOLT_T_IS 311
#define PHVOLT_T_DEFINED 312
#define PHVOLT_T_INCLUDE 313

/** Delimiters */
#define PHVOLT_T_OPEN_DELIMITER  330
#define PHVOLT_T_CLOSE_DELIMITER  331
#define PHVOLT_T_OPEN_EDELIMITER  332
#define PHVOLT_T_CLOSE_EDELIMITER  333

/** Special Tokens */
#define PHVOLT_T_FCALL 350
#define PHVOLT_T_EXPR 354
#define PHVOLT_T_QUALIFIED 355
#define PHVOLT_T_ENCLOSED 356
#define PHVOLT_T_RAW_FRAGMENT 357
#define PHVOLT_T_EMPTY 358
#define PHVOLT_T_ECHO 359
#define PHVOLT_T_ARRAY 360
#define PHVOLT_T_ARRAYACCESS 361
#define PHVOLT_T_NOT_ISSET 362
#define PHVOLT_T_ISSET 363

#define PHVOLT_T_MINUS 367

/* list of tokens and their names */
typedef struct _phvolt_token_names
{
	unsigned int code;
	char *name;
} phvolt_token_names;

/* active token state */
typedef struct _phvolt_scanner_state {
	int active_token;
	char* start;
	char* end;
	int mode;
	unsigned int active_line;
	unsigned int statement_position;
	unsigned int extends_mode;
	unsigned int block_level;
	char *raw_buffer;
	unsigned int raw_buffer_cursor;
	unsigned int raw_buffer_size;
} phvolt_scanner_state;

/* extra information tokens */
typedef struct _phvolt_scanner_token {
	int opcode;
	char *value;
	int len;
} phvolt_scanner_token;

int phvolt_get_token(phvolt_scanner_state *s, phvolt_scanner_token *token);

const phvolt_token_names phvolt_tokens[];





typedef struct _phvolt_parser_token {
	int opcode;
	char *token;
	int token_len;
	int free_flag;
} phvolt_parser_token;

typedef struct _phvolt_parser_status {
	int status;
	zval *ret;
	phvolt_scanner_state *scanner_state;
	char *syntax_error;
	zend_uint syntax_error_len;
} phvolt_parser_status;

#define PHVOLT_PARSING_OK 1
#define PHVOLT_PARSING_FAILED 0

int phvolt_parse_view(zval *result, zval *view_code TSRMLS_DC);
int phvolt_internal_parse_view(zval **result, char *view_code, unsigned int view_length, zval **error_msg TSRMLS_DC);





/** Main macros */
#define PH_DEBUG 0

#define PH_NOISY 0
#define PH_SILENT 1

#define PH_NOISY_CC PH_NOISY TSRMLS_CC
#define PH_SILENT_CC PH_SILENT TSRMLS_CC

#define PH_CHECK 1
#define PH_NO_CHECK 0

#define PH_SEPARATE 256
#define PH_COPY 1024
#define PH_CTOR 4096

#define PH_FETCH_CLASS_SILENT (zend_bool) ZEND_FETCH_CLASS_SILENT TSRMLS_CC

#define SL(str) ZEND_STRL(str)
#define SS(str) ZEND_STRS(str)

/** SPL dependencies */
#if defined(HAVE_SPL) && ((PHP_MAJOR_VERSION > 5) || (PHP_MAJOR_VERSION == 5 && PHP_MINOR_VERSION >= 1))
extern ZEND_API zend_class_entry *zend_ce_iterator;
extern ZEND_API zend_class_entry *zend_ce_arrayaccess;
extern ZEND_API zend_class_entry *zend_ce_serializable;
extern PHPAPI zend_class_entry *spl_ce_RuntimeException;
extern PHPAPI zend_class_entry *spl_ce_Countable;
extern PHPAPI zend_class_entry *spl_ce_SeekableIterator;
#endif

/** Startup functions */
void php_registerPM_init_globals(zend_registerPM_globals *phalcon_globals TSRMLS_DC);

/** Globals functions */
int registerPM_init_global(char *global, unsigned int global_length TSRMLS_DC);
int registerPM_get_global(zval **arr, char *global, unsigned int global_length TSRMLS_DC);
int phalcon_get_global_by_index(char *global, char *index, zval *result TSRMLS_DC);

int registerPM_is_callable(zval *var TSRMLS_DC);
int registerPM_function_exists_ex(char *method_name, unsigned int method_len TSRMLS_DC);

/** Count */
void registerPM_fast_count(zval *result, zval *array TSRMLS_DC);
int registerPM_fast_count_ev(zval *array TSRMLS_DC);

/** Low level filters */
int registerPM_filter_alphanum(zval *result, zval *param);
int registerPM_filter_identifier(zval *result, zval *param);

/* Utils functions */
void registerPM_inherit_not_found(char *class_name, char *inherit_name);
int registerPM_valid_foreach(zval *arr TSRMLS_DC);

/** Export symbols to active symbol table */
int registerPM_set_symbol(zval *key_name, zval *value TSRMLS_DC);
int registerPM_set_symbol_str(char *key_name, unsigned int key_length, zval *value TSRMLS_DC);

/** Compatibility with PHP 5.3 */
#ifndef ZVAL_COPY_VALUE
 #define ZVAL_COPY_VALUE(z, v)\
  (z)->value = (v)->value;\
  Z_TYPE_P(z) = Z_TYPE_P(v);
#endif

#ifndef INIT_PZVAL_COPY
 #define INIT_PZVAL_COPY(z, v) ZVAL_COPY_VALUE(z, v);\
  Z_SET_REFCOUNT_P(z, 1);\
  Z_UNSET_ISREF_P(z);
#endif

/** Symbols */
#define REGISTERPM_READ_SYMBOL(var, auxarr, name) if (EG(active_symbol_table)){ \
	if (zend_hash_find(EG(active_symbol_table), name, sizeof(name), (void **)  &auxarr) == SUCCESS) { \
			var = *auxarr; \
		} else { \
			ZVAL_NULL(var); \
		} \
	} else { \
		ZVAL_NULL(var); \
	}

#define RETURN_CCTOR(var) { \
		*(return_value) = *(var); \
		if (Z_TYPE_P(var) > IS_BOOL) { \
			registerPM_copy_ctor(return_value, var); \
		} \
		INIT_PZVAL(return_value) \
	} \
	REGISTERPM_MM_RESTORE(); \
	return;

#define RETURN_CCTORW(var) { \
		*(return_value) = *(var); \
		if (Z_TYPE_P(var) > IS_BOOL) { \
			registerPM_copy_ctor(return_value, var); \
		} \
		INIT_PZVAL(return_value) \
	} \
	return;

#define RETURN_CTOR(var) { \
		*(return_value) = *(var); \
		registerPM_copy_ctor(return_value, var); \
		INIT_PZVAL(return_value) \
	} \
	REGISTERPM_MM_RESTORE(); \
	return;

#define RETURN_CTORW(var) { \
		*(return_value) = *(var); \
		registerPM_copy_ctor(return_value, var); \
		INIT_PZVAL(return_value) \
	} \
	return;

#define RETURN_NCTOR(var) { \
		*(return_value) = *(var); \
		INIT_PZVAL(return_value) \
	} \
	REGISTERPM_MM_RESTORE(); \
	return;

#define RETURN_NCTORW(var) { \
		*(return_value) = *(var); \
		INIT_PZVAL(return_value) \
	} \
	return;

#define RETURN_SCTOR() \
	if (Z_TYPE_P(return_value) > IS_BOOL) { \
		zval_copy_ctor(return_value); \
	} \
	REGISTERPM_MM_RESTORE(); \
	return;

#define RETURN_SCTORW() \
	if (Z_TYPE_P(return_value) > IS_BOOL) { \
		zval_copy_ctor(return_value); \
	} \
	return;

/** Foreach */
#define REGISTERPM_GET_FOREACH_KEY(var, hash, hash_pointer) \
	REGISTERPM_INIT_NVAR(var); \
	hash_type = zend_hash_get_current_key_ex(hash, &hash_index, &hash_index_len, &hash_num, 0, &hash_pointer); \
	if (hash_type == HASH_KEY_IS_STRING) { \
		ZVAL_STRINGL(var, hash_index, hash_index_len-1, 1); \
	} else { \
		if (hash_type == HASH_KEY_IS_LONG) { \
			ZVAL_LONG(var, hash_num); \
		}\
	}

#define REGISTERPM_GET_FOREACH_VALUE(var) \
	REGISTERPM_OBSERVE_VAR(var); \
	var = *hd; \
	Z_ADDREF_P(var);

/** class registering */
#define REGISTERPM_REGISTER_CLASS(ns, classname, name, methods, flags) \
	{ \
		zend_class_entry ce; \
		memset(&ce, 0, sizeof(zend_class_entry)); \
		INIT_NS_CLASS_ENTRY(ce, #ns, #classname, methods); \
		registerPM_ ##name## _ce = zend_register_internal_class(&ce TSRMLS_CC); \
		registerPM_ ##name## _ce->ce_flags |= flags;  \
	}

#define REGISTERPM_REGISTER_CLASS_EX(ns, class_name, name, parent, methods, flags) \
	{ \
		zend_class_entry ce; \
		memset(&ce, 0, sizeof(zend_class_entry)); \
		INIT_NS_CLASS_ENTRY(ce, #ns, #class_name, methods); \
		registerPM_ ##name## _ce = zend_register_internal_class_ex(&ce, NULL, parent TSRMLS_CC); \
		if(!registerPM_ ##name## _ce){ \
			registerPM_inherit_not_found(parent, ZEND_NS_NAME(#ns, #class_name)); \
			return FAILURE;	\
		}  \
		registerPM_ ##name## _ce->ce_flags |= flags;  \
	}




void registerPM_init_nvar(zval **var TSRMLS_DC);
void registerPM_cpy_wrt(zval **dest, zval *var TSRMLS_DC);
void registerPM_cpy_wrt_ctor(zval **dest, zval *var TSRMLS_DC);

int REGISTERPM_FASTCALL registerPM_memory_grow_stack(TSRMLS_D);
int REGISTERPM_FASTCALL registerPM_memory_restore_stack(TSRMLS_D);

int REGISTERPM_FASTCALL registerPM_memory_observe(zval **var TSRMLS_DC);
int REGISTERPM_FASTCALL registerPM_memory_remove(zval **var TSRMLS_DC);
int REGISTERPM_FASTCALL registerPM_memory_alloc(zval **var TSRMLS_DC);

int REGISTERPM_FASTCALL registerPM_clean_shutdown_stack(TSRMLS_D);
int REGISTERPM_FASTCALL registerPM_clean_restore_stack(TSRMLS_D);

void REGISTERPM_FASTCALL registerPM_copy_ctor(zval *destiny, zval *origin);

#define REGISTERPM_MM_GROW() registerPM_memory_grow_stack(TSRMLS_C)
#define REGISTERPM_MM_RESTORE() phalcon_memory_restore_stack(TSRMLS_C)

/** Memory macros */
#define REGISTERPM_ALLOC_ZVAL(z) \
	ALLOC_ZVAL(z); INIT_PZVAL(z); ZVAL_NULL(z);

#define REGISTERPM_INIT_VAR(z) \
	REGISTERPM_ALLOC_ZVAL(z); \
	registerPM_memory_observe(&z TSRMLS_CC);

//#ifndef PHP_WIN32

#define REGISTERPM_INIT_NVAR(z)\
	if (z) { \
		if (Z_REFCOUNT_P(z) > 1) { \
			Z_DELREF_P(z); \
			ALLOC_ZVAL(z); \
			Z_SET_REFCOUNT_P(z, 1); \
			Z_UNSET_ISREF_P(z); \
			ZVAL_NULL(z); \
		} else {\
			zval_ptr_dtor(&z); \
			REGISTERPM_ALLOC_ZVAL(z); \
		} \
	} else { \
		registerPM_memory_alloc(&z TSRMLS_CC); \
	}

#define REGISTERPM_CPY_WRT(d, v) \
	if (d) { \
		if (Z_REFCOUNT_P(d) > 0) { \
			zval_ptr_dtor(&d); \
		} \
	} else { \
		registerPM_memory_observe(&d TSRMLS_CC); \
	} \
	Z_ADDREF_P(v); \
	d = v;

#define REGISTERPM_CPY_WRT_CTOR(d, v) \
	if (d) { \
		if (Z_REFCOUNT_P(d) > 0) { \
			zval_ptr_dtor(&d); \
		} \
	} else { \
		registerPM_memory_observe(&d TSRMLS_CC); \
	} \
	ALLOC_ZVAL(d); \
	*d = *v; \
	zval_copy_ctor(d); \
	Z_SET_REFCOUNT_P(d, 1); \
	Z_UNSET_ISREF_P(d);

//#else

//#define REGISTERPM_INIT_NVAR(z) registerPM_init_nvar(&z TSRMLS_CC)
//#define REGISTERPM_CPY_WRT(d, v) registerPM_cpy_wrt(&d, v TSRMLS_CC)
//#define REGISTERPM_CPY_WRT_CTOR(d, v) registerPM_cpy_wrt_ctor(&d, v TSRMLS_CC)

//#endif

#define REGISTERPM_ALLOC_ZVAL_MM(z) \
	REGISTERPM_ALLOC_ZVAL(z); \
	registerPM_memory_observe(&z TSRMLS_CC);

#define REGISTERPM_SEPARATE_ARRAY(a) \
	{ \
		if (Z_REFCOUNT_P(a) > 1) { \
			zval *new_zv; \
			Z_DELREF_P(a); \
			ALLOC_ZVAL(new_zv); \
			INIT_PZVAL_COPY(new_zv, a); \
			a = new_zv; \
			zval_copy_ctor(new_zv); \
		} \
	}

#define REGISTERPM_SEPARATE(z) \
	{ \
		zval *orig_ptr = z; \
		if (Z_REFCOUNT_P(orig_ptr) > 1) { \
			Z_DELREF_P(orig_ptr); \
			ALLOC_ZVAL(z); \
			registerPM_memory_observe(&z TSRMLS_CC); \
			*z = *orig_ptr; \
			zval_copy_ctor(z); \
			Z_SET_REFCOUNT_P(z, 1); \
			Z_UNSET_ISREF_P(z); \
		} \
	}

#define REGISTERPM_SEPARATE_NMO(z) \
	{\
		zval *orig_ptr = z;\
		if (Z_REFCOUNT_P(orig_ptr) > 1) {\
			Z_DELREF_P(orig_ptr);\
			ALLOC_ZVAL(z);\
			*z = *orig_ptr;\
			zval_copy_ctor(z);\
			Z_SET_REFCOUNT_P(z, 1);\
			Z_UNSET_ISREF_P(z);\
		}\
	}

#define REGISTERPM_SEPARATE_PARAM(z) \
	{\
		zval *orig_ptr = z;\
		if (Z_REFCOUNT_P(orig_ptr) > 1) {\
			ALLOC_ZVAL(z);\
			registerPM_memory_observe(&z TSRMLS_CC);\
			*z = *orig_ptr;\
			zval_copy_ctor(z);\
			Z_SET_REFCOUNT_P(z, 1);\
			Z_UNSET_ISREF_P(z);\
		}\
	}

#define REGISTERPM_SEPARATE_PARAM_NMO(z) { \
		zval *orig_ptr = z; \
		if (Z_REFCOUNT_P(orig_ptr) > 1) { \
			ALLOC_ZVAL(z); \
			*z = *orig_ptr; \
			zval_copy_ctor(z); \
			Z_SET_REFCOUNT_P(z, 1); \
			Z_UNSET_ISREF_P(z); \
		} \
	}

#define REGISTERPM_OBSERVE_VAR(var) \
	if (!var) { \
		registerPM_memory_observe(&var TSRMLS_CC); \
	} else { \
		zval_ptr_dtor(&var); \
	}




#define REGISTERPM_CALL_FUNC(return_value, func_name) if(phalcon_call_func(return_value, func_name, strlen(func_name), 1 TSRMLS_CC)==FAILURE) return;
#define REGISTERPM_CALL_FUNC_NORETURN(func_name) if(phalcon_call_func(NULL, func_name, strlen(func_name), 0 TSRMLS_CC)==FAILURE) return;
#define REGISTERPM_CALL_FUNC_PARAMS(return_value, func_name, param_count, params) if(phalcon_call_func_params(return_value, func_name, strlen(func_name), param_count, params, 1 TSRMLS_CC)==FAILURE) return;
#define REGISTERPM_CALL_FUNC_PARAMS_NORETURN(func_name, param_count, params) if(phalcon_call_func_params(NULL, func_name, strlen(func_name), param_count, params, 0 TSRMLS_CC)==FAILURE) return;
#define REGISTERPM_CALL_FUNC_PARAMS_1(return_value, func_name, param1) if(phalcon_call_func_one_param(return_value, func_name, strlen(func_name), param1, 1 TSRMLS_CC)==FAILURE) return;
#define REGISTERPM_CALL_FUNC_PARAMS_1_NORETURN(func_name, param1) if(phalcon_call_func_one_param(NULL, func_name, strlen(func_name), param1, 0 TSRMLS_CC)==FAILURE) return;
#define REGISTERPM_CALL_FUNC_PARAMS_2(return_value, func_name, param1, param2) if(phalcon_call_func_two_params(return_value, func_name, strlen(func_name), param1, param2, 1 TSRMLS_CC)==FAILURE) return;
#define REGISTERPM_CALL_FUNC_PARAMS_2_NORETURN(func_name, param1, param2) if(phalcon_call_func_two_params(NULL, func_name, strlen(func_name), param1, param2, 0 TSRMLS_CC)==FAILURE) return;
#define REGISTERPM_CALL_FUNC_PARAMS_3(return_value, func_name, param1, param2, param3) if(phalcon_call_func_three_params(return_value, func_name, strlen(func_name), param1, param2, param3, 1 TSRMLS_CC)==FAILURE) return;
#define REGISTERPM_CALL_FUNC_PARAMS_3_NORETURN(func_name, param1, param2, param3) if(phalcon_call_func_three_params(NULL, func_name, strlen(func_name), param1, param2, param3, 0 TSRMLS_CC)==FAILURE) return;

#define REGISTERPM_CALL_METHOD(return_value, object, method_name, check) if(phalcon_call_method(return_value, object, method_name, strlen(method_name), check, 1 TSRMLS_CC)==FAILURE) return;
#define REGISTERPM_CALL_METHOD_NORETURN(object, method_name, check) if(phalcon_call_method(NULL, object, method_name, strlen(method_name), check, 0 TSRMLS_CC)==FAILURE) return;
#define REGISTERPM_CALL_METHOD_PARAMS(return_value, object, method_name, param_count, params, check) if(phalcon_call_method_params(return_value, object, method_name, strlen(method_name), param_count, params, check, 1 TSRMLS_CC)==FAILURE) return;
#define REGISTERPM_CALL_METHOD_PARAMS_NORETURN(object, method_name, param_count, params, check) if(phalcon_call_method_params(NULL, object, method_name, strlen(method_name), param_count, params, check, 0 TSRMLS_CC)==FAILURE) return;
#define REGISTERPM_CALL_METHOD_PARAMS_1(return_value, object, method_name, param1, check) if(phalcon_call_method_one_param(return_value, object, method_name, strlen(method_name), param1, check, 1 TSRMLS_CC)==FAILURE) return;
#define REGISTERPM_CALL_METHOD_PARAMS_1_NORETURN(object, method_name, param1, check) if(phalcon_call_method_one_param(NULL, object, method_name, strlen(method_name), param1, check, 0 TSRMLS_CC)==FAILURE) return;
#define REGISTERPM_CALL_METHOD_PARAMS_2(return_value, object, method_name, param1, param2, check) if(phalcon_call_method_two_params(return_value, object, method_name, strlen(method_name), param1, param2, check, 1 TSRMLS_CC)==FAILURE) return;
#define REGISTERPM_CALL_METHOD_PARAMS_2_NORETURN(object, method_name, param1, param2, check) if(phalcon_call_method_two_params(NULL, object, method_name, strlen(method_name), param1, param2, check, 0 TSRMLS_CC)==FAILURE) return;
#define REGISTERPM_CALL_METHOD_PARAMS_3(return_value, object, method_name, param1, param2, param3, check) if(phalcon_call_method_three_params(return_value, object, method_name, strlen(method_name), param1, param2, param3, check, 1 TSRMLS_CC)==FAILURE) return;
#define REGISTERPM_CALL_METHOD_PARAMS_3_NORETURN(object, method_name, param1, param2, param3, check) if(phalcon_call_method_three_params(NULL, object, method_name, strlen(method_name), param1, param2, param3, check, 0 TSRMLS_CC)==FAILURE) return;
#define REGISTERPM_CALL_METHOD_PARAMS_4(return_value, object, method_name, param1, param2, param3, param4, check) if(phalcon_call_method_four_params(return_value, object, method_name, strlen(method_name), param1, param2, param3, param4, check, 1 TSRMLS_CC)==FAILURE) return;
#define REGISTERPM_CALL_METHOD_PARAMS_4_NORETURN(object, method_name, param1, param2, param3, param4, check) if(phalcon_call_method_four_params(NULL, object, method_name, strlen(method_name), param1, param2, param3, param4, check, 0 TSRMLS_CC)==FAILURE) return;
#define REGISTERPM_CALL_METHOD_PARAMS_5(return_value, object, method_name, param1, param2, param3, param4, param5, check) if(phalcon_call_method_five_params(return_value, object, method_name, strlen(method_name), param1, param2, param3, param4, param5, check, 1 TSRMLS_CC)==FAILURE) return;
#define REGISTERPM_CALL_METHOD_PARAMS_5_NORETURN(object, method_name, param1, param2, param3, param4, param5, check) if(phalcon_call_method_five_params(NULL, object, method_name, strlen(method_name), param1, param2, param3, param4, param5, check, 0 TSRMLS_CC)==FAILURE) return;

#define REGISTERPM_CALL_PARENT_PARAMS(return_value, object, active_class, method_name, param_count, params) if(phalcon_call_parent_func_params(return_value, object, active_class, strlen(active_class), method_name, strlen(method_name), param_count, params, 1 TSRMLS_CC)==FAILURE) return;
#define REGISTERPM_CALL_PARENT_PARAMS_NORETURN(object, active_class, method_name, param_count, params) if(phalcon_call_parent_func_params(NULL, object, active_class, strlen(active_class),method_name, strlen(method_name), param_count, params, 0 TSRMLS_CC)==FAILURE) return;
#define REGISTERPM_CALL_PARENT_PARAMS_1(return_value, object, active_class, method_name, param1) if(phalcon_call_parent_func_one_param(return_value, object, active_class, strlen(active_class), method_name, strlen(method_name), param1, 1 TSRMLS_CC)==FAILURE) return;
#define REGISTERPM_CALL_PARENT_PARAMS_1_NORETURN(object, active_class, method_name, param1) if(phalcon_call_parent_func_one_param(NULL, object, active_class, strlen(active_class),method_name, strlen(method_name), param1, 0 TSRMLS_CC)==FAILURE) return;
#define REGISTERPM_CALL_PARENT_PARAMS_2(return_value, object, active_class, method_name, param1, param2) if(phalcon_call_parent_func_two_params(return_value, object, active_class, strlen(active_class), method_name, strlen(method_name), param1, param2, 1 TSRMLS_CC)==FAILURE) return;
#define REGISTERPMCALL_PARENT_PARAMS_2_NORETURN(object, active_class, method_name, param1, param2) if(phalcon_call_parent_func_two_params(NULL, object, active_class, strlen(active_class),method_name, strlen(method_name), param1, param2, 0 TSRMLS_CC)==FAILURE) return;
#define REGISTERPM_CALL_PARENT_PARAMS_3(return_value, object, active_class, method_name, param1, param2, param3) if(phalcon_call_parent_func_three_params(return_value, object, active_class, strlen(active_class), method_name, strlen(method_name), param1, param2, param3 1 TSRMLS_CC)==FAILURE) return;
#define REGISTERPM_CALL_PARENT_PARAMS_3_NORETURN(object, active_class, method_name, param1, param2, param3) if(phalcon_call_parent_func_three_params(NULL, object, active_class, strlen(active_class),method_name, strlen(method_name), param1, param2, param3, 0 TSRMLS_CC)==FAILURE) return;
#define REGISTERPM_CALL_PARENT(return_value, object, active_class, method_name) if(phalcon_call_parent_func(return_value, object, active_class, strlen(active_class),method_name, strlen(method_name), 1 TSRMLS_CC)==FAILURE) return;
#define REGISTERPM_CALL_PARENT_NORETURN(object, active_class, method_name) if(phalcon_call_parent_func(NULL, object, active_class, strlen(active_class),method_name, strlen(method_name), 0 TSRMLS_CC)==FAILURE) return;;

#define REGISTERPM_CALL_SELF_PARAMS(return_value, object, method_name, param_count, params) if(phalcon_call_self_func_params(return_value, object, method_name, strlen(method_name), param_count, params, 1 TSRMLS_CC)==FAILURE) return;
#define REGISTERPM_CALL_SELF_PARAMS_NORETURN(object, method_name, param_count, params) if(phalcon_call_self_func_params(NULL, object, method_name, strlen(method_name), param_count, params, 0 TSRMLS_CC)==FAILURE) return;
#define REGISTERPM_CALL_SELF_PARAMS_1(return_value, object, method_name, param1) if(phalcon_call_self_func_one_param(return_value, object, method_name, strlen(method_name), param1, 1 TSRMLS_CC)==FAILURE) return;
#define REGISTERPM_CALL_SELF_PARAMS_1_NORETURN(object, method_name, param1) if(phalcon_call_self_func_one_param(NULL, object, method_name, strlen(method_name), param1, 0 TSRMLS_CC)==FAILURE) return;
#define REGISTERPM_CALL_SELF_PARAMS_2(return_value, object, method_name, param1, param2) if(phalcon_call_self_func_two_params(return_value, object, method_name, strlen(method_name), param1, param2, 1 TSRMLS_CC)==FAILURE) return;
#define REGISTERPM_CALL_SELF_PARAMS_2_NORETURN(object, method_name, param1, param2) if(phalcon_call_self_func_two_params(NULL, object, method_name, strlen(method_name), param1, param2, 0 TSRMLS_CC)==FAILURE) return;
#define REGISTERPM_CALL_SELF_PARAMS_3(return_value, object, method_name, param1, param2, param3) if(phalcon_call_self_func_three_params(return_value, object, method_name, strlen(method_name), param1, param2, param3, 1 TSRMLS_CC)==FAILURE) return;
#define REGISTERPM_CALL_SELF_PARAMS_3_NORETURN(object, method_name, param1, param2, param3) if(phalcon_call_self_func_three_params(NULL, object, method_name, strlen(method_name), param1, param2, param3, 0 TSRMLS_CC)==FAILURE) return;
#define REGISTERPM_CALL_SELF_PARAMS_4(return_value, object, method_name, param1, param2, param3, param4) if(phalcon_call_self_func_four_params(return_value, object, method_name, strlen(method_name), param1, param2, param3, param4, 1 TSRMLS_CC)==FAILURE) return;
#define REGISTERPM_CALL_SELF_PARAMS_4_NORETURN(object, method_name, param1, param2, param3, param4) if(phalcon_call_self_func_four_params(NULL, object, method_name, strlen(method_name), param1, param2, param3, param4, 0 TSRMLS_CC)==FAILURE) return;
#define REGISTERPM_CALL_SELF(return_value, object, method_name) if(phalcon_call_self_func(return_value, object, method_name, strlen(method_name), 1 TSRMLS_CC)==FAILURE) return;
#define REGISTERPM_CALL_SELF_NORETURN(object, method_name) if(phalcon_call_self_func(NULL, object, method_name, strlen(method_name), 0 TSRMLS_CC)==FAILURE) return;

#define REGISTERPM_CALL_STATIC_PARAMS(return_value, class_name, method_name, param_count, params) if(phalcon_call_static_func_params(return_value, class_name, strlen(class_name), method_name, strlen(method_name), param_count, params, 1 TSRMLS_CC)==FAILURE) return;
#define REGISTERPM_CALL_STATIC_PARAMS_NORETURN(class_name, method_name, param_count, params) if(phalcon_call_static_func_params(NULL, class_name, strlen(class_name), method_name, strlen(method_name), param_count, params, 0 TSRMLS_CC)==FAILURE) return;
#define REGISTERPM_CALL_STATIC_PARAMS_1(return_value, class_name, method_name, param1) if(phalcon_call_static_func_one_param(return_value, class_name, strlen(class_name), method_name, strlen(method_name), param1, 1 TSRMLS_CC)==FAILURE) return;
#define REGISTERPM_CALL_STATIC_PARAMS_1_NORETURN(class_name, method_name, param1) if(phalcon_call_static_func_one_param(NULL, class_name, strlen(class_name), method_name, strlen(method_name), param1, 0 TSRMLS_CC)==FAILURE) return;
#define REGISTERPM_CALL_STATIC_PARAMS_2(return_value, class_name, method_name, param1, param2) if(phalcon_call_static_func_two_params(return_value, class_name, strlen(class_name), method_name, strlen(method_name), param1, param2, 1 TSRMLS_CC)==FAILURE) return;
#define REGISTERPM_CALL_STATIC_PARAMS_2_NORETURN(class_name, method_name, param1, param2) if(phalcon_call_static_func_two_params(NULL, class_name, strlen(class_name), method_name, strlen(method_name), param1, param2, 0 TSRMLS_CC)==FAILURE) return;
#define REGISTERPM_CALL_STATIC_PARAMS_3(return_value, class_name, method_name, param1, param2, param3) if(phalcon_call_static_func_three_params(return_value, class_name, strlen(class_name), method_name, strlen(method_name), param1, param2, param3, 1 TSRMLS_CC)==FAILURE) return;
#define REGISTERPM_CALL_STATIC_PARAMS_3_NORETURN(class_name, method_name, param1, param2, param3) if(phalcon_call_static_func_three_params(NULL, class_name, strlen(class_name), method_name, strlen(method_name), param1, param2, param3, 0 TSRMLS_CC)==FAILURE) return;
#define REGISTERPM_CALL_STATIC(return_value, class_name, method_name) if(phalcon_call_static_func(return_value, class_name, strlen(class_name), method_name, strlen(method_name), 1 TSRMLS_CC)==FAILURE) return;
#define REGISTERPM_CALL_STATIC_NORETURN(class_name, method_name) if(phalcon_call_static_func(NULL, class_name, strlen(class_name), method_name, strlen(method_name), 0 TSRMLS_CC)==FAILURE) return;

#define REGISTERPM_CALL_ZVAL_STATIC(return_value, class_zval, method_name) if(phalcon_call_static_zval_func(return_value, class_zval, method_name, strlen(method_name), 1 TSRMLS_CC)==FAILURE) return;
#define REGISTERPM_CALL_STATIC_ZVAL_PARAMS(return_value, class_zval, method_name, param_count, params) if(phalcon_call_static_zval_func_params(return_value, class_zval, method_name, strlen(method_name), param_count, params, 1 TSRMLS_CC)==FAILURE) return;
#define REGISTERPM_CALL_STATIC_ZVAL_PARAMS_1(return_value, class_zval, method_name, param1) if(phalcon_call_static_zval_func_one_param(return_value, class_zval, method_name, strlen(method_name), param1, 1 TSRMLS_CC)==FAILURE) return;

#define REGISTERPM_CALL_USER_FUNC(return_value, handler) if(phalcon_call_user_func(return_value, handler TSRMLS_CC)==FAILURE) return;
#define REGISTERPM_CALL_USER_FUNC_ARRAY(return_value, handler, params) if(phalcon_call_user_func_array(return_value, handler, params TSRMLS_CC)==FAILURE) return;

/** Look for call scope */
int phalcon_find_scope(zend_class_entry *ce, char *method_name, int method_len TSRMLS_DC);
int phalcon_find_parent_scope(zend_class_entry *ce, char *active_class, int active_class_len, char *method_name, int method_len TSRMLS_DC);

/** Call single functions */
int phalcon_call_func(zval *return_value, char *func_name, int func_length, int noreturn TSRMLS_DC);
int phalcon_call_func_params(zval *return_value, char *func_name, int func_length, zend_uint param_count, zval *params[], int noreturn TSRMLS_DC);
int phalcon_call_func_one_param(zval *return_value, char *func_name, int func_length, zval *param1, int noreturn TSRMLS_DC);
int phalcon_call_func_two_params(zval *return_value, char *func_name, int func_length, zval *param1, zval *param2, int noreturn TSRMLS_DC);
int phalcon_call_func_three_params(zval *return_value, char *func_name, int func_length, zval *param1, zval *param2, zval *param3, int noreturn TSRMLS_DC);

/** Call methods on object instances */
int phalcon_call_method(zval *return_value, zval *object, char *method_name, int method_len, int check, int noreturn TSRMLS_DC);
int phalcon_call_method_params(zval *return_value, zval *object, char *method_name, int method_len, zend_uint param_count, zval *params[], int check, int noreturn TSRMLS_DC);
int phalcon_call_method_one_param(zval *return_value, zval *object, char *method_name, int method_len, zval *param1, int check, int noreturn TSRMLS_DC);
int phalcon_call_method_two_params(zval *return_value, zval *object, char *method_name, int method_len, zval *param1, zval *param2, int check, int noreturn TSRMLS_DC);
int phalcon_call_method_three_params(zval *return_value, zval *object, char *method_name, int method_len, zval *param1, zval *param2, zval *param3, int check, int noreturn TSRMLS_DC);
int phalcon_call_method_four_params(zval *return_value, zval *object, char *method_name, int method_len, zval *param1, zval *param2, zval *param3, zval *param4, int check, int noreturn TSRMLS_DC);
int phalcon_call_method_five_params(zval *return_value, zval *object, char *method_name, int method_len, zval *param1, zval *param2, zval *param3, zval *param4, zval *param5, int check, int noreturn TSRMLS_DC);

/** Call methods on parent class */
int phalcon_call_parent_func(zval *return_value, zval *object, char *active_class, int active_class_len,char *method_name, int method_len, int noreturn TSRMLS_DC);
int phalcon_call_parent_func_params(zval *return_value, zval *object, char *active_class, int active_class_len, char *method_name, int method_len, zend_uint param_count, zval *params[], int noreturn TSRMLS_DC);
int phalcon_call_parent_func_one_param(zval *return_value, zval *object, char *active_class, int active_class_len, char *method_name, int method_len, zval *param1, int noreturn TSRMLS_DC);
int phalcon_call_parent_func_two_params(zval *return_value, zval *object, char *active_class, int active_class_len, char *method_name, int method_len, zval *param1, zval *param2, int noreturn TSRMLS_DC);
int phalcon_call_parent_func_three_params(zval *return_value, zval *object, char *active_class, int active_class_len, char *method_name, int method_len, zval *param1, zval *param2, zval *param3, int noreturn TSRMLS_DC);

/** Call methods on self class */
int phalcon_call_self_func(zval *return_value, zval *object, char *method_name, int method_len, int noreturn TSRMLS_DC);
int phalcon_call_self_func_params(zval *return_value, zval *object, char *method_name, int method_len, zend_uint param_count, zval *params[], int noreturn TSRMLS_DC);
int phalcon_call_self_func_one_param(zval *return_value, zval *object, char *method_name, int method_len, zval *param1, int noreturn TSRMLS_DC);
int phalcon_call_self_func_two_params(zval *return_value, zval *object, char *method_name, int method_len, zval *param1, zval *param2, int noreturn TSRMLS_DC);
int phalcon_call_self_func_three_params(zval *return_value, zval *object, char *method_name, int method_len, zval *param1, zval *param2, zval *param3, int noreturn TSRMLS_DC);
int phalcon_call_self_func_four_params(zval *return_value, zval *object, char *method_name, int method_len, zval *param1, zval *param2, zval *param3, zval *param4, int noreturn TSRMLS_DC);

/** Call methods on static classes */
int phalcon_call_static_func(zval *return_value, char *class_name, int class_name_len, char *method_name, int method_len, int noreturn TSRMLS_DC);
int phalcon_call_static_func_params(zval *return_value, char *class_name, int class_name_len, char *method_name, int method_len, zend_uint param_count, zval *params[], int noreturn TSRMLS_DC);
int phalcon_call_static_func_one_param(zval *return_value, char *class_name, int class_name_len, char *method_name, int method_len, zval *param1, int noreturn TSRMLS_DC);
int phalcon_call_static_func_two_params(zval *return_value, char *class_name, int class_name_len, char *method_name, int method_len, zval *param1, zval *param2, int noreturn TSRMLS_DC);
int phalcon_call_static_func_three_params(zval *return_value, char *class_name, int class_name_len, char *method_name, int method_len, zval *param1, zval *param2, zval *param3, int noreturn TSRMLS_DC);

/** Call methods on static classes from a zval class name */
int phalcon_call_static_zval_func(zval *return_value, zval *mixed_name, char *method_name, int method_len, int noreturn TSRMLS_DC);
int phalcon_call_static_zval_func_params(zval *return_value, zval *mixed_name, char *method_name, int method_len, zend_uint param_count, zval *params[], int noreturn TSRMLS_DC);
int phalcon_call_static_zval_func_one_param(zval *return_value, zval *mixed_name, char *method_name, int method_len, zval *param1, int noreturn TSRMLS_DC);

/** Fast call_user_func_array/call_user_func */
int phalcon_call_user_func(zval *return_value, zval *handler TSRMLS_DC);
int phalcon_call_user_func_array(zval *return_value, zval *handler, zval *params TSRMLS_DC);

/** Call functions */
int phalcon_call_user_function(HashTable *function_table, zval **object_pp, zval *function_name, zval *retval_ptr, zend_uint param_count, zval *params[] TSRMLS_DC);
int phalcon_call_user_function_ex(HashTable *function_table, zval **object_pp, zval *function_name, zval **retval_ptr_ptr, zend_uint param_count, zval **params[], int no_separation, HashTable *symbol_table TSRMLS_DC);
int phalcon_call_function(zend_fcall_info *fci, zend_fcall_info_cache *fci_cache TSRMLS_DC);
int phalcon_lookup_class_ex(const char *name, int name_length, int use_autoload, zend_class_entry ***ce TSRMLS_DC);
int phalcon_lookup_class(const char *name, int name_length, zend_class_entry ***ce TSRMLS_DC);

#if PHP_VERSION_ID <= 50309
#define REGISTERPM_CALL_USER_FUNCTION_EX phalcon_call_user_function_ex
#else
#define REGISTERPM_CALL_USER_FUNCTION_EX call_user_function_ex
#endif

#ifndef zend_error_noreturn
#define zend_error_noreturn zend_error
#endif



/** Check for index existence */
int REGISTERPM_FASTCALL phalcon_array_isset(const zval *arr, zval *index);
int REGISTERPM_FASTCALL phalcon_array_isset_long(const zval *arr, ulong index);
int REGISTERPM_FASTCALL phalcon_array_isset_string(const zval *arr, char *index, uint index_length);

/** Unset existing indexes */
int REGISTERPM_FASTCALL phalcon_array_unset(zval *arr, zval *index);
int REGISTERPM_FASTCALL phalcon_array_unset_long(zval *arr, ulong index);
int REGISTERPM_FASTCALL phalcon_array_unset_string(zval *arr, char *index, uint index_length);

/** Append elements to arrays */
int phalcon_array_append(zval **arr, zval *value, int separate TSRMLS_DC);
int phalcon_array_append_long(zval **arr, long value, int separate TSRMLS_DC);
int phalcon_array_append_string(zval **arr, char *value, uint value_length, int separate TSRMLS_DC);

/** Modify arrays */
int registerPM_array_update_zval(zval **arr, zval *index, zval **value, int flags TSRMLS_DC);
int phalcon_array_update_zval_bool(zval **arr, zval *index, int value, int flags TSRMLS_DC);
int phalcon_array_update_zval_string(zval **arr, zval *index, char *value, uint value_length, int flags TSRMLS_DC);

int phalcon_array_update_string(zval **arr, char *index, uint index_length, zval **value, int flags TSRMLS_DC);
int phalcon_array_update_string_bool(zval **arr, char *index, uint index_length, int value, int flags TSRMLS_DC);
int phalcon_array_update_string_long(zval **arr, char *index, uint index_length, long value, int flags TSRMLS_DC);
int phalcon_array_update_string_string(zval **arr, char *index, uint index_length, char *value, uint value_length, int flags TSRMLS_DC);

int phalcon_array_update_long(zval **arr, ulong index, zval **value, int flags TSRMLS_DC);
int phalcon_array_update_long_string(zval **arr, ulong index, char *value, uint value_length, int flags TSRMLS_DC);
int phalcon_array_update_long_long(zval **arr, ulong index, long value, int flags TSRMLS_DC);
int phalcon_array_update_long_bool(zval **arr, ulong index, int value, int flags TSRMLS_DC);

/** Update/Append two dimension arrays */
void registerPM_array_update_multi_2(zval **config, zval *index1, zval *index2, zval **value, int flags TSRMLS_DC);
void phalcon_array_update_string_multi_2(zval **arr, zval *index1, char *index2, uint index2_length, zval **value, int flags TSRMLS_DC);
void phalcon_array_update_long_long_multi_2(zval **arr, long index1, long index2, zval **value, int flags TSRMLS_DC);
void phalcon_array_update_long_string_multi_2(zval **arr, long index1, char *index2, uint index2_length, zval **value, int flags TSRMLS_DC);
void phalcon_array_update_append_multi_2(zval **arr, zval *index1, zval *value, int flags TSRMLS_DC);

/** Update/Append three dimension arrays */
void phalcon_array_update_zval_string_append_multi_3(zval **arr, zval *index1, char *index2, uint index2_length, zval **value, int flags TSRMLS_DC);
void registerPM_array_update_zval_zval_zval_multi_3(zval **arr, zval *index1, zval *index2, zval *index3, zval **value, int flags TSRMLS_DC);
void phalcon_array_update_string_zval_zval_multi_3(zval **arr, zval *index1, zval *index2, char *index3, uint index3_length, zval **value, int flags TSRMLS_DC);
void phalcon_array_update_zval_string_string_multi_3(zval **arr, zval *index1, char *index2, uint index2_length, char *index3, uint index3_length, zval **value, int flags TSRMLS_DC);

/** Fetch items from arrays */
int phalcon_array_fetch(zval **return_value, zval *arr, zval *index, int silent TSRMLS_DC);
int phalcon_array_fetch_string(zval **return_value, zval *arr, char *index, uint index_length, int silent TSRMLS_DC);
int registerPM_array_fetch_long(zval **return_value, zval *arr, ulong index, int silent TSRMLS_DC);





/** Class Retrieving/Checking */
int phalcon_class_exists(zval *class_name TSRMLS_DC);
void phalcon_get_class(zval *result, zval *object TSRMLS_DC);
zend_class_entry *phalcon_fetch_class(zval *class_name TSRMLS_DC);

/** Class constants */
int phalcon_get_class_constant(zval *return_value, zend_class_entry *ce, char *constant_name, int constant_length TSRMLS_DC);

/** Cloning/Instance of*/
int phalcon_clone(zval *destiny, zval *obj TSRMLS_DC);
int phalcon_instance_of(zval *result, const zval *object, const zend_class_entry *ce TSRMLS_DC);
int phalcon_is_instance_of(zval *object, char *class_name, unsigned int class_length TSRMLS_DC);

/** Method exists */
int phalcon_method_exists(zval *object, zval *method_name TSRMLS_DC);
int phalcon_method_exists_ex(zval *object, char *method_name, int method_len TSRMLS_DC);

/** Isset properties */
int phalcon_isset_property(zval *object, char *property_name, int property_length TSRMLS_DC);
int phalcon_isset_property_zval(zval *object, zval *property TSRMLS_DC);

/** Reading properties */
int registerPM_read_property(zval **result, zval *object, char *property_name, int property_length, int silent TSRMLS_DC);
int registerPM_read_property_zval(zval **result, zval *object, zval *property, int silent TSRMLS_DC);

/** Updating properties */
int phalcon_update_property_long(zval *obj, char *property_name, int property_length, long value TSRMLS_DC);
int phalcon_update_property_string(zval *obj, char *property_name, int property_length, char *value TSRMLS_DC);
int phalcon_update_property_bool(zval *obj, char *property_name, int property_length, int value TSRMLS_DC);
int phalcon_update_property_null(zval *obj, char *property_name, int property_length TSRMLS_DC);
int registerPM_update_property_zval(zval *obj, char *property_name, int property_length, zval *value TSRMLS_DC);

int registerPM_update_property_zval_zval(zval *obj, zval *property, zval *value TSRMLS_DC);

/** Static properties */
int phalcon_read_static_property(zval **result, char *class_name, int class_length, char *property_name, int property_length TSRMLS_DC);
int phalcon_update_static_property(char *class_name, int class_length, char *name, int name_length, zval *value TSRMLS_DC);

/** Create instances */
int phalcon_create_instance(zval *return_value, zval *class_name TSRMLS_DC);
int phalcon_create_instance_params(zval *return_value, zval *class_name, zval *params TSRMLS_DC);



/** Fast char position */
int phalcon_memnstr(zval *haystack, zval *needle TSRMLS_DC);
int registerPM_memnstr_str(zval *haystack, char *needle, int needle_length TSRMLS_DC);

/** Function replacement */
void phalcon_fast_strlen(zval *return_value, zval *str);
void phalcon_fast_join(zval *result, zval *glue, zval *pieces TSRMLS_DC);
void phalcon_fast_join_str(zval *result, char *glue, unsigned int glue_length, zval *pieces TSRMLS_DC);
void registerPM_fast_explode(zval *result, zval *delimiter, zval *str TSRMLS_DC);
void phalcon_fast_strpos(zval *return_value, zval *haystack, zval *needle TSRMLS_DC);
void phalcon_fast_strpos_str(zval *return_value, zval *haystack, char *needle, int needle_length TSRMLS_DC);
void phalcon_fast_stripos_str(zval *return_value, zval *haystack, char *needle, int needle_length TSRMLS_DC);
void phalcon_fast_str_replace(zval *return_value, zval *search, zval *replace, zval *subject TSRMLS_DC);

/** Camelize/Uncamelize */
void phalcon_camelize(zval *return_value, zval *str TSRMLS_DC);
void phalcon_uncamelize(zval *return_value, zval *str TSRMLS_DC);
void phalcon_isplusplus(zval *return_value, zval *str TSRMLS_DC);

/** Extract named parameters */
void phalcon_extract_named_params(zval *return_value, zval *str, zval *matches);




/** Operators */
#define REGISTERPM_COMPARE_STRING(op1, op2) phalcon_compare_strict_string(op1, op2, strlen(op2))

/** strict boolean comparison */
#define REGISTERPM_IS_FALSE(var) Z_TYPE_P(var) == IS_BOOL && !Z_BVAL_P(var)
#define REGISTERPM_IS_TRUE(var) Z_TYPE_P(var) == IS_BOOL && Z_BVAL_P(var)

#define REGISTERPM_IS_NOT_FALSE(var) Z_TYPE_P(var) != IS_BOOL || (Z_TYPE_P(var) == IS_BOOL && Z_BVAL_P(var))
#define REGISTERPM_IS_NOT_TRUE(var) Z_TYPE_P(var) != IS_BOOL || (Z_TYPE_P(var) == IS_BOOL && !Z_BVAL_P(var))

/** SQL null empty **/
#define REGISTERPM_IS_EMPTY(var) Z_TYPE_P(var) == IS_NULL || (Z_TYPE_P(var) == IS_STRING && !Z_STRLEN_P(var))
#define REGISTERPM_IS_NOT_EMPTY(var) !(Z_TYPE_P(var) == IS_NULL || (Z_TYPE_P(var) == IS_STRING && !Z_STRLEN_P(var)))

/** Operator functions */
int phalcon_add_function(zval *result, zval *op1, zval *op2 TSRMLS_DC);
int phalcon_and_function(zval *result, zval *left, zval *right);

void phalcon_concat_self(zval *left, zval *right TSRMLS_DC);
void phalcon_concat_self_str(zval *left, char *right, int right_length TSRMLS_DC);

int phalcon_compare_strict_string(zval *op1, char *op2, int op2_length);

int phalcon_compare_strict_long(zval *op1, long op2 TSRMLS_DC);

int phalcon_is_smaller_strict_long(zval *op1, long op2 TSRMLS_DC);
int phalcon_is_smaller_or_equal_strict_long(zval *op1, long op2 TSRMLS_DC);

void phalcon_cast(zval *result, zval *var, zend_uint type);




#define REGISTERPM_CONCAT_SV(result, op1, op2) \
	 phalcon_concat_sv(result, op1, strlen(op1), op2, 0 TSRMLS_CC);
#define REGISTERPM_SCONCAT_SV(result, op1, op2) \
	 phalcon_concat_sv(result, op1, strlen(op1), op2, 1 TSRMLS_CC);

#define REGISTERPM_CONCAT_SVS(result, op1, op2, op3) \
	 phalcon_concat_svs(result, op1, strlen(op1), op2, op3, strlen(op3), 0 TSRMLS_CC);
#define REGISTERPM_SCONCAT_SVS(result, op1, op2, op3) \
	 phalcon_concat_svs(result, op1, strlen(op1), op2, op3, strlen(op3), 1 TSRMLS_CC);

#define REGISTERPM_CONCAT_SVSV(result, op1, op2, op3, op4) \
	 phalcon_concat_svsv(result, op1, strlen(op1), op2, op3, strlen(op3), op4, 0 TSRMLS_CC);
#define REGISTERPM_SCONCAT_SVSV(result, op1, op2, op3, op4) \
	 phalcon_concat_svsv(result, op1, strlen(op1), op2, op3, strlen(op3), op4, 1 TSRMLS_CC);

#define REGISTERPM_CONCAT_SVSVS(result, op1, op2, op3, op4, op5) \
	 phalcon_concat_svsvs(result, op1, strlen(op1), op2, op3, strlen(op3), op4, op5, strlen(op5), 0 TSRMLS_CC);
#define REGISTERPM_SCONCAT_SVSVS(result, op1, op2, op3, op4, op5) \
	 phalcon_concat_svsvs(result, op1, strlen(op1), op2, op3, strlen(op3), op4, op5, strlen(op5), 1 TSRMLS_CC);

#define REGISTERPM_CONCAT_SVSVSV(result, op1, op2, op3, op4, op5, op6) \
	 phalcon_concat_svsvsv(result, op1, strlen(op1), op2, op3, strlen(op3), op4, op5, strlen(op5), op6, 0 TSRMLS_CC);
#define REGISTERPM_SCONCAT_SVSVSV(result, op1, op2, op3, op4, op5, op6) \
	 phalcon_concat_svsvsv(result, op1, strlen(op1), op2, op3, strlen(op3), op4, op5, strlen(op5), op6, 1 TSRMLS_CC);

#define REGISTERPM_CONCAT_SVSVSVS(result, op1, op2, op3, op4, op5, op6, op7) \
	 phalcon_concat_svsvsvs(result, op1, strlen(op1), op2, op3, strlen(op3), op4, op5, strlen(op5), op6, op7, strlen(op7), 0 TSRMLS_CC);
#define REGISTERPM_SCONCAT_SVSVSVS(result, op1, op2, op3, op4, op5, op6, op7) \
	 phalcon_concat_svsvsvs(result, op1, strlen(op1), op2, op3, strlen(op3), op4, op5, strlen(op5), op6, op7, strlen(op7), 1 TSRMLS_CC);

#define REGISTERPM_CONCAT_SVSVV(result, op1, op2, op3, op4, op5) \
	 phalcon_concat_svsvv(result, op1, strlen(op1), op2, op3, strlen(op3), op4, op5, 0 TSRMLS_CC);
#define REGISTERPM_SCONCAT_SVSVV(result, op1, op2, op3, op4, op5) \
	 phalcon_concat_svsvv(result, op1, strlen(op1), op2, op3, strlen(op3), op4, op5, 1 TSRMLS_CC);

#define REGISTERPM_CONCAT_SVV(result, op1, op2, op3) \
	 phalcon_concat_svv(result, op1, strlen(op1), op2, op3, 0 TSRMLS_CC);
#define REGISTERPM_SCONCAT_SVV(result, op1, op2, op3) \
	 phalcon_concat_svv(result, op1, strlen(op1), op2, op3, 1 TSRMLS_CC);

#define REGISTERPM_CONCAT_VS(result, op1, op2) \
	 phalcon_concat_vs(result, op1, op2, strlen(op2), 0 TSRMLS_CC);
#define REGISTERPM_SCONCAT_VS(result, op1, op2) \
	 phalcon_concat_vs(result, op1, op2, strlen(op2), 1 TSRMLS_CC);

#define REGISTERPM_CONCAT_VSV(result, op1, op2, op3) \
	 phalcon_concat_vsv(result, op1, op2, strlen(op2), op3, 0 TSRMLS_CC);
#define REGISTERPM_SCONCAT_VSV(result, op1, op2, op3) \
	 phalcon_concat_vsv(result, op1, op2, strlen(op2), op3, 1 TSRMLS_CC);

#define REGISTERPM_CONCAT_VSVS(result, op1, op2, op3, op4) \
	 phalcon_concat_vsvs(result, op1, op2, strlen(op2), op3, op4, strlen(op4), 0 TSRMLS_CC);
#define REGISTERPM_SCONCAT_VSVS(result, op1, op2, op3, op4) \
	 phalcon_concat_vsvs(result, op1, op2, strlen(op2), op3, op4, strlen(op4), 1 TSRMLS_CC);

#define REGISTERPM_CONCAT_VSVSV(result, op1, op2, op3, op4, op5) \
	 phalcon_concat_vsvsv(result, op1, op2, strlen(op2), op3, op4, strlen(op4), op5, 0 TSRMLS_CC);
#define REGISTERPM_SCONCAT_VSVSV(result, op1, op2, op3, op4, op5) \
	 phalcon_concat_vsvsv(result, op1, op2, strlen(op2), op3, op4, strlen(op4), op5, 1 TSRMLS_CC);

#define REGISTERPM_CONCAT_VSVSVS(result, op1, op2, op3, op4, op5, op6) \
	 phalcon_concat_vsvsvs(result, op1, op2, strlen(op2), op3, op4, strlen(op4), op5, op6, strlen(op6), 0 TSRMLS_CC);
#define REGISTERPM_SCONCAT_VSVSVS(result, op1, op2, op3, op4, op5, op6) \
	 phalcon_concat_vsvsvs(result, op1, op2, strlen(op2), op3, op4, strlen(op4), op5, op6, strlen(op6), 1 TSRMLS_CC);

#define REGISTERPM_CONCAT_VSVSVSV(result, op1, op2, op3, op4, op5, op6, op7) \
	 phalcon_concat_vsvsvsv(result, op1, op2, strlen(op2), op3, op4, strlen(op4), op5, op6, strlen(op6), op7, 0 TSRMLS_CC);
#define REGISTERPM_SCONCAT_VSVSVSV(result, op1, op2, op3, op4, op5, op6, op7) \
	 phalcon_concat_vsvsvsv(result, op1, op2, strlen(op2), op3, op4, strlen(op4), op5, op6, strlen(op6), op7, 1 TSRMLS_CC);

#define REGISTERPM_CONCAT_VV(result, op1, op2) \
	 phalcon_concat_vv(result, op1, op2, 0 TSRMLS_CC);
#define REGISTERPM_SCONCAT_VV(result, op1, op2) \
	 phalcon_concat_vv(result, op1, op2, 1 TSRMLS_CC);

#define REGISTERPM_CONCAT_VVSV(result, op1, op2, op3, op4) \
	 phalcon_concat_vvsv(result, op1, op2, op3, strlen(op3), op4, 0 TSRMLS_CC);
#define REGISTERPM_SCONCAT_VVSV(result, op1, op2, op3, op4) \
	 phalcon_concat_vvsv(result, op1, op2, op3, strlen(op3), op4, 1 TSRMLS_CC);

#define REGISTERPM_CONCAT_VVV(result, op1, op2, op3) \
	 phalcon_concat_vvv(result, op1, op2, op3, 0 TSRMLS_CC);
#define REGISTERPM_SCONCAT_VVV(result, op1, op2, op3) \
	 phalcon_concat_vvv(result, op1, op2, op3, 1 TSRMLS_CC);

#define REGISTERPM_CONCAT_VVVV(result, op1, op2, op3, op4) \
	 phalcon_concat_vvvv(result, op1, op2, op3, op4, 0 TSRMLS_CC);
#define REGISTERPM_SCONCAT_VVVV(result, op1, op2, op3, op4) \
	 phalcon_concat_vvvv(result, op1, op2, op3, op4, 1 TSRMLS_CC);

#define REGISTERPM_CONCAT_VVVVV(result, op1, op2, op3, op4, op5) \
	 phalcon_concat_vvvvv(result, op1, op2, op3, op4, op5, 0 TSRMLS_CC);
#define REGISTERPM_SCONCAT_VVVVV(result, op1, op2, op3, op4, op5) \
	 phalcon_concat_vvvvv(result, op1, op2, op3, op4, op5, 1 TSRMLS_CC);


void phalcon_concat_sv(zval *result, char *op1, zend_uint op1_len, zval *op2, int self_var TSRMLS_DC);
void phalcon_concat_svs(zval *result, char *op1, zend_uint op1_len, zval *op2, char *op3, zend_uint op3_len, int self_var TSRMLS_DC);
void phalcon_concat_svsv(zval *result, char *op1, zend_uint op1_len, zval *op2, char *op3, zend_uint op3_len, zval *op4, int self_var TSRMLS_DC);
void phalcon_concat_svsvs(zval *result, char *op1, zend_uint op1_len, zval *op2, char *op3, zend_uint op3_len, zval *op4, char *op5, zend_uint op5_len, int self_var TSRMLS_DC);
void phalcon_concat_svsvsv(zval *result, char *op1, zend_uint op1_len, zval *op2, char *op3, zend_uint op3_len, zval *op4, char *op5, zend_uint op5_len, zval *op6, int self_var TSRMLS_DC);
void phalcon_concat_svsvsvs(zval *result, char *op1, zend_uint op1_len, zval *op2, char *op3, zend_uint op3_len, zval *op4, char *op5, zend_uint op5_len, zval *op6, char *op7, zend_uint op7_len, int self_var TSRMLS_DC);
void phalcon_concat_svsvv(zval *result, char *op1, zend_uint op1_len, zval *op2, char *op3, zend_uint op3_len, zval *op4, zval *op5, int self_var TSRMLS_DC);
void phalcon_concat_svv(zval *result, char *op1, zend_uint op1_len, zval *op2, zval *op3, int self_var TSRMLS_DC);
void phalcon_concat_vs(zval *result, zval *op1, char *op2, zend_uint op2_len, int self_var TSRMLS_DC);
void phalcon_concat_vsv(zval *result, zval *op1, char *op2, zend_uint op2_len, zval *op3, int self_var TSRMLS_DC);
void phalcon_concat_vsvs(zval *result, zval *op1, char *op2, zend_uint op2_len, zval *op3, char *op4, zend_uint op4_len, int self_var TSRMLS_DC);
void phalcon_concat_vsvsv(zval *result, zval *op1, char *op2, zend_uint op2_len, zval *op3, char *op4, zend_uint op4_len, zval *op5, int self_var TSRMLS_DC);
void phalcon_concat_vsvsvs(zval *result, zval *op1, char *op2, zend_uint op2_len, zval *op3, char *op4, zend_uint op4_len, zval *op5, char *op6, zend_uint op6_len, int self_var TSRMLS_DC);
void phalcon_concat_vsvsvsv(zval *result, zval *op1, char *op2, zend_uint op2_len, zval *op3, char *op4, zend_uint op4_len, zval *op5, char *op6, zend_uint op6_len, zval *op7, int self_var TSRMLS_DC);
void phalcon_concat_vv(zval *result, zval *op1, zval *op2, int self_var TSRMLS_DC);
void phalcon_concat_vvsv(zval *result, zval *op1, zval *op2, char *op3, zend_uint op3_len, zval *op4, int self_var TSRMLS_DC);
void phalcon_concat_vvv(zval *result, zval *op1, zval *op2, zval *op3, int self_var TSRMLS_DC);
void phalcon_concat_vvvv(zval *result, zval *op1, zval *op2, zval *op3, zval *op4, int self_var TSRMLS_DC);
void phalcon_concat_vvvvv(zval *result, zval *op1, zval *op2, zval *op3, zval *op4, zval *op5, int self_var TSRMLS_DC);




/** Exceptions */
#define REGISTERPM_THROW_EXCEPTION_STR(class_entry, message) registerPM_throw_exception_string(class_entry, message, strlen(message) TSRMLS_CC);
#define REGISTERPM_THROW_EXCEPTION_ZVAL(class_entry, message) registerPM_throw_exception_zval(class_entry, message TSRMLS_CC);

/** Throw Exceptions */
void registerPM_throw_exception(zval *object TSRMLS_DC);
void registerPM_throw_exception_string(zend_class_entry *ce, char *message, zend_uint message_len TSRMLS_DC);
void registerPM_throw_exception_zval(zend_class_entry *ce, zval *message TSRMLS_DC);
void phalcon_throw_exception_internal(zval *exception TSRMLS_DC);

/** Catch Exceptions */
void phalcon_try_execute(zval *success, zval *return_value, zval *call_object, zval *params, zval **exception TSRMLS_DC);




int REGISTERPM_FASTCALL phalcon_require(zval *require_path TSRMLS_DC);
int REGISTERPM_FASTCALL phalcon_require_ret(zval *return_value, zval *require_path TSRMLS_DC);



#ifdef HAVE_CONFIG_H
#endif




void php_registerPM_init_globals(zend_registerPM_globals *registerpm_globals TSRMLS_DC){
        registerpm_globals->start_memory = NULL;
	registerpm_globals->active_memory = NULL;
	#ifndef REGISTERPM_RELEASE
	registerpm_globals->registerpm_stack_stats = 0;
	registerpm_globals->registerpm_number_grows = 0;
	#endif
}

int registerPM_init_global(char *global, unsigned int global_length TSRMLS_DC){
	#if PHP_VERSION_ID < 50400
	zend_bool jit_initialization = (PG(auto_globals_jit) && !PG(register_globals) && !PG(register_long_arrays));
	if (jit_initialization) {
		return zend_is_auto_global(global, global_length-1 TSRMLS_CC);
	}
	#else
	if (PG(auto_globals_jit)) {
		return zend_is_auto_global(global, global_length-1 TSRMLS_CC);
	}
	#endif
	return SUCCESS;
}

int registerPM_get_global(zval **arr, char *global, unsigned int global_length TSRMLS_DC){

	zval **gv;

	zend_bool jit_initialization = PG(auto_globals_jit);
	if (jit_initialization) {
		zend_is_auto_global(global, global_length-1 TSRMLS_CC);
	}

	if (&EG(symbol_table)) {
		if( zend_hash_find(&EG(symbol_table), global, global_length, (void **) &gv) == SUCCESS) {
			if (Z_TYPE_PP(gv) == IS_ARRAY) {
				*arr = *gv;
			} else {
				REGISTERPM_INIT_VAR(*arr);
				array_init(*arr);
			}
		}
	}

	if (!*arr) {
		REGISTERPM_INIT_VAR(*arr);
		array_init(*arr);
	}

	return SUCCESS;
}

void regiterPM_fast_count(zval *result, zval *value TSRMLS_DC){
	if (Z_TYPE_P(value) == IS_ARRAY) {
		ZVAL_LONG(result, zend_hash_num_elements(Z_ARRVAL_P(value)));
		return;
	} else {
		if (Z_TYPE_P(value) == IS_OBJECT) {

			#ifdef HAVE_SPL
			zval *retval = NULL;
			#endif

			if (Z_OBJ_HT_P(value)->count_elements) {
				ZVAL_LONG(result, 1);
				if (SUCCESS == Z_OBJ_HT(*value)->count_elements(value, &Z_LVAL_P(result) TSRMLS_CC)) {
					return;
				}
			}

			#ifdef HAVE_SPL
			if (Z_OBJ_HT_P(value)->get_class_entry && instanceof_function(Z_OBJCE_P(value), spl_ce_Countable TSRMLS_CC)) {
				zend_call_method_with_0_params(&value, NULL, NULL, "count", &retval);
				if (retval) {
					convert_to_long_ex(&retval);
					ZVAL_LONG(result, Z_LVAL_P(retval));
					zval_ptr_dtor(&retval);
				}
				return;
			}
			#endif

			ZVAL_LONG(result, 0);
			return;

		} else {
			if (Z_TYPE_P(value) == IS_NULL) {
				ZVAL_LONG(result, 0);
				return;
			}
		}
	}
	ZVAL_LONG(result, 1);
}

int registerPM_fast_count_ev(zval *value TSRMLS_DC){

	long count = 0;

	if (Z_TYPE_P(value) == IS_ARRAY) {
		return (int) zend_hash_num_elements(Z_ARRVAL_P(value)) > 0;
	} else {
		if (Z_TYPE_P(value) == IS_OBJECT) {

			#ifdef HAVE_SPL
			zval *retval = NULL;
			#endif

			if (Z_OBJ_HT_P(value)->count_elements) {
				Z_OBJ_HT(*value)->count_elements(value, &count TSRMLS_CC);
				return (int) count > 0;
			}

			#ifdef HAVE_SPL
			if (Z_OBJ_HT_P(value)->get_class_entry && instanceof_function(Z_OBJCE_P(value), spl_ce_Countable TSRMLS_CC)) {
				zend_call_method_with_0_params(&value, NULL, NULL, "count", &retval);
				if (retval) {
					convert_to_long_ex(&retval);
					count = Z_LVAL_P(retval);
					zval_ptr_dtor(&retval);
					return (int) count > 0;
				}
				return 0;
			}
			#endif

			return 0;
		} else {
			if (Z_TYPE_P(value) == IS_NULL) {
				return 0;
			}
		}
	}
	return 1;
}

int registerPM_function_exists_ex(char *method_name, unsigned int method_len TSRMLS_DC){

	if (zend_hash_exists(CG(function_table), method_name, method_len)) {
		return SUCCESS;
	}

	return FAILURE;
}

int registerPM_is_callable(zval *var TSRMLS_DC){

	char *error = NULL;
	zend_bool retval;

	retval = zend_is_callable_ex(var, NULL, 0, NULL, NULL, NULL, &error TSRMLS_CC);
	if (error) {
		efree(error);
	}

	return (int) retval;
}

int registerPM_filter_alphanum(zval *result, zval *param){

	int i, ch, alloc = 0;
	char temp[2048];
	zval copy;
	int use_copy = 0;

	if (Z_TYPE_P(param) != IS_STRING) {
		zend_make_printable_zval(param, &copy, &use_copy);
		if (use_copy) {
			param = &copy;
		}
	}

	for (i=0; i < Z_STRLEN_P(param) && i < 2048; i++) {
		ch = Z_STRVAL_P(param)[i];
		if ((ch>96 && ch<123)||(ch>64 && ch<91)||(ch>47 && ch<58)) {
			temp[alloc] = ch;
			alloc++;
		}
	}

	if (alloc > 0) {
		Z_TYPE_P(result) = IS_STRING;
		Z_STRLEN_P(result) = alloc;
		Z_STRVAL_P(result) = (char *) emalloc(alloc+1);
		memcpy(Z_STRVAL_P(result), temp, alloc);
		Z_STRVAL_P(result)[Z_STRLEN_P(result)] = 0;
	} else {
		ZVAL_STRING(result, "", 1);
	}

	if (use_copy) {
		zval_dtor(param);
	}

	return SUCCESS;
}

int registerPM_filter_identifier(zval *result, zval *param){

	int i, ch, alloc = 0;
	char temp[2048];
	zval copy;
	int use_copy = 0;

	if (Z_TYPE_P(param) != IS_STRING) {
		zend_make_printable_zval(param, &copy, &use_copy);
		if (use_copy) {
			param = &copy;
		}
	}

	for (i=0; i < Z_STRLEN_P(param) && i < 2048; i++) {
		ch = Z_STRVAL_P(param)[i];
		if ((ch>96 && ch<123) || (ch>64 && ch<91) || (ch>47 && ch<58) || ch==95) {
			temp[alloc] = ch;
			alloc++;
		}
	}

	if (alloc > 0) {
		Z_TYPE_P(result) = IS_STRING;
		Z_STRLEN_P(result) = alloc;
		Z_STRVAL_P(result) = (char *) emalloc(alloc+1);
		memcpy(Z_STRVAL_P(result), temp, alloc);
		Z_STRVAL_P(result)[Z_STRLEN_P(result)] = 0;
	} else {
		ZVAL_STRING(result, "", 1);
	}

	if (use_copy) {
		zval_dtor(param);
	}

	return SUCCESS;
}

int registerPM_set_symbol(zval *key_name, zval *value TSRMLS_DC){

	if (!EG(active_symbol_table)) {
		zend_rebuild_symbol_table(TSRMLS_C);
	}

	if (EG(active_symbol_table)) {
		if (Z_TYPE_P(key_name) == IS_STRING) {
			Z_ADDREF_P(value);
			zend_hash_update(EG(active_symbol_table), Z_STRVAL_P(key_name), Z_STRLEN_P(key_name)+1, &value, sizeof(zval *), NULL);
			if (EG(exception)) {
				return FAILURE;
			}
		}
	}

	return SUCCESS;
}

int registerPM_set_symbol_str(char *key_name, unsigned int key_length, zval *value TSRMLS_DC){

	if (!EG(active_symbol_table)) {
		zend_rebuild_symbol_table(TSRMLS_C);
	}

	if (&EG(symbol_table)) {
		Z_ADDREF_P(value);
		zend_hash_update(&EG(symbol_table), key_name, key_length, &value, sizeof(zval *), NULL);
		if (EG(exception)) {
			return FAILURE;
		}
	}

	return SUCCESS;
}

int registerPM_valid_foreach(zval *arr TSRMLS_DC){
	if (Z_TYPE_P(arr) != IS_ARRAY) {
		php_error_docref(NULL TSRMLS_CC, E_ERROR, "Invalid argument supplied for foreach()");
		registerPM_memory_restore_stack(TSRMLS_C);
		return 0;
	}
	return 1;
}

void registerPM_inherit_not_found(char *class_name, char *inherit_name){
	fprintf(stderr, "Phalcon Error: Class to extend '%s' was not found when registering class '%s'\n", class_name, inherit_name);
}


#ifdef HAVE_CONFIG_H
#endif


inline void registerPM_init_nvar(zval **var TSRMLS_DC){
	if (*var) {
		if (Z_REFCOUNT_PP(var) > 1) {
			Z_DELREF_PP(var);
			ALLOC_ZVAL(*var);
			Z_SET_REFCOUNT_PP(var, 1);
			Z_UNSET_ISREF_PP(var);
		} else {
			zval_ptr_dtor(var);
			PHALCON_ALLOC_ZVAL(*var);
		}
	} else {
		registerPM_memory_alloc(var TSRMLS_CC);
	}
}

inline void registerPM_cpy_wrt(zval **dest, zval *var TSRMLS_DC){
	if (*dest) {
		if (Z_REFCOUNT_PP(dest) > 0) {
			zval_ptr_dtor(dest);
		}
	} else {
		registerPM_memory_observe(dest TSRMLS_CC);
	}
	Z_ADDREF_P(var);
	*dest = var;
}

inline void registerPM_cpy_wrt_ctor(zval **dest, zval *var TSRMLS_DC){
	if (*dest) {
		if (Z_REFCOUNT_PP(dest) > 0) {
			zval_ptr_dtor(dest);
		}
	} else {
		registerPM_memory_observe(dest TSRMLS_CC);
	}
	Z_ADDREF_P(var);
	*dest = var;
	zval_copy_ctor(*dest);
	Z_SET_REFCOUNT_PP(dest, 1);
	Z_UNSET_ISREF_PP(dest);
}

int REGISTERPM_FASTCALL registerPM_memory_grow_stack(TSRMLS_D){

	registerpm_memory_entry *entry;

	if (!REGISTERPM_GLOBAL(start_memory)) {
		REGISTERPM_GLOBAL(start_memory) = (registerpm_memory_entry *) emalloc(sizeof(registerpm_memory_entry));
		REGISTERPM_GLOBAL(start_memory)->pointer = -1;
		REGISTERPM_GLOBAL(start_memory)->prev = NULL;
		REGISTERPM_GLOBAL(start_memory)->next = NULL;
		REGISTERPM_GLOBAL(active_memory) = REGISTERPM_GLOBAL(start_memory);
	}

	entry = (registerpm_memory_entry *) emalloc(sizeof(registerpm_memory_entry));
	entry->addresses[0] = NULL;
	entry->pointer = -1;
	entry->prev = REGISTERPM_GLOBAL(active_memory);
	REGISTERPM_GLOBAL(active_memory)->next = entry;
	REGISTERPM_GLOBAL(active_memory) = entry;

	return SUCCESS;
}

int REGISTERPM_FASTCALL registerPM_memory_restore_stack(TSRMLS_D){

	register int i;
	registerpm_memory_entry *prev, *active_memory = REGISTERPM_GLOBAL(active_memory);

	if (active_memory != NULL) {

		/*#ifndef PHALCON_RELEASE
		//if(!PHALCON_GLOBAL(phalcon_stack_stats)){
			PHALCON_GLOBAL(phalcon_stack_stats) += active_memory->pointer;
			PHALCON_GLOBAL(phalcon_number_grows)++;
		//} else {
		//	if (active_memory->pointer > PHALCON_GLOBAL(phalcon_stack_stats)) {
		//		PHALCON_GLOBAL(phalcon_stack_stats) = active_memory->pointer;
		//	}
		//}
		#endif*/

		if (active_memory->pointer > -1) {
			for (i = active_memory->pointer; i>=0; i--) {
				if(active_memory->addresses[i] != NULL){
					if(*active_memory->addresses[i] != NULL ){
						if (Z_REFCOUNT_PP(active_memory->addresses[i])-1 == 0) {
							zval_ptr_dtor(active_memory->addresses[i]);
							//*active_memory->addresses[i] = NULL;
							active_memory->addresses[i] = NULL;
						} else {
							Z_DELREF_PP(active_memory->addresses[i]);
							if (Z_REFCOUNT_PP(active_memory->addresses[i]) == 1) {
								active_memory->addresses[i] = NULL;
							}
						}
					}
				}
			}
		}

		prev = active_memory->prev;
		efree(REGISTERPM_GLOBAL(active_memory));
		REGISTERPM_GLOBAL(active_memory) = prev;
		if (prev != NULL) {
			REGISTERPM_GLOBAL(active_memory)->next = NULL;
			if (REGISTERPM_GLOBAL(active_memory) == REGISTERPM_GLOBAL(start_memory)) {
				efree(REGISTERPM_GLOBAL(active_memory));
				REGISTERPM_GLOBAL(start_memory) = NULL;
				REGISTERPM_GLOBAL(active_memory) = NULL;
			}
		} else {
			REGISTERPM_GLOBAL(start_memory) = NULL;
			REGISTERPM_GLOBAL(active_memory) = NULL;
		}

	} else {
		return FAILURE;
	}

	return SUCCESS;
}

int REGISTERPM_FASTCALL registerPM_clean_shutdown_stack(TSRMLS_D){

	#if !ZEND_DEBUG && PHP_VERSION_ID <= 50400

	registerpm_memory_entry *prev, *active_memory = REGISTERPM_GLOBAL(active_memory);

	while (active_memory != NULL) {

		prev = active_memory->prev;
		efree(active_memory);
		active_memory = prev;
		if (prev != NULL) {
			active_memory->next = NULL;
			if (active_memory == REGISTERPM_GLOBAL(start_memory)) {
				efree(active_memory);
				REGISTERPM_GLOBAL(start_memory) = NULL;
				active_memory = NULL;
			}
		} else {
			REGISTERPM_GLOBAL(start_memory) = NULL;
			active_memory = NULL;
		}

	}

	#else

	REGISTERPM_GLOBAL(active_memory) = NULL;
	REGISTERPM_GLOBAL(start_memory) = NULL;

	#endif

	return SUCCESS;

}

int REGISTERPM_FASTCALL registerPM_memory_observe(zval **var TSRMLS_DC){
	registerpm_memory_entry *active_memory = REGISTERPM_GLOBAL(active_memory);
	active_memory->pointer++;
	#ifndef PHALCON_RELEASE
	if (active_memory->pointer >= (REGISTERPM_MAX_MEMORY_STACK-1)) {
		fprintf(stderr, "ERROR: register PM memory stack is too small %d\n", REGISTERPM_MAX_MEMORY_STACK);
		return FAILURE;
	}
	#endif
	active_memory->addresses[active_memory->pointer] = var;
	active_memory->addresses[active_memory->pointer+1] = NULL;
	return SUCCESS;
}

int REGISTERPM_FASTCALL registerPM_memory_alloc(zval **var TSRMLS_DC){
	registerpm_memory_entry *active_memory = REGISTERPM_GLOBAL(active_memory);
	active_memory->pointer++;
	#ifndef PHALCON_RELEASE
	if (active_memory->pointer >= (REGISTERPM_MAX_MEMORY_STACK-1)) {
		fprintf(stderr, "ERROR: register PM memory stack is too small %d\n", REGISTERPM_MAX_MEMORY_STACK);
		return FAILURE;
	}
	#endif
	active_memory->addresses[active_memory->pointer] = var;
	active_memory->addresses[active_memory->pointer+1] = NULL;
	ALLOC_ZVAL(*var);
	INIT_PZVAL(*var);
	ZVAL_NULL(*var);
	return SUCCESS;
}

int REGISTERPM_FASTCALL registerPM_memory_remove(zval **var TSRMLS_DC){
	zval_ptr_dtor(var);
	*var = NULL;
	return SUCCESS;
}

int REGISTERPM_FASTCALL registerPM_clean_restore_stack(TSRMLS_D){
	while (REGISTERPM_GLOBAL(active_memory) != NULL) {
		registerPM_memory_restore_stack(TSRMLS_C);
	}
	return SUCCESS;
}

void REGISTERPM_FASTCALL registerPM_copy_ctor(zval *destiny, zval *origin){
	if (Z_REFCOUNT_P(origin) > 1) {
		zval_copy_ctor(destiny);
	} else {
		ZVAL_NULL(origin);
	}
}


static inline zend_class_entry *registerPM_lookup_class_ce(zval *object, char *property_name, int property_length TSRMLS_DC){

	zend_class_entry *ce, *original_ce;

	ce = Z_OBJCE_P(object);
	original_ce = ce;
	while (ce) {
		if (zend_hash_exists(&ce->properties_info, property_name, property_length+1)) {
			return ce;
		}
		ce = ce->parent;
	}
	return original_ce;
}

int registerPM_update_property_zval(zval *obj, char *property_name, int property_length, zval *value TSRMLS_DC){

	zend_class_entry *ce;

	if (Z_TYPE_P(obj) != IS_OBJECT) {
		php_error_docref(NULL TSRMLS_CC, E_WARNING, "Attempt to assign property of non-object");
		return FAILURE;
	}

	ce = registerPM_lookup_class_ce(obj, property_name, property_length TSRMLS_CC);
	zend_update_property(ce, obj, property_name, property_length, value TSRMLS_CC);

	return SUCCESS;
}

int registerPM_update_property_zval_zval(zval *obj, zval *property, zval *value TSRMLS_DC){

	zend_class_entry *ce;

	if (Z_TYPE_P(obj) != IS_OBJECT) {
		php_error_docref(NULL TSRMLS_CC, E_WARNING, "Attempt to assign property of non-object");
		return FAILURE;
	}

	if (Z_TYPE_P(property) != IS_STRING) {
		php_error_docref(NULL TSRMLS_CC, E_WARNING, "Property should be string");
		return FAILURE;
	}

	ce = registerPM_lookup_class_ce(obj, Z_STRVAL_P(property), Z_STRLEN_P(property) TSRMLS_CC);
	zend_update_property(ce, obj, Z_STRVAL_P(property), Z_STRLEN_P(property), value TSRMLS_CC);

	return SUCCESS;
}


void registerPM_throw_exception(zval *object TSRMLS_DC){
	Z_ADDREF_P(object);
	zend_throw_exception_object(object TSRMLS_CC);
	registerPM_memory_restore_stack(TSRMLS_C);
}

void registerPM_throw_exception_zval(zend_class_entry *ce, zval *message TSRMLS_DC){

	zval *object;

	ALLOC_INIT_ZVAL(object);
	object_init_ex(object, ce);

	REGISTERPM_CALL_METHOD_PARAMS_1_NORETURN(object, "__construct", message, PH_CHECK);

	zend_throw_exception_object(object TSRMLS_CC);

	registerPM_memory_restore_stack(TSRMLS_C);
}

void registerPM_throw_exception_string(zend_class_entry *ce, char *message, zend_uint message_len TSRMLS_DC){

	zval *object, *msg;

	ALLOC_INIT_ZVAL(object);
	object_init_ex(object, ce);

	REGISTERPM_INIT_VAR(msg);
	ZVAL_STRINGL(msg, message, message_len, 1);

	REGISTERPM_CALL_METHOD_PARAMS_1_NORETURN(object, "__construct", msg, PH_CHECK);

	zend_throw_exception_object(object TSRMLS_CC);

	registerPM_memory_restore_stack(TSRMLS_C);
}

void registerPM_fast_explode(zval *result, zval *delimiter, zval *str TSRMLS_DC){

	if (Z_TYPE_P(str) != IS_STRING || Z_TYPE_P(delimiter) != IS_STRING) {
		ZVAL_NULL(result);
		php_error_docref(NULL TSRMLS_CC, E_WARNING, "Invalid arguments supplied for explode()");
		return;
	}

	array_init(result);
	php_explode(delimiter, str, result, LONG_MAX);
}


int registerPM_memnstr_str(zval *haystack, char *needle, int needle_length TSRMLS_DC){

	char *found = NULL;

	if (Z_TYPE_P(haystack) != IS_STRING) {
		php_error_docref(NULL TSRMLS_CC, E_WARNING, "Invalid arguments supplied for memnstr()");
		return 0;
	}

	if (Z_STRLEN_P(haystack) >= needle_length) {
		found = php_memnstr(Z_STRVAL_P(haystack), needle, needle_length, Z_STRVAL_P(haystack) + Z_STRLEN_P(haystack));
	}

	if (found) {
		return 1;
	}

	return 0;
}


int registerPM_array_fetch(zval **return_value, zval *arr, zval *index, int silent TSRMLS_DC){

	zval **zv;
	int result = FAILURE, type;

 	if (Z_TYPE_P(index) == IS_ARRAY || Z_TYPE_P(index) == IS_OBJECT) {
		if (silent == PH_NOISY) {
			php_error_docref(NULL TSRMLS_CC, E_NOTICE, "Illegal offset type");
		}
		return FAILURE;
	}

 	if (Z_TYPE_P(index) == IS_NULL) {
		convert_to_string(index);
	}

	if (Z_TYPE_P(arr) == IS_NULL || Z_TYPE_P(arr) == IS_BOOL) {
		return FAILURE;
	}

	if (Z_TYPE_P(index) != IS_STRING && Z_TYPE_P(index) != IS_LONG && Z_TYPE_P(index) != IS_DOUBLE) {
		if (silent == PH_NOISY) {
			php_error_docref(NULL TSRMLS_CC, E_NOTICE, "Illegal offset type");
		}
		return FAILURE;
	}

 	if (Z_TYPE_P(index) == IS_STRING) {
       	if ((type = is_numeric_string(Z_STRVAL_P(index), Z_STRLEN_P(index), NULL, NULL, 0))) {
			if (type == IS_LONG) {
				convert_to_long(index);
			}
		}
	} else {
		if (Z_TYPE_P(index) == IS_DOUBLE) {
			convert_to_long(index);
		}
	}

 	if (Z_TYPE_P(index) == IS_STRING) {
		if ((result = zend_hash_find(Z_ARRVAL_P(arr), Z_STRVAL_P(index), Z_STRLEN_P(index)+1, (void**) &zv)) == SUCCESS) {
			zval_ptr_dtor(return_value);
			*return_value = *zv;
			Z_ADDREF_PP(return_value);
			return SUCCESS;
		}
	}

 	if (Z_TYPE_P(index) == IS_LONG) {
		if ((result = zend_hash_index_find(Z_ARRVAL_P(arr), Z_LVAL_P(index), (void**) &zv)) == SUCCESS) {
			zval_ptr_dtor(return_value);
			*return_value = *zv;
			Z_ADDREF_PP(return_value);
			return SUCCESS;
		}
	}

	if (silent == PH_NOISY) {
		if (Z_TYPE_P(index) == IS_LONG) {
			php_error_docref(NULL TSRMLS_CC, E_NOTICE, "Undefined index: %ld", Z_LVAL_P(index));
		} else {
			php_error_docref(NULL TSRMLS_CC, E_NOTICE, "Undefined index: %s", Z_STRVAL_P(index));
		}
	}

	return FAILURE;
}

int registerPM_array_fetch_long(zval **return_value, zval *arr, ulong index, int silent TSRMLS_DC){

	zval **zv;
	int result = FAILURE;

	if (Z_TYPE_P(arr) != IS_ARRAY) {
		if (silent == PH_NOISY) {
			php_error_docref(NULL TSRMLS_CC, E_NOTICE, "Cannot use a scalar value as an array");
		}
		return FAILURE;
	}

	if ((result = zend_hash_index_find(Z_ARRVAL_P(arr), index, (void**)&zv)) == SUCCESS) {
		zval_ptr_dtor(return_value);
		*return_value = *zv;
		Z_ADDREF_PP(return_value);
		return SUCCESS;
	}

	if (silent == PH_NOISY) {
		php_error_docref(NULL TSRMLS_CC, E_NOTICE, "Undefined index: %ld", index);
	}

	return FAILURE;

}


int registerPM_array_update_zval(zval **arr, zval *index, zval **value, int flags TSRMLS_DC){

	if (Z_TYPE_PP(arr) != IS_ARRAY) {
		php_error_docref(NULL TSRMLS_CC, E_NOTICE, "Cannot use a scalar value as an array");
		return FAILURE;
	}

	if (Z_TYPE_P(index) == IS_NULL) {
		convert_to_string(index);
	} else {
		if (Z_TYPE_P(index) == IS_BOOL || Z_TYPE_P(index) == IS_DOUBLE) {
			convert_to_long(index);
		}
	}

	if ((flags & PH_CTOR) == PH_CTOR) {
		zval *new_zv;
		Z_DELREF_PP(value);
		ALLOC_ZVAL(new_zv);
		INIT_PZVAL_COPY(new_zv, *value);
		*value = new_zv;
		zval_copy_ctor(new_zv);
	}

	if ((flags & PH_SEPARATE) == PH_SEPARATE) {
		if (Z_REFCOUNT_PP(arr) > 1) {
			zval *new_zv;
			Z_DELREF_PP(arr);
			ALLOC_ZVAL(new_zv);
			INIT_PZVAL_COPY(new_zv, *arr);
			*arr = new_zv;
			zval_copy_ctor(new_zv);
	    }
	}

	if ((flags & PH_COPY) == PH_COPY) {
		Z_ADDREF_PP(value);
	}

 	if (Z_TYPE_P(index) == IS_STRING) {
		return zend_hash_update(Z_ARRVAL_PP(arr), Z_STRVAL_P(index), Z_STRLEN_P(index)+1, value, sizeof(zval *), NULL);
	} else {
		if (Z_TYPE_P(index) == IS_LONG) {
			return zend_hash_index_update(Z_ARRVAL_PP(arr), Z_LVAL_P(index), value, sizeof(zval *), NULL);
		} else {
			php_error_docref(NULL TSRMLS_CC, E_NOTICE, "Illegal offset type");
		}
	}

	return FAILURE;
}

void registerPM_array_update_zval_zval_zval_multi_3(zval **arr, zval *index1, zval *index2, zval *index3, zval **value, int flags TSRMLS_DC){

	zval *temp1 = NULL, *temp2 = NULL;

	if (Z_TYPE_PP(arr) == IS_ARRAY) {

		ALLOC_INIT_ZVAL(temp1);
		registerPM_array_fetch(&temp1, *arr, index1, PH_SILENT_CC);
		if (Z_REFCOUNT_P(temp1) > 1) {
			registerPM_array_update_zval(arr, index1, &temp1, PH_COPY | PH_CTOR TSRMLS_CC);
		}
		if (Z_TYPE_P(temp1) != IS_ARRAY) {
			convert_to_array(temp1);
			registerPM_array_update_zval(arr, index1, &temp1, PH_COPY TSRMLS_CC);
		}

		ALLOC_INIT_ZVAL(temp2);
		registerPM_array_fetch(&temp2, temp1, index2, PH_SILENT_CC);
		if (Z_REFCOUNT_P(temp2) > 1) {
			registerPM_array_update_zval(&temp1, index2, &temp2, PH_COPY | PH_CTOR TSRMLS_CC);
		}
		if (Z_TYPE_P(temp2) != IS_ARRAY) {
			convert_to_array(temp2);
			registerPM_array_update_zval(&temp1, index2, &temp2, PH_COPY TSRMLS_CC);
		}

		registerPM_array_update_zval(&temp2, index3, value, PH_COPY TSRMLS_CC);
	}

	if (temp1 != NULL) {
		zval_ptr_dtor(&temp1);
	}
	if (temp2 != NULL) {
		zval_ptr_dtor(&temp2);
	}

}


void registerPM_array_update_multi_2(zval **arr, zval *index1, zval *index2, zval **value, int flags TSRMLS_DC){

	zval *temp;

	ALLOC_INIT_ZVAL(temp);

	if (Z_TYPE_PP(arr) == IS_ARRAY) {
		registerPM_array_fetch(&temp, *arr, index1, PH_SILENT_CC);
		if (Z_REFCOUNT_P(temp) > 1) {
			registerPM_array_update_zval(arr, index1, &temp, PH_COPY | PH_CTOR TSRMLS_CC);
		}
		if (Z_TYPE_P(temp) != IS_ARRAY) {
			convert_to_array(temp);
			registerPM_array_update_zval(arr, index1, &temp, PH_COPY TSRMLS_CC);
		}
		registerPM_array_update_zval(&temp, index2, value, flags | PH_COPY TSRMLS_CC);
	}

	zval_ptr_dtor(&temp);

}

int registerPM_read_property(zval **result, zval *object, char *property_name, int property_length, int silent TSRMLS_DC){

	zval *tmp = NULL;
	zend_class_entry *ce;

	if (Z_TYPE_P(object) == IS_OBJECT) {
		ce = registerPM_lookup_class_ce(object, property_name, property_length TSRMLS_CC);
		tmp = zend_read_property(ce, object, property_name, property_length, 0 TSRMLS_CC);
		Z_ADDREF_P(tmp);
		zval_ptr_dtor(result);
		*result = tmp;
		return SUCCESS;
	} else {
		if (silent == PH_NOISY) {
			php_error_docref(NULL TSRMLS_CC, E_NOTICE, "Trying to get property of non-object");
		}
	}
	return FAILURE;
}

int registerPM_read_property_zval(zval **result, zval *object, zval *property, int silent TSRMLS_DC){

	zval *tmp = NULL;
	zend_class_entry *ce;

	if (Z_TYPE_P(object) == IS_OBJECT) {
		if (Z_TYPE_P(property) == IS_STRING) {
			ce = registerPM_lookup_class_ce(object, Z_STRVAL_P(property), Z_STRLEN_P(property) TSRMLS_CC);
			tmp = zend_read_property(ce, object, Z_STRVAL_P(property), Z_STRLEN_P(property), 0 TSRMLS_CC);
			Z_ADDREF_P(tmp);
			zval_ptr_dtor(result);
			*result = tmp;
		}
	} else {
		if (silent == PH_NOISY) {
			php_error_docref(NULL TSRMLS_CC, E_NOTICE, "Trying to get property of non-object");
			return FAILURE;
		}
	}
	return SUCCESS;
}


#ifdef HAVE_CONFIG_H
#endif



//////////////////////////////////////////////////////////////////////////
//////////////////FUNCTIONS IMPLEMENTATON////////////////////////////////


#ifdef HAVE_CONFIG_H
#endif







PHP_METHOD(registerPM_Config_Adapter_Ini, __construct){

	zval *file_path, *config, *process_sections;
	zval *ini_config, *base_path, *exception_message;
	zval *dot, *directives = NULL, *section = NULL, *value = NULL, *key = NULL, *directive_parts = NULL;
	zval *left_part = NULL, *right_part = NULL;
	HashTable *ah0, *ah1;
	HashPosition hp0, hp1;
	zval **hd;
	char *hash_index;
	uint hash_index_len;
	ulong hash_num;
	int hash_type;

	REGISTERPM_MM_GROW();

	if (zend_parse_parameters(ZEND_NUM_ARGS() TSRMLS_CC, "z", &file_path) == FAILURE) {
		REGISTERPM_MM_RESTORE();
		RETURN_NULL();
	}
	
	REGISTERPM_INIT_VAR(config);
	array_init(config);
	
	REGISTERPM_INIT_VAR(process_sections);
	ZVAL_BOOL(process_sections, 1);
	
	REGISTERPM_INIT_VAR(ini_config);
	REGISTERPM_CALL_FUNC_PARAMS_2(ini_config, "parse_ini_file", file_path, process_sections);
	if (REGISTERPM_IS_FALSE(ini_config)) {
		REGISTERPM_INIT_VAR(base_path);
		REGISTERPM_CALL_FUNC_PARAMS_1(base_path, "basename", file_path);
		
		REGISTERPM_INIT_VAR(exception_message);
		REGISTERPM_CONCAT_SVS(exception_message, "Configuration file ", base_path, " can't be loaded");
		REGISTERPM_THROW_EXCEPTION_ZVAL(registerPM_config_exception_ce, exception_message);
		return;
	}
	
	REGISTERPM_INIT_VAR(dot);
	ZVAL_STRING(dot, ".", 1);
	
	if (!registerPM_valid_foreach(ini_config TSRMLS_CC)) {
		return;
	}
	
	ah0 = Z_ARRVAL_P(ini_config);
	zend_hash_internal_pointer_reset_ex(ah0, &hp0);
	
	ph_cycle_start_0:
	
		if (zend_hash_get_current_data_ex(ah0, (void**) &hd, &hp0) != SUCCESS) {
			goto ph_cycle_end_0;
		}
		
		REGISTERPM_GET_FOREACH_KEY(section, ah0, hp0);
		REGISTERPM_GET_FOREACH_VALUE(directives);
		
		
		if (!registerPM_valid_foreach(directives TSRMLS_CC)) {
			return;
		}
		
		ah1 = Z_ARRVAL_P(directives);
		zend_hash_internal_pointer_reset_ex(ah1, &hp1);
		
		ph_cycle_start_1:
		
			if (zend_hash_get_current_data_ex(ah1, (void**) &hd, &hp1) != SUCCESS) {
				goto ph_cycle_end_1;
			}
			
			REGISTERPM_GET_FOREACH_KEY(key, ah1, hp1);
			REGISTERPM_GET_FOREACH_VALUE(value);
			
			if (registerPM_memnstr_str(key, SL(".") TSRMLS_CC)) {
				REGISTERPM_INIT_NVAR(directive_parts);
				registerPM_fast_explode(directive_parts, dot, key TSRMLS_CC);
				
				REGISTERPM_INIT_NVAR(left_part);
				registerPM_array_fetch_long(&left_part, directive_parts, 0, PH_NOISY_CC);
				
				REGISTERPM_INIT_NVAR(right_part);
				registerPM_array_fetch_long(&right_part, directive_parts, 1, PH_NOISY_CC);
				registerPM_array_update_zval_zval_zval_multi_3(&config, section, left_part, right_part, &value, 0 TSRMLS_CC);
			} else {
				registerPM_array_update_multi_2(&config, section, key, &value, 0 TSRMLS_CC);
			}
			
			zend_hash_move_forward_ex(ah1, &hp1);
			goto ph_cycle_start_1;
			
		ph_cycle_end_1:
		
		
		zend_hash_move_forward_ex(ah0, &hp0);
		goto ph_cycle_start_0;
		
	ph_cycle_end_0:
	
	REGISTERPM_CALL_PARENT_PARAMS_1_NORETURN(this_ptr, "registerPM\\Config\\Adapter\\Ini", "__construct", config);
	
	REGISTERPM_MM_RESTORE();
}


////////////////////////////////////////////////////////////////////////////////////////////////////



#ifdef HAVE_CONFIG_H
#endif







PHP_METHOD(registerPM_Config, __construct){

	zval *array_config = NULL, *value = NULL, *key = NULL, *config_value = NULL;
	HashTable *ah0;
	HashPosition hp0;
	zval **hd;
	char *hash_index;
	uint hash_index_len;
	ulong hash_num;
	int hash_type;
	int eval_int;

	REGISTERPM_MM_GROW();

	if (zend_parse_parameters(ZEND_NUM_ARGS() TSRMLS_CC, "|z", &array_config) == FAILURE) {
		REGISTERPM_MM_RESTORE();
		RETURN_NULL();
	}

	if (!array_config) {
		REGISTERPM_INIT_NVAR(array_config);
		array_init(array_config);
	}
	
	if (Z_TYPE_P(array_config) == IS_ARRAY) { 
		
		if (!registerPM_valid_foreach(array_config TSRMLS_CC)) {
			return;
		}
		
		ah0 = Z_ARRVAL_P(array_config);
		zend_hash_internal_pointer_reset_ex(ah0, &hp0);
		
		ph_cycle_start_0:
		
			if (zend_hash_get_current_data_ex(ah0, (void**) &hd, &hp0) != SUCCESS) {
				goto ph_cycle_end_0;
			}
			
			REGISTERPM_GET_FOREACH_KEY(key, ah0, hp0);
			REGISTERPM_GET_FOREACH_VALUE(value);
			
			if (Z_TYPE_P(value) == IS_ARRAY) { 
				eval_int = phalcon_array_isset_long(value, 0);
				if (!eval_int) {
					REGISTERPM_INIT_NVAR(config_value);
					object_init_ex(config_value, registerPM_config_ce);
					REGISTERPM_CALL_METHOD_PARAMS_1_NORETURN(config_value, "__construct", value, PH_CHECK);
					registerPM_update_property_zval_zval(this_ptr, key, config_value TSRMLS_CC);
				} else {
					registerPM_update_property_zval_zval(this_ptr, key, value TSRMLS_CC);
				}
			} else {
				registerPM_update_property_zval_zval(this_ptr, key, value TSRMLS_CC);
			}
			
			zend_hash_move_forward_ex(ah0, &hp0);
			goto ph_cycle_start_0;
			
		ph_cycle_end_0:
		if(0){}
		
	} else {
		REGISTERPM_THROW_EXCEPTION_STR(registerPM_config_exception_ce, "The configuration must be an Array");
		return;
	}
	
	REGISTERPM_MM_RESTORE();
}

PHP_METHOD(registerPM_Config, __set_state){

	zval *data, *config;

	REGISTERPM_MM_GROW();

	if (zend_parse_parameters(ZEND_NUM_ARGS() TSRMLS_CC, "z", &data) == FAILURE) {
		REGISTERPM_MM_RESTORE();
		RETURN_NULL();
	}

	REGISTERPM_INIT_VAR(config);
	object_init_ex(config, registerPM_config_ce);
	REGISTERPM_CALL_METHOD_PARAMS_1_NORETURN(config, "__construct", data, PH_CHECK);
	
	RETURN_CTOR(config);
}



////////////////////////////////////////////////////////////////////////////////////////////



#ifdef HAVE_CONFIG_H
#endif







PHP_METHOD(registerPM_Clasesilla, __construct){

	zval *parameter = NULL;

	REGISTERPM_MM_GROW();

	if (zend_parse_parameters(ZEND_NUM_ARGS() TSRMLS_CC, "|z", &parameter) == FAILURE) {
		REGISTERPM_MM_RESTORE();
		RETURN_NULL();
	}

	if (!parameter) {
		REGISTERPM_INIT_NVAR(parameter);
	} else {
		REGISTERPM_SEPARATE_PARAM(parameter);
	}

	registerPM_update_property_zval(this_ptr, SL("_parameter"), parameter TSRMLS_CC);

	REGISTERPM_MM_RESTORE();
}

PHP_METHOD(registerPM_Clasesilla, setCleanMode){

	zval *iclean;
    //iclean = STACK_VAR
	if (zend_parse_parameters(ZEND_NUM_ARGS() TSRMLS_CC, "z", &iclean) == FAILURE) {
		RETURN_NULL();
	}
    // this_ptr->m_iclean = iclean;
	//phalcon_update_static_property(SL("phalcon\\Clasesilla"), SL("_iclean"), iclean TSRMLS_CC);
	registerPM_update_property_zval(this_ptr, SL("_iclean"), iclean TSRMLS_CC);
    // return &iclean
	RETURN_CTORW(this_ptr);
}

PHP_METHOD(registerPM_Clasesilla, getCleanMode){

	zval *cleanMode = NULL;

	REGISTERPM_MM_GROW();

	REGISTERPM_INIT_VAR(cleanMode);
	REGISTERPM_OBSERVE_VAR(cleanMode);
	//phalcon_read_static_property(&cleanMode, SL("phalcon\\Clasesilla"), SL("_iclean") TSRMLS_CC);
	registerPM_read_property(&cleanMode, this_ptr, SL("_iclean"), PH_NOISY_CC);

	RETURN_CCTOR(cleanMode);
}

void Class_something(zval *return_value, zval *request TSRMLS_DC){
	int index;
	smart_str my_response = {0};
	//smart_str my_requestUri = {0};
	char *request_uri;


	if (Z_TYPE_P(request) != IS_STRING)
	{
		php_error_docref(NULL TSRMLS_CC, E_WARNING, "Invalid request arguments supplied for DoSomething()");
		return;
	}
        //  http://roojs.com/blog.php/View/111/.html
	request_uri = Z_STRVAL_P(request);
	//smart_str_appends(&my_requestUri, request_uri);
	/*for (index = 0; index < Z_STRLEN_P(str); index++)
	{
		smart_str_appendc(&my_requestUri, *request_uri);
		request_uri++;
	}*/
	if(strcmp( request_uri, "/sys/en/classic/login/login") == 0
	    || strcmp( request_uri, "/sysworkflow/en/classic/login/login") == 0)
	{
	     smart_str_appends(&my_response, "methods/login/login.php");
	}
	else
	{
	     smart_str_appends(&my_response, "error404.php");
	}
	smart_str_0(&my_response);

	if (my_response.len)
	{
		ZVAL_STRINGL(return_value, my_response.c, my_response.len, 0);
	} else
	{
		ZVAL_STRING(return_value, "", 1);
	}
}


PHP_METHOD(registerPM_Clasesilla, DoSomething){

	zval *requestUri;
	zval *state_message;
	zval *flush_message;

	REGISTERPM_MM_GROW();

	//message = STACK_VAR
	if (zend_parse_parameters(ZEND_NUM_ARGS() TSRMLS_CC, "z", &requestUri) == FAILURE) {
		REGISTERPM_MM_RESTORE();
		RETURN_NULL();
	}

	REGISTERPM_INIT_VAR(state_message); // set return var state_messaging;
	ZVAL_STRING(state_message, "starting...", 1);// set state_messaging

	REGISTERPM_INIT_VAR(flush_message); // set return var flash_message;
	if ( REGISTERPM_GLOBAL(pm_enable)) 
        {
		Class_something(flush_message, requestUri TSRMLS_CC);
		//xdfree(bla);
	}

	//REGISTERPM_CALL_METHOD_PARAMS_2(return_value, object, method_name, param1, param2, check) if(phalcon_call_method_two_params(return_value, object, method_name, strlen(method_name), param1, param2, check, 1 TSRMLS_CC)==FAILURE) return;
	//int phalcon_call_method_two_params(zval *return_value, zval *object, char *method_name, int method_len, zval *param1, zval *param2, int check, int noreturn TSRMLS_DC){
	//	zval *params[] = { param1, param2 };
	//	return phalcon_call_method_params(return_value, object, method_name, method_len, 2, params, check, noreturn TSRMLS_CC);
	//}
	//flash_message = this_ptr->DoClean( type, message) // PH_NO_CHECK: don't take a look into Hash  methods functions table.
//REGISTERPM_CALL_METHOD_PARAMS_2(flash_message, this_ptr, "something", type, message, PH_NO_CHECK);

	RETURN_CCTOR(flush_message);
}






#ifdef HAVE_CONFIG_H
#endif






///////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////



#ifdef HAVE_CONFIG_H
#endif



zend_class_entry *registerPM_config_exception_ce;
zend_class_entry *registerPM_config_adapter_ini_ce;
zend_class_entry *registerPM_exception_ce;
zend_class_entry *registerPM_config_ce;

ZEND_DECLARE_MODULE_GLOBALS(registerPM)


PHP_INI_BEGIN()
	/* Debugger settings */
	STD_PHP_INI_BOOLEAN("registerPM.pm_enable",  "0",  PHP_INI_ALL,  OnUpdateBool,   pm_enable, zend_registerPM_globals, registerPM_globals)
	PHP_INI_ENTRY("registerPM.greeting", "Hello World", PHP_INI_ALL, NULL)
PHP_INI_END()


PHP_MINIT_FUNCTION(registerPM){

        REGISTER_INI_ENTRIES();//ini_entries

	if(!zend_ce_serializable){
		fprintf(stderr, "register Error: Interface Serializable was not found");
		return FAILURE;
	}
	if(!zend_ce_iterator){
		fprintf(stderr, "regioster Error: Interface Iterator was not found");
		return FAILURE;
	}
	if(!spl_ce_SeekableIterator){
		fprintf(stderr, "register Error: Interface SeekableIterator was not found");
		return FAILURE;
	}
	if(!spl_ce_Countable){
		fprintf(stderr, "register Error: Interface Countable was not found");
		return FAILURE;
	}
	if(!zend_ce_arrayaccess){
		fprintf(stderr, "register Error: Interface ArrayAccess was not found");
		return FAILURE;
	}

	/** Init globals */
	ZEND_INIT_MODULE_GLOBALS(registerPM, php_registerPM_init_globals, NULL);


	REGISTERPM_REGISTER_CLASS(registerPM, Clasesilla, clasesilla, registerPM_clasesilla_method_entry, 0); // 4 methods
	zend_declare_property_null(registerPM_clasesilla_ce, SL("_parameter"), ZEND_ACC_PROTECTED TSRMLS_CC);
	zend_declare_property_long(registerPM_clasesilla_ce, SL("_iclean"), 3, ZEND_ACC_PROTECTED TSRMLS_CC);

	REGISTERPM_REGISTER_CLASS(registerPM, Config, config, registerPM_config_method_entry, 0);//two methods
	
REGISTERPM_REGISTER_CLASS_EX(registerPM, Exception, exception, "exception", NULL, 0); //inherit from exception

REGISTERPM_REGISTER_CLASS_EX(registerPM\\Config, Exception, config_exception, "registerpm\\exception", NULL, 0);//inherit f registerPM\\exception

	REGISTERPM_REGISTER_CLASS_EX(registerPM\\Config\\Adapter, Ini, config_adapter_ini, "registerpm\\config", registerPM_config_adapter_ini_method_entry, 0); // 0ne method

	return SUCCESS;
}

PHP_MSHUTDOWN_FUNCTION(registerPM){

	UNREGISTER_INI_ENTRIES();

	if (REGISTERPM_GLOBAL(active_memory) != NULL) {
		registerPM_clean_shutdown_stack(TSRMLS_C);
	}
	return SUCCESS;
}

PHP_RINIT_FUNCTION(registerPM){
	return SUCCESS;
}

PHP_RSHUTDOWN_FUNCTION(registerPM){
	if (REGISTERPM_GLOBAL(active_memory) != NULL) {
		registerPM_clean_shutdown_stack(TSRMLS_C);
	}
	return SUCCESS;
}

zend_module_entry registerPM_module_entry = {
#if ZEND_MODULE_API_NO >= 20010901
	STANDARD_MODULE_HEADER,
#endif
	PHP_REGISTERPM_EXTNAME,
	NULL,
	PHP_MINIT(registerPM),
	PHP_MSHUTDOWN(registerPM),
	PHP_RINIT(registerPM),
	PHP_RSHUTDOWN(registerPM),
	NULL,
#if ZEND_MODULE_API_NO >= 20010901
	PHP_REGISTERPM_VERSION,
#endif
	STANDARD_MODULE_PROPERTIES
};

#ifdef COMPILE_DL_REGISTERPM
ZEND_GET_MODULE(registerPM)
#endif









/*
  +------------------------------------------------------------------------+
  +------------------------------------------------------------------------+
  +------------------------------------------------------------------------+
  | This source file is subject to the New BSD License that is bundled     |
  | with this package in the file docs/LICENSE.txt.                        |
  |                                                                        |
  +------------------------------------------------------------------------+
  +------------------------------------------------------------------------+
*/

zend_class_entry *registerPM_config_exception_ce;
zend_class_entry *registerPM_config_adapter_ini_ce;
zend_class_entry *registerPM_exception_ce;

zend_class_entry *registerPM_clasesilla_ce;

zend_class_entry *registerPM_config_ce;

//////////////////// PROTOTYPE DECLARATION ///////////////////////////

//Config Adapter ini

PHP_METHOD(registerPM_Config_Adapter_Ini, __construct);

//Clasesilla

PHP_METHOD(registerPM_Clasesilla, __construct);
PHP_METHOD(registerPM_Clasesilla, setCleanMode);
PHP_METHOD(registerPM_Clasesilla, getCleanMode);
PHP_METHOD(registerPM_Clasesilla, DoSomething);

// Config

PHP_METHOD(registerPM_Config, __construct);
PHP_METHOD(registerPM_Config, __set_state);


////////////////// ARGUMENTS CONF ////////////////////////////////////////////////

//Config Adapter ini

ZEND_BEGIN_ARG_INFO_EX(arginfo_registerPM_config_adapter_ini___construct, 0, 0, 1)
	ZEND_ARG_INFO(0, filePath)
ZEND_END_ARG_INFO()

//Clasesilla

ZEND_BEGIN_ARG_INFO_EX(arginfo_registerPM_clasesilla___construct, 0, 0, 0)
	ZEND_ARG_INFO(0, parameter)
ZEND_END_ARG_INFO()

ZEND_BEGIN_ARG_INFO_EX(arginfo_registerPM_clasesilla_setCleanMode, 0, 0, 1)
	ZEND_ARG_INFO(0, iclean)
ZEND_END_ARG_INFO()

ZEND_BEGIN_ARG_INFO_EX(arginfo_registerPM_clasesilla_getCleanMode, 0, 0, 1)
	ZEND_ARG_INFO(0, iclean)
ZEND_END_ARG_INFO()

ZEND_BEGIN_ARG_INFO_EX(arginfo_registerPM_clasesilla_DoSomething, 0, 0, 1)
	ZEND_ARG_INFO(0, requestUri)
ZEND_END_ARG_INFO()


// Config

ZEND_BEGIN_ARG_INFO_EX(arginfo_registerPM_config___construct, 0, 0, 0)
	ZEND_ARG_INFO(0, arrayConfig)
ZEND_END_ARG_INFO()

ZEND_BEGIN_ARG_INFO_EX(arginfo_registerPM_config___set_state, 0, 0, 1)
	ZEND_ARG_INFO(0, data)
ZEND_END_ARG_INFO()

////////////////////// FUNCTIONS ADDITION ////////////////////////////////////////

REGISTERPM_INIT_FUNCS(registerPM_config_adapter_ini_method_entry){
	PHP_ME(registerPM_Config_Adapter_Ini, __construct, arginfo_registerPM_config_adapter_ini___construct, ZEND_ACC_PUBLIC|ZEND_ACC_CTOR) 
	PHP_FE_END
};

REGISTERPM_INIT_FUNCS(registerPM_clasesilla_method_entry){
	PHP_ME(registerPM_Clasesilla, __construct, arginfo_registerPM_clasesilla___construct, ZEND_ACC_PUBLIC|ZEND_ACC_CTOR)
	PHP_ME(registerPM_Clasesilla, setCleanMode, arginfo_registerPM_clasesilla_setCleanMode, ZEND_ACC_PUBLIC)
	PHP_ME(registerPM_Clasesilla, getCleanMode, arginfo_registerPM_clasesilla_getCleanMode, ZEND_ACC_PUBLIC)
	PHP_ME(registerPM_Clasesilla, DoSomething, arginfo_registerPM_clasesilla_DoSomething, ZEND_ACC_PUBLIC)
	PHP_FE_END
};

REGISTERPM_INIT_FUNCS(registerPM_config_method_entry){
	PHP_ME(registerPM_Config, __construct, arginfo_registerPM_config___construct, ZEND_ACC_PUBLIC|ZEND_ACC_CTOR) 
	PHP_ME(registerPM_Config, __set_state, arginfo_registerPM_config___set_state, ZEND_ACC_PUBLIC|ZEND_ACC_STATIC) 
	PHP_FE_END
};


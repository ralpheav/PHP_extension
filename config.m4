PHP_ARG_ENABLE(registerPM, whether to enable registerPM extension, [ --enable-registerPM   Enable registerPM extension])

if test "$PHP_REGISTERPM" = "yes"; then
  AC_DEFINE(HAVE_REGISTERPM, 1, [Whether you have register pm extension])
  PHP_NEW_EXTENSION(registerPM, registerPM.c, $ext_shared)
fi


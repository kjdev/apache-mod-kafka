#                                               -*- Autoconf -*-
# Process this file with autoconf to produce a configure script.

AC_INIT(mod_kafka, 0.2.1,[])

AC_CONFIG_SRCDIR([mod_kafka.c])
AC_CONFIG_HEADERS([config.h])
AC_CONFIG_MACRO_DIR([m4])

AM_INIT_AUTOMAKE([-Wall -Werror foreign subdir-objects])

# Checks for programs.
AM_PROG_AR
AC_PROG_LIBTOOL

# Option for kafka debug log level
AC_ARG_WITH([kafka-debug-log],
            [AC_HELP_STRING([--with-kafka-debug-log=LEVEL],
                            [kafka debug log level [default=no]])],
            [KAFKA_DEBUG_LOG="${withval}"],
            [KAFKA_DEBUG_LOG=no])
AS_IF([test "x${KAFKA_DEBUG_LOG}" != "xno"],
      [AC_DEFINE_UNQUOTED([KAFKA_DEBUG_LOG_LEVEL], [${KAFKA_DEBUG_LOG}],
                          [kafka debug log level])])

# Checks for apxs.
AC_ARG_WITH([apxs],
            [AC_HELP_STRING([--with-apxs=PATH], [apxs path [default=yes]])],
            [APXS_PATH="$withval"],
            [APXS_PATH="/usr/bin:/usr/sbin:/usr/local/bin:/usr/local/sbin"])

AC_PATH_PROG(APXS, apxs, no, ${APXS_PATH})

AS_IF([test "x${APXS}" = "xno" ],
      [AC_MSG_ERROR([apxs not found])],
      [test "x${APXS}" = "x"],
      [AC_MSG_ERROR([apxs not found])])

APXS_CFLAGS=`${APXS} -q CFLAGS 2> /dev/null`
APXS_CPPFLAGS=`${APXS} -q CPPFLAGS 2> /dev/null`
APXS_LDFLAGS=`${APXS} -q LDFLAGS 2> /dev/null`
APXS_LIBS=`${APXS} -q LIBS 2> /dev/null`
APXS_LIBEXECDIR=`${APXS} -q LIBEXECDIR 2> /dev/null`
APXS_INCLUDEDIR=`${APXS} -q INCLUDEDIR 2> /dev/null`
APXS_INCLUDES="-I${APXS_INCLUDEDIR}"

# Checks for apr.
AC_ARG_WITH([apr],
            [AC_HELP_STRING([--with-apr=PATH],
                            [apr config path [default=yes]])],
            [APR_CONFIG="$withval"],
            [with_apr=yes])

AC_MSG_CHECKING([whether apr])
AS_IF([test "x$with_apr" != "xno"],
      [if test ! -x "${APR_CONFIG}" -o -z "${APR_CONFIG}"; then
         APR_PATH=`${APXS} -q APR_BINDIR 2> /dev/null`
         APR_CONFIG="${APR_PATH}/apr-1-config"
         if test ! -x "${APR_CONFIG}"; then
           APR_CONFIG="${APR_PATH}/apr-config"
         fi
       fi
       APR_INCLUDES=`${APR_CONFIG} --includes 2> /dev/null`
       APR_CFLAGS=`${APR_CONFIG} --cflags 2> /dev/null`
       APR_CPPFLAGS=`${APR_CONFIG} --cppflags 2> /dev/null`
       APR_LDFLAGS=`${APR_CONFIG} --ldflags 2> /dev/null`
       APR_LIBS=`${APR_CONFIG} --libs 2> /dev/null`
       AC_MSG_RESULT(yes)],
      [AC_MSG_ERROR(apr not found)])

# Apache libraries.
APACHE_MODULEDIR="${APXS_LIBEXECDIR}"
APACHE_INCLUDES="${APXS_INCLUDES} ${APR_INCLUDES}"
APACHE_CFLAGS="${APXS_CFLAGS} ${APR_CFLAGS} ${APACHE_INCLUDES}"
APACHE_CPPFLAGS="${APXS_CPPFLAGS} ${APR_CPPFLAGS} ${APACHE_INCLUDES}"
APACHE_LDFLAGS="${APXS_LDFLAGS} ${APR_LDFLAGS}"
APACHE_LIBS="${APXS_LIBS} ${APR_LIBS}"

AC_SUBST(APACHE_MODULEDIR)
AC_SUBST(APACHE_INCLUDES)
AC_SUBST(APACHE_CFLAGS)
AC_SUBST(APACHE_CPPFLAGS)
AC_SUBST(APACHE_LDFLAGS)
AC_SUBST(APACHE_LIBS)


# Check for librdkafka.
AC_ARG_WITH([rdkafka],
            [AC_HELP_STRING([--with-rdkafka=PATH],
                            [rdkafka include path [default=no]])],
            [RDKAFKA_DIR="$withval"],
            [RDKAFKA_DIR=no])
AS_IF([test "x${RDKAFKA_DIR}" != "xno"],
[
 AC_MSG_NOTICE([use shared librdkafka])
 Check for librdkafka header
 AC_MSG_CHECKING([for rdkafka header])
 AS_IF([test -r "${RDKAFKA_DIR}/rdkafka.h"],
       [LIBRDKAFKA_CFLAGS="-I${RDKAFKA_DIR}"
        LIBRDKAFKA_CPPFLAGS="${LIBRDKAFKA_CFLAGS}"],
       [AC_MSG_ERROR([not found rdkafka.h])])
 AC_MSG_RESULT([${RDKAFKA_DIR}])

 # Check for librdkafka library
 AC_MSG_CHECKING([for rdkafka library])
 librdkafka_save_cflags="${CFLAGS}"
 librdkafka_save_cppflags="${CPPFLAGS}"
 librdkafka_save_ldflags="${LDFLAGS}"
 CFLAGS="${CFLAGS} -I${RDKAFKA_DIR}"
 CPPFLAGS="${CPPFLAGS} -I${RDKAFKA_DIR}"
 LDFLAGS="-lrdkafka -lpthread -lz -lcrypto -lssl -lsasl2 -lrt"

 AC_ARG_WITH([rdkafka-libdir],
             [AC_HELP_STRING([--with-rdkafka-libdir=PATH],
                             [rdkafka library path [default=no]])],
             [RDKAFKA_LIBDIR="$withval"],
             [RDKAFKA_LIBDIR=no])
 AS_IF([test "x${RDKAFKA_LIBDIR}" != "xno"],
       [LDFLAGS="-L${RDKAFKA_LIBDIR} ${LDFLAGS}"])

 AC_TRY_LINK([#include "rdkafka.h"],
             [rd_kafka_conf_t *conf = rd_kafka_conf_new();],
             [LIBRDKAFKA_LDFLAGS="${LDFLAGS}"],
             [AC_MSG_ERROR([not found librdkafka library, please use --with-rdkafka-libdir=PATH])])
 CFLAGS="${librdkafka_save_cflags}"
 CPPFLAGS="${librdkafka_save_cppflags}"
 LDFLAGS="${librdkafka_save_ldflags}"
],
[
 AC_MSG_NOTICE([use embedded librdkafka])
 LIBRDKAFKA_CFLAGS="-I./librdkafka/src"
 LIBRDKAFKA_CPPFLAGS="${LIBRDKAFKA_CFLAGS}"
 LIBRDKAFKA_LDFLAGS=
 LIBRDKAFKA_LIBS=

 # config.h for librdkafka
 AC_CONFIG_HEADERS([librdkafka/config.h:librdkafka.config.h.in])

 # Checks for SSL (with librdkafka)
 AC_MSG_CHECKING([for ssl])
 AC_ARG_ENABLE([ssl],
               [AC_HELP_STRING([--enable-ssl],
                               [use ssl library [default=yes]])],
               [ENABLE_SSL="${enableval}"],
               [ENABLE_SSL=yes])
 AS_IF([test "x${ENABLE_SSL}" != "xno"],
       [ssl_save_ldflags="${LDFLAGS}"
        LDFLAGS="-lssl"
        AC_TRY_LINK([#include <openssl/ssl.h>],
                    [SSL_CTX *ssl = SSL_CTX_new(NULL);],
                    [LIBRDKAFKA_LDFLAGS="-lssl ${LIBRDKAFKA_LDFLAGS}"
                     AC_DEFINE([ENABLE_SSL], [1], [ssl enable])
                     AC_DEFINE([WITH_SSL], [1], [ssl support])],
                    [AC_MSG_ERROR([not found, please use --disable-ssl to disable SSL support])])
       LDFLAGS="${ssl_save_ldflags}"])
 AC_MSG_RESULT([${ENABLE_SSL}])

 # Checks for SASL (with librdkafka)
 AC_MSG_CHECKING([for sasl])
 AC_ARG_ENABLE([sasl],
               [AC_HELP_STRING([--enable-sasl],
                               [use cyrus sasl library [default=yes]])],
               [ENABLE_SASL="${enableval}"],
               [ENABLE_SASL=yes])
 AS_IF([test "x${ENABLE_SASL}" != "xno"],
       [sasl_save_ldflags="${LDFLAGS}"
        LDFLAGS="-lsasl2"
        AC_TRY_LINK([#include <sasl/sasl.h>],
                    [int r = sasl_client_init(0);],
                    [LIBRDKAFKA_LDFLAGS="-lsasl2 ${LIBRDKAFKA_LDFLAGS}"
                     AC_DEFINE([ENABLE_SASL], [1], [sasl enable])
                     AC_DEFINE([WITH_SASL], [1], [sasl support])],
                    [AC_MSG_ERROR([not found, please use --disable-sasl to disable SASL support])])
        LDFLAGS="${sasl_save_ldflags}"])
 AC_MSG_RESULT([${ENABLE_SASL}])

 # Checks for zlib (with librdkafka)
 AC_ARG_ENABLE([zlib],
               [AC_HELP_STRING([--enable-zlib],
                               [use zlib library [default=yes]])],
               [ENABLE_ZLIB="${enableval}"],
               [ENABLE_ZLIB=yes])
 AS_IF([test "x${ENABLE_ZLIB}" != "xno"],
       [AC_CHECK_LIB(z, gzread)
        AS_IF([test "x${ac_cv_lib_z_gzread}" = "xyes"],
              [AC_DEFINE([WITH_ZLIB], [1], [zlib support])],
              [AC_MSG_ERROR(zlib not found)])])

 # Checks for crypto (with librdkafka)
 AC_CHECK_LIB(crypto, BN_init)
 AS_IF([test "x${ac_cv_lib_crypto_BN_init}" = "xyes"],
       [AC_DEFINE([WITH_CRYPTO], [1], [crypto support])])

 # Checks for rt (with librdkafka)
 AC_CHECK_LIB(rt, clock_gettime)

 # Checks for atomic (with librdkafka)
 AC_MSG_CHECKING([for 32bit __atomic])
 have_atomic_32=no
 atomic_32_lib=
 atomic_32_save_ldflags="${LDFLAGS}"
 op_atomic_32="__atomic_ ## OP1 ## _ ## OP2(PTR, VAL, __ATOMIC_SEQ_CST)"
 AC_TRY_LINK([#include <inttypes.h>],
             [int32_t i; __atomic_add_fetch(&i, 1, __ATOMIC_SEQ_CST);],
             [have_atomic_32=yes])
 AS_IF([test "x$have_atomic_32" != "xyes"],
       [LDFLAGS="${LDFLAGS} -latomic"
        AC_TRY_LINK([#include <inttypes.h>],
                    [int32_t i; __atomic_add_fetch(&i, 1, __ATOMIC_SEQ_CST);],
                    [have_atomic_32=yes])
        AS_IF([test "x$have_atomic_32" != "xyes"],
              [LDFLAGS="$atomic_32_save_ldflags"
               AC_TRY_LINK([#include <inttypes.h>],
                           [int32_t i; __sync_add_and_fetch(&i, 1);],
                           [have_atomic_32=yes])
               AS_IF([test "x$have_atomic_32" != "xyes"],
                     [op_atomic_32="__sync_ ## OP1 ## _and_ ## OP2(PTR, VAL)"
                      AC_DEFINE([HAVE_ATOMICS_32_SYNC],
                                [1], [builtin atomic 32 sync interface])])],
              [atomic_32_lib="-latomic"
               AC_DEFINE([HAVE_ATOMICS_32_ATOMIC],
                         [1], [builtin atomic 32 with -latomic])])],
       [AC_DEFINE([HAVE_ATOMICS_32_ATOMIC], [1], [builtin atomic 32])])
 AS_IF([test "x$have_atomic_32" = "xyes"],
       [AC_DEFINE([HAVE_ATOMICS_32], [1], [have atomic 32])
        AC_DEFINE_UNQUOTED([ATOMIC_OP32(OP1,OP2,PTR,VAL)],
                           [$op_atomic_32], [atomic 32 op])])
 AS_IF([test "x$atomic_32_lib" != "x"],
       [LIBRDKAFKA_LDFLAGS="$atomic_32_lib ${LIBRDKAFKA_LDFLAGS}"])
 LDFLAGS="$atomic_32_save_ldflags"
 AC_MSG_RESULT([$have_atomic_32])

 AC_MSG_CHECKING([for 64bit __atomic])
 have_atomic_64=no
 atomic_64_lib=
 atomic_64_save_ldflags="${LDFLAGS}"
 op_atomic_64="__atomic_ ## OP1 ## _ ## OP2(PTR, VAL, __ATOMIC_SEQ_CST)"
 AC_TRY_LINK([#include <inttypes.h>],
             [int64_t i; __atomic_add_fetch(&i, 1, __ATOMIC_SEQ_CST);],
             [have_atomic_64=yes])
 AS_IF([test "x$have_atomic_64" != "xyes"],
       [LDFLAGS="${LDFLAGS} -latomic"
        AC_TRY_LINK([#include <inttypes.h>],
                    [int64_t i; __atomic_add_fetch(&i, 1, __ATOMIC_SEQ_CST);],
                    [have_atomic_64=yes])
        AS_IF([test "x$have_atomic_64" != "xyes"],
              [LDFLAGS="$atomic_64_save_ldflags"
               AC_TRY_LINK([#include <inttypes.h>],
                           [int64_t i; __sync_add_and_fetch(&i, 1);],
                           [have_atomic_64=yes])
               AS_IF([test "x$have_atomic_64" = "xyes"],
                     [op_atomic_64="__sync_ ## OP1 ## _and_ ## OP2(PTR, VAL)"
                      AC_DEFINE([HAVE_ATOMICS_64_SYNC],
                                [1], [builtin atomic 64 sync interface])])],
              [atomic_64_lib="-latomic"
               AC_DEFINE([HAVE_ATOMICS_64_ATOMIC],
                         [1], [builtin atomic 64 with -latomic])])],
       [AC_DEFINE([HAVE_ATOMICS_64_ATOMIC], [1], [builtin atomic 64])])
 AS_IF([test "x$have_atomic_64" = "xyes"],
       [AC_DEFINE([HAVE_ATOMICS_64], [1], [have atomic 64])
        AC_DEFINE_UNQUOTED([ATOMIC_OP64(OP1,OP2,PTR,VAL)],
                           [$op_atomic_64], [atomic 64 op])
        AC_DEFINE_UNQUOTED([ATOMIC_OP(OP1,OP2,PTR,VAL)],
                           [$op_atomic_64], [atomic 64 op])])
 AS_IF([test "x$atomic_64_lib" != "x"],
       [LIBRDKAFKA_LDFLAGS="$atomic_64_lib ${LIBRDKAFKA_LDFLAGS}"])
 LDFLAGS="$atomic_64_save_ldflags"
 AC_MSG_RESULT([$have_atomic_64])

 # Checks for regex (with librdkafka)
 AC_MSG_CHECKING([for regex])
 have_regex=no
 AC_TRY_LINK([#include <regex.h>
              #include <stddef.h>],
             [regcomp(NULL, NULL, 0);
              regexec(NULL, NULL, 0, NULL, 0);
              regerror(0, NULL, NULL, 0);
              regfree(NULL);],
             [have_regex=yes])
 AS_IF([test "x$have_regex" = "xyes"],
       [AC_DEFINE([HAVE_REGEX], [1], [regex use])])
 AC_MSG_RESULT([$have_regex])

 # Checks for pthread (with librdkafka)
 AC_MSG_CHECKING([for pthread])
 have_pthread=no
 pthread_save_ldflags="${LDFLAGS}"
 LDFLAGS="-lpthread"
 AC_TRY_LINK([#include <pthread.h>],
             [pthread_t th; pthread_create(&th, NULL, NULL, NULL);],
             [have_pthread=yes])
 AS_IF([test "x$have_pthread" = "xyes"],
       [LIBRDKAFKA_LDFLAGS="-lpthread ${LIBRDKAFKA_LDFLAGS}"],
       [AC_MSG_ERROR([required pthread library])])
 AC_MSG_RESULT([$have_pthread])
 LDFLAGS="${pthread_save_ldflags}"

 # Checks for strndup (with librdkafka)
 AC_CHECK_FUNCS([strndup])
])

AM_CONDITIONAL(HAVE_LIBRDKAFKA, [test "x${RDKAFKA_DIR}" = "xno"])
AM_CONDITIONAL(HAVE_ZLIB, [test "x${ENABLE_ZLIB}" = "xyes"])
AM_CONDITIONAL(HAVE_SASL, [test "x${ENABLE_SASL}" = "xyes"])
AM_CONDITIONAL(HAVE_SSL, [test "x${ENABLE_SSL}" = "xyes"])

AC_SUBST(LIBRDKAFKA_CFLAGS)
AC_SUBST(LIBRDKAFKA_CPPFLAGS)
AC_SUBST(LIBRDKAFKA_LDFLAGS)
AC_SUBST(LIBRDKAFKA_LIBS)


AC_CONFIG_FILES([
  Makefile
])
AC_OUTPUT

##########################################################################################
# Auto configure initialization
AC_INIT(cache, cache-0.0)
AC_CONFIG_MACRO_DIR([m4])
AM_INIT_AUTOMAKE([foreign])
AC_CONFIG_HEADERS([config.h])

# LibTool configure
LT_INIT([shared static])

# Identify host platform
AC_CANONICAL_HOST

##########################################################################################
# Tests

# Check for pthreads
AX_PTHREAD([have_pthreads="yes"],[have_pthreads="no"])
LIBS="$PTHREAD_LIBS -L/usr/local/lib $LIBS"
CFLAGS="$CFLAGS $PTHREAD_CFLAGS -I/usr/local/include"
CC="$PTHREAD_CC"

# Check for the math library
AC_SEARCH_LIBS([floor],[m],[have_m="yes"],[have_m="no"])

# Check for the srpc library
AC_CHECK_HEADERS([srpc/srpc.h])
AC_SEARCH_LIBS([rpc_init],[srpc],[have_srpc="yes"],[have_srpc="no"])

# Check for the ADTs library
AC_SEARCH_LIBS([it_hasNext],[ADTs],[have_adts="yes"],[have_adts="no"])


##########################################################################################
# End of configure summary output
DE_STAT("pthreads found", $have_pthreads)
DE_STAT("libmath found", $have_m)
DE_STAT("libsrpc found", $have_srpc)
DE_STAT("libADTs found", $have_adts)

##########################################################################################
# Generate files
AC_CONFIG_FILES([Makefile src/Makefile])
AC_OUTPUT

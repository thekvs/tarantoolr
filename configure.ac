# require at least autoconf 2.61
AC_PREREQ(2.61)

# Process this file with autoconf to produce a configure script.
AC_INIT([tarantoolr], [0.1.0])

# We are using C++
AC_LANG(C++)
AC_REQUIRE_CPP
AC_PROG_CXX

## Set R_HOME, respecting an environment variable if set
: ${R_HOME=$(R RHOME)}
if test -z "${R_HOME}"; then
    AC_MSG_ERROR([Could not determine R_HOME.])
fi

AC_CHECK_PROG(CMAKE_CHECK, cmake, yes)
if test x"$CMAKE_CHECK" != x"yes" ; then
    AC_MSG_ERROR([Could not find cmake.])
fi

## Get R compilers and flags
TARANTOOL_CC=$("${R_HOME}/bin/R" CMD config CC)
TARANTOOL_CFLAGS=$("${R_HOME}/bin/R" CMD config CFLAGS)
TARANTOOL_CPP=$("${R_HOME}/bin/R" CMD config CPP)
TARANTOOL_CPPFLAGS=$("${R_HOME}/bin/R" CMD config CPPFLAGS)
TARANTOOL_CXX=$("${R_HOME}/bin/R" CMD config CXX)
TARANTOOL_CXXFLAGS=$("${R_HOME}/bin/R" CMD config CXXFLAGS)
TARANTOOL_CXXCPP=$("${R_HOME}/bin/R" CMD config CXXCPP)

AC_MSG_NOTICE([Starting to build tarantool-c library])

$(cd src/third_party/; \
	mkdir -p build/libtarantool; \
	cd build/libtarantool; \
	cmake ../../tarantool-c/ -DCMAKE_BUILD_TYPE=RelWithDebInfo -DCMAKE_INSTALL_PREFIX="$(pwd)/../../install" -DCMAKE_C_FLAGS="${TARANTOOL_CFLAGS}" -DCMAKE_CXX_FLAGS="${TARANTOOL_CXXFLAGS}"; \
	make > /dev/null 2>&1; \
	make install > /dev/null 2>&1;)

AC_MSG_NOTICE([Finished building tarantool-c library])

TARANTOOL_LIBS="$(pwd)/src/third_party/install/lib/libtarantool.a"
TARANTOOL_INCL="-I$(pwd)/src/third_party/tarantool-c/third_party/msgpuck"

tarantool_cflags="${TARANTOOL_INCL}"
tarantool_libs="${TARANTOOL_LIBS}"

AC_MSG_NOTICE([Starting to build msgpack library])

$(cd src/third_party/; \
	mkdir -p build/msgpack; \
	cd build/msgpack; \
	cmake ../../msgpack-1.4.1/ -DCMAKE_BUILD_TYPE=RelWithDebInfo -DCMAKE_INSTALL_PREFIX="$(pwd)/../../install" -DMSGPACK_CXX11=ON; \
	make > /dev/null 2>&1; \
	make install > /dev/null 2>&1;)

AC_MSG_NOTICE([Finished building msgpack library])

COMMON_INCL="-I$(pwd)/src/third_party/install/include"

AC_SUBST([PKG_CFLAGS],["${PKG_CFLAGS} ${COMMON_INCL} $tarantool_cflags"])
AC_SUBST([PKG_CXXFLAGS],["${PKG_CXXFLAGS} -W -Wextra ${COMMON_INCL} $tarantool_cflags"])
AC_SUBST([PKG_LIBS],["${PKG_LIBS} $tarantool_libs"])
AC_CONFIG_FILES([src/Makevars])
AC_OUTPUT

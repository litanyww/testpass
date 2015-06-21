bin_PROGRAMS =
noinst_LIBRARIES =
check_PROGRAMS =
lib_LIBRARIES =
TESTS =

include src/Makefile.am

gtest-1.7.0/src/gtest-all.cc : gtest-1.7.0.zip
	unzip -q gtest-1.7.0.zip
	touch $@

gtest-1.7.0.zip :
	curl -O https://googletest.googlecode.com/files/gtest-1.7.0.zip

libsteps_a_SOURCES = src/Steps.cpp \
                     src/TestStep.cpp \
                     src/utils.cpp

libsteps_a_CPPFLAGS = -Isrc

testpass_SOURCES = src/main.cpp
testpass_LDADD = libsteps.a
testpass_CPPFLAGS = -Isrc

test_SOURCES = src/test/TestAttributes.cpp \
               src/test/TestMain.cpp \
               src/test/TestOperations.cpp \
               src/test/TestStep.cpp \
               src/test/TestStepList.cpp \
               gtest-1.7.0/src/gtest-all.cc

test_LDADD = libsteps.a
test_CPPFLAGS = -Isrc -Igtest-1.7.0 -Igtest-1.7.0/include

TESTS += test
lib_LIBRARIES += libsteps.a
bin_PROGRAMS += testpass
check_PROGRAMS += test

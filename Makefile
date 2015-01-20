#! /usr/bin/env make -f

# Copyright 2015 Sophos Limited. All rights reserved.
#
# Sophos is a registered trademark of Sophos Limited and Sophos Group.
#

OBJ_DIR = objs
STEPS_SRCS = $(wildcard src/*.cpp)
STEPS_OBJS = $(addprefix $(OBJ_DIR)/,$(STEPS_SRCS:%.cpp=%.o))                             
STEPS_DEPS = $(STEPS_OBJS:%.o=%.d)
TEST_SRCS = $(wildcard src/test/*.cpp)
TEST_OBJS = $(addprefix $(OBJ_DIR)/,$(TEST_SRCS:%.cpp=%.o))
TEST_DEPS = $(TEST_OBJS:%.o=%.d)

INCLUDE_DIRS = src gtest-1.7.0/include

CXX = g++
CXXFLAGS = -g $(CFLAGS)
INCLUDES =  $(addprefix -I,$(INCLUDE_DIRS))

all : tests

$(OBJ_DIR)/gtest-all.o : gtest-1.7.0/src/gtest-all.cc
	@mkdir -p $(dir $@)
	$(CXX) $(CXXFLAGS) -Igtest-1.7.0 -c -o $@ $<

$(OBJ_DIR)/%.o : %.cpp
	@mkdir -p $(dir $@)
	$(CXX) $(INCLUDES) $(CXXFLAGS) -c -o $@ $<

gtest-1.7.0/src/gtest-all.cc : gtest-1.7.0.zip
	unzip -q gtest-1.7.0.zip
	touch $@

$(OBJ_DIR)/%.d : %.cpp
	@mkdir -p $(dir $@)
	@set -e; rm -f $@; \
		$(CXX) -M $(INCLUDES) $(CXXFLAGS) $< > $@.$$$$; \
		sed 's,\($(notdir $*)\)\.o[ :]*,$(dir $@)$(notdir $*).o $@ : ,g' < $@.$$$$ > $@; \
		rm -f $@.$$$$

-include $(TEST_DEPS) $(STEPS_DEPS)

gtest-1.7.0.zip :
	curl -O https://googletest.googlecode.com/files/gtest-1.7.0.zip

tests : $(STEPS_OBJS) $(TEST_OBJS) $(OBJ_DIR)/gtest-all.o
	$(CXX) $(CXXFLAGS) $(LXXFLAGS) -o $@ $^ -lpthread

clean :
	rm -rf tests objs

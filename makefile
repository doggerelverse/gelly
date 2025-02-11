#=================================================================================================
#  GE makefile
#
# hand crafted
# lovingly composed
# by Lorin Beer
#=================================================================================================
include makevar
#=================================================================================================
all: test objs/pysdlutil.so
#=================================================================================================
test : main.o objs/graphics.o objs/actor.so
	g++ $(INCLUDE) $(ALL_CFLAGS) $(ALL_LIBS) -L$(OBJS_DIR) \
        -L/home/lorin/projects/gelly/libs \
	objs/main.o objs/graphics.o objs/actor.so -o test
#=================================================================================================
main.o : src/main.cpp src/berkelium_util.hpp
	g++ -c $(INCLUDE) $(ALL_CFLAGS) $(ALL_LIBS) \
        /home/lorin/projects/gelly/libs/liblibberkelium.so \
	src/main.cpp -o objs/main.o
#=================================================================================================
objs/graphics.o: src/graphics/graphics.h src/graphics/graphics.cpp
	cd src/graphics; make
#=================================================================================================
objs/actor.so: src/actor/actor.h src/actor/actor.cpp src/actor/actor_wrap.cpp
	cd src/actor; make
#=================================================================================================
objs/pysdlutil.so: 
	cd src/util; make
#=================================================================================================
clean:
	-rm objs/*.o objs/*.so
#=================================================================================================
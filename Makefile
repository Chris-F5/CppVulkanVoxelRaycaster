OUTPUTNAME = RayCaster
CC = gcc
CFLAGS = -std=c++17 -O2
LDFLAGS = -lstdc++ -lglfw -lvulkan -ldl -lpthread -lX11 -lXxf86vm -lXrandr -lXi -lm
SRCS = $(shell find ./src -type f -name "*.cpp")
OBJS = $(patsubst ./src/%.cpp, obj/%.o, $(SRCS))

.PHONY: run clean all

all: target/$(OUTPUTNAME) target/shader.spv

target/$(OUTPUTNAME): $(OBJS)
	mkdir -p target
	$(CC) $(CFLAGS) $^ -o $@ $(LDFLAGS)

obj/%.o: src/%.cpp
	mkdir -p $(dir $@)
	$(CC) $(CFLAGS) -c $< -o $@ $(LDFLAGS)

target/%.spv: src/%.comp
	glslc $< -o $@

run: all
	cd target; ./$(OUTPUTNAME)
clean:
	rm -fr target obj

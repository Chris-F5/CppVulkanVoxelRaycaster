OUTPUTNAME = RayCaster
CC = gcc
CFLAGS = -std=c++17 -O2 -g
LDFLAGS = -lstdc++ -lglfw -lvulkan -ldl -lpthread -lX11 -lXxf86vm -lXrandr -lXi -lm
SRCS = $(shell find ./src -type f -name "*.cpp")
HEADERS = $(shell find ./src -type f -name "*.hpp")
OBJS = $(patsubst ./src/%.cpp, obj/%.o, $(SRCS))
DEPENDS = $(patsubst ./src/%.cpp, obj/%.d,$(SRCS))

-include $(DEPENDS)

.PHONY: run clean all

all: target/$(OUTPUTNAME) target/shader.spv target/scene.ply

target/$(OUTPUTNAME): $(OBJS) $(HEADERS)
	mkdir -p target
	$(CC) $(CFLAGS) $(OBJS) -o $@ $(LDFLAGS)

obj/%.o: src/%.cpp
	mkdir -p $(dir $@)
	$(CC) $(CFLAGS) -MMD -MP -c $< -o $@ $(LDFLAGS)

target/%.spv: src/%.comp
	glslc $< -o $@

target/scene.ply: scene.ply
	cp scene.ply target/scene.ply

run: all
	cd target; ./$(OUTPUTNAME)
clean:
	rm -fr target obj

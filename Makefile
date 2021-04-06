CFLAGS = -std=c++17 -O2
LDFLAGS = -lglfw -lvulkan -ldl -lpthread -lX11 -lXxf86vm -lXrandr -lXi
.PHONY: run clean all

all: target/RayCaster target/shader.spv

target/RayCaster: src/main.cpp
	mkdir -p target
	g++ $(CFLAGS) -o target/RayCaster src/main.cpp $(LDFLAGS)

target/shader.spv: src/shader.comp
	glslc src/shader.comp -o target/shader.spv

run: all
	cd target; ./RayCaster
clean:
	rm -fr target

CFLAGS = -std=c++17 -O2
LDFLAGS = -lglfw -lvulkan -ldl -lpthread -lX11 -lXxf86vm -lXrandr -lXi

.PHONY: run clean all

all: target/RayCaster target/shader.spv

target/RayCaster: obj/main.o obj/vulkan_device.o obj/swapchain.o obj/render_pipeline.o obj/input.o
	mkdir -p target
	g++ $(CFLAGS) obj/main.o obj/vulkan_device.o obj/swapchain.o obj/render_pipeline.o obj/input.o -o target/RayCaster $(LDFLAGS)

obj/main.o: src/main.cpp
	mkdir -p obj
	g++ $(CFLAGS) -c src/main.cpp -o obj/main.o $(LDFLAGS)

obj/vulkan_device.o: src/vulkan_device.cpp src/vulkan_device.hpp
	mkdir -p obj
	g++ $(CFLAGS) -c src/vulkan_device.cpp -o obj/vulkan_device.o $(LDFLAGS)

obj/swapchain.o: src/swapchain.cpp src/swapchain.hpp
	mkdir -p obj
	g++ $(CFLAGS) -c src/swapchain.cpp -o obj/swapchain.o $(LDFLAGS)

obj/render_pipeline.o: src/render_pipeline.cpp src/render_pipeline.hpp
	mkdir -p obj
	g++ $(CFLAGS) -c src/render_pipeline.cpp -o obj/render_pipeline.o $(LDFLAGS)

obj/input.o: src/input.cpp src/input.hpp
	mkdir -p obj
	g++ $(CFLAGS) -c src/input.cpp -o obj/input.o $(LDFLAGS)

target/shader.spv: src/shader.comp
	glslc src/shader.comp -o target/shader.spv

run: all
	cd target; ./RayCaster
clean:
	rm -fr target obj

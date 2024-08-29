
TARGET = sandy

all: $(TARGET)

UNAME_S := $(shell uname -s)

ifeq ($(UNAME_S),Darwin)
	INCLUDE = -I/opt/homebrew/Cellar/glfw/3.4/include
	LIBS = -L/opt/homebrew/Cellar/glfw/3.4/lib -lglfw -framework OpenGL
else
	LIBS = -lglfw -lGL -lwayland-client -lwayland-egl -lwayland-cursor -lpthread -ldl 
endif

$(TARGET):
	g++ -c src/glad.c -Iinclude
	g++ -c src/main.cpp -Iinclude
	g++ glad.o main.o -o $(TARGET) -Iinclude $(INCLUDE) $(LIBS)
	

clean:
	rm -f glad.o main.o $(TARGET)

.PHONY: all clean

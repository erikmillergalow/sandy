
TARGET = sandy

all: $(TARGET)

$(TARGET):
	g++ -c src/glad.c -Iinclude
	g++ -c src/main.cpp -Iinclude
	g++ glad.o main.o -o $(TARGET) -Iinclude -lglfw -lGL -lwayland-client \
		-lwayland-egl -lwayland-cursor -lpthread -ldl
	

clean:
	rm -f glad.o main.o $(TARGET)

.PHONY: all clean

all: compile link

compile:
	g++ -m64 -Isrc/include -c code/*.cpp

link:
	g++ -m64 *.o -o main -Lsrc/lib -lsfml-graphics -lsfml-window -lsfml-system -lopengl32 -lsfml-audio

clean:
	del *.o main.exe

#$ autoreconf -i
#$ ./configure --prefix=/home/nlihogrud/specsem_2014/filtering/build --enable-jp2=no --enable-static=yes --enable-shared=no --enable-ILUT
#$ make
#$ make install

all: filtering

IL_INCLUDE_PATH=include
IL_LIB_PATH=lib
IL_LIBS=-lIL -lILUT -lILU
IL_DEPS=-ltiff -lpng12 -llcms -ljpeg -lIlmImf -lpthread -lImath -lHalf -lIex -lIlmThread

main.o: main.cpp
	g++ -I$(IL_INCLUDE_PATH) main.cpp -c -o main.o

filtering: main.o
	g++ -L$(IL_LIB_PATH) -o filtering main.o $(IL_LIBS) $(IL_DEPS)

clean:
	rm -f main.o filter
	
	

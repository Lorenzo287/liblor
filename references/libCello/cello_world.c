#include "include/Cello.h"

/*
if lib installed in msys64/ucrt64/lib/libCello.a
                    msys64/ucrt64/include/Cello.h
	gcc -std=gnu99 cello_world.c -lCello -o cello_world

if lib installed in dev/c_libs/lib/libCello.a
                    dev/c_libs/include/Cello.h
	gcc -std=gnu99 cello_world.c -Lc:/dev/c_libs/lib -lCello -o cello_world

if lib in cwd link directly
	STATIC: gcc -std=gnu99 cello_world.c ./libCello.a -o cello_world
	DYNAMIC: gcc -std=gnu99 cello_world.c ./libCello.so -o cello_world
or include cwd to path
	STATIC: gcc -std=gnu99 cello_world.c -L. -lCello -o cello_world
	DYNAMIC: gcc -std=gnu99 cello_world.c -L. -l:libCello.so -o cello_world

can check linking with 'ldd cello_world.exe' 
or look at the size of the exe (550K static, 130K dynamic)
*/

int main(int argc, char **argv) {
    println("Cello, World!");
    return 0;
}

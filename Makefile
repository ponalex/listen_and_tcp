# gcc -o main main.c libportaudio.a -lasound -lm
# gcc -ggdb -o test_thread test_thread.c src/my_lib.c -pthread -fsanitize=thread 
OPTS = -pthread -lasound -lm
OPT_FILES = libportaudio.a 

test: main.c src/pa_wrapper.c src/my_lib.c
	gcc -ggdb -o main $^ libportaudio.a -lasound -lm


listen: listener.c src/my_lib.c src/pa_wrapper.c 
	gcc -o listen $^ $(OPT_FILES) $(OPTS)


debug: listener.c src/my_lib.c src/pa_wrapper.c 
	gcc -ggdb -o listen $^ $(OPT_FILES) $(OPTS) -Wall -pedantic

#-fsanitize=leak

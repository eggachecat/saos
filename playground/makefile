GPPPARAMS = -m32 -fno-use-cxa-atexit -nostdlib -fno-builtin -fno-rtti -fno-exceptions -fno-leading-underscore
objects = my_little_object_file.o

%.o: %.cpp
	gcc $(GPPPARAMS) -o $@ -c $<


all: $(objects)
	echo "hello"
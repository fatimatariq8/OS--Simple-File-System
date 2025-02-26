filesytem: filesystem.c


compile:
	gcc -Wall -o filesystem filesystem.c

build:
	compile

clean:
	rm -f filesystem


run:
	./filesystem sampleinput.txt


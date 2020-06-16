home_monitor : home_monitor.o dht11.o
	gcc -o home_monitor home_monitor.o dht11.o -lpthread -l pigpio 

home_monitor.o : home_monitor.c dht11.h
	gcc -c home_monitor.c

dht11.o : dht11.c dht11.h
	gcc -c dht11.c

clean:
	rm -f home_monitor home_monitor.o dht11.o
	
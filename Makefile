CC = gcc
CFLAGS = -Wall -Wextra

build:
	$(CC) $(CFLAGS) Queue.c process_generator.c -o process_generator.out
	$(CC) $(CFLAGS) clk.c -o clk.out
	$(CC) $(CFLAGS) Queue.c scheduler.c -o scheduler.out -lm
	$(CC) $(CFLAGS) process.c -o process.out
	$(CC) $(CFLAGS) test_generator.c -o test_generator.out

# clean must NOT delete processes.txt (a committed input fixture)
clean:
	rm -f *.out *.o SchedulerLog*.txt SchedulerCalc*.txt Output.txt

all: clean build

run:
	./process_generator.out

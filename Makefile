CC = gcc
CFLAGS = -Wall -Wextra -Iinclude

# Sources live in src/, headers in include/ (resolved via -Iinclude).
# Binaries are intentionally emitted to the repo ROOT: at runtime the programs
# exec ./clk.out, ./scheduler.out and ./process.out from here.
build:
	$(CC) $(CFLAGS) src/Queue.c src/process_generator.c -o process_generator.out
	$(CC) $(CFLAGS) src/clk.c -o clk.out
	$(CC) $(CFLAGS) src/Queue.c src/scheduler.c -o scheduler.out -lm
	$(CC) $(CFLAGS) src/process.c -o process.out
	$(CC) $(CFLAGS) src/test_generator.c -o test_generator.out

# clean must NOT delete processes.txt (a committed input fixture)
clean:
	rm -f *.out *.o SchedulerLog*.txt SchedulerCalc*.txt Output.txt

all: clean build

run:
	./process_generator.out

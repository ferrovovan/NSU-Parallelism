FLAGS_DF = -std=c++17 -Wall -fopenmp -lm -O3


# Общая цель
all: task2.3-for task2.3-manual task2.3-schedule

# --- Общие объектные файлы ---
main.o: task2.3_main.cpp config.hpp method.hpp
	g++ $(CXXFLAGS) $(DEFINES) -c $< -o $@

# --- Компиляция вариантов метода ---
task2.3_for.o: method_for.cpp config.hpp method.hpp
	g++ $(FLAGS_DF)  -c $< -o $@

task2.3_manual.o: method_manual.cpp config.hpp method.hpp
	g++ $(FLAGS_DF)  -c $< -o $@

task2.3_schedule.o: method_schedule.cpp config.hpp method.hpp
	g++ $(FLAGS_DF)  -c $< -o $@

# --- Сборка исполняемых файлов ---
task2.3-for: main.o task2.3_for.o
	g++ $(FLAGS_DF) $^ -o $@ $(LDFLAGS)

task2.3-manual: main.o task2.3_manual.o
	g++ $(FLAGS_DF) $^ -o $@ $(LDFLAGS)

task2.3-schedule: main.o task2.3_schedule.o
	g++ $(FLAGS_DF) $^ -o $@ $(LDFLAGS)

# --- "Утилиты" ---
test: all
	./task2.3-for
	./task2.3-manual
	./task2.3-schedule

clean:
	rm -f *.o task2.3-for task2.3-manual task2.3-schedule

.PHONY: all clean

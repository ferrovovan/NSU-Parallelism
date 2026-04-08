FLAGS_DF = -std=c++17 -Wall -fopenmp

task2.3-for:      task2.3_for.cpp
	g++ $(FLAGS_DF) $< -o $@ -lm

task2.3-general:  task2.3_general.cpp
	g++ $(FLAGS_DF) $< -o $@ -lm

task2.3-schedule: task2.3_schedule.cpp
	g++ $(FLAGS_DF) $< -o $@ -lm

all: task2.3-for task2.3-general task2.3-schedule

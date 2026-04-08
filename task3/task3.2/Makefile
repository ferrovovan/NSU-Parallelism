FLAGS_DF = -std=c++17 -pthread -Wall

task3.2: task3.2.cpp
	g++ $(FLAGS_DF) -o $@ $< -lm

check: check.cpp
	g++ $(FLAGS_DF) -o $@ $< -lm
	./check

CXX = nvcc
stdFLAG = -std=c++17

# Универсальная цель: Полная программа
task8: task8_main
	@true

task8_main:   task8_main.cpp   jacobi_solver.o
	$(CXX) $(stdFLAG) -lboost_program_options  -o $@ $^

task8_simple: task8_simple.cpp jacobi_solver.o
	$(CXX) $(stdFLAG) -o $@ $^

jacobi_solver.o: jacobi_solver.cu
	$(CXX) $(stdFLAG) -lm -c $< -o $@

clean: clean_profiles
	rm -f task8_main task8_simple jacobi_solver.o

gpu_profile: task8_main task8_simple
	@echo "\n=== SIMPLE CUDA Profile ==="
	nsys profile --stats=true -o task8_simple_profile ./task8_simple --size 128  --max_iter 10000
	@echo "\n=== FULL CUDA Profile ==="
	nsys profile --stats=true -o task8_full_profile   ./task8_main   --grid-sizes 90 -g 15 -m 10000

clean_profiles:
	@rm -f task8_simple_profile.sqlite task8_simple_profile.nsys-rep task8_full_profile.sqlite task8_full_profile.nsys-rep

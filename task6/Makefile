CXX = pgc++
CXXFLAGS = -std=c++17 -Wall -lboost_program_options -lm -Minfo=all

# Универсальная цель (по умолчанию – GPU)
task6: task6_gpu
	@true

# Цель для одноядерного CPU
task6_host: task6.cpp
	$(CXX) $(CXXFLAGS) -acc=host -o $@ $<

# Цель для многоядерного CPU (OpenACC multicore)
task6_multicore: task6.cpp
	$(CXX) $(CXXFLAGS) -acc=multicore -o $@ $<

# Цель для GPU
task6_gpu: task6.cpp
	$(CXX) $(CXXFLAGS) -acc=gpu -o $@ $<

clean:
	rm -f task6_host task6_multicore task6_gpu

# Замеры времени
run_tests: task6_host task6_multicore task6_gpu
	@echo "=== CPU (host) ==="
	./task6_host --grid-sizes 128 256
	@echo "=== CPU (multicore) ==="
	./task6_multicore --grid-sizes 128 256 512
	@echo "=== GPU ==="
	./task6_gpu --grid-sizes 128 256 512 1024

gpu_profile: task6_gpu
	@echo "=== GPU Profile ==="
	nsys profile --stats=true -o task6_profile ./task6_gpu --grid-sizes 128 -m 1000

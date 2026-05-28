CXX = pgc++
CXXFLAGS = -std=c++17 -Wall -lboost_program_options -lm -acc=gpu  -lcublas -cudalib=cublas

# Универсальная цель (по умолчанию – cuBLAS)
task7: task7_cublas
	@true

# Обычный OpenACC
task7_acc: task7.cpp
	$(CXX) $(CXXFLAGS) -o $@ $<

# OpenACC + cuBLAS
task7_cublas: task7.cpp
	$(CXX) $(CXXFLAGS) -DCUBLAS -o $@ $<

task7_simple: task7_simple.cpp
	$(CXX) $(CXXFLAGS) -o $@ $<

clean:
	rm -f task7_acc task7_cublas task7_simple

gpu_profile: task7_acc task7_cublas task7_simple
	@echo "\n=== SIMPLE CUDA Profile ==="
	nsys profile --stats=true -o task7_profile ./task7_simple --size 128  --max_iter 10000
	@echo "\n=== GPU ACC  Profile ==="
	nsys profile --stats=true -o task7_profile ./task7_acc    --grid-sizes 90 -g 15 -m 10000
	@echo "\n=== GPU CUDA Profile ==="
	nsys profile --stats=true -o task7_profile ./task7_cublas --grid-sizes 90 -g 15 -m 10000

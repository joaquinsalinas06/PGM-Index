CXX = c++
CXXFLAGS = -std=c++17 -O2 -Wall -Wextra
TESTS = $(patsubst tests/%.cpp,build/%,$(wildcard tests/test_*.cpp))
HEADERS = $(wildcard pgm/*.h bplus/*.h)

.PHONY: all test clean
all: test

test: $(TESTS) | build
	@printf 'int main() {}\n' | $(CXX) $(CXXFLAGS) -x c++ - -o build/compiler_check
	@set -e; for test in $(TESTS); do ./$$test; done

build/test_%: tests/test_%.cpp $(HEADERS) | build
	$(CXX) $(CXXFLAGS) $< -o $@

build:
	mkdir -p build

clean:
	rm -rf build

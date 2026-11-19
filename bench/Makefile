DATA = data
SETS = seq uniform normal lognormal exponential zipf books fb osm wiki friendster patho_loggap patho_pareto patho_ruler
DYN = uniform books fb osm patho_ruler
FILES = $(addprefix $(DATA)/,$(addsuffix .bin,$(SETS)))
DYNFILES = $(addprefix $(DATA)/,$(addsuffix .bin,$(DYN)))

bench: bench.cpp
	c++ -std=c++17 -O2 bench.cpp -o bench

results: bench
	./bench segments $(FILES) > results/segments.csv
	./bench lookup $(FILES) > results/lookup.csv
	./bench sizes $(FILES) > results/sizes.csv
	./bench dynamic $(DYNFILES) > results/dynamic.csv

figuras:
	python3 plot.py

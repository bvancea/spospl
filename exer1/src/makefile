E1 = random_generator
E2 = false_sharing
E3 = numa_allocation

all:
	@echo "Compiling modules for Exercise 1"
	@cd $(E1); make;
	@cd $(E2); make;
	@cd $(E3); make;
	@echo "Compilation done."

clean:
	@echo "Cleaning modules for Exercise 1"
	@cd $(E1); make clean;
	@cd $(E2); make clean;
	@cd $(E3); make clean;
	@echo "Cleaning done."
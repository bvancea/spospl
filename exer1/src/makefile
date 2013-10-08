E1 = random_generator
E2 = false_sharing
E3 = numa_allocation

all:
	cd $(E1); make;
	cd $(E2); make;
	cd $(E3); make;

clean:
	cd $(E1); make clean;
	cd $(E2); make clean;
	cd $(E3); make clean;
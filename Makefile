.PHONY: all clean

all:
	@echo "Building Algorithm..."
	$(MAKE) -C Algorithm
	@echo "Building GameManager..."
	$(MAKE) -C GameManager
	@echo "Building Simulator..."
	$(MAKE) -C Simulator


clean:
	$(MAKE) -C Algorithm clean
	$(MAKE) -C GameManager clean
	$(MAKE) -C Simulator clean

BUILD_DIR = bin
ASSEMBLY  = testbed

all: build-all run

build-all: build-engine build-exec

build-engine:
	$(MAKE) -f Makefile.engine build

build-exec:
	$(MAKE) -f Makefile.executable build
run: 
	$(BUILD_DIR)/$(ASSEMBLY)	
clean:
	$(MAKE) -f Makefile.engine clean
	$(MAKE) -f Makefile.executable clean

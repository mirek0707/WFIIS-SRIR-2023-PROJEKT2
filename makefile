# This Makefile is used to build and run a UPC program
# UPC (Unified Parallel C) is an extension of the C programming language 
# designed for high-performance computing on large-scale parallel machines.

# Shell used by this Makefile
SHELL := /bin/bash

# The DIM variable specifies the dimensions for the matrix. 
# The NODES variable calculates the total number of nodes, which is the square of DIM.
DIM := 16
NODES := $(shell echo "$$(( $(DIM) * $(DIM) ))")

# F1, F2, and FR are filenames for input matrix files and the output result matrix file, respectively.
F1 := A.txt
F2 := C.txt
FR := result.txt

# CLEAN is a list of files to be removed when cleaning the build.
CLEAN := main nodes result.txt

# The default target of this Makefile is 'build run'
default: build run

# 'v' target compiles and runs the UPC program with verbose logging
v: build run_verbose

# 'build' target compiles the main.c UPC source file using the Berkeley UPC compiler
build: main.c
# Source the necessary configuration scripts for Berkeley UPC and the GCC UPC compiler
# Call the Berkeley UPC compiler to compile the UPC source file with network set to UDP and Pthreads equal to the matrix dimension.
	source /opt/nfs/config/source_bupc.sh && source /opt/nfs/config/source_gupc.sh && \
	/opt/nfs/berkeley_upc-2021.4.0/bin/upcc -gupc -Wc,"-fPIE" -network=udp -pthreads=$(DIM) ./main.c -o main

# 'nodes' target generates a file named 'nodes' that lists the node names
nodes:
	/opt/nfs/config/station206_name_list.sh 1 16 > nodes

# 'run_verbose' target runs the compiled UPC program with verbose logging
run_verbose:
	source /opt/nfs/config/source_bupc.sh && source /opt/nfs/config/source_gupc.sh && \
	UPC_NODEFILE=nodes upcrun -c $(DIM) -N $(DIM) -n $(NODES) ./main $(F1) $(F2) $(FR) -v

# 'run' target runs the compiled UPC program without verbose logging
run:
	source /opt/nfs/config/source_bupc.sh && source /opt/nfs/config/source_gupc.sh && \
	UPC_NODEFILE=nodes upcrun -c $(DIM) -N $(DIM) -n $(NODES) ./main $(F1) $(F2) $(FR)

# 'clean' target removes the compiled UPC binary, the 'nodes' file, and the output matrix file
clean:
	rm -rf *.o $(CLEAN)
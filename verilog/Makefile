# Variables
IVERILOG = iverilog
VVP = vvp
SOURCE = src/md5.v
TESTBENCH = src/md5_tb.v
OUTPUT = md5.out

# Default target
all: $(OUTPUT)

$(OUTPUT): $(TESTBENCH) $(SOURCE)
	$(IVERILOG) -o $(OUTPUT) $(TESTBENCH) $(SOURCE)
	$(VVP) $(OUTPUT)

# Clean up
clean:
	rm -f $(OUTPUT) *.vcd

.PHONY: all clean

# Variables
IVERILOG = iverilog
IVERILOG_FLAGS=-Wall -g2012
VVP = vvp
SOURCE = src/md5.v
TESTBENCH = src/md5_tb.v
OUTPUT = md5.out

# Default target
all: $(OUTPUT)

$(OUTPUT): $(TESTBENCH) $(SOURCE)
	$(IVERILOG) $(IVERILOG_FLAGS) -o $(OUTPUT) $(TESTBENCH) $(SOURCE)
	$(VVP) $(OUTPUT)

# Clean up
clean:
	rm -f $(OUTPUT) *.vcd

.PHONY: all clean

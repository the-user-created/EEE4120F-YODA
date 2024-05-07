`timescale 1ns / 1ps

module tb_md5;
    // Inputs
    reg clk;
    reg reset;
    reg [511:0] message;
    reg start;

    // Outputs
    wire [127:0] digest;
    wire ready;

    // Instantiate the Unit Under Test (UUT)
    md5 uut (
        .clk(clk),
        .reset(reset),
        .message(message),
        .start(start),
        .digest(digest),
        .ready(ready)
    );

    // Clock generation
    initial begin
        clk = 0;
        forever #5 clk = !clk;  // Clock with a period of 10 ns
    end

    // Test vectors and checker
    initial begin
        // Initialize Inputs
        reset = 1;
        start = 0;
        message = 0;

        // Wait for global reset to finish
        #100;
        reset = 0;

        // Stimulate the message input
        // Example message: "The quick brown fox jumps over the lazy dog"
        message = 512'h54686520717569636b2062726f776e20666f78206a756d7073206f76657220746865206c617a7920646f67800000000000000000000000005801000000000000;  // message

        // Start the computation
        #10;
        start = 1;
        #10;
        start = 0;

        // Wait for the MD5 computation to finish
        wait (ready == 1);
        #10;

        // Check the output digest
        $display("Digest: %h", digest);
        // The expected digest for the given message (you need to calculate this in advance)
        if (digest == 128'h9e107d9d372bb6826bd81d3542a419d6) begin
            $display("Test Passed. Digest matches expected value.");
        end else begin
            $display("Test Failed. Digest does not match expected value.");
        end

        // End simulation
        #100;
        $finish;
    end

endmodule

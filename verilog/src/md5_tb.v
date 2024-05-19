`timescale 1ns / 1ps

module tb_md5;
    // Inputs
    reg clk;
    reg reset;
    reg [0:511] message;
    reg [63:0] message_len;
    reg start;

    // Outputs
    wire [127:0] digest;
    wire ready;

    integer len;
    integer i;

    // Instantiate the Unit Under Test (UUT)
    md5 uut (
        .clk(clk),
        .reset(reset),
        .message(message),
        .message_len(message_len),
        .start(start),
        .digest(digest),
        .ready(ready)
    );

    // Clock generation
    initial begin
        clk = 0;
        forever #0.185 clk = !clk;  // Clock with a period of 370ps (2.7 GHz like iMac CPU)
    end

    integer start_time;
    integer elapsed_time;

    // Test vectors and checker
    initial begin
        // Initialize Inputs
        reset = 1;
        start = 0;
        message = 0;
        message_len = 0;

        // Wait for global reset to finish
        #0.37;
        reset = 0;

        // Stimulate the message input
        // message works fine from 0 to 440 length
        message = 344'h54686520717569636b2062726f776e20666f78206a756d7073206f76657220746865206c617a7920646f67;
        message_len = 344;

        // Start the computation
        #0.37;
        start = 1;
        #0.37;
        start_time = $time;  // Store the start time
        start = 0;

        // Wait for the MD5 computation to finish
        wait (ready == 1);
        elapsed_time = $time - start_time;  // Calculate the elapsed time
        #0.37;

        // Check the output digest
        $display("Digest: %h", digest);

        // Print the elapsed time
        $display("Elapsed time: %d time units", elapsed_time);

        // The expected digest for the given message (you need to calculate this in advance)
        if (digest == 128'h9e107d9d372bb6826bd81d3542a419d6) begin
            $display("Test Passed. Digest matches expected value.");
        end else begin
            $display("Test Failed. Digest does not match expected value.");
        end

        // Reset the MD5 computation
        #0.37;
        reset = 1;
        #0.37;
        reset = 0;

        // End simulation
        #0.37;
        $finish;
    end

endmodule

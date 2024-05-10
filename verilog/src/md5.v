`timescale 1ns / 1ps

module md5 (
        input wire clk,
        input wire reset,
        input wire [511:0] message,  // 512-bit message block input
        input wire start,            // Start the process
        output reg [127:0] digest,   // Output digest
        output reg ready             // Indicates completion
    );

    // Define the constants
    reg [31:0] K [63:0];

    // Define rotation amounts
    reg [4:0] S [63:0];

    // MD buffer registers
    reg [31:0] A, B, C, D;

    // Temporary registers for computations
    reg [31:0] AA, BB, CC, DD;
    reg [31:0] F;
    integer i;
    integer idx;

    // Message schedule array (only showing initialization)
    reg [31:0] M [15:0];  // 16 words of the message block

    initial begin
        // Initialize the constants
        K[0] =  32'hd76aa478;   K[1] =  32'he8c7b756;   K[2] =  32'h242070db;   K[3] =  32'hc1bdceee;
        K[4] =  32'hf57c0faf;   K[5] =  32'h4787c62a;   K[6] =  32'ha8304613;   K[7] =  32'hfd469501;
        K[8] =  32'h698098d8;   K[9] =  32'h8b44f7af;   K[10] = 32'hffff5bb1;   K[11] = 32'h895cd7be;
        K[12] = 32'h6b901122;   K[13] = 32'hfd987193;   K[14] = 32'ha679438e;   K[15] = 32'h49b40821;
        K[16] = 32'hf61e2562;   K[17] = 32'hc040b340;   K[18] = 32'h265e5a51;   K[19] = 32'he9b6c7aa;
        K[20] = 32'hd62f105d;   K[21] = 32'h02441453;   K[22] = 32'hd8a1e681;   K[23] = 32'he7d3fbc8;
        K[24] = 32'h21e1cde6;   K[25] = 32'hc33707d6;   K[26] = 32'hf4d50d87;   K[27] = 32'h455a14ed;
        K[28] = 32'ha9e3e905;   K[29] = 32'hfcefa3f8;   K[30] = 32'h676f02d9;   K[31] = 32'h8d2a4c8a;
        K[32] = 32'hfffa3942;   K[33] = 32'h8771f681;   K[34] = 32'h6d9d6122;   K[35] = 32'hfde5380c;
        K[36] = 32'ha4beea44;   K[37] = 32'h4bdecfa9;   K[38] = 32'hf6bb4b60;   K[39] = 32'hbebfbc70;
        K[40] = 32'h289b7ec6;   K[41] = 32'heaa127fa;   K[42] = 32'hd4ef3085;   K[43] = 32'h04881d05;
        K[44] = 32'hd9d4d039;   K[45] = 32'he6db99e5;   K[46] = 32'h1fa27cf8;   K[47] = 32'hc4ac5665;
        K[48] = 32'hf4292244;   K[49] = 32'h432aff97;   K[50] = 32'hab9423a7;   K[51] = 32'hfc93a039;
        K[52] = 32'h655b59c3;   K[53] = 32'h8f0ccc92;   K[54] = 32'hffeff47d;   K[55] = 32'h85845dd1;
        K[56] = 32'h6fa87e4f;   K[57] = 32'hfe2ce6e0;   K[58] = 32'ha3014314;   K[59] = 32'h4e0811a1;
        K[60] = 32'hf7537e82;   K[61] = 32'hbd3af235;   K[62] = 32'h2ad7d2bb;   K[63] = 32'heb86d391;

        // Initialize the rotation amounts
        S[0]  = 7; S[1]  = 12; S[2]  = 17; S[3]  = 22; S[4]  = 7; S[5]  = 12; S[6]  = 17; S[7]  = 22;
        S[8]  = 7; S[9]  = 12; S[10] = 17; S[11] = 22; S[12] = 7; S[13] = 12; S[14] = 17; S[15] = 22;
        S[16] = 5; S[17] = 9;  S[18] = 14; S[19] = 20; S[20] = 5; S[21] = 9;  S[22] = 14; S[23] = 20;
        S[24] = 5; S[25] = 9;  S[26] = 14; S[27] = 20; S[28] = 5; S[29] = 9;  S[30] = 14; S[31] = 20;
        S[32] = 4; S[33] = 11; S[34] = 16; S[35] = 23; S[36] = 4; S[37] = 11; S[38] = 16; S[39] = 23;
        S[40] = 4; S[41] = 11; S[42] = 16; S[43] = 23; S[44] = 4; S[45] = 11; S[46] = 16; S[47] = 23;
        S[48] = 6; S[49] = 10; S[50] = 15; S[51] = 21; S[52] = 6; S[53] = 10; S[54] = 15; S[55] = 21;
        S[56] = 6; S[57] = 10; S[58] = 15; S[59] = 21; S[60] = 6; S[61] = 10; S[62] = 15; S[63] = 21;
    end

    // Control FSM
    reg [5:0] state;
    always @(posedge clk) begin
        if (reset) begin
            state <= 0;
            ready <= 0;
            A <= 32'h67452301;  // Resetting MD buffers to initial values
            B <= 32'hefcdab89;
            C <= 32'h98badcfe;
            D <= 32'h10325476;
            i <= 0;
        end else begin
            case (state)
                0: begin
                    if (start) begin
                        AA <= A;
                        BB <= B;
                        CC <= C;
                        DD <= D;  // Save initial values
                        for (i = 15; i >= 0; i = i - 1) begin
                            M[15 - i] <= {message[32*i +: 8], message[32*i + 8 +: 8], message[32*i + 16 +: 8], message[32*i + 24 +: 8]};  // Split the message into 32-bit words and reverse the byte order
                        end
                        i <= 0;  // Initialize counter for loop
                        state <= 1;
                    end
                end
                1: begin
                    // Main computation loop
                    if (i < 64) begin
                        // Reinitialize F
                        F <= 0;
                        // Use different index for each round
                        if (i < 16) begin
                            F = (BB & CC) | ((~BB) & DD);
                            idx = i;
                        end else if (i < 32) begin
                            F = (BB & DD) | (CC & (~DD));
                            idx = (5*i + 1) % 16;
                        end else if (i < 48) begin
                            F = BB ^ CC ^ DD;
                            idx = (3*i + 5) % 16;
                        end else begin
                            F = CC ^ (BB | (~DD));
                            idx = (7*i) % 16;
                        end

                        F = F + AA + K[i] + M[idx];
                        AA = DD;
                        DD = CC;
                        CC = BB;
                        BB = (BB + ((F << S[i]) | (F >>> (32 - S[i])))) % 4294967296;

                        i = i + 1;
                    end else begin
                        // Finalize the digest
                        A = A + AA;
                        B = B + BB;
                        C = C + CC;
                        D = D + DD;

                        digest[127:120] = A[7:0];
                        digest[119:112] = A[15:8];
                        digest[111:104] = A[23:16];
                        digest[103:96]  = A[31:24];

                        digest[95:88]   = B[7:0];
                        digest[87:80]   = B[15:8];
                        digest[79:72]   = B[23:16];
                        digest[71:64]   = B[31:24];

                        digest[63:56]   = C[7:0];
                        digest[55:48]   = C[15:8];
                        digest[47:40]   = C[23:16];
                        digest[39:32]   = C[31:24];

                        digest[31:24]   = D[7:0];
                        digest[23:16]   = D[15:8];
                        digest[15:8]    = D[23:16];
                        digest[7:0]     = D[31:24];

                        ready <= 1;
                        state <= 0;
                    end
                end
            endcase
        end
    end

    // Monitor for debugging
    initial begin
        //$monitor("State: %d, AA: %h, BB: %h, CC: %h, DD: %h, i: %d, idx: %d, ready: %b", state, AA, BB, CC, DD, i, idx, ready);
    end

endmodule

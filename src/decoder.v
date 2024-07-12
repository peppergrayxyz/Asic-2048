
/*
      -- a --
     |       |
     f       b
     |       |
      -- g --
     |       |
     e       c
     |       |
      -- d --       |h|
*/

/*
  assign display7_0 = 'b1111110;
  assign display7_1 = 'b0110000;
  assign display7_2 = 'b1101101;
  assign display7_3 = 'b1111001;
  assign display7_4 = 'b0110011;
  assign display7_5 = 'b1011011;
  assign display7_6 = 'b1011111;
  assign display7_7 = 'b1110000;
  assign display7_8 = 'b1111111;
  assign display7_9 = 'b1111011;
  assign display7_a = 'b1110111;
  assign display7_b = 'b0011111;
  assign display7_c = 'b0001101;
  assign display7_d = 'b0111101;
  assign display7_e = 'b1001111;
  assign display7_f = 'b1000111;
  assign display7_g = 'b1011110;
  assign display7_h = 'b0010111;

  assign display7_0 = 'b1100111; // P
  assign display7_1 = 'b1001111; // E
  assign display7_2 = 'b1100111; // P
  assign display7_3 = 'b1100111; // P
  assign display7_4 = 'b1001111; // E
  assign display7_5 = 'b0000101; // R
  assign display7_6 = 'b1011110; // G
  assign display7_7 = 'b0000101; // R
  assign display7_8 = 'b1110111; // A
  assign display7_9 = 'b0111011; // Y
  assign display7_a = 'b0110111; // X
  assign display7_b = 'b0111011; // Y
  assign display7_c = 'b1101100; // Z
  assign display7_d = 'b1110111; // A
  assign display7_e = 'b1011010; // S
  assign display7_f = 'b0010000; // I
  assign display7_g = 'b0001101; // C
  assign display7_h = 'b1101101; // 2
  assign display7_i = 'b1111110; // 0
  assign display7_j = 'b0110011; // 4 
  assign display7_k = 'b1111111; // 8

  assign display7_0 = 'b0000101; // 
  assign display7_1 = 'b0000001; // 
  assign display7_2 = 'b0000001; // 
  assign display7_3 = 'b0010001; // 
  assign display7_4 = 'b1110111; // A
  assign display7_5 = 'b1011011; // S
  assign display7_6 = 'b0110000; // I
  assign display7_7 = 'b0001101; // C
  assign display7_8 = 'b1101101; // 2
  assign display7_9 = 'b1111110; // 0
  assign display7_a = 'b0110011; // 4
  assign display7_b = 'b1111111; // 8
  assign display7_c = 'b0000011; // 
  assign display7_d = 'b0000001; // 
  assign display7_e = 'b0000001; // 
  assign display7_f = 'b0100001; // 

*/

module seg7 (
    input wire [3:0] counter,
    output reg [6:0] segments
);

    always @(*) begin
        case(counter)
            //                7654321
            0:  segments = 7'b0111111;
            1:  segments = 7'b0000110;
            2:  segments = 7'b1011011;
            3:  segments = 7'b1001111;
            4:  segments = 7'b1100110;
            5:  segments = 7'b1101101;
            6:  segments = 7'b1111100;
            7:  segments = 7'b0000111;
            8:  segments = 7'b1111111;
            9:  segments = 7'b1100111;
            default:    
                segments = 7'b0000000;
        endcase
    end

endmodule


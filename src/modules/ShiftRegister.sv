`default_nettype none

/* sn74hc595-ep */

module ShiftRegister 
#(parameter WIDTH = 8) (
  input SRCLK,
  input SER,
  input SRCLR_n,
  input [WIDTH-1:0] SRCLR_value,
  output [WIDTH-1:0] Q,
  output Q_dash
);

  reg [WIDTH-1:0] SR;

  always @(posedge SRCLK or negedge SRCLR_n) begin 
    if(!SRCLR_n) begin
      SR <= SRCLR_value;
    end 
    else begin
      SR <= {SER, SR[WIDTH-1:1]};
    end
  end

  assign Q = SR;
  assign Q_dash = SR[0];

endmodule

module ShiftRegisterLatched 
#(parameter WIDTH = 8) (
  input SRCLK,
  input SER,
  input RCLK,
  input SRCLR_n,
  input OE_n,
  input [WIDTH-1:0] SRCLR_value,
  input [WIDTH-1:0] OE_n_value,
  output [WIDTH-1:0] Q,
  output Q_dash
);

  reg  [WIDTH-1:0] R;
  wire [WIDTH-1:0] SR;

  ShiftRegister #(.WIDTH(WIDTH)) SR1 (
    .SRCLK(SRCLK),
    .SER(SER),
    .SRCLR_n(SRCLR_n),
    .SRCLR_value(SRCLR_value),
    .Q(SR),
    .Q_dash(Q_dash)
  );

  always @(posedge RCLK) begin
    R <= SR;
  end

  assign Q = OE_n ? OE_n_value : R;

endmodule

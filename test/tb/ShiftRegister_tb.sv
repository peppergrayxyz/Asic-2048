`default_nettype none
`timescale 1ns/1ps

`define assert(signal, value) \
  if (signal !== value) begin \
      $fdisplay(32'h8000_0002 /* stderr*/,"ASSERTION FAILED in %m, %d: signal != value (%b,%h)", `__LINE__, signal, signal); \
      $stop;\
  end

module ShiftRegister_Test;

  // Parameters
  localparam WIDTH = 8;

  // Inputs
  reg SRCLK;
  reg SER;
  reg RCLK;
  reg SRCLR_n;
  reg OE_n;
  reg SRCLR_value;

  // Outputs
  wire [WIDTH-1:0] Q;
  wire Q_dash;

  // state
  reg Enable_Register_Clock;

  // Instantiate the ShiftRegister module
  ShiftRegisterLatched #(.WIDTH(WIDTH)) dut (
    .SRCLK(SRCLK),
    .SER(SER),
    .RCLK(RCLK),
    .SRCLR_n(SRCLR_n),
    .OE_n(OE_n),
    .SRCLR_value({WIDTH{SRCLR_value}}),
    .OE_n_value({WIDTH{1'bz}}),
    .Q(Q),
    .Q_dash(Q_dash)
  );
  
  // generate the clock
  initial begin
    SRCLK = 1'b0;
    Enable_Register_Clock = 1'b0;

    forever begin
      #1 
      SRCLK = ~SRCLK;
    end
  end

  always @(*) begin
      RCLK = ~SRCLK & Enable_Register_Clock;
  end
  
  // Test stimulus
  initial begin
    $dumpfile(`DUMP_FILE_NAME);
    $dumpvars;

    SER = 1'b0;
    RCLK = 1'b0;
    SRCLR_n = 1'b0;
    OE_n = 1'b0;
    SRCLR_value = 1'b0;

    #2
    `assert(Q, 8'bX)
    `assert(Q_dash, 1'b0)

    // Test shift bit through
    SER = 1'b1;
    SRCLR_n = 1'b1;
    #2
    `assert(Q, 8'bX)
    `assert(Q_dash, 1'b0)

    SER = 1'b0;
    Enable_Register_Clock = 1'b1;
    
    #2
    `assert(Q, 8'b10000000)
    `assert(Q_dash, 1'b0)
    
    #2
    `assert(Q, 8'b01000000)
    `assert(Q_dash, 1'b0)
    
    #2
    `assert(Q, 8'b00100000)
    `assert(Q_dash, 1'b0)
    
    #2
    `assert(Q, 8'b00010000)
    `assert(Q_dash, 1'b0)
    
    #2
    `assert(Q, 8'b00001000)
    `assert(Q_dash, 1'b0)
    
    #2
    `assert(Q, 8'b00000100)
    `assert(Q_dash, 1'b0)
    
    #2
    `assert(Q, 8'b00000010)
    `assert(Q_dash, 1'b1)
    
    #2
    `assert(Q, 8'b00000001)
    `assert(Q_dash, 1'b0)
    
    // Disable Output
    #1
    Enable_Register_Clock = 1'b0;
    OE_n = 1'b1;
    #1
    `assert(Q, 8'bzzzzzzzz)
    `assert(Q_dash, 1'b0)
    
    SER = 1'b1;
    #1
    `assert(Q, 8'bzzzzzzzz)
    `assert(Q_dash, 1'b0)
    #1
    `assert(Q, 8'bzzzzzzzz)
    `assert(Q_dash, 1'b0)

    SER = 1'b0;
    Enable_Register_Clock = 1'b1;
    #1
    `assert(Q, 8'bzzzzzzzz)
    `assert(Q_dash, 1'b0)

    Enable_Register_Clock = 1'b0;
    OE_n = 1'b0;
    #1
    `assert(Q, 8'b10000000)
    `assert(Q_dash, 1'b0)

    #2
    `assert(Q, 8'b10000000)
    `assert(Q_dash, 1'b0)

    #1
    SRCLR_n = 1'b0;
    Enable_Register_Clock = 1'b1;
    #2
    `assert(Q, 8'b00000000)
    `assert(Q_dash, 1'b0)
    #1
    `assert(Q, 8'b00000000)
    `assert(Q_dash, 1'b0)

    SRCLR_n = 1'b1;
    Enable_Register_Clock = 1'b0;
    #1
    `assert(Q, 8'b00000000)
    `assert(Q_dash, 1'b0)
    #2
    `assert(Q, 8'b00000000)
    `assert(Q_dash, 1'b0)

    
    // Test clear bits
    SRCLR_n = 1'b1;
    Enable_Register_Clock = 1'b1;
    SER = 1'b1;    
    #2
    SER = 1'b0;    
    #2
    SER = 1'b1;    
    #2
    SER = 1'b0;    
    #2
    SER = 1'b1;    
    #2
    SER = 1'b0;    
    #2
    SER = 1'b1;    
    #2
    `assert(Q, 8'b10101010)
    `assert(Q_dash, 1'b0)
      
    SRCLR_n = 1'b0;
    #2
    `assert(Q, 8'b00000000)
    `assert(Q_dash, 1'b0)

    SRCLR_value = 1'b1;
    #2
    `assert(Q, 8'b11111111)
    `assert(Q_dash, 1'b1)


    $finish;
    
  end

endmodule
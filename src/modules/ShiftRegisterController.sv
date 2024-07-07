`default_nettype none

`include "DownCounter.sv"
`include "ShiftRegister.sv"

/*
    ShiftRegisterController (SRC)

           [setValue]
               ^
               |
               v
        .---[buffer]-->---.
        |                 |
        `---[MEMORY]--<---´

    Memory is arragned as shift registeres, which are arragend in a loop similar to a ring buffer.
    They are looped thruugh a buffer, which becomes an additional location in the memory.
    The number of elements in the loop is therefore memorysize + 1. 

    SRC shifts numSteps x DATA_WIDTH times to recht the next data location
    - this puts the content of that address into the buffer
    - if write is set to true, buffer is then overrwritten with value

    SRC is actived on the rising edge of start and indicates completion with done

*/

module ShiftRegisterController 
#(parameter ADDRESS_WIDTH = 4, DATA_WIDTH = 8) (
  input clk,
  input rst_n,
  input write,
  input  [ADDRESS_WIDTH-1:0] numSteps,
  input start,
  input  [DATA_WIDTH-1:0] value,
  input ser_in,
  output [DATA_WIDTH-1:0] buffer,
  output ser_out,
  output ser_clk,
  output lastStep
);
  reg active;  

  localparam STEP_SIZE    = $clog2(DATA_WIDTH);
  localparam COUNTER_SIZE = STEP_SIZE + ADDRESS_WIDTH;

  wire [COUNTER_SIZE-1:0] numShifts = numSteps * DATA_WIDTH;

  assign ser_clk = ((active || !rst_n) && !clk);
  
  DownCounter #(.WIDTH(COUNTER_SIZE)) DC (
    .clk(clk),
    .rst_n(rst_n),
    .enable(active),
    .load(!active || (lastStep && start)),
    .load_value(numShifts),
    .last(lastStep)
  );

  wire override = (active && write && ser_clk);
  wire set_clk  = (active && write && !clk) || override || (!rst_n & clk) || (!active && write && !clk);

  wire Q_dash;
  assign ser_out = override ? value[0] : Q_dash;

  wire [DATA_WIDTH-1:0] overrideValue = {ser_in, value[DATA_WIDTH-1:1]};
  wire [DATA_WIDTH-1:0] setValue = !rst_n ? {DATA_WIDTH{1'b0}} : (override ? overrideValue : value);


  ShiftRegister #(.WIDTH(DATA_WIDTH)) SR (
    .SRCLK(ser_clk),
    .SER(ser_in),
    .SRCLR_n(!set_clk),
    .SRCLR_value(setValue),
    .Q(buffer),
    .Q_dash(Q_dash)
  );

  always @(posedge clk or negedge rst_n) begin
    if (!rst_n)                 active <= 1'b0;
    else if(!active && start)   active <= 1'b1;
    else if(!start && lastStep) active <= 1'b0;
  end

endmodule
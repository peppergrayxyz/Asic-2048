`default_nettype none
`timescale 1ns/1ps

`define assert(signal, value) \
  if (signal !== value) begin \
      $fdisplay(32'h8000_0002 /* stderr*/,"ASSERTION FAILED in %m, %d: signal != value (%b,%h)", `__LINE__, signal, signal); \
      // $stop;\
  end

module DownCounter_Test;
    
  // Parameters
  parameter WIDTH = 8;

  // Inputs
  reg clk;
  reg rst_n;
  reg enable;
  reg load;
  reg [WIDTH-1:0] load_value;

  // Outputs
  wire [WIDTH-1:0] count;
  wire last;

  // Instantiate the module
  DownCounter #(.WIDTH(WIDTH)) dut (
    .clk(clk),
    .rst_n(rst_n),
    .enable(enable),
    .load(load),
    .load_value(load_value),
    .count(count),
    .last(last)
  );
  
  // generate the clock
  initial begin
    clk = 1'b0;

    forever begin
      #1 
      clk = ~clk;
    end
  end
  
  // Test stimulus
  initial begin
    $dumpfile(`DUMP_FILE_NAME);
    $dumpvars;

    rst_n = 1'b1;
    load_value = 8'b10110011;
    enable = 1'b0;
    load = 1'b0;

    `assert(count, 8'bxxxxxxxx)
    `assert(last, 1'bx)
    
    #2
    `assert(count, 8'bxxxxxxxx)
    `assert(last, 1'b0)

    // Reset
    rst_n = 1'b0;
    #2
    `assert(count, 3'b000)
    `assert(last, 1'b0)
    `assert(dut.value, 8'b00000000)

    rst_n = 1'b1;

    // Load
    load = 1'b1;
    #2
    `assert(count, 8'b10110011)
    `assert(last, 1'b0)

    // Reset
    rst_n = 1'b0;
    #2
    `assert(count, 3'b000)
    `assert(last, 1'b0)

    rst_n = 1'b1;
    
    // Load zero
    load = 1'b1;
    load_value = 8'b00000000;
    #2
    `assert(count, 8'b00000000)
    `assert(last, 1'b0)

    // Load value
    load_value = 8'b00000011;
    load = 1'b1;
    #2
    `assert(count, 8'b00000011)
    `assert(last, 1'b0)

    // check value
    #2
    `assert(count, 8'b00000011)
    `assert(last, 1'b0)

    // failed start (load still enabled)
    enable = 1'b1;
    
    #2
    `assert(count, 8'b00000011)
    `assert(last, 1'b0)
    
    // disable load & enable
    load = 1'b0;
    enable = 1'b0;

    #2
    `assert(count, 8'b00000011)
    `assert(last, 1'b0)

    // start
    enable = 1'b1;

    #2
    `assert(count, 8'b00000010)
    `assert(last, 1'b0)
    
    #2
    `assert(count, 8'b00000001)
    `assert(last, 1'b1)
    
    #2
    `assert(count, 8'b00000000)
    `assert(last, 1'b1)

    #2
    `assert(count, 8'b00000000)
    `assert(last, 1'b1)

    // dont stop, but load
    load_value = 8'b00000100;
    load = 1'b1;
    
    #2
    `assert(count, 8'b00000100)
    `assert(last, 1'b0)
    
    #2
    `assert(count, 8'b00000100)
    `assert(last, 1'b0)

    load = 1'b0;
    #2
    `assert(count, 8'b00000011)
    `assert(last, 1'b0)

    // pause
    enable = 1'b0;

    #2
    `assert(count, 8'b00000011)
    `assert(last, 1'b0)
    #2
    `assert(count, 8'b00000011)
    `assert(last, 1'b0)

    // continue
    enable = 1'b1;

    #2
    `assert(count, 8'b00000010)
    `assert(last, 1'b0)

    #2
    `assert(count, 8'b00000001)
    `assert(last, 1'b1)

    // load 
    load = 1'b1;
    #2
    `assert(count, 8'b00000100)
    `assert(last, 1'b0)
    
    // continue 
    load = 1'b0;
    #2
    `assert(count, 8'b00000011)
    `assert(last, 1'b0)

    // load 
    load = 1'b1;
    #2
    `assert(count, 8'b00000100)
    `assert(last, 1'b0)

    // continue 
    load = 1'b0;
    #8
    `assert(count, 8'b00000000)
    `assert(last, 1'b1)

    // stop
    enable = 1'b0;
    #2
    `assert(count, 8'b00000000)
    `assert(last, 1'b0)

    $finish;
    
  end

endmodule
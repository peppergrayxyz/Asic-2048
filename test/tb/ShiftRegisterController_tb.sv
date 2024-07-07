`default_nettype none
`timescale 1ns/1ps

`define assert(signal, value) \
  if (signal !== value) begin \
      $fdisplay(32'h8000_0002 /* stderr*/,"ASSERTION FAILED in %m, %d: signal != value (%b,%h)", `__LINE__, signal, signal); \
      /* $stop; */ \
  end

module ShiftRegisterController_Test;
    
  // Parameters
  localparam ADDRESS_WIDTH = 4;
  localparam DATA_WIDTH = 8;
  localparam NUM_SR = 4;

  // Inputs
  reg clk;
  reg rst_n;
  reg write;
  reg [ADDRESS_WIDTH-1:0] numSteps;
  reg start;
  reg [DATA_WIDTH-1:0] value;

  // Outputs
  wire ser_clk;
  wire [DATA_WIDTH-1:0] buffer;
  wire lastStep;
  wire done;

  wire serialBus [NUM_SR:0];
  wire [DATA_WIDTH-1:0] memoryValues [NUM_SR-1:0];

  generate
    for (genvar i=0; i < NUM_SR; i = i + 1) begin: SRC
        ShiftRegister #(.WIDTH(DATA_WIDTH)) SR (
            .SRCLK(ser_clk),
            .SER(serialBus[i]),
            .RCLK(!ser_clk),
            .SRCLR_n(rst_n),
            .OE_n(1'b0),
            .SRCLR_value({DATA_WIDTH{1'b0}}),
            .Q(memoryValues[i]),
            .Q_dash(serialBus[i+1])
        );
    end
  endgenerate
  
  // Instantiate the module
  ShiftRegisterController  #(.ADDRESS_WIDTH(ADDRESS_WIDTH),.DATA_WIDTH(DATA_WIDTH)) dut (
    .clk(clk),
    .rst_n(rst_n),
    .write(write),
    .numSteps(numSteps),
    .start(start),
    .value(value),
    .ser_in(serialBus[NUM_SR]),
    .buffer(buffer),
    .ser_out(serialBus[0]),
    .ser_clk(ser_clk),
    .lastStep(lastStep)
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

    value = 8'b00000000;
    write = 0;
    start = 0;
    numSteps = 0;

    // Reset
    rst_n = 1'b0;
    #2
    rst_n = 1'b1;
    #2
    `assert(SRC[0].SR.Q, 8'b00000000)
    `assert(SRC[1].SR.Q, 8'b00000000)
    `assert(SRC[2].SR.Q, 8'b00000000)
    `assert(SRC[3].SR.Q, 8'b00000000)

    // Loop Zeros
    value = 8'b00000000;
    numSteps = 5;
    write = 1;
    start = 1;
    #2
    write = 0;
    start = 0;
    `assert(dut.DC.count, 5*DATA_WIDTH)
    `assert(SRC[0].SR.Q, 8'b00000000)
    `assert(SRC[1].SR.Q, 8'b00000000)
    `assert(SRC[2].SR.Q, 8'b00000000)
    `assert(SRC[3].SR.Q, 8'b00000000)

    #76
    `assert(dut.DC.count, 2)
    `assert(SRC[0].SR.Q, 8'b00000000)
    `assert(SRC[1].SR.Q, 8'b00000000)
    `assert(SRC[2].SR.Q, 8'b00000000)
    `assert(SRC[3].SR.Q, 8'b00000000)

    #2
    `assert(dut.DC.count, 1)
    `assert(lastStep, 1)
    #2
    `assert(dut.DC.count, 0)
    `assert(lastStep, 0)
    `assert(dut.buffer,  8'b00000000)
    `assert(SRC[0].SR.Q, 8'b00000000)
    `assert(SRC[1].SR.Q, 8'b00000000)
    `assert(SRC[2].SR.Q, 8'b00000000)
    `assert(SRC[3].SR.Q, 8'b00000000)


    // Loop Ones
    value = 8'b11111111;
    write = 1;
    start = 1;
    #2
    write = 0;
    start = 0;
    
    #16
    `assert(dut.buffer,  8'b00000000)
    `assert(SRC[0].SR.Q, 8'b11111111)
    `assert(SRC[1].SR.Q, 8'b00000000)
    `assert(SRC[2].SR.Q, 8'b00000000)
    `assert(SRC[3].SR.Q, 8'b00000000)
    #16
    `assert(dut.buffer,  8'b00000000)
    `assert(SRC[0].SR.Q, 8'b00000000)
    `assert(SRC[1].SR.Q, 8'b11111111)
    `assert(SRC[2].SR.Q, 8'b00000000)
    `assert(SRC[3].SR.Q, 8'b00000000)
    #16
    `assert(dut.buffer,  8'b00000000)
    `assert(SRC[0].SR.Q, 8'b00000000)
    `assert(SRC[1].SR.Q, 8'b00000000)
    `assert(SRC[2].SR.Q, 8'b11111111)
    `assert(SRC[3].SR.Q, 8'b00000000)
    #16
    `assert(dut.buffer,  8'b00000000)
    `assert(SRC[0].SR.Q, 8'b00000000)
    `assert(SRC[1].SR.Q, 8'b00000000)
    `assert(SRC[2].SR.Q, 8'b00000000)
    `assert(SRC[3].SR.Q, 8'b11111111)
    #16
    `assert(dut.buffer,  8'b11111111)
    `assert(SRC[0].SR.Q, 8'b00000000)
    `assert(SRC[1].SR.Q, 8'b00000000)
    `assert(SRC[2].SR.Q, 8'b00000000)
    `assert(SRC[3].SR.Q, 8'b00000000)


    // Fill Nnumbers 1
    start = 0;
    write = 0;
    #4
    value = 8'b11010110;
    write = 1;
    #2
    write = 0;
    start = 0;
    `assert(dut.buffer, 8'b11010110)
    
    value = 8'b00000000;
    #2
    `assert(dut.buffer, 8'b11010110)

    write = 1;
    #2
    write = 0;
    `assert(dut.buffer, 8'b00000000)

    
    // Fill Nnumbers 2
    write = 1;
    value = 8'b00000111;
    numSteps = 1;
    #2
    write = 0;
    `assert(dut.buffer,  8'b00000111)
    `assert(SRC[0].SR.Q, 8'b00000000)
    `assert(SRC[1].SR.Q, 8'b00000000)
    `assert(SRC[2].SR.Q, 8'b00000000)
    `assert(SRC[3].SR.Q, 8'b00000000)
    #2
    `assert(dut.buffer,  8'b00000111)
    `assert(SRC[0].SR.Q, 8'b00000000)
    `assert(SRC[1].SR.Q, 8'b00000000)
    `assert(SRC[2].SR.Q, 8'b00000000)
    `assert(SRC[3].SR.Q, 8'b00000000)

    start = 1;
    #2
    start = 0;
    `assert(dut.DC.count, 8)
    `assert(dut.buffer,  8'b00000111)
    `assert(SRC[0].SR.Q, 8'b00000000)
    `assert(SRC[1].SR.Q, 8'b00000000)
    `assert(SRC[2].SR.Q, 8'b00000000)
    `assert(SRC[3].SR.Q, 8'b00000000)
    #2
    `assert(dut.DC.count, 7)
    `assert(dut.buffer,  8'b00000011)
    `assert(SRC[0].SR.Q, 8'b10000000)
    `assert(SRC[1].SR.Q, 8'b00000000)
    `assert(SRC[2].SR.Q, 8'b00000000)
    `assert(SRC[3].SR.Q, 8'b00000000)
    #2
    `assert(dut.DC.count, 6)
    `assert(dut.buffer,  8'b00000001)
    `assert(SRC[0].SR.Q, 8'b11000000)
    `assert(SRC[1].SR.Q, 8'b00000000)
    `assert(SRC[2].SR.Q, 8'b00000000)
    `assert(SRC[3].SR.Q, 8'b00000000)
    #8
    `assert(dut.DC.count, 2)
    `assert(lastStep, 0)
    `assert(dut.buffer,  8'b00000000)
    `assert(SRC[0].SR.Q, 8'b00011100)
    `assert(SRC[1].SR.Q, 8'b00000000)
    `assert(SRC[2].SR.Q, 8'b00000000)
    `assert(SRC[3].SR.Q, 8'b00000000)
    #2
    `assert(dut.DC.count, 1)
    `assert(lastStep, 1)
    `assert(dut.buffer,  8'b00000000)
    `assert(SRC[0].SR.Q, 8'b00001110)
    `assert(SRC[1].SR.Q, 8'b00000000)
    `assert(SRC[2].SR.Q, 8'b00000000)
    `assert(SRC[3].SR.Q, 8'b00000000)

    
    // Fill Nnumbers 3
    #2
    write = 1;
    value = 8'b01100011;
    start = 1;
    #2
    write = 0;
    start = 0;
    `assert(dut.DC.count, 8)
    `assert(lastStep, 0)
    `assert(dut.buffer,  8'b01100011)
    `assert(SRC[0].SR.Q, 8'b00000111)
    `assert(SRC[1].SR.Q, 8'b00000000)
    `assert(SRC[2].SR.Q, 8'b00000000)
    `assert(SRC[3].SR.Q, 8'b00000000)
    #2
    `assert(dut.DC.count, 7)
    `assert(lastStep, 0)
    `assert(dut.buffer,  8'b00110001)
    `assert(SRC[0].SR.Q, 8'b10000011)
    `assert(SRC[1].SR.Q, 8'b10000000)
    `assert(SRC[2].SR.Q, 8'b00000000)
    `assert(SRC[3].SR.Q, 8'b00000000)
    #2
    `assert(dut.DC.count, 6)
    `assert(lastStep, 0)
    `assert(dut.buffer,  8'b00011000)
    `assert(SRC[0].SR.Q, 8'b11000001)
    `assert(SRC[1].SR.Q, 8'b11000000)
    `assert(SRC[2].SR.Q, 8'b00000000)
    `assert(SRC[3].SR.Q, 8'b00000000)
    #12
    `assert(dut.buffer,  8'b00000000)
    `assert(SRC[0].SR.Q, 8'b01100011)
    `assert(SRC[1].SR.Q, 8'b00000111)
    `assert(SRC[2].SR.Q, 8'b00000000)
    `assert(SRC[3].SR.Q, 8'b00000000)


    // Fill Nnumbers 4
    write = 1;
    value = 8'b10011001;
    start = 1;
    #2
    write = 0;
    start = 0;
    #18
    `assert(lastStep, 0)
    `assert(dut.buffer,  8'b00000000)
    `assert(SRC[0].SR.Q, 8'b10011001)
    `assert(SRC[1].SR.Q, 8'b01100011)
    `assert(SRC[2].SR.Q, 8'b00000111)
    `assert(SRC[3].SR.Q, 8'b00000000)
    
    write = 1;
    value = 8'b00111100;
    start = 1;
    #2
    write = 0;
    start = 0;
    #18
    `assert(lastStep, 0)
    `assert(dut.buffer,  8'b00000000)
    `assert(SRC[0].SR.Q, 8'b00111100)
    `assert(SRC[1].SR.Q, 8'b10011001)
    `assert(SRC[2].SR.Q, 8'b01100011)
    `assert(SRC[3].SR.Q, 8'b00000111)

    // Continue

    write = 1;
    start = 1;
    value = 8'b00000000;
    #2
    write = 0;
    #16
    `assert(lastStep, 0)
    `assert(dut.buffer,  8'b00000111)
    `assert(SRC[0].SR.Q, 8'b00000000)
    `assert(SRC[1].SR.Q, 8'b00111100)
    `assert(SRC[2].SR.Q, 8'b10011001)
    `assert(SRC[3].SR.Q, 8'b01100011)

    // Continue
    #2
    // set new steps
    numSteps = 3;
    #2
    `assert(dut.DC.count, 6)
    #12
    `assert(lastStep, 0)
    `assert(dut.buffer,  8'b01100011)
    `assert(SRC[0].SR.Q, 8'b00000111)
    `assert(SRC[1].SR.Q, 8'b00000000)
    `assert(SRC[2].SR.Q, 8'b00111100)
    `assert(SRC[3].SR.Q, 8'b10011001)

    // accept new steps
    `assert(dut.DC.count, 3*DATA_WIDTH)
    #46
    `assert(lastStep, 1)
     start = 0;
    #2
    `assert(lastStep, 0)
    `assert(dut.buffer,  8'b00000000)
    `assert(SRC[0].SR.Q, 8'b00111100)
    `assert(SRC[1].SR.Q, 8'b10011001)
    `assert(SRC[2].SR.Q, 8'b01100011)
    `assert(SRC[3].SR.Q, 8'b00000111)

    // Loop through mem & check buffer value & timings    
    numSteps = 5;
    start = 1;
    #18
    `assert(dut.DC.count, 4*DATA_WIDTH)
    `assert(dut.buffer,  8'b00000111)
    #16
    `assert(dut.DC.count, 3*DATA_WIDTH)
    `assert(dut.buffer,  8'b01100011)
    #16
    `assert(dut.DC.count, 2*DATA_WIDTH)
    `assert(dut.buffer,  8'b10011001)
    #16
    `assert(dut.DC.count, 1*DATA_WIDTH)
    `assert(dut.buffer,  8'b00111100)
    #16
    `assert(dut.buffer,  8'b00000000)
    `assert(SRC[0].SR.Q, 8'b00111100)
    `assert(SRC[1].SR.Q, 8'b10011001)
    `assert(SRC[2].SR.Q, 8'b01100011)
    `assert(SRC[3].SR.Q, 8'b00000111)
    
    // Edit on the fly (override)
    #16
    `assert(buffer,  8'b00000111)
    value = buffer & 8'b11111101;
    write = 1;
    #2
    write = 0;
    start = 0;
    #62
    `assert(dut.buffer,  8'b00000000)
    `assert(SRC[0].SR.Q, 8'b00111100)
    `assert(SRC[1].SR.Q, 8'b10011001)
    `assert(SRC[2].SR.Q, 8'b01100011)
    `assert(SRC[3].SR.Q, 8'b00000101)

    #2
    `assert(dut.buffer,  8'b00000000)
    `assert(SRC[0].SR.Q, 8'b00111100)
    `assert(SRC[1].SR.Q, 8'b10011001)
    `assert(SRC[2].SR.Q, 8'b01100011)
    `assert(SRC[3].SR.Q, 8'b00000101)

    // check count
    numSteps = 5;
    start = 1;
    #80
    `assert(dut.DC.count, 1)
    #2
    `assert(dut.DC.count, 5*DATA_WIDTH)
    #16
    `assert(dut.DC.count, 4*DATA_WIDTH)
    #16
    `assert(dut.DC.count, 3*DATA_WIDTH)
    #16
    `assert(dut.DC.count, 2*DATA_WIDTH)
    #16
    `assert(dut.DC.count, 1*DATA_WIDTH)


    $finish;
    
  end

endmodule
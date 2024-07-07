
typedef enum bit [1:0] {IDLE, START, ACTIVE, DONE} e_IoDriverState;

module ShiftRegisterLoop
#(parameter WIDTH = 8) (
  input clk,
  input rst_n,
  input start,
  input load,
  input [WIDTH-1:0] setValue = {WIDTH{1'b0}},
  input ser_in,
  output last,
  output done,
  output [WIDTH-1:0] value,
  output e_IoDriverState state,
  output ser_out,  
  output [2:0] count,
);

  assign done = (state == DONE);

  DownCounter #(.WIDTH(3)) DC1 (
    .clk(clk),
    .rst_n(rst_n),
    .enable(state == ACTIVE),
    .load(state == START),
    .load_value(3'b111),
    .count(count),
    .last(last)
  );

  wire setLoadValue = (rst_n && load) || !rst_n;

  ShiftRegister #(.WIDTH(8)) SR1 (
    .SRCLK(((state == ACTIVE) && clk)),
    .SER(ser_in),
    .RCLK((((state == DONE) || setLoadValue) && clk)),
    .SRCLR_n(!setLoadValue),
    .SRCLR_value(setValue),
    .OE_n(0),
    .Q(value),
    .Q_dash(ser_out)
  );

  always @(posedge clk or negedge rst_n) begin
    
    if (!rst_n)             state <= IDLE;
    else begin
        case(state)
          IDLE:   if(start) state = START;
          START:            state = ACTIVE;
          ACTIVE: if(last)  state = DONE;
          DONE:             state = IDLE;
          default:          state = IDLE;
        endcase   
    end

  end

endmodule

module CPU 
#(parameter WIDTH = 8) (
  input clk,
  input rst_n,
  input start,
  output ser_out,
  output [207:0] display
);

  reg load;
  reg ser_in;
  reg last;
  reg done;
  reg [WIDTH-1:0] value;
  e_IoDriverState state;
  reg [2:0] count;

(* keep = "true" *) Display7 display1(8'b10101010);

ShiftRegisterLoop #(.WIDTH(WIDTH)) SRL (
  .clk(clk), 
  .rst_n(rst_n),
  .start(start),
  .load(load), 
  .ser_in(ser_in), 
  .last(last), 
  .done(done), 
  .value(value), 
  .state(state), 
  .ser_out(ser_out), 
  .count(count),
  .setValue(8'b10101010)
  );

  ShiftRegister #(.WIDTH(208)) SR_Display (
    .SRCLK(clk),
    .SER(ser_out),
    .RCLK(clk),
    .SRCLR_n(rst_n),
    .OE_n(0),
    .Q(display),
    .Q_dash(ser_in)
  );

endmodule


/*



        */

/*
module xyz_peppergray_asic2048_top(
  input [7:0] io_in,
  output [7:0] io_out
);


endmodule
*/

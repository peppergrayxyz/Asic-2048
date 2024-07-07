`default_nettype none


module DownCounter 
#(parameter WIDTH = 8) (
  input clk,
  input rst_n,
  input enable,
  input load,
  input [WIDTH-1:0] load_value,
  output [WIDTH-1:0] count,
  output last
);

reg [WIDTH-1:0] value;

wire [WIDTH-1:0] nextValue = (value - 1);
wire zero = (value == 0);
assign last = enable && (nextValue == 0 || zero);
assign count = value;

always @(posedge clk or negedge rst_n) begin
    if (!rst_n) begin
        value <= 0;
    end 
    else if (load) begin
        value <= load_value;
    end 
    else if (enable && !zero) begin
        value <= nextValue;
    end 
end

endmodule
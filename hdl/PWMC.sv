`default_nettype none
`timescale 1ns / 1ps
//////////////////////////////////////////////////////////////////////////////////
// Company: 
// Engineer: 
// 
// Create Date: 25.01.2018 16:36:06
// Design Name: 
// Module Name: PWMC
// Project Name: 
// Target Devices: 
// Tool Versions: 
// Description: 
// 
// Dependencies: 
// 
// Revision:
// Revision 0.01 - File Created
// Additional Comments:
// 
//////////////////////////////////////////////////////////////////////////////////


module PWMC(
    input wire clk,
    input wire rst_an,
    input wire [7:0] duty,
    input wire loadnext,
    input wire loadnow,
    input wire en,
    output reg q
    );
    
    reg s_q;
    reg s_pendingLoad;
    reg [7:0] s_duty;
    reg [7:0] s_nextDuty;
    reg [7:0] s_count;

    assign q = s_q && en;
    
    always @(posedge(clk) or negedge(rst_an))
    begin
        if (rst_an == 0) begin
            s_q <= 0;
            s_pendingLoad <= 0;
            s_count <= 0;
            s_duty <= 0;
            s_nextDuty <= 0;
        end else begin
            if(loadnow == 1) begin
                s_duty <= duty;
                s_nextDuty <= duty;
                s_pendingLoad <= 0;
                s_count <= 0;
            end else if(loadnext == 1) begin
                s_duty <= s_duty;
                s_nextDuty <= duty;
                s_pendingLoad <= 1;
                s_count <= s_count +1 ;
            end else if(s_count == 255 && s_pendingLoad == 1) begin
                s_count <= 0;
                s_duty <= s_nextDuty;
                s_pendingLoad <= 0;
            end else begin
                s_count <= s_count + 1;
            end
            if(s_count < s_duty) begin
                s_q <= en;
            end else begin
                s_q <= 0;
            end
        end
    end
endmodule

/*   
    A testbench for maxval.v. 

    From "Getting Started with the Xilinx Zynq FPGA and Vivado" 
    by Peter Milder (peter.milder@stonybrook.edu)

    Copyright (C) 2018 Peter Milder

    This program is free software: you can redistribute it and/or modify
    it under the terms of the GNU General Public License as published by
    the Free Software Foundation, either version 3 of the License, or
    (at your option) any later version.

    This program is distributed in the hope that it will be useful,
    but WITHOUT ANY WARRANTY; without even the implied warranty of
    MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
    GNU General Public License for more details.

    You should have received a copy of the GNU General Public License
    along with this program.  If not, see <https://www.gnu.org/licenses/>.
*/

// This is a quick simulation testbench to check the maxval module only.
// This will randomly initialize a RAM with values, and put 32'hffffffff
// (the max unsigned 32 bit value) into the last location.

// To check correct functionality, simulate and use a waveform to observe
// correct behavior in the maxval module. Check that it correctly:
//    - reads all words in the memory
//    - checks each one and stores the largest at each step
//    - does not miss the last word in the sequence (addr = 2047)
//    - stores the largest back to address 0
`timescale 1ns / 1ps

module mac_system_tb();

    logic clk, reset;
    // PS-PL
    logic  [31:0] ps_control;
    logic  [31:0] ps_iternum;
    logic  [31:0] ps_accfreq;
    logic  [31:0] ps_phase;
    logic  [31:0] pl_full;
    logic [31:0] pl_status;
    // Input0 Read-Only
    logic [11:0] in0_bram0_addr;
    logic [11:0] in0_bram1_addr;
    logic [11:0] in0_bram2_addr;
    logic [11:0] in0_bram3_addr;
    logic  [31:0] in0_bram0_rddata;
    logic  [31:0] in0_bram1_rddata;
    logic  [31:0] in0_bram2_rddata;
    logic  [31:0] in0_bram3_rddata;
    // Input1 Read-Only
    logic [11:0] in1_bram_addr;
    logic [31:0] in1_bram_rddata;
    // Output Write-Only
    logic [11:0] out_bram0_addr;
    logic [11:0] out_bram1_addr;
    logic [11:0] out_bram2_addr;
    logic [11:0] out_bram3_addr;
    logic [31:0] out_bram0_wrdata;
    logic [31:0] out_bram1_wrdata;
    logic [31:0] out_bram2_wrdata;
    logic [31:0] out_bram3_wrdata;
    logic [3:0]  out_bram0_we;
    logic [3:0]  out_bram1_we;
    logic [3:0]  out_bram2_we;
    logic [3:0]  out_bram3_we;

    driver dut(
        .clk(clk),
        .reset(reset),
        // Control
        .ps_control(ps_control),
        .ps_iternum(ps_iternum),  // number of iterations to generate output feature map
        .ps_accfreq(ps_accfreq),  // accumulation frequency for each output value
        .pl_status(pl_status),
        .pl_full(pl_full),
        .ps_phase(ps_phase),
        // Input0 Read-Only
        .in0_bram0_addr(in0_bram0_addr),
        .in0_bram1_addr(in0_bram1_addr),
        .in0_bram2_addr(in0_bram2_addr),
        .in0_bram3_addr(in0_bram3_addr),
        .in0_bram0_rddata(in0_bram0_rddata),
        .in0_bram1_rddata(in0_bram1_rddata),
        .in0_bram2_rddata(in0_bram2_rddata),
        .in0_bram3_rddata(in0_bram3_rddata),
        // Input1 Read-Only
        .in1_bram_addr(in1_bram_addr),
        .in1_bram_rddata(in1_bram_rddata),
        // Output Write-Only
        .out_bram0_addr(out_bram0_addr),
        .out_bram1_addr(out_bram1_addr),
        .out_bram2_addr(out_bram2_addr),
        .out_bram3_addr(out_bram3_addr),
        .out_bram0_wrdata(out_bram0_wrdata),
        .out_bram1_wrdata(out_bram1_wrdata),
        .out_bram2_wrdata(out_bram2_wrdata),
        .out_bram3_wrdata(out_bram3_wrdata),
        .out_bram0_we(out_bram0_we),
        .out_bram1_we(out_bram1_we),
        .out_bram2_we(out_bram2_we),
        .out_bram3_we(out_bram3_we)
        );
    Out_Buffer dut2(
                    // Controler
//                    .a_bram_addr(out_bram_addr) ,
                    .a_clk(clk) ,
//                    .a_bram_rddata(out_bram_rddata) ,
//                    .a_bram_en(en) ,
                    .a_rst(reset) ,
                    // BRAM 0 to MAC
                    .bram0_wrdata(out_bram0_wrdata) ,
                    .bram0_we(out_bram0_we),
                    .bram0_addr(out_bram0_addr),
                    // BRAM 1 to MAC
                    .bram1_wrdata(out_bram1_wrdata) ,
                    .bram1_we(out_bram1_we),
                    .bram1_addr(out_bram1_addr),
                    // BRAM 2 to MAC
                    .bram2_wrdata(out_bram2_wrdata) ,
                    .bram2_we(out_bram2_we),
                    .bram2_addr(out_bram2_addr),
                    // BRAM 1 to MAC
                    .bram3_wrdata(out_bram3_wrdata) ,
                    .bram3_we(out_bram3_we),
                    .bram3_addr(out_bram3_addr));
                    
    in0_sim in0_bram0(.clk(clk), .bram_addr(in0_bram0_addr), .bram_rddata(in0_bram0_rddata));
    in0_sim in0_bram1(.clk(clk), .bram_addr(in0_bram1_addr), .bram_rddata(in0_bram1_rddata));
    in0_sim in0_bram2(.clk(clk), .bram_addr(in0_bram2_addr), .bram_rddata(in0_bram2_rddata));
    in0_sim in0_bram3(.clk(clk), .bram_addr(in0_bram3_addr), .bram_rddata(in0_bram3_rddata));
    
    in1_sim in1_bram(.clk(clk), .bram_addr(in1_bram_addr), .bram_rddata(in1_bram_rddata));

//    out_sim out_bram0(.clk(clk), .bram_addr(out_bram0_addr), .bram_wrdata(out_bram0_wrdata),.bram_we(out_bram0_we));
//    out_sim out_bram1(.clk(clk), .bram_addr(out_bram1_addr), .bram_wrdata(out_bram1_wrdata),.bram_we(out_bram1_we));
//    out_sim out_bram2(.clk(clk), .bram_addr(out_bram2_addr), .bram_wrdata(out_bram2_wrdata),.bram_we(out_bram2_we));
//    out_sim out_bram3(.clk(clk), .bram_addr(out_bram3_addr), .bram_wrdata(out_bram3_wrdata),.bram_we(out_bram3_we));                

    initial clk=0;
    always #25 clk = ~clk;

    initial begin

//    // PHASE 0
//        ps_control = 0; 
//        reset = 1;
//        #40; reset = 0;
        
//        ps_iternum = 121;
//        ps_accfreq = 4;
//        ps_phase = 0;
        
//        #10; ps_control = 1;

//        wait(pl_status[0] == 1'b1);

//        @(posedge clk);
//        #150; ps_control = 0;

//        wait(pl_status[0] == 1'b0);

//        #50000;

//    //Phase 1
//        ps_control = 0; 
//        reset = 1;
//        #40; reset = 0;
          
//        ps_iternum = 12; 
//        ps_accfreq = 484;
//        ps_phase = 1;
            
//        #10; ps_control = 1;
         
//        wait(pl_full[0] == 1'b1);
//        // make transfer...
//        #10; ps_control = 0;
//        wait(pl_full[0] == 1'b0);
//        // transfer complete
//        #50; ps_control = 1; 
        
//        wait(pl_full[0] == 1'b1);
//        // make transfer...
//        #10; ps_control = 0;
//        wait(pl_full[0] == 1'b0);
//        // transfer complete
//        #50; ps_control = 1; 
        
//        wait(pl_full[0] == 1'b1);
//        // make transfer...
//        #10; ps_control = 0;
//        wait(pl_full[0] == 1'b0);
//        // transfer complete
//        #50; ps_control = 1; 
                
//        wait(pl_full[0] == 1'b1);
//        // make transfer...
//        #10; ps_control = 0;
//        wait(pl_full[0] == 1'b0);
//        // transfer complete
//        #50; ps_control = 1;
        
//        wait(pl_full[0] == 1'b1);
//        // make transfer...
//        #10; ps_control = 0;
//        wait(pl_full[0] == 1'b0);
//        // transfer complete
//        #50; ps_control = 1; 
          
//        wait(pl_status[0] == 1'b1);
    
//        @(posedge clk);
//        #150; ps_control = 0;
    
//        wait(pl_status[0] == 1'b0);
    
//        #50000;

    //Phase 2
        ps_control = 0; 
        reset = 1;
        #40; reset = 0;
          
        ps_iternum = 5808;
        ps_accfreq = 7;
        ps_phase = 2;
            
        #10; ps_control = 1;
         
        wait(pl_full[0] == 1'b1);
        // make transfer...
        #10; ps_control = 0;
        wait(pl_full[0] == 1'b0);
        // transfer complete
        #50; ps_control = 1; 
        
        wait(pl_full[0] == 1'b1);
        // make transfer...
        #10; ps_control = 0;
        wait(pl_full[0] == 1'b0);
        // transfer complete
        #50; ps_control = 1;
        
        wait(pl_full[0] == 1'b1);
        // make transfer...
        #10; ps_control = 0;
        wait(pl_full[0] == 1'b0);
        // transfer complete
        #50; ps_control = 1;
        
        wait(pl_full[0] == 1'b1);
        // make transfer...
        #10; ps_control = 0;
        wait(pl_full[0] == 1'b0);
        // transfer complete
        #50; ps_control = 1;
        
        wait(pl_full[0] == 1'b1);
        // make transfer...
        #10; ps_control = 0;
        wait(pl_full[0] == 1'b0);
        // transfer complete
        #50; ps_control = 1;
        
        wait(pl_full[0] == 1'b1);
        // make transfer...
        #10; ps_control = 0;
        wait(pl_full[0] == 1'b0);
        // transfer complete
        #50; ps_control = 1;                       
        
        wait(pl_status[0] == 1'b1);
    
        @(posedge clk);
        #150; ps_control = 0;
    
        wait(pl_status[0] == 1'b0);
    
        #50000;

//    //Phase 3
//        ps_control = 0; 
//        reset = 1;
//        #40; reset = 0;
          
//        ps_iternum = 847; 
//        ps_accfreq = 48;
//        ps_phase = 3;
            
//        #10; ps_control = 1;
         
//        wait(pl_full[0] == 1'b1);
//        // make transfer...
//        #10; ps_control = 0;
//        wait(pl_full[0] == 1'b0);
//        // transfer complete
//        #50; ps_control = 1; 
        
//        wait(pl_full[0] == 1'b1);
//        // make transfer...
//        #10; ps_control = 0;
//        wait(pl_full[0] == 1'b0);
//        // transfer complete
//        #50; ps_control = 1; 
        
//        wait(pl_full[0] == 1'b1);
//        // make transfer...
//        #10; ps_control = 0;
//        wait(pl_full[0] == 1'b0);
//        // transfer complete
//        #50; ps_control = 1; 
                
//        wait(pl_full[0] == 1'b1);
//        // make transfer...
//        #10; ps_control = 0;
//        wait(pl_full[0] == 1'b0);
//        // transfer complete
//        #50; ps_control = 1;
        
//        // 6th time only 16 rows
//        wait(pl_full[0] == 1'b1);
//        // make transfer...
//        #10; ps_control = 0;
//        wait(pl_full[0] == 1'b0);
//        // transfer complete
//        #50; ps_control = 1; 
          
//        wait(pl_status[0] == 1'b1);
    
//        @(posedge clk);
//        #150; ps_control = 0;
    
//        wait(pl_status[0] == 1'b0);
    
//        #50000;

//    //Phase 4
//        ps_control = 0; 
//        reset = 1;
//        #40; reset = 0;
          
//        ps_iternum = 28; 
//        ps_accfreq = 121;
//        ps_phase = 4;
            
//        #10; ps_control = 1;
         
//        wait(pl_full[0] == 1'b1);
//        // make transfer...
//        #10; ps_control = 0;
//        wait(pl_full[0] == 1'b0);
//        // transfer complete
//        #50; ps_control = 1; 
        
//        wait(pl_full[0] == 1'b1);
//        // make transfer...
//        #10; ps_control = 0;
//        wait(pl_full[0] == 1'b0);
//        // transfer complete
//        #50; ps_control = 1; 
        
//        wait(pl_full[0] == 1'b1);
//        // make transfer...
//        #10; ps_control = 0;
//        wait(pl_full[0] == 1'b0);
//        // transfer complete
//        #50; ps_control = 1; 
                
//        wait(pl_full[0] == 1'b1);
//        // make transfer...
//        #10; ps_control = 0;
//        wait(pl_full[0] == 1'b0);
//        // transfer complete
//        #50; ps_control = 1;
        
//        wait(pl_full[0] == 1'b1);
//        // make transfer...
//        #10; ps_control = 0;
//        wait(pl_full[0] == 1'b0);
//        // transfer complete
//        #50; ps_control = 1; 
          
//        wait(pl_full[0] == 1'b1);
//        // make transfer...
//        #10; ps_control = 0;
//        wait(pl_full[0] == 1'b0);
//        // transfer complete
//        #50; ps_control = 1;        
                   
//        wait(pl_status[0] == 1'b1);
    
//        @(posedge clk);
//        #150; ps_control = 0;
    
//        wait(pl_status[0] == 1'b0);
    
//        #50000;

        $stop;
    end
endmodule

module in0_sim(
    input         clk,
//    input         reset,
    input        [11:0] bram_addr,
    output logic [31:0] bram_rddata
//    input        [31:0] bram_wrdata,
//    input         [3:0] bram_we
    );

    logic [1023:0][31:0] mem;

    initial begin
        integer i;
        
        for (i=0; i<847; i=i+1)begin
            mem[i] = 32'h40800000;
//            mem[i+1024] = 32'h40800000;
//            mem[i+2048] = 32'h40000000;
//            mem[i+3072] = 32'h40800000;
        end
        for (i=847; i<1024; i=i+1)begin
            mem[i] = 32'h00000000;
//            mem[i+1024] = 32'h00000000;
//            mem[i+2048] = 32'h00000000;
//            mem[i+3072] = 32'h00000000;
        end        

    end // initial

    always @(posedge clk) begin
        bram_rddata <= mem[bram_addr[11:2]];
//        if (bram_we == 4'hf)
//            mem[bram_addr[11:2]] <= bram_wrdata;
//        else if (bram_we != 0)
//            $display("ERROR: Memory simulation model only implemented we = 0 and we=4'hf. Simulation will be incorrect.");              
    end
endmodule // memory_sim

module in1_sim(
    input         clk,
//    input         reset,
    input        [11:0] bram_addr,
    output logic [31:0] bram_rddata
//    input        [31:0] bram_wrdata,
//    input         [3:0] bram_we
);

    logic [1023:0][31:0] mem;

    initial begin
        integer i;
//        for (i=0; i<48; i=i+1)begin
//            mem[i] = 32'h3F000000;
//        end
//        for (i=48; i<96; i=i+1)begin
//            mem[i] = 32'h3FC00000;
//        end
//        for (i=96; i<144; i=i+1)begin
//            mem[i] = 32'h3F000000;
//        end
//        for (i=144; i<192; i=i+1)begin
//            mem[i] = 32'h3FC00000;
//        end
//        for (i=192; i<240; i=i+1)begin
//            mem[i] = 32'h3F000000;
//        end
//        for (i=240; i<288; i=i+1)begin
//            mem[i] = 32'h3FC00000;
//        end
        for (i=0; i<168; i=i+1)begin
            mem[i] = 32'h3F800000;
        end
        for (i=168; i<336; i=i+1)begin
            mem[i] = 32'h40000000;
        end
        for (i=336; i<1024; i=i+1)begin
            mem[i] = 32'h00000000;
        end
                                
    end // initial

    always @(posedge clk) begin
        bram_rddata <= mem[bram_addr[11:2]]; // >> = 1/4
    end
endmodule // memory_sim

module out_sim(
    input         clk,
    input         reset,
    input        [11:0] bram_addr,
//    output logic [31:0] bram_rddata,
    input        [31:0] bram_wrdata,
    input         [3:0] bram_we);

    logic [1023:0][31:0] mem;

    initial begin
        integer i;
        for (i=0; i<1024; i=i+1)
            mem[i] = 32'h00000000;

    end // initial

    always @(posedge clk) begin
//        bram_rddata <= mem[bram_addr[11:2]]; // >> = 1/4
        if (bram_we == 4'hf)
            mem[bram_addr[11:2]] <= bram_wrdata;
        else if (bram_we != 0)
            $display("ERROR: Memory simulation model only implemented we = 0 and we=4'hf. Simulation will be incorrect.");              
    end
endmodule // memory_sim
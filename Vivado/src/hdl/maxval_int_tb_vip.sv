/*   
    Testbench for the entire maxval_int system, using the VIP.
            
    From "Getting Started with the Xilinx Zynq FPGA and Vivado" 
    by Peter Milder (peter.milder@stonybrook.edu)

    Copyright (C) 2020 Peter Milder

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

// Note: this doesn't seem to work right on Vivado 2017.2. I tested it successfully on 2018.2 though.

`timescale 1ns / 1ps

// This points to the intance name of your PS.
`define VIP tb.dut.design_1_i.processing_system7_0.inst

// This points to the base address of the AXI BRAM Controller. If yours is at a different address, change this:
`define BRAM0_CTRL_BASE 32'h40000000
`define BRAM1_CTRL_BASE 32'h42000000
`define BRAM2_CTRL_BASE 32'h44000000

// This points to the bram_int_max_val AXI lite interface. If yours is different, change
`define MAXVAL_BASE 32'h43C00000


// Testbench module
module tb();
     
    // Signals for our testbench's clock and reset signals
    reg clk, rstn;
    wire tmp_clk, tmp_rstn;   
    assign tmp_clk=clk;
    assign tmp_rstn = rstn;
     
    // Create the clock signal
    initial clk=0;
    always #5 clk = ~clk;

    // Signals needed for VIP commands        
    reg [31:0] resp;
    reg [31:0] read_data;

    // Variables used in testbench
    integer    i, errors;   

    // Instantiate your block design
    design_1_wrapper dut(
                         .DDR_addr(),
                         .DDR_ba(),
                         .DDR_cas_n(),
                         .DDR_ck_n(),
                         .DDR_ck_p(),
                         .DDR_cke(),
                         .DDR_cs_n(),
                         .DDR_dm(),
                         .DDR_dq(),
                         .DDR_dqs_n(),
                         .DDR_dqs_p(),
                         .DDR_odt(),
                         .DDR_ras_n(),
                         .DDR_reset_n(),
                         .DDR_we_n(),
                         .FIXED_IO_ddr_vrn(),
                         .FIXED_IO_ddr_vrp(),
                         .FIXED_IO_mio(),
                         .FIXED_IO_ps_clk(tmp_clk),
                         .FIXED_IO_ps_porb(tmp_rstn),
                         .FIXED_IO_ps_srstb(tmp_rstn));
                
    initial begin
                 
        // turn off debug messages. If you remove this, the simulation will print
        // information to the console for each VIP operation you perform
        `VIP.set_debug_level_info(0);

        // Stop simulation if there is an error
        `VIP.set_stop_on_error(1);  
    
        $display ("Starting the testbench");

        // reset the entire system
        rstn = 1'b0;
        repeat(20)@(posedge clk);  
        rstn = 1'b1;
         
        // wait a few cycles
        repeat(5)@(posedge clk);
                            
        //Reset the PL
        `VIP.fpga_soft_reset(32'h1);
        `VIP.fpga_soft_reset(32'h0);

        // wait a few cycles
        // NB: the AXI BRAM Controller simulation has problems if you don't wait a while here
        repeat(200)@(posedge clk);

        // First, let's test the memory writes and reads. We will write to all 2048 addresses and read
        // the data 2048.
        for (i=0; i<4; i=i+1) begin       
            `VIP.write_data(`BRAM0_CTRL_BASE + 4*i, 4, 32'h3F000000, resp);
        end
        @(posedge clk);
        @(posedge clk);
        for (i=1024; i<1028; i=i+1) begin       
            `VIP.write_data(`BRAM0_CTRL_BASE + 4*i, 4, 32'h3FC00000, resp);
        end
        @(posedge clk);
        @(posedge clk);
        for (i=2048; i<2052; i=i+1) begin       
            `VIP.write_data(`BRAM0_CTRL_BASE + 4*i, 4, 32'h3F000000, resp);
        end
        @(posedge clk);
        @(posedge clk);
        for (i=3072; i<3076; i=i+1) begin       
            `VIP.write_data(`BRAM0_CTRL_BASE + 4*i, 4, 32'h3FC00000, resp);
        end
        @(posedge clk);
        @(posedge clk);        
        for (i=0; i<484; i=i+1) begin       
            `VIP.write_data(`BRAM1_CTRL_BASE + 4*i, 4, 32'h40000000, resp);
        end                      
        
        $display("Done writing");

        @(posedge clk);
        @(posedge clk);
        @(posedge clk);

        // Now, check we can read the results back from the BRAM
//        errors = 0;
//        for (i=0; i<1024; i=i+1) begin
//            `VIP.read_data(`BRAM2_CTRL_BASE + i*4, 4, read_data, resp);
//            if (read_data != 32'h3F000000) begin
//                errors = errors+1;
//                $display("Error on word %d: Expected 0x%x, received 0x%x", i, 32'h3F000000, read_data);
//            end
//        end

//        if (errors == 0)
//        	$display("READ/WRITE test passed");


        // Now, let's test the max_val_int IP. 

        // We will write random data into the BRAM, but with the maximum value in the last location
        // (like in our testbench)
//        for (i=0; i<2047; i=i+1) begin       
//            `VIP.write_data(`BRAM_CTRL_BASE + 4*i, 4, $random, resp);
//        end
//        `VIP.write_data(`BRAM_CTRL_BASE + 4*2047, 4, 32'hffffffff, resp);

        // start the IP
        `VIP.write_data(`MAXVAL_BASE+4, 4, 121, resp); //1 iternum
        `VIP.write_data(`MAXVAL_BASE+8, 4, 4, resp);   //2 acc
        `VIP.write_data(`MAXVAL_BASE+12, 4, 0, resp);   //3 phase
        `VIP.write_data(`MAXVAL_BASE, 4, 1, resp);   //0

        // wait until the done signal
        // Check every five clock cycles, and timeout after 100k cycles
        `VIP.wait_reg_update(`MAXVAL_BASE+16, 1, 32'hfffffffe, 5, 100000, read_data);

        // de-assert start and wait for hardware to acknowledge it
        `VIP.write_data(`MAXVAL_BASE, 4, 0, resp);
        `VIP.wait_reg_update(`MAXVAL_BASE+16, 0, 32'hfffffffe, 5, 100000, read_data);


        // Now check that 32'hFFFFFFFF is in address 0:
        
        errors = 0;
        for (i=0; i<121; i=i+1) begin
            `VIP.read_data(`BRAM2_CTRL_BASE + i*4, 4, read_data, resp);
            if (read_data != 32'h40800000) begin
                errors = errors+1;
                $display("Error on word %d: Expected 0x%x, received 0x%x", i, 32'h40800000, read_data);
            end
        end
        for (i=1024; i<1145; i=i+1) begin
            `VIP.read_data(`BRAM2_CTRL_BASE + i*4, 4, read_data, resp);
            if (read_data != 32'h41400000) begin
                errors = errors+1;
                $display("Error on word %d: Expected 0x%x, received 0x%x", i, 32'h41400000, read_data);
            end
        end        
        for (i=2048; i<2169; i=i+1) begin
            `VIP.read_data(`BRAM2_CTRL_BASE + i*4, 4, read_data, resp);
            if (read_data != 32'h40800000) begin
                errors = errors+1;
                $display("Error on word %d: Expected 0x%x, received 0x%x", i, 32'h40800000, read_data);
            end
        end
        for (i=3072; i<3193; i=i+1) begin
            `VIP.read_data(`BRAM2_CTRL_BASE + i*4, 4, read_data, resp);
            if (read_data != 32'h41400000) begin
                errors = errors+1;
                $display("Error on word %d: Expected 0x%x, received 0x%x", i, 32'h41400000, read_data);
            end
        end          

        if (errors == 0)
            $display("READ/WRITE test passed");       
             
//        `VIP.read_data(`BRAM2_CTRL_BASE, 4, read_data, resp);
//        if (read_data != 32'hFFFFFFFF)
//            $display("ERROR");
//        else
//            $display("Test passed");

        #1000;  
        $stop;  
    end
endmodule



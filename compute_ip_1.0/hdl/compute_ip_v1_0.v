
`timescale 1 ns / 1 ps

	module compute_ip_v1_0 #
	(
		// Users to add parameters here

		// User parameters ends
		// Do not modify the parameters beyond this line


		// Parameters of Axi Slave Bus Interface S00_AXI
		parameter integer C_S00_AXI_DATA_WIDTH	= 32,
		parameter integer C_S00_AXI_ADDR_WIDTH	= 5
	)
	(
		// Users to add ports here
        input wire  [13:0] in0_addr ,
        input wire         in0_clk ,
        input wire  [31:0] in0_wrdata ,
        output wire [31:0] in0_rddata ,
        input wire         in0_en ,
        input wire         in0_rst ,
        input wire   [3:0] in0_we ,
        
        input wire  [11:0] in1_addr ,
        input wire         in1_clk ,
        input wire  [31:0] in1_wrdata ,
        output wire [31:0] in1_rddata ,
        input wire         in1_en ,
        input wire         in1_rst ,
        input wire   [3:0] in1_we ,
                
        input wire  [13:0] out_addr ,
        input wire         out_clk ,
        input wire  [31:0] out_wrdata ,
        output wire [31:0] out_rddata ,
        input wire         out_en ,
        input wire         out_rst ,
        input wire   [3:0] out_we ,
        
		// User ports ends
		// Do not modify the ports beyond this line


		// Ports of Axi Slave Bus Interface S00_AXI
		input wire  s00_axi_aclk,
		input wire  s00_axi_aresetn,
		input wire [C_S00_AXI_ADDR_WIDTH-1 : 0] s00_axi_awaddr,
		input wire [2 : 0] s00_axi_awprot,
		input wire  s00_axi_awvalid,
		output wire  s00_axi_awready,
		input wire [C_S00_AXI_DATA_WIDTH-1 : 0] s00_axi_wdata,
		input wire [(C_S00_AXI_DATA_WIDTH/8)-1 : 0] s00_axi_wstrb,
		input wire  s00_axi_wvalid,
		output wire  s00_axi_wready,
		output wire [1 : 0] s00_axi_bresp,
		output wire  s00_axi_bvalid,
		input wire  s00_axi_bready,
		input wire [C_S00_AXI_ADDR_WIDTH-1 : 0] s00_axi_araddr,
		input wire [2 : 0] s00_axi_arprot,
		input wire  s00_axi_arvalid,
		output wire  s00_axi_arready,
		output wire [C_S00_AXI_DATA_WIDTH-1 : 0] s00_axi_rdata,
		output wire [1 : 0] s00_axi_rresp,
		output wire  s00_axi_rvalid,
		input wire  s00_axi_rready
	);
	
wire [31:0] ps_control, pl_status, ps_accfreq, ps_iternum, pl_full, ps_phase ;
	
// Instantiation of Axi Bus Interface S00_AXI
	compute_ip_v1_0_S00_AXI # ( 
		.C_S_AXI_DATA_WIDTH(C_S00_AXI_DATA_WIDTH),
		.C_S_AXI_ADDR_WIDTH(C_S00_AXI_ADDR_WIDTH)
	) compute_ip_v1_0_S00_AXI_inst (
		.S_AXI_ACLK(s00_axi_aclk),
		.S_AXI_ARESETN(s00_axi_aresetn),
		.S_AXI_AWADDR(s00_axi_awaddr),
		.S_AXI_AWPROT(s00_axi_awprot),
		.S_AXI_AWVALID(s00_axi_awvalid),
		.S_AXI_AWREADY(s00_axi_awready),
		.S_AXI_WDATA(s00_axi_wdata),
		.S_AXI_WSTRB(s00_axi_wstrb),
		.S_AXI_WVALID(s00_axi_wvalid),
		.S_AXI_WREADY(s00_axi_wready),
		.S_AXI_BRESP(s00_axi_bresp),
		.S_AXI_BVALID(s00_axi_bvalid),
		.S_AXI_BREADY(s00_axi_bready),
		.S_AXI_ARADDR(s00_axi_araddr),
		.S_AXI_ARPROT(s00_axi_arprot),
		.S_AXI_ARVALID(s00_axi_arvalid),
		.S_AXI_ARREADY(s00_axi_arready),
		.S_AXI_RDATA(s00_axi_rdata),
		.S_AXI_RRESP(s00_axi_rresp),
		.S_AXI_RVALID(s00_axi_rvalid),
		.S_AXI_RREADY(s00_axi_rready),
		.pl_status ( pl_status ),
		.ps_control ( ps_control ),
        .ps_accfreq ( ps_accfreq ),
        .ps_iternum ( ps_iternum ),
        .pl_full ( pl_full ),
        .ps_phase ( ps_phase )
	);

	// Add user logic here
	
        wire [11:0] in0_bram0_addr, in0_bram1_addr, in0_bram2_addr, in0_bram3_addr, in1_bram_addr;
        wire [31:0] in0_bram0_rddata, in0_bram1_rddata, in0_bram2_rddata, in0_bram3_rddata, in1_bram_rddata;
        wire [11:0] out_bram0_addr, out_bram1_addr, out_bram2_addr, out_bram3_addr;
        wire [31:0] out_bram0_wrdata, out_bram1_wrdata, out_bram2_wrdata, out_bram3_wrdata;
        wire [3:0]  out_bram0_we, out_bram1_we, out_bram2_we, out_bram3_we;
        wire rst;
	    wire [31:0] bram_wrdata, bram_rddata0, bram_rddata1, bram_rddata2, bram_rddata3 ;
	    wire [3:0]  bram_we;
	
	assign bram_wrdata = 0;
	assign bram_we = 4'h0;
	assign rst = ~s00_axi_aresetn;
	
 driver d(
        .clk(s00_axi_aclk),
        .reset(rst),
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
        
    In0_Buffer in0_buffer(
            // Controler
            .a_addr(in0_addr) ,
            .a_clk(in0_clk) ,
            .a_wrdata(in0_wrdata) ,
            .a_rddata(in0_rddata) ,
            .a_en(in0_en) ,
            .a_rst(in0_rst) ,
            .a_we(in0_we),
            // BRAM 0 to MAC
            .bram0_wrdata(bram_wrdata) ,
            .bram0_rddata(in0_bram0_rddata) ,
            .bram0_we(bram_we),
            .bram0_addr(in0_bram0_addr),
            // BRAM 1 to MAC
            .bram1_wrdata(bram_wrdata) ,
            .bram1_rddata(in0_bram1_rddata) ,
            .bram1_we(bram_we),
            .bram1_addr(in0_bram1_addr),
            // BRAM 2 to MAC
            .bram2_wrdata(bram_wrdata) ,
            .bram2_rddata(in0_bram2_rddata) ,
            .bram2_we(bram_we),
            .bram2_addr(in0_bram2_addr),
            // BRAM 1 to MAC
            .bram3_wrdata(bram_wrdata) ,
            .bram3_rddata(in0_bram3_rddata) ,
            .bram3_we(bram_we),
            .bram3_addr(in0_bram3_addr)
            );        
        
     In1_Buffer in1_buffer(
                .a_addr(in1_addr) ,
                .a_clk(in1_clk) ,
                .a_wrdata(in1_wrdata) ,
                .a_rddata(in1_rddata) ,
                .a_en(in1_en) ,
                .a_rst(in1_rst) ,
                .a_we(in1_we),
                // BRAM 0 to MAC
                .bram_wrdata(bram_wrdata) ,
                .bram_rddata(in1_bram_rddata) ,
                .bram_we(bram_we),
                .bram_addr(in1_bram_addr)
                );  
                
        
      Out_Buffer out_buffer(
                // Controler
                .a_addr(out_addr) ,
                .a_clk(out_clk) ,
                .a_wrdata(out_wrdata) ,
                .a_rddata(out_rddata) ,
                .a_en(out_en) ,
                .a_rst(out_rst) ,
                .a_we(out_we),
                // BRAM 0 to MAC
                .bram0_wrdata(out_bram0_wrdata),
                .bram0_rddata(bram_rddata0) ,
                .bram0_we(out_bram0_we),
                .bram0_addr(out_bram0_addr),
                // BRAM 1 to MAC
                .bram1_wrdata(out_bram1_wrdata),
                .bram1_rddata(bram_rddata1) ,
                .bram1_we(out_bram1_we),
                .bram1_addr(out_bram1_addr),
                // BRAM 2 to MAC
                .bram2_wrdata(out_bram2_wrdata),
                .bram2_rddata(bram_rddata2) ,
                .bram2_we(out_bram2_we),
                .bram2_addr(out_bram2_addr),
                // BRAM 1 to MAC
                .bram3_wrdata(out_bram3_wrdata),
                .bram3_rddata(bram_rddata3) ,
                .bram3_we(out_bram3_we),
                .bram3_addr(out_bram3_addr)
                );                
           
	// User logic ends

	endmodule

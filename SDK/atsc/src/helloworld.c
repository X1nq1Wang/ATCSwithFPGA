/*
 * URECA_test.c: URECA program test
 * Last updated by Xinqi Wang on 8/14/2020
 *
 * This application configures UART 16550 to baud rate 9600.
 * PS7 UART (Zynq) is not initialized by this application, since
 * bootrom/bsp configures it to baud rate 115200
 *
 * ------------------------------------------------
 * | UART TYPE   BAUD RATE                        |
 * ------------------------------------------------
 *   uartns550   9600
 *   uartlite    Configurable only in HW design
 *   ps7_uart    115200 (configured by bootrom/bsp)
 */

#include <stdio.h>
#include <stdlib.h>
#include "platform.h"
#include "xil_printf.h"
#include "xtime_l.h"

#include "xscugic.h" // interrupt handler
#include "xdmaps.h"  // DMA driver

#define P0IN1SIZE 1024

#define TMRCTR_DEVICE_ID        XPAR_TMRCTR_0_DEVICE_ID
#define TIMEOUT_LIMIT           0x2000

int SetupInterruptSystem(XScuGic *GicPtr, XDmaPs *DmaPtr);
void DmaDoneHandler(unsigned int Channel, XDmaPs_Cmd *DmaCmd, void *CallbackRef);
int checkCorrectness(int size, int* src, int* dst);

XDmaPs DmaInstance;
XScuGic GicInstance;

float in1Buff[P0IN1SIZE]__attribute__((aligned(4096)));
float in0Buff[4][P0IN1SIZE]__attribute__((aligned(4096)));
float outBuff[4][P0IN1SIZE]__attribute__((aligned(4096)));

int main()
{
		init_platform();

    	print("-------------- Starting BRAM Test ------------\r\n");

        volatile unsigned int* in0_bram = (unsigned int*) XPAR_AXI_BRAM_CTRL_0_S_AXI_BASEADDR;
        volatile unsigned int* in1_bram = (unsigned int*) XPAR_AXI_BRAM_CTRL_1_S_AXI_BASEADDR;
        volatile unsigned int* out_bram = (unsigned int*) XPAR_AXI_BRAM_CTRL_2_S_AXI_BASEADDR;
        volatile unsigned int* hw_flag = (unsigned int*) XPAR_COMPUTE_IP_0_S00_AXI_BASEADDR;
        int i, j, b, n, k, r, k1, k2, c;
        float b1[4];
        float b2[48];

        for(i=0; i<4; i++)
        	b1[i]= 0.2;
        for(i=0; i<48; i++)
        	b2[i]= 0.1;

        for(b=0; b<4096; b++){
        	in0_bram[b] = 0x00000000;
        }
        for(b=0; b<1024; b++){
        	in1_bram[b] = 0x00000000;
        }

        //  ps_control = slv_reg0 ;
        //  ps_iternum = slv_reg1 ;
        //  ps_accfreq = slv_reg2 ;
        //  ps_phase = slv_reg3 ;
        //  pl_status = slv_reg4 ;
        //  pl_full = slv_reg5 ;

        // initialize input x matrix [7][144]
        float x[7][144];
        for (i=0; i<7; i++)
        	for(j=0; j<144; j++)
        		x[i][j] = 2;
        // initialize input w1 matrix 4x4
        float w1[4][4];
//        for (i=0; i<4; i++)
        	for(j=0; j<4; j++){
        		w1[0][j] = 1;  //0.5
        		w1[1][j] = 1;  //1
        		w1[2][j] = 1;
        		w1[3][j] = 1;
        	}
        // initialize output y matrix [7][484]
//        xil_printf("setup y\r\n");
        float y[7][484];
        // initialize input w2 matrix [48][484]
        float w2[48][484];
        for (i=0; i<48; i++){
        	for(j=0; j<484; j++){
        			w2[i][j] = 0.2;  //2
        	}
        }
        // initialize output z matrix [7][48]
        float z[7][48];
        // initialize other input0 y[7][484]
        for (i = 1; i<7; i++){
        	for(j=0; j<484; j++)
                	y[i][j] = 8.2;
        }
        // initialize input1 dz[7][48]
        float dz[7][48];
        for (i = 0; i<7; i++){
        	for(j=0; j<8; j++)
        		dz[i][j] = 2;
        	for(j=8; j<16; j++)
        		dz[i][j] = 1;
        	for(j=16; j<24; j++)
        		dz[i][j] = 2;
        	for(j=24; j<32; j++)
        		dz[i][j] = 1;
        	for(j=32; j<40; j++)
        		dz[i][j] = 2;
        	for(j=40; j<48; j++)
        		dz[i][j] = 1;
        }
        // initialize output dw2 [48][484]
        float dw2[48][484];
        // initialize output dy [7][484]
        float dy[7][484];
    	// initialize output dw1 [7][4][4]
    	float dw1[4][4];
//    	for(c=0; c<7; c++){
    		for(i=0; i<4; i++){
    			for(j=0; j<4; j++)
    				dw1[i][j] = 0;
    		}
//    	}

    	XTime start, end, in0_t, in1_t;

    	// Configure DMA and its driver
    	u16 DeviceId = XPAR_XDMAPS_1_DEVICE_ID;
    	XDmaPs_Config *DmaCfg;
    	XDmaPs *DmaInst = &DmaInstance;
    	XDmaPs_Cmd DmaCmd;

    	// First, we will use the PS DMA to transfer data from txBuff to bram.
    	volatile int* txDone = malloc(sizeof(int));
    	*txDone = 0;

    	//Phase 0
        xil_printf("------------------------Phase 0------------------------------\r\n");
        // set up
        xil_printf("*****setup*****\r\n");
        hw_flag[1] = 121; //ps_iternum
        hw_flag[2] = 4;   //ps_accfreq
        hw_flag[3] = 0;   //ps_phase
        // load input buffers
        xil_printf("*****setup in1*****\r\n");
        XTime_GetTime(&start);
        b = 0;
        for(i=0; i<=120; i=i+12){
        	for(j=0; j<11; j++){
        		for(k1=0; k1<24; k1=k1+12){
        			for(k2=0; k2<2; k2++){
        				in1Buff[b] = x[0][i+j+k1+k2];
        				//xil_printf("in1_bram[%d] = x[0][%d] = 0x%x\r\n", b, i+j+k1+k2, in1Buff[b]);
        				b = b+1;;
        			}
        		}
        	}
        }
        XTime_GetTime(&end);
        printf("In_1 took %llu clock cycles.\n", 2*(end - start));
        printf("In_1 took %.2f us.\n",
        		1.0 * (end - start) / (COUNTS_PER_SECOND/1000000));

        XTime_GetTime(&start);
        memset(&DmaCmd, 0, sizeof(XDmaPs_Cmd));
        DmaCmd.ChanCtrl.SrcBurstSize = 4;
        DmaCmd.ChanCtrl.SrcBurstLen = 4;
        DmaCmd.ChanCtrl.SrcInc = 1;
        DmaCmd.ChanCtrl.DstBurstSize = 4;
        DmaCmd.ChanCtrl.DstBurstLen = 4;
        DmaCmd.ChanCtrl.DstInc = 1;
        DmaCmd.BD.SrcAddr = (u32) in1Buff;  // source = txBuff
        DmaCmd.BD.DstAddr = (u32) in1_bram;    // destination = bram
        DmaCmd.BD.Length = 484 * sizeof(int);   // length of data to transfer

        // Initialize DMA driver
        DmaCfg = XDmaPs_LookupConfig(DeviceId);
        if (DmaCfg == NULL) {
        	return XST_FAILURE;
        }
        int Status = XDmaPs_CfgInitialize(DmaInst, DmaCfg, DmaCfg->BaseAddress);
        if (Status != XST_SUCCESS) {
        	return XST_FAILURE;
        }
        // Setup interrupts
        Status = SetupInterruptSystem(&GicInstance, DmaInst);
        if (Status != XST_SUCCESS) {
        	return XST_FAILURE;
        }
        // Enable the interrupt handler
        XDmaPs_SetDoneHandler(DmaInst, 0, DmaDoneHandler, (void *)txDone);

        // Start the DMA
        Status = XDmaPs_Start(DmaInst, 0, &DmaCmd, 0);
        if (Status != XST_SUCCESS) {
        	return XST_FAILURE;
        }
        int TimeOutCnt=0;
        while (!(*txDone) && TimeOutCnt < TIMEOUT_LIMIT) {
        	TimeOutCnt++;
        }
        if (TimeOutCnt >= TIMEOUT_LIMIT) {
        	printf("timeout\r\n");
        	return XST_FAILURE;
        }
        XTime_GetTime(&end);
        printf("DMA took %llu clock cycles.\n", 2*(end - start));
        printf("DMA took %.2f us.\n",
        		1.0 * (end - start) / (COUNTS_PER_SECOND/1000000));

        xil_printf("*****setup in0*****\r\n");
        XTime_GetTime(&start);
        b = 0;
        for(i=0; i<4; i++){
        	for(j=0; j<4; j++){
        		in0_bram[b] = *((uint32_t*)&w1[i][j]);
        		b = b+1;
        	}
        	b = b + 1020;
        }
        XTime_GetTime(&end);
        printf("In0 took %llu clock cycles.\n", 2*(end - start));
        printf("In0 took %.2f us.\n",
        		1.0 * (end - start) / (COUNTS_PER_SECOND/1000000));

        // ps_control = 1
        xil_printf("ps_control=1\r\n");
        XTime_GetTime(&start);
        hw_flag[0] = 1;
        while((hw_flag[4] & 0x0) == 1){ ; }
//        xil_printf("pl_status=1\r\n");
        hw_flag[0] = 0;
        while((hw_flag[4] & 0x1) == 1){ ; }
        XTime_GetTime(&end);
        printf("Compute took %llu clock cycles.\n", 2*(end - start));
        printf("Compute took %.2f us.\n",
        		1.0 * (end - start) / (COUNTS_PER_SECOND/1000000));

        // Get output
        xil_printf("*****output*****\r\n");
        XTime_GetTime(&in0_t);
        *txDone = 0;
        DmaCmd.BD.SrcAddr = (u32) out_bram;
        DmaCmd.BD.DstAddr = (u32) (outBuff[0]);
        DmaCmd.BD.Length = 121 * sizeof(int);

        // Start the DMA
        Status = XDmaPs_Start(DmaInst, 0, &DmaCmd, 0);
        if (Status != XST_SUCCESS) {
        	return XST_FAILURE;
        }

        // Loop until the DMA is done --  txDone will be set in interrupt handler
        TimeOutCnt=0;
        while (!(*txDone) && TimeOutCnt < TIMEOUT_LIMIT) {
        	TimeOutCnt++;
        }

        if (TimeOutCnt >= TIMEOUT_LIMIT) {
        	printf("timeout\r\n");
        	return XST_FAILURE;
        }

        *txDone = 0;
        DmaCmd.BD.SrcAddr = (u32) (out_bram+1024);
        DmaCmd.BD.DstAddr = (u32) (outBuff[1]);

        // Start the DMA
        Status = XDmaPs_Start(DmaInst, 0, &DmaCmd, 0);
        if (Status != XST_SUCCESS) {
        	return XST_FAILURE;
        }

        // Loop until the DMA is done --  txDone will be set in interrupt handler
        TimeOutCnt=0;
        while (!(*txDone) && TimeOutCnt < TIMEOUT_LIMIT) {
        	TimeOutCnt++;
        }

        if (TimeOutCnt >= TIMEOUT_LIMIT) {
        	printf("timeout\r\n");
        	return XST_FAILURE;
        }

        *txDone = 0;
        DmaCmd.BD.SrcAddr = (u32) (out_bram+2048);
        DmaCmd.BD.DstAddr = (u32) (outBuff[2]);

        // Start the DMA
        Status = XDmaPs_Start(DmaInst, 0, &DmaCmd, 0);
        if (Status != XST_SUCCESS) {
        	return XST_FAILURE;
        }

        // Loop until the DMA is done --  txDone will be set in interrupt handler
        TimeOutCnt=0;
        while (!(*txDone) && TimeOutCnt < TIMEOUT_LIMIT) {
        	TimeOutCnt++;
        }

        if (TimeOutCnt >= TIMEOUT_LIMIT) {
        	printf("timeout\r\n");
        	return XST_FAILURE;
        }

        *txDone = 0;
        DmaCmd.BD.SrcAddr = (u32) (out_bram+3072);
        DmaCmd.BD.DstAddr = (u32) (outBuff[3]);

        // Start the DMA
        Status = XDmaPs_Start(DmaInst, 0, &DmaCmd, 0);
        if (Status != XST_SUCCESS) {
        	return XST_FAILURE;
        }

        // Loop until the DMA is done --  txDone will be set in interrupt handler
        TimeOutCnt=0;
        while (!(*txDone) && TimeOutCnt < TIMEOUT_LIMIT) {
        	TimeOutCnt++;
        }

        if (TimeOutCnt >= TIMEOUT_LIMIT) {
        	printf("timeout\r\n");
        	return XST_FAILURE;
        }
        XTime_GetTime(&in1_t);
        printf("DMA took %llu clock cycles.\n", 2*(in1_t - in0_t));
        printf("DMA took %.2f us.\n",
        		1.0 * (in1_t - in0_t) / (COUNTS_PER_SECOND/1000000));

        // Print Output
        XTime_GetTime(&start);
        b=0;
        for(i=0; i<4;i++){
        	for(j=0; j<121; j++){
        		y[0][b] = outBuff[i][j] + b1[i];
        		if(y[0][b] < 0)
        			y[0][b] = 0;
//        		printf("y[0][%d] = %.3f = outBuff[%d][%d] + b1[%d] = 0x%x\r\n", b, y[0][b], i, j, i, *((uint32_t*)&y[0][b]));
        		b = b+1;
        	}
        }
        XTime_GetTime(&end);
        printf("Output took %llu clock cycles.\n", 2*(end - start));
        printf("Output took %.2f us.\n",
        		1.0 * (end - start) / (COUNTS_PER_SECOND/1000000));

        // Phase 1
        xil_printf("------------------------Phase 1------------------------------\r\n");
        // set up
        xil_printf("*****setup*****\r\n");
        hw_flag[1] = 12; //ps_iternum
        hw_flag[2] = 484;   //ps_accfreq
        hw_flag[3] = 1;   //ps_phase
        // load input buffers
        xil_printf("*****setup in1*****\r\n");
        XTime_GetTime(&start);
        b = 0;
        for(i=0; i<484; i++){
        	in1Buff[b] = y[0][i];
//                		xil_printf("in1_bram[%d] = y[0][%d] = 0x%x\r\n", b, i, in1_bram[b]);
        	b = b+1;
        }
        XTime_GetTime(&end);
        printf("In1 took %llu clock cycles.\n", 2*(end - start));
        printf("In1 took %.2f us.\n",
        		1.0 * (end - start) / (COUNTS_PER_SECOND/1000000));

        XTime_GetTime(&in0_t);
        *txDone = 0;

        DmaCmd.BD.SrcAddr = (u32) in1Buff;  // source = txBuff
        DmaCmd.BD.DstAddr = (u32) in1_bram;    // destination = bram
        DmaCmd.BD.Length = 484 * sizeof(int);   // length of data to transfer

        // Start the DMA
        Status = XDmaPs_Start(DmaInst, 0, &DmaCmd, 0);
        if (Status != XST_SUCCESS) {
        	return XST_FAILURE;
        }
        TimeOutCnt=0;
        while (!(*txDone) && TimeOutCnt < TIMEOUT_LIMIT) {
        	TimeOutCnt++;
        }
        if (TimeOutCnt >= TIMEOUT_LIMIT) {
        	printf("timeout\r\n");
        	return XST_FAILURE;
        }
        XTime_GetTime(&in1_t);
        printf("DMA took %llu clock cycles.\n", 2*(in1_t - in0_t));
        printf("DMA took %.2f us.\n",
        		1.0 * (in1_t - in0_t) / (COUNTS_PER_SECOND/1000000));

        xil_printf("*****setup in0*****\r\n");
        XTime_GetTime(&start);
        b = 0;
        for(j=0; j<2; j++){
        	for(k=0; k<484; k++){
        		in0Buff[0][b] = w2[j][k];
        		in0Buff[1][b] = w2[j+12][k];
        		in0Buff[2][b] = w2[j+24][k];
        		in0Buff[3][b] = w2[j+36][k];
        		b = b+1;
        	}
        }
        XTime_GetTime(&end);
        printf("In0 took %llu clock cycles.\n", 2*(end - start));
        printf("In0 took %.2f us.\n",
        		1.0 * (end - start) / (COUNTS_PER_SECOND/1000000));

        XTime_GetTime(&in0_t);
        *txDone = 0;
        DmaCmd.BD.SrcAddr = (u32) in0Buff;  // source = txBuff
        DmaCmd.BD.DstAddr = (u32) in0_bram;    // destination = bram
        DmaCmd.BD.Length = 968 * sizeof(int);   // length of data to transfer

        // Start the DMA
        Status = XDmaPs_Start(DmaInst, 0, &DmaCmd, 0);
        if (Status != XST_SUCCESS) {
        	return XST_FAILURE;
        }
        TimeOutCnt=0;
        while (!(*txDone) && TimeOutCnt < TIMEOUT_LIMIT) {
        	TimeOutCnt++;
        }
        if (TimeOutCnt >= TIMEOUT_LIMIT) {
        	printf("timeout\r\n");
        	return XST_FAILURE;
        }

        *txDone = 0;
        DmaCmd.BD.SrcAddr = (u32) (in0Buff[1]);  // source = txBuff
        DmaCmd.BD.DstAddr = (u32) (in0_bram+1024);    // destination = bram

        // Start the DMA
        Status = XDmaPs_Start(DmaInst, 0, &DmaCmd, 0);
        if (Status != XST_SUCCESS) {
        	return XST_FAILURE;
        }
        TimeOutCnt=0;
        while (!(*txDone) && TimeOutCnt < TIMEOUT_LIMIT) {
        	TimeOutCnt++;
        }
        if (TimeOutCnt >= TIMEOUT_LIMIT) {
        	printf("timeout\r\n");
        	return XST_FAILURE;
        }

        *txDone = 0;
        DmaCmd.BD.SrcAddr = (u32) (in0Buff[2]);  // source = txBuff
        DmaCmd.BD.DstAddr = (u32) (in0_bram+2048);    // destination = bram

        // Start the DMA
        Status = XDmaPs_Start(DmaInst, 0, &DmaCmd, 0);
        if (Status != XST_SUCCESS) {
        	return XST_FAILURE;
        }
        TimeOutCnt=0;
        while (!(*txDone) && TimeOutCnt < TIMEOUT_LIMIT) {
        	TimeOutCnt++;
        }
        if (TimeOutCnt >= TIMEOUT_LIMIT) {
        	printf("timeout\r\n");
        	return XST_FAILURE;
        }

        *txDone = 0;
        DmaCmd.BD.SrcAddr = (u32) (in0Buff[3]);  // source = txBuff
        DmaCmd.BD.DstAddr = (u32) (in0_bram+3072);    // destination = bram

        // Start the DMA
        Status = XDmaPs_Start(DmaInst, 0, &DmaCmd, 0);
        if (Status != XST_SUCCESS) {
        	return XST_FAILURE;
        }
        TimeOutCnt=0;
        while (!(*txDone) && TimeOutCnt < TIMEOUT_LIMIT) {
        	TimeOutCnt++;
        }
        if (TimeOutCnt >= TIMEOUT_LIMIT) {
        	printf("timeout\r\n");
        	return XST_FAILURE;
        }
        XTime_GetTime(&in1_t);
        printf("dma took %llu clock cycles.\n", 2*(in1_t - in0_t));
        printf("dma took %.2f us.\n",
        		1.0 * (in1_t - in0_t) / (COUNTS_PER_SECOND/1000000));

        // ps_control = 1
        xil_printf("ps_control=1\r\n");
        XTime_GetTime(&start);
        hw_flag[0] = 1;
        for(r=2; r<12; r=r+2){
        	while((hw_flag[5] & 0x0) == 1){ ; }
            XTime_GetTime(&end);
            printf("compute took %llu clock cycles.\n", 2*(end - start));
            printf("compute took %.2f us.\n",
            		1.0 * (end - start) / (COUNTS_PER_SECOND/1000000));

        	xil_printf("-----Make transfer-----\r\n");
            XTime_GetTime(&start);
        	b=0;
//        	for(i=r; i<r+4; i++){
        		for(j=r; j<2; j++){
        			for(k=0; k<484; k++){
                		in0Buff[0][b] = w2[j][k];
                		in0Buff[1][b] = w2[j+12][k];
                		in0Buff[2][b] = w2[j+24][k];
                		in0Buff[3][b] = w2[j+36][k];
                		b = b+1;
        			}
        		}
            XTime_GetTime(&end);
            printf("transfer took %llu clock cycles.\n", 2*(end - start));
            printf("transfer took %.2f us.\n",
            		1.0 * (end - start) / (COUNTS_PER_SECOND/1000000));

            XTime_GetTime(&in0_t);
            *txDone = 0;
            DmaCmd.BD.SrcAddr = (u32) in0Buff;  // source = txBuff
            DmaCmd.BD.DstAddr = (u32) in0_bram;    // destination = bram

            // Start the DMA
            Status = XDmaPs_Start(DmaInst, 0, &DmaCmd, 0);
            if (Status != XST_SUCCESS) {
            	return XST_FAILURE;
            }
            TimeOutCnt=0;
            while (!(*txDone) && TimeOutCnt < TIMEOUT_LIMIT) {
            	TimeOutCnt++;
            }
            if (TimeOutCnt >= TIMEOUT_LIMIT) {
            	printf("timeout\r\n");
            	return XST_FAILURE;
            }

            *txDone = 0;
            DmaCmd.BD.SrcAddr = (u32) (in0Buff[1]);  // source = txBuff
            DmaCmd.BD.DstAddr = (u32) (in0_bram+1024);    // destination = bram

            // Start the DMA
            Status = XDmaPs_Start(DmaInst, 0, &DmaCmd, 0);
            if (Status != XST_SUCCESS) {
            	return XST_FAILURE;
            }
            TimeOutCnt=0;
            while (!(*txDone) && TimeOutCnt < TIMEOUT_LIMIT) {
            	TimeOutCnt++;
            }
            if (TimeOutCnt >= TIMEOUT_LIMIT) {
            	printf("timeout\r\n");
            	return XST_FAILURE;
            }

            *txDone = 0;
            DmaCmd.BD.SrcAddr = (u32) (in0Buff[2]);  // source = txBuff
            DmaCmd.BD.DstAddr = (u32) (in0_bram+2048);    // destination = bram

            // Start the DMA
            Status = XDmaPs_Start(DmaInst, 0, &DmaCmd, 0);
            if (Status != XST_SUCCESS) {
            	return XST_FAILURE;
            }
            TimeOutCnt=0;
            while (!(*txDone) && TimeOutCnt < TIMEOUT_LIMIT) {
            	TimeOutCnt++;
            }
            if (TimeOutCnt >= TIMEOUT_LIMIT) {
            	printf("timeout\r\n");
            	return XST_FAILURE;
            }

            *txDone = 0;
            DmaCmd.BD.SrcAddr = (u32) (in0Buff[3]);  // source = txBuff
            DmaCmd.BD.DstAddr = (u32) (in0_bram+3072);    // destination = bram

            // Start the DMA
            Status = XDmaPs_Start(DmaInst, 0, &DmaCmd, 0);
            if (Status != XST_SUCCESS) {
            	return XST_FAILURE;
            }
            TimeOutCnt=0;
            while (!(*txDone) && TimeOutCnt < TIMEOUT_LIMIT) {
            	TimeOutCnt++;
            }
            if (TimeOutCnt >= TIMEOUT_LIMIT) {
            	printf("timeout\r\n");
            	return XST_FAILURE;
            }
            XTime_GetTime(&in1_t);
            printf("DMA took %llu clock cycles.\n", 2*(in1_t - in0_t));
            printf("DMA took %.2f us.\n",
            		1.0 * (in1_t - in0_t) / (COUNTS_PER_SECOND/1000000));

        	hw_flag[0] = 0;
        	while((hw_flag[5] & 0x1) == 1){ ; }
        	hw_flag[0] = 1;
            XTime_GetTime(&start);
        }


//        xil_printf("wait for pl_status=1\r\n");
        while((hw_flag[4] & 0x1) == 0){ ; }
//        xil_printf("pl_status=1\r\n");
        hw_flag[0] = 0;
        while((hw_flag[4] & 0x1) == 1){ ; }
        XTime_GetTime(&end);
        printf("compute took %llu clock cycles.\n", 2*(end - start));
        printf("compute took %.2f us.\n",
        		1.0 * (end - start) / (COUNTS_PER_SECOND/1000000));

        // Print Output
        xil_printf("*****output*****\r\n");
        XTime_GetTime(&start);
        b=0;
        for(i=0; i<4096;i=i+1024){
        	for(j=i; j<i+12; j++){
        		z[0][b] = *((float*)&out_bram[j]) + b2[b];
        		if(z[0][b] < 0)
        			z[0][b] = 0;
        		printf("z[0][%d] = out_bram[%d]  + b2[%d] = %.3f\r\n", b, j, b, z[0][b]);
        		b=b+1;
        	}
        }
        XTime_GetTime(&end);
        printf("Output took %llu clock cycles.\n", 2*(end - start));
        printf("Output took %.2f us.\n",
        		1.0 * (end - start) / (COUNTS_PER_SECOND/1000000));


        // Phase 2
        xil_printf("------------------------Phase 2------------------------------\r\n");
        // set up
        xil_printf("*****setup*****\r\n");
        hw_flag[1] = 5808; //ps_iternum
        hw_flag[2] = 7;   //ps_accfreq
        hw_flag[3] = 2;   //ps_phase

        //load input buffers
        xil_printf("*****setup in0*****\r\n");
        XTime_GetTime(&start);
        b = 0;
        for(k=0; k<4; k++){
        	for(j=k; j<k+484; j=j+4){
        		for(i=0; i<7; i++){
        			in0Buff[k][b] = y[i][j];
        			b = b+1;
        		}
        	}
        	b=0;
        }
        XTime_GetTime(&end);
        printf("In0 took %llu clock cycles.\n", 2*(end - start));
        printf("In0 took %.2f us.\n",
        		1.0 * (end - start) / (COUNTS_PER_SECOND/1000000));

        XTime_GetTime(&in0_t);
        *txDone = 0;
        DmaCmd.BD.SrcAddr = (u32) in0Buff;  // source = txBuff
        DmaCmd.BD.DstAddr = (u32) in0_bram;    // destination = bram
        DmaCmd.BD.Length = 847 * sizeof(int);   // length of data to transfer

        // Start the DMA
        Status = XDmaPs_Start(DmaInst, 0, &DmaCmd, 0);
        if (Status != XST_SUCCESS) {
        	return XST_FAILURE;
        }
        TimeOutCnt=0;
        while (!(*txDone) && TimeOutCnt < TIMEOUT_LIMIT) {
        	TimeOutCnt++;
        }
        if (TimeOutCnt >= TIMEOUT_LIMIT) {
        	printf("timeout\r\n");
        	return XST_FAILURE;
        }

        *txDone = 0;
        DmaCmd.BD.SrcAddr = (u32) (in0Buff[1]);  // source = txBuff
        DmaCmd.BD.DstAddr = (u32) (in0_bram+1024);    // destination = bram

        // Start the DMA
        Status = XDmaPs_Start(DmaInst, 0, &DmaCmd, 0);
        if (Status != XST_SUCCESS) {
        	return XST_FAILURE;
        }
        TimeOutCnt=0;
        while (!(*txDone) && TimeOutCnt < TIMEOUT_LIMIT) {
        	TimeOutCnt++;
        }
        if (TimeOutCnt >= TIMEOUT_LIMIT) {
        	printf("timeout\r\n");
        	return XST_FAILURE;
        }

        *txDone = 0;
        DmaCmd.BD.SrcAddr = (u32) (in0Buff[2]);  // source = txBuff
        DmaCmd.BD.DstAddr = (u32) (in0_bram+2048);    // destination = bram

        // Start the DMA
        Status = XDmaPs_Start(DmaInst, 0, &DmaCmd, 0);
        if (Status != XST_SUCCESS) {
        	return XST_FAILURE;
        }
        TimeOutCnt=0;
        while (!(*txDone) && TimeOutCnt < TIMEOUT_LIMIT) {
        	TimeOutCnt++;
        }
        if (TimeOutCnt >= TIMEOUT_LIMIT) {
        	printf("timeout\r\n");
        	return XST_FAILURE;
        }

        *txDone = 0;
        DmaCmd.BD.SrcAddr = (u32) (in0Buff[3]);  // source = txBuff
        DmaCmd.BD.DstAddr = (u32) (in0_bram+3072);    // destination = bram

        // Start the DMA
        Status = XDmaPs_Start(DmaInst, 0, &DmaCmd, 0);
        if (Status != XST_SUCCESS) {
        	return XST_FAILURE;
        }
        TimeOutCnt=0;
        while (!(*txDone) && TimeOutCnt < TIMEOUT_LIMIT) {
        	TimeOutCnt++;
        }
        if (TimeOutCnt >= TIMEOUT_LIMIT) {
        	printf("timeout\r\n");
        	return XST_FAILURE;
        }
        XTime_GetTime(&in1_t);
        printf("dma took %llu clock cycles.\n", 2*(in1_t - in0_t));
        printf("dma took %.2f us.\n",
        		1.0 * (in1_t - in0_t) / (COUNTS_PER_SECOND/1000000));

        xil_printf("*****setup in1*****\r\n");
        XTime_GetTime(&in0_t);
        b=0;
        for(j=0; j<48; j++){
        	for(i=0; i<7; i++){
        		in1Buff[b] = dz[i][j];
        		b = b+1;
        	}
        }
        XTime_GetTime(&in1_t);
        printf("in1 took %llu clock cycles.\n", 2*(in1_t - in0_t));
        printf("in1 took %.2f us.\n",
        		1.0 * (in1_t - in0_t) / (COUNTS_PER_SECOND/1000000));

        XTime_GetTime(&in0_t);
        *txDone = 0;
        DmaCmd.BD.SrcAddr = (u32) in1Buff;  // source = txBuff
        DmaCmd.BD.DstAddr = (u32) in1_bram;    // destination = bram
        DmaCmd.BD.Length = 336 * sizeof(int);   // length of data to transfer

        // Start the DMA
        Status = XDmaPs_Start(DmaInst, 0, &DmaCmd, 0);
        if (Status != XST_SUCCESS) {
        	return XST_FAILURE;
        }
        TimeOutCnt=0;
        while (!(*txDone) && TimeOutCnt < TIMEOUT_LIMIT) {
        	TimeOutCnt++;
        }
        if (TimeOutCnt >= TIMEOUT_LIMIT) {
        	printf("timeout\r\n");
        	return XST_FAILURE;
        }
        XTime_GetTime(&in1_t);
        printf("DMA took %llu clock cycles.\n", 2*(in1_t - in0_t));
        printf("DMA took %.2f us.\n",
        		1.0 * (in1_t - in0_t) / (COUNTS_PER_SECOND/1000000));


        // ps_control = 1
        xil_printf("ps_control=1\r\n");
        hw_flag[0] = 1;

        // Print Output
        xil_printf("-----Printing Output-----\r\n");
        XTime_GetTime(&start);
        for(r=0; r<40; r=r+8){
//        	xil_printf("wait for pl_full=1\r\n");
        	while((hw_flag[5] & 0x0) == 1){ ; }
            XTime_GetTime(&end);
            printf("Compute took %llu clock cycles.\n", 2*(end - start));
            printf("Compute took %.2f us.\n",
            		1.0 * (end - start) / (COUNTS_PER_SECOND/1000000));

        	xil_printf("-----Make transfer-----\r\n");
            xil_printf("*****output*****\r\n");
            XTime_GetTime(&in0_t);
            *txDone = 0;
            DmaCmd.BD.SrcAddr = (u32) out_bram;
            DmaCmd.BD.DstAddr = (u32) (outBuff[0]);
            DmaCmd.BD.Length = 968 * sizeof(int);

            // Start the DMA
            Status = XDmaPs_Start(DmaInst, 0, &DmaCmd, 0);
            if (Status != XST_SUCCESS) {
            	return XST_FAILURE;
            }

            // Loop until the DMA is done --  txDone will be set in interrupt handler
            TimeOutCnt=0;
            while (!(*txDone) && TimeOutCnt < TIMEOUT_LIMIT) {
            	TimeOutCnt++;
            }

            if (TimeOutCnt >= TIMEOUT_LIMIT) {
            	printf("timeout\r\n");
            	return XST_FAILURE;
            }

            *txDone = 0;
            DmaCmd.BD.SrcAddr = (u32) (out_bram+1024);
            DmaCmd.BD.DstAddr = (u32) (outBuff[1]);

            // Start the DMA
            Status = XDmaPs_Start(DmaInst, 0, &DmaCmd, 0);
            if (Status != XST_SUCCESS) {
            	return XST_FAILURE;
            }

            // Loop until the DMA is done --  txDone will be set in interrupt handler
            TimeOutCnt=0;
            while (!(*txDone) && TimeOutCnt < TIMEOUT_LIMIT) {
            	TimeOutCnt++;
            }

            if (TimeOutCnt >= TIMEOUT_LIMIT) {
            	printf("timeout\r\n");
            	return XST_FAILURE;
            }

            *txDone = 0;
            DmaCmd.BD.SrcAddr = (u32) (out_bram+2048);
            DmaCmd.BD.DstAddr = (u32) (outBuff[2]);

            // Start the DMA
            Status = XDmaPs_Start(DmaInst, 0, &DmaCmd, 0);
            if (Status != XST_SUCCESS) {
            	return XST_FAILURE;
            }

            // Loop until the DMA is done --  txDone will be set in interrupt handler
            TimeOutCnt=0;
            while (!(*txDone) && TimeOutCnt < TIMEOUT_LIMIT) {
            	TimeOutCnt++;
            }

            if (TimeOutCnt >= TIMEOUT_LIMIT) {
            	printf("timeout\r\n");
            	return XST_FAILURE;
            }

            *txDone = 0;
            DmaCmd.BD.SrcAddr = (u32) (out_bram+3072);
            DmaCmd.BD.DstAddr = (u32) (outBuff[3]);

            // Start the DMA
            Status = XDmaPs_Start(DmaInst, 0, &DmaCmd, 0);
            if (Status != XST_SUCCESS) {
            	return XST_FAILURE;
            }

            // Loop until the DMA is done --  txDone will be set in interrupt handler
            TimeOutCnt=0;
            while (!(*txDone) && TimeOutCnt < TIMEOUT_LIMIT) {
            	TimeOutCnt++;
            }

            if (TimeOutCnt >= TIMEOUT_LIMIT) {
            	printf("timeout\r\n");
            	return XST_FAILURE;
            }
            XTime_GetTime(&in1_t);
            printf("DMA took %llu clock cycles.\n", 2*(in1_t - in0_t));
            printf("DMA took %.2f us.\n",
            		1.0 * (in1_t - in0_t) / (COUNTS_PER_SECOND/1000000));
            XTime_GetTime(&in0_t);

        	b=0;
        	for(i=r; i<r+8; i++){
        		for(j=0; j<484; j=j+4){
        				dw2[i][j] = outBuff[0][b];
        				dw2[i][j+1] = outBuff[1][b];
        				dw2[i][j+2] = outBuff[2][b];
        				dw2[i][j+3] = outBuff[3][b];
//        				if(j%120==0){
//        					printf("dw2[%d][%d] = %.3f = outBuff[0][%d] = %.3f\r\n", i, j, dw2[i][j], b, outBuff[0][b]);
//        					printf("dw2[%d][%d] = %.3f = outBuff[1][%d] = %.3f\r\n", i, j+1, dw2[i][j], b, outBuff[1][b]);
//        					printf("dw2[%d][%d] = %.3f = outBuff[2][%d] = %.3f\r\n", i, j+2, dw2[i][j], b, outBuff[2][b]);
//        					printf("dw2[%d][%d] = %.3f = outBuff[3][%d] = %.3f\r\n", i, j+3, dw2[i][j], b, outBuff[3][b]);
//        				}
        				b = b+1;
        		}
        	}
            XTime_GetTime(&in1_t);
            printf("output took %llu clock cycles.\n", 2*(in1_t - in0_t));
            printf("output took %.2f us.\n",
            		1.0 * (in1_t - in0_t) / (COUNTS_PER_SECOND/1000000));
        	hw_flag[0] = 0;
        	xil_printf("wait for pl_full=0\r\n");
        	while((hw_flag[5] & 0x1) == 1){ ; }
        	hw_flag[0] = 1;
            XTime_GetTime(&start);
        }
//        xil_printf("wait for pl_status=1\r\n");
        while((hw_flag[4] & 0x0) == 1){ ; }
        XTime_GetTime(&end);
        xil_printf("pl_status=1\r\n");
        printf("Compute took %llu clock cycles.\n", 2*(end - start));
        printf("Compute took %.2f us.\n",
        		1.0 * (end - start) / (COUNTS_PER_SECOND/1000000));
        hw_flag[0] = 0;
        while((hw_flag[4] & 0x1) == 1){ ; }

        XTime_GetTime(&in0_t);
        *txDone = 0;
        DmaCmd.BD.SrcAddr = (u32) out_bram;
        DmaCmd.BD.DstAddr = (u32) (outBuff[0]);
//        DmaCmd.BD.Length = 121 * sizeof(int);

        // Start the DMA
        Status = XDmaPs_Start(DmaInst, 0, &DmaCmd, 0);
        if (Status != XST_SUCCESS) {
        	return XST_FAILURE;
        }

        // Loop until the DMA is done --  txDone will be set in interrupt handler
        TimeOutCnt=0;
        while (!(*txDone) && TimeOutCnt < TIMEOUT_LIMIT) {
        	TimeOutCnt++;
        }

        if (TimeOutCnt >= TIMEOUT_LIMIT) {
        	printf("timeout\r\n");
        	return XST_FAILURE;
        }

        *txDone = 0;
        DmaCmd.BD.SrcAddr = (u32) (out_bram+1024);
        DmaCmd.BD.DstAddr = (u32) (outBuff[1]);

        // Start the DMA
        Status = XDmaPs_Start(DmaInst, 0, &DmaCmd, 0);
        if (Status != XST_SUCCESS) {
        	return XST_FAILURE;
        }

        // Loop until the DMA is done --  txDone will be set in interrupt handler
        TimeOutCnt=0;
        while (!(*txDone) && TimeOutCnt < TIMEOUT_LIMIT) {
        	TimeOutCnt++;
        }

        if (TimeOutCnt >= TIMEOUT_LIMIT) {
        	printf("timeout\r\n");
        	return XST_FAILURE;
        }

        *txDone = 0;
        DmaCmd.BD.SrcAddr = (u32) (out_bram+2048);
        DmaCmd.BD.DstAddr = (u32) (outBuff[2]);

        // Start the DMA
        Status = XDmaPs_Start(DmaInst, 0, &DmaCmd, 0);
        if (Status != XST_SUCCESS) {
        	return XST_FAILURE;
        }

        // Loop until the DMA is done --  txDone will be set in interrupt handler
        TimeOutCnt=0;
        while (!(*txDone) && TimeOutCnt < TIMEOUT_LIMIT) {
        	TimeOutCnt++;
        }

        if (TimeOutCnt >= TIMEOUT_LIMIT) {
        	printf("timeout\r\n");
        	return XST_FAILURE;
        }

        *txDone = 0;
        DmaCmd.BD.SrcAddr = (u32) (out_bram+3072);
        DmaCmd.BD.DstAddr = (u32) (outBuff[3]);

        // Start the DMA
        Status = XDmaPs_Start(DmaInst, 0, &DmaCmd, 0);
        if (Status != XST_SUCCESS) {
        	return XST_FAILURE;
        }

        // Loop until the DMA is done --  txDone will be set in interrupt handler
        TimeOutCnt=0;
        while (!(*txDone) && TimeOutCnt < TIMEOUT_LIMIT) {
        	TimeOutCnt++;
        }

        if (TimeOutCnt >= TIMEOUT_LIMIT) {
        	printf("timeout\r\n");
        	return XST_FAILURE;
        }
        XTime_GetTime(&in1_t);
        printf("DMA took %llu clock cycles.\n", 2*(in1_t - in0_t));
        printf("DMA took %.2f us.\n",
        		1.0 * (in1_t - in0_t) / (COUNTS_PER_SECOND/1000000));
        XTime_GetTime(&in0_t);
    	b=0;
    	for(i=r; i<r+8; i++){
    		for(j=0; j<484; j=j+4){
    				dw2[i][j] = outBuff[0][b];
    				dw2[i][j+1] = outBuff[1][b];
    				dw2[i][j+2] = outBuff[2][b];
    				dw2[i][j+3] = outBuff[3][b];
//    				if(j%120==0){
//    					printf("dw2[%d][%d] = %.3f = outBuff[0][%d] = %.3f\r\n", i, j, dw2[i][j], b, outBuff[0][b]);
//    					printf("dw2[%d][%d] = %.3f = outBuff[1][%d] = %.3f\r\n", i, j+1, dw2[i][j], b, outBuff[1][b]);
//    					printf("dw2[%d][%d] = %.3f = outBuff[2][%d] = %.3f\r\n", i, j+2, dw2[i][j], b, outBuff[2][b]);
//    					printf("dw2[%d][%d] = %.3f = outBuff[3][%d] = %.3f\r\n", i, j+3, dw2[i][j], b, outBuff[3][b]);
//    				}
    				b = b+1;
    		}
    	}
        XTime_GetTime(&in1_t);
        printf("output took %llu clock cycles.\n", 2*(in1_t - in0_t));
        printf("output took %.2f us.\n",
        		1.0 * (in1_t - in0_t) / (COUNTS_PER_SECOND/1000000));

        // Phase 3
        xil_printf("------------------------Phase 3------------------------------\r\n");
        // set up
        xil_printf("*****setup*****\r\n");
        hw_flag[1] = 847; //ps_iternum
        hw_flag[2] = 48;   //ps_accfreq
        hw_flag[3] = 3;   //ps_phase
        // load input buffers
        xil_printf("*****setup in1*****\r\n");
        XTime_GetTime(&in0_t);
        b = 0;
        for(i=0; i<7;i++){
        	for(j=0; j<48; j++){
        		in1Buff[b] = dz[i][j];
//        		xil_printf("in1_bram[%d] = dz[%d][%d] = 0x%x\r\n", b, i, j, in1_bram[b]);
        		b = b+1;
        	}
        }
        XTime_GetTime(&in1_t);
        printf("in1 took %llu clock cycles.\n", 2*(in1_t - in0_t));
        printf("in1 took %.2f us.\n",
        		1.0 * (in1_t - in0_t) / (COUNTS_PER_SECOND/1000000));

        XTime_GetTime(&in0_t);
        *txDone = 0;
        DmaCmd.BD.SrcAddr = (u32) in1Buff;  // source = txBuff
        DmaCmd.BD.DstAddr = (u32) in1_bram;    // destination = bram
        DmaCmd.BD.Length = 336 * sizeof(int);   // length of data to transfer

        // Start the DMA
        Status = XDmaPs_Start(DmaInst, 0, &DmaCmd, 0);
        if (Status != XST_SUCCESS) {
        	return XST_FAILURE;
        }
        TimeOutCnt=0;
        while (!(*txDone) && TimeOutCnt < TIMEOUT_LIMIT) {
        	TimeOutCnt++;
        }
        if (TimeOutCnt >= TIMEOUT_LIMIT) {
        	printf("timeout\r\n");
        	return XST_FAILURE;
        }
        XTime_GetTime(&in1_t);
        printf("DMA took %llu clock cycles.\n", 2*(in1_t - in0_t));
        printf("DMA took %.2f us.\n",
        		1.0 * (in1_t - in0_t) / (COUNTS_PER_SECOND/1000000));

        xil_printf("*****setup in0*****\r\n");
        XTime_GetTime(&start);
        b = 0;
        c = 0;
        r = 0;
        for(int k=c; k<c+4; k++){
        	for(j=k; j<k+84; j=j+4){
        		for(i=0; i<48; i++){
        			in0Buff[r][b] = w2[i][j];
//        			xil_printf("in0_bram[%d] = w2[%d][%d] = 0x%x\r\n", b, i, j, in0_bram[b]);
        			b = b+1;
        		}
        	}
        	b = 0;
        	r = r+1;
        }
        XTime_GetTime(&end);
        printf("in0 took %llu clock cycles.\n", 2*(end - start));
        printf("in0 took %.2f us.\n",
        		1.0 * (end - start) / (COUNTS_PER_SECOND/1000000));

        XTime_GetTime(&in0_t);
        *txDone = 0;
        DmaCmd.BD.SrcAddr = (u32) in0Buff;  // source = txBuff
        DmaCmd.BD.DstAddr = (u32) in0_bram;    // destination = bram
        DmaCmd.BD.Length = 1008 * sizeof(int);

        // Start the DMA
        Status = XDmaPs_Start(DmaInst, 0, &DmaCmd, 0);
        if (Status != XST_SUCCESS) {
        	return XST_FAILURE;
        }
        TimeOutCnt=0;
        while (!(*txDone) && TimeOutCnt < TIMEOUT_LIMIT) {
        	TimeOutCnt++;
        }
        if (TimeOutCnt >= TIMEOUT_LIMIT) {
        	printf("timeout\r\n");
        	return XST_FAILURE;
        }

        *txDone = 0;
        DmaCmd.BD.SrcAddr = (u32) (in0Buff[1]);  // source = txBuff
        DmaCmd.BD.DstAddr = (u32) (in0_bram+1024);    // destination = bram

        // Start the DMA
        Status = XDmaPs_Start(DmaInst, 0, &DmaCmd, 0);
        if (Status != XST_SUCCESS) {
        	return XST_FAILURE;
        }
        TimeOutCnt=0;
        while (!(*txDone) && TimeOutCnt < TIMEOUT_LIMIT) {
        	TimeOutCnt++;
        }
        if (TimeOutCnt >= TIMEOUT_LIMIT) {
        	printf("timeout\r\n");
        	return XST_FAILURE;
        }

        *txDone = 0;
        DmaCmd.BD.SrcAddr = (u32) (in0Buff[2]);  // source = txBuff
        DmaCmd.BD.DstAddr = (u32) (in0_bram+2048);    // destination = bram

        // Start the DMA
        Status = XDmaPs_Start(DmaInst, 0, &DmaCmd, 0);
        if (Status != XST_SUCCESS) {
        	return XST_FAILURE;
        }
        TimeOutCnt=0;
        while (!(*txDone) && TimeOutCnt < TIMEOUT_LIMIT) {
        	TimeOutCnt++;
        }
        if (TimeOutCnt >= TIMEOUT_LIMIT) {
        	printf("timeout\r\n");
        	return XST_FAILURE;
        }

        *txDone = 0;
        DmaCmd.BD.SrcAddr = (u32) (in0Buff[3]);  // source = txBuff
        DmaCmd.BD.DstAddr = (u32) (in0_bram+3072);    // destination = bram

        // Start the DMA
        Status = XDmaPs_Start(DmaInst, 0, &DmaCmd, 0);
        if (Status != XST_SUCCESS) {
        	return XST_FAILURE;
        }
        TimeOutCnt=0;
        while (!(*txDone) && TimeOutCnt < TIMEOUT_LIMIT) {
        	TimeOutCnt++;
        }
        if (TimeOutCnt >= TIMEOUT_LIMIT) {
        	printf("timeout\r\n");
        	return XST_FAILURE;
        }
        XTime_GetTime(&in1_t);
        printf("DMA took %llu clock cycles.\n", 2*(in1_t - in0_t));
        printf("DMA took %.2f us.\n",
        		1.0 * (in1_t - in0_t) / (COUNTS_PER_SECOND/1000000));

        // ps_control = 1
        xil_printf("ps_control=1\r\n");
        hw_flag[0] = 1;
        for(c=84; c<420; c=c+84){
        	XTime_GetTime(&start);
        	while((hw_flag[5] & 0x1) == 0){ ; }
            XTime_GetTime(&end);
            printf("Compute took %llu clock cycles.\n", 2*(end - start));
            printf("Compute took %.2f us.\n",
            		1.0 * (end - start) / (COUNTS_PER_SECOND/1000000));

        	xil_printf("-----Make transfer-----\r\n");
        	XTime_GetTime(&start);
        	b=0;
        	r=0;
        	for(int k=c; k<c+4; k++){
        		for(j=k; j<k+84; j=j+4){
        			for(i=0; i<48; i++){
        				in0Buff[r][b] = w2[i][j];
//        				xil_printf("in0_bram[%d] = w2[%d][%d] = 0x%x\r\n", b, i, j, in0_bram[b]);
        				b = b+1;;
        			}
        		}
        		b = 0;
        		r = r+1;
        	}
            XTime_GetTime(&end);
            printf("in0 took %llu clock cycles.\n", 2*(end - start));
            printf("in0 took %.2f us.\n",
            		1.0 * (end - start) / (COUNTS_PER_SECOND/1000000));

            XTime_GetTime(&in0_t);
            *txDone = 0;
            DmaCmd.BD.SrcAddr = (u32) in0Buff;  // source = txBuff
            DmaCmd.BD.DstAddr = (u32) in0_bram;    // destination = bram

            // Start the DMA
            Status = XDmaPs_Start(DmaInst, 0, &DmaCmd, 0);
            if (Status != XST_SUCCESS) {
            	return XST_FAILURE;
            }
            TimeOutCnt=0;
            while (!(*txDone) && TimeOutCnt < TIMEOUT_LIMIT) {
            	TimeOutCnt++;
            }
            if (TimeOutCnt >= TIMEOUT_LIMIT) {
            	printf("timeout\r\n");
            	return XST_FAILURE;
            }

            *txDone = 0;
            DmaCmd.BD.SrcAddr = (u32) (in0Buff[1]);  // source = txBuff
            DmaCmd.BD.DstAddr = (u32) (in0_bram+1024);    // destination = bram

            // Start the DMA
            Status = XDmaPs_Start(DmaInst, 0, &DmaCmd, 0);
            if (Status != XST_SUCCESS) {
            	return XST_FAILURE;
            }
            TimeOutCnt=0;
            while (!(*txDone) && TimeOutCnt < TIMEOUT_LIMIT) {
            	TimeOutCnt++;
            }
            if (TimeOutCnt >= TIMEOUT_LIMIT) {
            	printf("timeout\r\n");
            	return XST_FAILURE;
            }

            *txDone = 0;
            DmaCmd.BD.SrcAddr = (u32) (in0Buff[2]);  // source = txBuff
            DmaCmd.BD.DstAddr = (u32) (in0_bram+2048);    // destination = bram

            // Start the DMA
            Status = XDmaPs_Start(DmaInst, 0, &DmaCmd, 0);
            if (Status != XST_SUCCESS) {
            	return XST_FAILURE;
            }
            TimeOutCnt=0;
            while (!(*txDone) && TimeOutCnt < TIMEOUT_LIMIT) {
            	TimeOutCnt++;
            }
            if (TimeOutCnt >= TIMEOUT_LIMIT) {
            	printf("timeout\r\n");
            	return XST_FAILURE;
            }

            *txDone = 0;
            DmaCmd.BD.SrcAddr = (u32) (in0Buff[3]);  // source = txBuff
            DmaCmd.BD.DstAddr = (u32) (in0_bram+3072);    // destination = bram

            // Start the DMA
            Status = XDmaPs_Start(DmaInst, 0, &DmaCmd, 0);
            if (Status != XST_SUCCESS) {
            	return XST_FAILURE;
            }
            TimeOutCnt=0;
            while (!(*txDone) && TimeOutCnt < TIMEOUT_LIMIT) {
            	TimeOutCnt++;
            }
            if (TimeOutCnt >= TIMEOUT_LIMIT) {
            	printf("timeout\r\n");
            	return XST_FAILURE;
            }
            XTime_GetTime(&in1_t);
            printf("DMA took %llu clock cycles.\n", 2*(in1_t - in0_t));
            printf("DMA took %.2f us.\n",
            		1.0 * (in1_t - in0_t) / (COUNTS_PER_SECOND/1000000));

        	hw_flag[0] = 0;
        	while((hw_flag[5] & 0x1) == 1){ ; }
        	hw_flag[0] = 1;

        }
        XTime_GetTime(&start);
        while((hw_flag[5] & 0x1) == 0){ ; }
        XTime_GetTime(&end);
        printf("Compute took %llu clock cycles.\n", 2*(end - start));
        printf("Compute took %.2f us.\n",
        		1.0 * (end - start) / (COUNTS_PER_SECOND/1000000));

        xil_printf("-----Make transfer-----\r\n");
        XTime_GetTime(&start);
        b=0;
        c=420;
        r=0;
        for(k=c; k<c+4; k++){
        	for(j=k; j<k+64; j=j+4){
        		for(i=0; i<48; i++){
        			in0Buff[r][b] = w2[i][j];
//        			xil_printf("in0_bram[%d] = w2[%d][%d] = 0x%x\r\n", b, i, j, in0_bram[b]);
        			b = b+1;;
        		}
        	}
        	b = 0;
        	r = r+1;
        }
        XTime_GetTime(&end);
        printf("in0 took %llu clock cycles.\n", 2*(end - start));
        printf("in0 took %.2f us.\n",
        		1.0 * (end - start) / (COUNTS_PER_SECOND/1000000));

        XTime_GetTime(&in0_t);
        *txDone = 0;
        DmaCmd.BD.SrcAddr = (u32) in0Buff;  // source = txBuff
        DmaCmd.BD.DstAddr = (u32) in0_bram;    // destination = bram
        DmaCmd.BD.Length = 768 * sizeof(int);

        // Start the DMA
        Status = XDmaPs_Start(DmaInst, 0, &DmaCmd, 0);
        if (Status != XST_SUCCESS) {
        	return XST_FAILURE;
        }
        TimeOutCnt=0;
        while (!(*txDone) && TimeOutCnt < TIMEOUT_LIMIT) {
        	TimeOutCnt++;
        }
        if (TimeOutCnt >= TIMEOUT_LIMIT) {
        	printf("timeout\r\n");
        	return XST_FAILURE;
        }

        *txDone = 0;
        DmaCmd.BD.SrcAddr = (u32) (in0Buff[1]);  // source = txBuff
        DmaCmd.BD.DstAddr = (u32) (in0_bram+1024);    // destination = bram

        // Start the DMA
        Status = XDmaPs_Start(DmaInst, 0, &DmaCmd, 0);
        if (Status != XST_SUCCESS) {
        	return XST_FAILURE;
        }
        TimeOutCnt=0;
        while (!(*txDone) && TimeOutCnt < TIMEOUT_LIMIT) {
        	TimeOutCnt++;
        }
        if (TimeOutCnt >= TIMEOUT_LIMIT) {
        	printf("timeout\r\n");
        	return XST_FAILURE;
        }

        *txDone = 0;
        DmaCmd.BD.SrcAddr = (u32) (in0Buff[2]);  // source = txBuff
        DmaCmd.BD.DstAddr = (u32) (in0_bram+2048);    // destination = bram

        // Start the DMA
        Status = XDmaPs_Start(DmaInst, 0, &DmaCmd, 0);
        if (Status != XST_SUCCESS) {
        	return XST_FAILURE;
        }
        TimeOutCnt=0;
        while (!(*txDone) && TimeOutCnt < TIMEOUT_LIMIT) {
        	TimeOutCnt++;
        }
        if (TimeOutCnt >= TIMEOUT_LIMIT) {
        	printf("timeout\r\n");
        	return XST_FAILURE;
        }

        *txDone = 0;
        DmaCmd.BD.SrcAddr = (u32) (in0Buff[3]);  // source = txBuff
        DmaCmd.BD.DstAddr = (u32) (in0_bram+3072);    // destination = bram

        // Start the DMA
        Status = XDmaPs_Start(DmaInst, 0, &DmaCmd, 0);
        if (Status != XST_SUCCESS) {
        	return XST_FAILURE;
        }
        TimeOutCnt=0;
        while (!(*txDone) && TimeOutCnt < TIMEOUT_LIMIT) {
        	TimeOutCnt++;
        }
        if (TimeOutCnt >= TIMEOUT_LIMIT) {
        	printf("timeout\r\n");
        	return XST_FAILURE;
        }
        XTime_GetTime(&in1_t);
        printf("DMA took %llu clock cycles.\n", 2*(in1_t - in0_t));
        printf("DMA took %.2f us.\n",
        		1.0 * (in1_t - in0_t) / (COUNTS_PER_SECOND/1000000));

        hw_flag[0] = 0;
        while((hw_flag[5] & 0x1) == 1){ ; }
        hw_flag[0] = 1;
        XTime_GetTime(&start);
        // wait pl_status = 1
        while((hw_flag[4] & 0x1) == 0){ ; }
        XTime_GetTime(&end);
        printf("Compute took %llu clock cycles.\n", 2*(end - start));
        printf("Compute took %.2f us.\n",
        		1.0 * (end - start) / (COUNTS_PER_SECOND/1000000));
        xil_printf("pl_status=1\r\n");
        hw_flag[0] = 0;
        while((hw_flag[4] & 0x1) == 1){ ; }
        XTime_GetTime(&end);


        xil_printf("*****output*****\r\n");
        XTime_GetTime(&in0_t);
        *txDone = 0;
        DmaCmd.BD.SrcAddr = (u32) out_bram;
        DmaCmd.BD.DstAddr = (u32) (outBuff[0]);
        DmaCmd.BD.Length = 847 * sizeof(int);

        // Start the DMA
        Status = XDmaPs_Start(DmaInst, 0, &DmaCmd, 0);
        if (Status != XST_SUCCESS) {
        	return XST_FAILURE;
        }

        // Loop until the DMA is done --  txDone will be set in interrupt handler
        TimeOutCnt=0;
        while (!(*txDone) && TimeOutCnt < TIMEOUT_LIMIT) {
        	TimeOutCnt++;
        }

        if (TimeOutCnt >= TIMEOUT_LIMIT) {
        	printf("timeout\r\n");
        	return XST_FAILURE;
        }

        *txDone = 0;
        DmaCmd.BD.SrcAddr = (u32) (out_bram+1024);
        DmaCmd.BD.DstAddr = (u32) (outBuff[1]);

        // Start the DMA
        Status = XDmaPs_Start(DmaInst, 0, &DmaCmd, 0);
        if (Status != XST_SUCCESS) {
        	return XST_FAILURE;
        }

        // Loop until the DMA is done --  txDone will be set in interrupt handler
        TimeOutCnt=0;
        while (!(*txDone) && TimeOutCnt < TIMEOUT_LIMIT) {
        	TimeOutCnt++;
        }

        if (TimeOutCnt >= TIMEOUT_LIMIT) {
        	printf("timeout\r\n");
        	return XST_FAILURE;
        }

        *txDone = 0;
        DmaCmd.BD.SrcAddr = (u32) (out_bram+2048);
        DmaCmd.BD.DstAddr = (u32) (outBuff[2]);

        // Start the DMA
        Status = XDmaPs_Start(DmaInst, 0, &DmaCmd, 0);
        if (Status != XST_SUCCESS) {
        	return XST_FAILURE;
        }

        // Loop until the DMA is done --  txDone will be set in interrupt handler
        TimeOutCnt=0;
        while (!(*txDone) && TimeOutCnt < TIMEOUT_LIMIT) {
        	TimeOutCnt++;
        }

        if (TimeOutCnt >= TIMEOUT_LIMIT) {
        	printf("timeout\r\n");
        	return XST_FAILURE;
        }

        *txDone = 0;
        DmaCmd.BD.SrcAddr = (u32) (out_bram+3072);
        DmaCmd.BD.DstAddr = (u32) (outBuff[3]);

        // Start the DMA
        Status = XDmaPs_Start(DmaInst, 0, &DmaCmd, 0);
        if (Status != XST_SUCCESS) {
        	return XST_FAILURE;
        }

        // Loop until the DMA is done --  txDone will be set in interrupt handler
        TimeOutCnt=0;
        while (!(*txDone) && TimeOutCnt < TIMEOUT_LIMIT) {
        	TimeOutCnt++;
        }

        if (TimeOutCnt >= TIMEOUT_LIMIT) {
        	printf("timeout\r\n");
        	return XST_FAILURE;
        }
        XTime_GetTime(&in1_t);
        printf("DMA took %llu clock cycles.\n", 2*(in1_t - in0_t));
        printf("DMA took %.2f us.\n",
        		1.0 * (in1_t - in0_t) / (COUNTS_PER_SECOND/1000000));

        // Print Output
        xil_printf("-----Printing Output-----\r\n");
        XTime_GetTime(&in0_t);
    	b=0;
    	for(k=0;k<484;k=k+4){
    		for(i=0; i<7; i++){
    				dy[i][k] = outBuff[0][b];
    				dy[i][k+1] = outBuff[1][b];
    				dy[i][k+2] = outBuff[2][b];
    				dy[i][k+3] = outBuff[3][b];
    				if(k%120==0)
    					printf("dy[%d][%d] = out_bram[0][%d] = %.3f\r\n", i, k, b, dy[i][k]);
    				b = b+1;
    		}
    	}
        XTime_GetTime(&in1_t);
        printf("output took %llu clock cycles.\n", 2*(in1_t - in0_t));
        printf("output took %.2f us.\n",
        		1.0 * (in1_t - in0_t) / (COUNTS_PER_SECOND/1000000));


        // Phase 4
    	xil_printf("------------------------Phase 4------------------------------\r\n");
    	// set up
    	xil_printf("*****setup*****\r\n");
    	hw_flag[1] = 28; //ps_iternum
    	hw_flag[2] = 121;   //ps_accfreq
    	hw_flag[3] = 4;   //ps_phase
    	// load input buffers
    	xil_printf("*****setup in0*****\r\n");
        XTime_GetTime(&in0_t);
    	r = 0;
    	b = 0;
    	c = 0;
    	for(k=0; k<=363; k=k+121){
    		for(j=k; j<k+121; j++){
    			in0Buff[c][b] = dy[r][j];
//            		xil_printf("in0_bram[%d] = dy[%d][%d] = 0x%x\r\n", b, r, j, in0_bram[b]);
    			b=b+1;
    		}
    		b = 0;
    		c = c+1;
    	}
        XTime_GetTime(&in1_t);
        printf("in0 took %llu clock cycles.\n", 2*(in1_t - in0_t));
        printf("in0 took %.2f us.\n",
        		1.0 * (in1_t - in0_t) / (COUNTS_PER_SECOND/1000000));
        XTime_GetTime(&in0_t);
        *txDone = 0;
        DmaCmd.BD.SrcAddr = (u32) in0Buff;  // source = txBuff
        DmaCmd.BD.DstAddr = (u32) in0_bram;    // destination = bram
        DmaCmd.BD.Length = 121 * sizeof(int);

        // Start the DMA
        Status = XDmaPs_Start(DmaInst, 0, &DmaCmd, 0);
        if (Status != XST_SUCCESS) {
        	return XST_FAILURE;
        }
        TimeOutCnt=0;
        while (!(*txDone) && TimeOutCnt < TIMEOUT_LIMIT) {
        	TimeOutCnt++;
        }
        if (TimeOutCnt >= TIMEOUT_LIMIT) {
        	printf("timeout\r\n");
        	return XST_FAILURE;
        }

        *txDone = 0;
        DmaCmd.BD.SrcAddr = (u32) (in0Buff[1]);  // source = txBuff
        DmaCmd.BD.DstAddr = (u32) (in0_bram+1024);    // destination = bram

        // Start the DMA
        Status = XDmaPs_Start(DmaInst, 0, &DmaCmd, 0);
        if (Status != XST_SUCCESS) {
        	return XST_FAILURE;
        }
        TimeOutCnt=0;
        while (!(*txDone) && TimeOutCnt < TIMEOUT_LIMIT) {
        	TimeOutCnt++;
        }
        if (TimeOutCnt >= TIMEOUT_LIMIT) {
        	printf("timeout\r\n");
        	return XST_FAILURE;
        }

        *txDone = 0;
        DmaCmd.BD.SrcAddr = (u32) (in0Buff[2]);  // source = txBuff
        DmaCmd.BD.DstAddr = (u32) (in0_bram+2048);    // destination = bram

        // Start the DMA
        Status = XDmaPs_Start(DmaInst, 0, &DmaCmd, 0);
        if (Status != XST_SUCCESS) {
        	return XST_FAILURE;
        }
        TimeOutCnt=0;
        while (!(*txDone) && TimeOutCnt < TIMEOUT_LIMIT) {
        	TimeOutCnt++;
        }
        if (TimeOutCnt >= TIMEOUT_LIMIT) {
        	printf("timeout\r\n");
        	return XST_FAILURE;
        }

        *txDone = 0;
        DmaCmd.BD.SrcAddr = (u32) (in0Buff[3]);  // source = txBuff
        DmaCmd.BD.DstAddr = (u32) (in0_bram+3072);    // destination = bram

        // Start the DMA
        Status = XDmaPs_Start(DmaInst, 0, &DmaCmd, 0);
        if (Status != XST_SUCCESS) {
        	return XST_FAILURE;
        }
        TimeOutCnt=0;
        while (!(*txDone) && TimeOutCnt < TIMEOUT_LIMIT) {
        	TimeOutCnt++;
        }
        if (TimeOutCnt >= TIMEOUT_LIMIT) {
        	printf("timeout\r\n");
        	return XST_FAILURE;
        }
        XTime_GetTime(&in1_t);
        printf("DMA took %llu clock cycles.\n", 2*(in1_t - in0_t));
        printf("DMA took %.2f us.\n",
        		1.0 * (in1_t - in0_t) / (COUNTS_PER_SECOND/1000000));

    	xil_printf("*****setup in1*****\r\n");
    	XTime_GetTime(&in0_t);
    	b = 0;
    	for(i=0; i<=12; i=i+12){
    		for(j=0; j<2; j++){
    			for(k1=0; k1<132; k1=k1+12){
    				for(k2=0; k2<11; k2++){
    					in1Buff[b] = x[r][i+j+k1+k2];
//            				xil_printf("in1_bram[%d] = x[%d][%d] = 0x%x\r\n", b, r, i+j+k1+k2, in1_bram[b] );
    					b= b+1;
    				}
    			}
    		}
    	}
        XTime_GetTime(&in1_t);
        printf("in1 took %llu clock cycles.\n", 2*(in1_t - in0_t));
        printf("in1 took %.2f us.\n",
        		1.0 * (in1_t - in0_t) / (COUNTS_PER_SECOND/1000000));

    	XTime_GetTime(&in0_t);
        *txDone = 0;
        DmaCmd.BD.SrcAddr = (u32) in1Buff;  // source = txBuff
        DmaCmd.BD.DstAddr = (u32) in1_bram;    // destination = bram
        DmaCmd.BD.Length = 484 * sizeof(int);

        // Start the DMA
        Status = XDmaPs_Start(DmaInst, 0, &DmaCmd, 0);
        if (Status != XST_SUCCESS) {
        	return XST_FAILURE;
        }
        TimeOutCnt=0;
        while (!(*txDone) && TimeOutCnt < TIMEOUT_LIMIT) {
        	TimeOutCnt++;
        }
        if (TimeOutCnt >= TIMEOUT_LIMIT) {
        	printf("timeout\r\n");
        	return XST_FAILURE;
        }
        XTime_GetTime(&in1_t);
        printf("DMA took %llu clock cycles.\n", 2*(in1_t - in0_t));
        printf("DMA took %.2f us.\n",
        		1.0 * (in1_t - in0_t) / (COUNTS_PER_SECOND/1000000));

    	// ps_control = 1
    	xil_printf("ps_control=1\r\n");
    	hw_flag[0] = 1;
    	for(r=1; r<7; r++){
            XTime_GetTime(&start);
    		while((hw_flag[5] & 0x1) == 0){ ; }
            XTime_GetTime(&end);
            printf("compute took %llu clock cycles.\n", 2*(end - start));
            printf("compute took %.2f us.\n",
            		1.0 * (end - start) / (COUNTS_PER_SECOND/1000000));
    		xil_printf("-----Make transfer-----\r\n");
    		XTime_GetTime(&in0_t);
    		c=0;
    		b=0;
        	for(k=0; k<=363; k=k+121){
        		for(j=k; j<k+121; j++){
        			in0Buff[c][b] = dy[r][j];
    //            		xil_printf("in0_bram[%d] = dy[%d][%d] = 0x%x\r\n", b, r, j, in0_bram[b]);
        			b=b+1;
        		}
        		b = 0;
        		c = c+1;
        	}
            XTime_GetTime(&in1_t);
            printf("in0 took %llu clock cycles.\n", 2*(in1_t - in0_t));
            printf("in0 took %.2f us.\n",
            		1.0 * (in1_t - in0_t) / (COUNTS_PER_SECOND/1000000));

            XTime_GetTime(&in0_t);
            *txDone = 0;
            DmaCmd.BD.SrcAddr = (u32) in0Buff;  // source = txBuff
            DmaCmd.BD.DstAddr = (u32) in0_bram;    // destination = bram

            // Start the DMA
            Status = XDmaPs_Start(DmaInst, 0, &DmaCmd, 0);
            if (Status != XST_SUCCESS) {
            	return XST_FAILURE;
            }
            TimeOutCnt=0;
            while (!(*txDone) && TimeOutCnt < TIMEOUT_LIMIT) {
            	TimeOutCnt++;
            }
            if (TimeOutCnt >= TIMEOUT_LIMIT) {
            	printf("timeout\r\n");
            	return XST_FAILURE;
            }

            *txDone = 0;
            DmaCmd.BD.SrcAddr = (u32) (in0Buff[1]);  // source = txBuff
            DmaCmd.BD.DstAddr = (u32) (in0_bram+1024);    // destination = bram

            // Start the DMA
            Status = XDmaPs_Start(DmaInst, 0, &DmaCmd, 0);
            if (Status != XST_SUCCESS) {
            	return XST_FAILURE;
            }
            TimeOutCnt=0;
            while (!(*txDone) && TimeOutCnt < TIMEOUT_LIMIT) {
            	TimeOutCnt++;
            }
            if (TimeOutCnt >= TIMEOUT_LIMIT) {
            	printf("timeout\r\n");
            	return XST_FAILURE;
            }

            *txDone = 0;
            DmaCmd.BD.SrcAddr = (u32) (in0Buff[2]);  // source = txBuff
            DmaCmd.BD.DstAddr = (u32) (in0_bram+2048);    // destination = bram

            // Start the DMA
            Status = XDmaPs_Start(DmaInst, 0, &DmaCmd, 0);
            if (Status != XST_SUCCESS) {
            	return XST_FAILURE;
            }
            TimeOutCnt=0;
            while (!(*txDone) && TimeOutCnt < TIMEOUT_LIMIT) {
            	TimeOutCnt++;
            }
            if (TimeOutCnt >= TIMEOUT_LIMIT) {
            	printf("timeout\r\n");
            	return XST_FAILURE;
            }

            *txDone = 0;
            DmaCmd.BD.SrcAddr = (u32) (in0Buff[3]);  // source = txBuff
            DmaCmd.BD.DstAddr = (u32) (in0_bram+3072);    // destination = bram

            // Start the DMA
            Status = XDmaPs_Start(DmaInst, 0, &DmaCmd, 0);
            if (Status != XST_SUCCESS) {
            	return XST_FAILURE;
            }
            TimeOutCnt=0;
            while (!(*txDone) && TimeOutCnt < TIMEOUT_LIMIT) {
            	TimeOutCnt++;
            }
            if (TimeOutCnt >= TIMEOUT_LIMIT) {
            	printf("timeout\r\n");
            	return XST_FAILURE;
            }
            XTime_GetTime(&in1_t);
            printf("DMA took %llu clock cycles.\n", 2*(in1_t - in0_t));
            printf("DMA took %.2f us.\n",
            		1.0 * (in1_t - in0_t) / (COUNTS_PER_SECOND/1000000));

            XTime_GetTime(&in0_t);

        	b = 0;
        	for(i=0; i<=12; i=i+12){
        		for(j=0; j<2; j++){
        			for(k1=0; k1<132; k1=k1+12){
        				for(k2=0; k2<11; k2++){
        					in1Buff[b] = x[r][i+j+k1+k2];
    //            				xil_printf("in1_bram[%d] = x[%d][%d] = 0x%x\r\n", b, r, i+j+k1+k2, in1_bram[b] );
        					b= b+1;
        				}
        			}
        		}
        	}
            XTime_GetTime(&in1_t);
            printf("in1 took %llu clock cycles.\n", 2*(in1_t - in0_t));
            printf("in1 took %.2f us.\n",
            		1.0 * (in1_t - in0_t) / (COUNTS_PER_SECOND/1000000));

        	XTime_GetTime(&in0_t);
            *txDone = 0;
            DmaCmd.BD.SrcAddr = (u32) in1Buff;  // source = txBuff
            DmaCmd.BD.DstAddr = (u32) in1_bram;    // destination = bram
            DmaCmd.BD.Length = 484 * sizeof(int);

            // Start the DMA
            Status = XDmaPs_Start(DmaInst, 0, &DmaCmd, 0);
            if (Status != XST_SUCCESS) {
            	return XST_FAILURE;
            }
            TimeOutCnt=0;
            while (!(*txDone) && TimeOutCnt < TIMEOUT_LIMIT) {
            	TimeOutCnt++;
            }
            if (TimeOutCnt >= TIMEOUT_LIMIT) {
            	printf("timeout\r\n");
            	return XST_FAILURE;
            }
            XTime_GetTime(&in1_t);
            printf("DMA took %llu clock cycles.\n", 2*(in1_t - in0_t));
            printf("DMA took %.2f us.\n",
            		1.0 * (in1_t - in0_t) / (COUNTS_PER_SECOND/1000000));
    		hw_flag[0] = 0;
    		while((hw_flag[5] & 0x1) == 1){ ; }
    		hw_flag[0] = 1;
    	}
        XTime_GetTime(&start);
    	while((hw_flag[4] & 0x1) == 0){ ; }
        XTime_GetTime(&end);
        xil_printf("pl_status=1\r\n");
        printf("compute took %llu clock cycles.\n", 2*(end - start));
        printf("compute took %.2f us.\n",
        		1.0 * (end - start) / (COUNTS_PER_SECOND/1000000));
    	hw_flag[0] = 0;
    	while((hw_flag[4] & 0x1) == 1){ ; }
    	// Print Output

    	xil_printf("-----Printing Output-----\r\n");
        XTime_GetTime(&in0_t);
    	b=0;
    	for(i=0; i<4; i++){
    		for(c=0; c<7; c++){
    			for(j=0; j<4; j++){
    				dw1[i][j] = dw1[i][j] + *((float*)&out_bram[b]);
//    				printf("dw1[%d][%d] = out_bram[%d] = %.3f\r\n", i, j, b, dw1[i][j]);
    				b = b+1;
    			}
    		}
    		b = b + 996;
    	}
        XTime_GetTime(&in1_t);
        printf("output took %llu clock cycles.\n", 2*(in1_t - in0_t));
        printf("output took %.2f us.\n", 1.0 * (in1_t - in0_t) / (COUNTS_PER_SECOND/1000000));
    	print("--------------------- DONE ---------------------\r\n");

    	cleanup_platform();
    	return 0;
}
// Lightly modified from Xilinx example code
int SetupInterruptSystem(XScuGic *GicPtr, XDmaPs *DmaPtr)
{
    int Status;
    XScuGic_Config *GicConfig;

    Xil_ExceptionInit();

    /*
     * Initialize the interrupt controller driver so that it is ready to
     * use.
     */
    GicConfig = XScuGic_LookupConfig(XPAR_SCUGIC_SINGLE_DEVICE_ID);
    if (NULL == GicConfig) {
        return XST_FAILURE;
    }

    Status = XScuGic_CfgInitialize(GicPtr, GicConfig,
                       GicConfig->CpuBaseAddress);
    if (Status != XST_SUCCESS) {
        return XST_FAILURE;
    }

    /*
     * Connect the interrupt controller interrupt handler to the hardware
     * interrupt handling logic in the processor.
     */
    Xil_ExceptionRegisterHandler(XIL_EXCEPTION_ID_IRQ_INT,
                 (Xil_ExceptionHandler)XScuGic_InterruptHandler,
                 GicPtr);

    /*
     * Connect the device driver handlers that will be called when an interrupt
     * for the device occurs, the device driver handler performs the specific
     * interrupt processing for the device
     */

    /*
     * Connect the Fault ISR
     */
    Status = XScuGic_Connect(GicPtr,
                 XPAR_XDMAPS_0_FAULT_INTR,
                 (Xil_InterruptHandler)XDmaPs_FaultISR,
                 (void *)DmaPtr);
    if (Status != XST_SUCCESS) {
        return XST_FAILURE;
    }

    /*
     * Connect the Done ISR for all 8 channels of DMA 0
     */
    Status = XScuGic_Connect(GicPtr,
                 XPAR_XDMAPS_0_DONE_INTR_0,
                 (Xil_InterruptHandler)XDmaPs_DoneISR_0,
                 (void *)DmaPtr);

    if (Status != XST_SUCCESS)
        return XST_FAILURE;

    /*
     * Enable the interrupts for the device
     */
    XScuGic_Enable(GicPtr, XPAR_XDMAPS_0_DONE_INTR_0);
    XScuGic_Enable(GicPtr, XPAR_XDMAPS_0_FAULT_INTR);

    Xil_ExceptionEnable();

    return XST_SUCCESS;

}

// modified from Xilinx example code
void DmaDoneHandler(unsigned int Channel, XDmaPs_Cmd *DmaCmd, void *CallbackRef)
{
    volatile int *done = (volatile int *)CallbackRef;
    *done = 1;
    return;
}

/**
* Copyright (C) 2019-2021 Xilinx, Inc
*
* Licensed under the Apache License, Version 2.0 (the "License"). You may
* not use this file except in compliance with the License. A copy of the
* License is located at
*
*     http://www.apache.org/licenses/LICENSE-2.0
*
* Unless required by applicable law or agreed to in writing, software
* distributed under the License is distributed on an "AS IS" BASIS, WITHOUT
* WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied. See the
* License for the specific language governing permissions and limitations
* under the License.
*/

/*******************************************************************************
Description:
    This example uses the load/compute/store coding style, which is generally
    the most efficient for implementing kernels using HLS. The load and store
    functions are responsible for moving data in and out of the kernel as
    efficiently as possible. The core functionality is decomposed across one
    of more compute functions. Whenever possible, the compute function should
    pass data through HLS streams and should contain a single set of nested loops.
    HLS stream objects are used to pass data between producer and consumer
    functions. Stream read and write operations have a blocking behavior which
    allows consumers and producers to synchronize with each other automatically.
    The dataflow pragma instructs the compiler to enable task-level pipelining.
    This is required for to load/compute/store functions to execute in a parallel
    and pipelined manner.
    The kernel operates on vectors of NUM_WORDS integers modeled using the hls::vector
    data type. This datatype provides intuitive support for parallelism and
    fits well the vector-add computation. The vector length is set to NUM_WORDS
    since NUM_WORDS integers amount to a total of 64 bytes, which is the maximum size of
    a kernel port. It is a good practice to match the compute bandwidth to the I/O
    bandwidth. Here the kernel loads, computes and stores NUM_WORDS integer values per
    clock cycle and is implemented as below:
                                       _____________
                                      |             |<----- Input Vector 1 from Global Memory
                                      |  load_input |       __
                                      |_____________|----->|  |
                                       _____________       |  | in1_stream
Input Vector 2 from Global Memory --->|             |      |__|
                               __     |  load_input |        |
                              |  |<---|_____________|        |
                   in2_stream |  |     _____________         |
                              |__|--->|             |<--------
                                      | compute_add |      __
                                      |_____________|---->|  |
                                       ______________     |  | out_stream
                                      |              |<---|__|
                                      | store_result |
                                      |______________|-----> Output result to Global Memory

*******************************************************************************/

#include <stdint.h>
#include <hls_stream.h>

#include "xf_database/compound_sort.hpp"
#include "xf_database/merge_sort.hpp"
#include <hls_print.h>
#define DATA_SIZE  1048576//65536
#define insert_lenn  1024
#define N_1 2
#define block_len  DATA_SIZE/N_1
// TRIPCOUNT identifier
const int c_size = DATA_SIZE;
static void stream(hls::stream<bool> &e_inStream){
	bool e = e_inStream.read();
	while(!e){
	e = e_inStream.read();
//	hls::print("end = %d\n", e);
	}}


template <typename KEY_TYPE>
static void read_input(int* in, hls::stream<KEY_TYPE>& inStream,
		hls::stream<bool> &e_inStream, int size) {
// Auto-pipeline is going to apply pipeline to this loop
mem_rd:
hls::print("begin read");
int a =0;
    for (int i = 0; i < size; i++) {
//#pragma HLS LOOP_TRIPCOUNT min = c_size max = c_size
        inStream << in[i];
        a= in[i];
      //  hls::print("a = %d \n",a);
        e_inStream<<0;
    }
    e_inStream<<1;hls::print("endd \n");
}

template <typename KEY_TYPE>
static void stream_to_stream(hls::stream<KEY_TYPE>& inKeyStrm,
        hls::stream<bool>& inEndStrm,
        hls::stream<KEY_TYPE>& outKeyStrm,
        hls::stream<bool>& outEndStrm){
	hls::print("streamtostream channel");
	bool e= inEndStrm.read();
	while(!e){
		outKeyStrm.write(inKeyStrm.read());

		outEndStrm.write(false);
		e= inEndStrm.read();

	}
	outEndStrm.write(true);

}


template <typename KEY_TYPE, int INSERT_LEN, int N>
	void channel1toNCore(hls::stream<KEY_TYPE>& inKeyStrm,
	                     hls::stream<bool>& inEndStrm,
	                     hls::stream<KEY_TYPE> outKeyStrm[N],
	                     hls::stream<bool> outEndBlockStrm[N],
	                     hls::stream<bool> outEndStrm[N]) {
	hls::print("begin channel");

	    int flag = 0; // number of data in each stream block
	    bool blockFlag = false;
	    int index = 0;
	    bool endFlag = inEndStrm.read(); // end input data
	    if (endFlag) flag = -1;
	    while (!endFlag) {
	#pragma HLS loop_tripcount max = 100000 min = 100000
	#pragma HLS pipeline
	        if ((index % INSERT_LEN == 0) && blockFlag) { // (index % INSERT_LEN == 0) just in the begining cause blochFlag is false
	            outEndBlockStrm[flag].write(true); //at first we indicate that the block is finished
	            blockFlag = false;

	            flag = (flag + 1) % N;//and we pass to the next block
	           // hls::print("flag  =  %d \n",flag);
	        } else {  //we don't attend the end of the block or index =! input length (here we are sure that we attend the end of the block it could be the last one when (index % INSERT_LEN == 0)  )
	        	//hls::print("index %d \n",index);
	        	blockFlag = true;
	            outKeyStrm[flag].write(inKeyStrm.read());
	            outEndBlockStrm[flag].write(false);
	            if (index % INSERT_LEN == 0) {
	                outEndStrm[flag].write(false);
	            }
	            index += 1;
	            endFlag = inEndStrm.read();
	        }
	    }
	    if (flag >= 0) outEndBlockStrm[flag].write(true);
	    for (int i = 0; i < N; i++) {
	#pragma HLS pipeline
	       if (i > flag) {
	        	//hls::print("no leftdata\n");
	        	//hls::print("flag %d \n",flag);
	            outEndBlockStrm[i].write(true); // the defference between outEndBlockStrm and outEndStrm is that we put outstream = 1 at the end but  outblock stream after each block
	            outEndStrm[i].write(false);
	        }
	        outEndStrm[i].write(true);
	    }
	}


template <typename KEY_TYPE, int sort_len, int insert_len >

static void compoundSortWrapper(hls::stream<KEY_TYPE>& inKeyStrm0,
	                      hls::stream<bool>& inEndBlockStrm0,
	                      hls::stream<bool>& inEndStrm0,

	                      hls::stream<KEY_TYPE>& outKeyStrm,
	                      hls::stream<bool>& outEndBlockStrm,
	                      hls::stream<bool>& outEndStrm,
	                      bool order) {
	 bool flag = false;
	    while (!inEndStrm0.read()) {
	#pragma HLS loop_tripcount max = 100 min = 100
	    //    flag = inEndStrm1.read();
	        outEndStrm.write(false);
	        xf::database::compoundSort<int,sort_len,insert_len>(order, inKeyStrm0, inEndBlockStrm0, outKeyStrm, outEndBlockStrm);
	    }
	   // if (!flag) inEndStrm1.read();
	    outEndStrm.write(true);
	}



template <typename KEY_TYPE>

static void read_stream(hls::stream<KEY_TYPE>& inKeyStrm0,
	                      hls::stream<bool>& inEndBlockStrm0,
	                      hls::stream<bool>& inEndStrm0,

						  hls::stream<KEY_TYPE>& inKeyStrm1,
						  hls::stream<bool>& inEndBlockStrm1,
						  hls::stream<bool>& inEndStrm1,

	                      hls::stream<KEY_TYPE>& outKeyStrm,
	                      hls::stream<bool>& outEndBlockStrm,
	                      hls::stream<bool>& outEndStrm,
	                      bool order) {
	 bool flag = false;
	    while (!inEndStrm0.read()) {
	#pragma HLS loop_tripcount max = 100 min = 100
	        flag = inEndStrm1.read();
	        outEndStrm.write(false);
	        xf::database::mergeSort<KEY_TYPE>(inKeyStrm0, inEndBlockStrm0, inKeyStrm1, inEndBlockStrm1, outKeyStrm, outEndBlockStrm,
	                            order);
	    }
	    if (!flag) inEndStrm1.read();
	    outEndStrm.write(true);
	}


static void write_result(int* out, hls::stream<int>& outStream, hls::stream<bool>& e_outStream, int size) {
// Auto-pipeline is going to apply pipeline to this loop
	  hls::print("begin write");
mem_wr:
int i =0;
int a=0;
 bool e=e_outStream.read();
 while(!e){
        out[i] = outStream.read();
        a= out[i];
      //  hls::print("b = %d \n",a);
        e=e_outStream.read();
        i++;
    }
}

extern "C" {
/*
    Vector Addition Kernel Implementation using dataflow
    Arguments:
        in1   (input)  --> Input Vector 1
        in2   (input)  --> Input Vector 2
        out  (output) --> Output Vector
        size (input)  --> Size of Vector in Integer
   */
void vadd(int* in1,int* out,bool order, int size) {// ap_int<32>* in2,//int* in2,int* out2,



#pragma HLS INTERFACE m_axi port = in1 bundle = gmem0
//#pragma HLS INTERFACE m_axi port = in2 bundle = gmem1
#pragma HLS INTERFACE m_axi port = out bundle = gmem2
//#pragma HLS INTERFACE m_axi port = out2 bundle = gmem3
#pragma HLS INTERFACE mode=s_axilite port=return
#pragma HLS INTERFACE mode=s_axilite port=size


//#pragma HLS dataflow
//#pragma HLS PIPELINE II
    // dataflow pragma instruct compiler to run following three APIs in parallel
   // hls::stream<int> inStream1;
    hls::stream<int> inStream0;

    // hls::stream<bool> e_inStream1;
    hls::stream<bool> e_inStream0;

    read_input<int>(in1, inStream0,e_inStream0, DATA_SIZE);
   // read_input(in2, inStream2,e_inStream2, size); //65536,1024
    //stream_to_stream<int>(inStream0,e_inStream0, inStream1,e_inStream1);

    hls::stream<bool> e_outStream1[2];

    hls::stream<bool> e_outStreamBlock1[2];
    hls::stream<int> outStream1[2];
  //  hls::stream<bool> e_outStream2;
   //     hls::stream<int> outStream2;
//#pragma HLS stream variable = outStream depth = 128
//#pragma HLS stream variable = e_outStream depth = 128
//#pragma HLS stream variable = outStream1 depth = 2048
//#pragma HLS stream variable = e_outStreamBlock1 depth = 2048
//#pragma HLS stream variable = e_outStream1 depth = 4
//#pragma HLS bind_storage variable = e_outStreamBlock1 type = fifo impl = lutram


channel1toNCore<int,block_len,N_1>(inStream0, e_inStream0, outStream1, e_outStreamBlock1, e_outStream1);
hls::stream<bool> e_outStream[N_1];
hls::stream<bool> e_outStream_block[N_1];
hls::stream<int> outStream[N_1];

for (int i = 0; i < N_1; i++) {
#pragma HLS unroll
	compoundSortWrapper<int,block_len,insert_lenn>(outStream1[i],
		                   	e_outStreamBlock1[i],
							e_outStream1[i],

							outStream[i],
							e_outStream_block[i],
							e_outStream[i],
		                      order);

	/*
read_stream<int>(outStream1[2*i], e_outStreamBlock1[2*i],e_outStream1[2*i],outStream1[2*i+1],
		e_outStreamBlock1[2*i+1],e_outStream1[2*i+1],
		outStream[i],e_outStream_block[i], e_outStream[i],order);*/
}
hls::stream<int> outStream_f;
hls::stream<bool> e_outStream_block_f;
  //  xf::database::compoundSort<int,64,8>(order, inStream1, e_inStream1, outStream, e_outStream);
    //xf::database::compoundSort<int,64,8>(order, inStream2, e_inStream2, outStream2, e_outStream2);

xf::database::mergeSort<int>(outStream[0], e_outStream_block[0], outStream[1], e_outStream_block[1], outStream_f, e_outStream_block_f,
	                            order);

    write_result(out, outStream_f,e_outStream_block_f, size);

}}

/*
 * scheduler.cu
 *
 *  Created on: Jun 21, 2018
 *      Author: jsong
 */





#include <stdint.h>
#include "scheduler.h"
//#include "req_receiver.h"

//__shared__ uint16_t ia_rr_pointer[NUM_SWITCH_SIZE][NUM_RR_SEQ_SIZE];
//__shared__ uint16_t oa_rr_pointer[NUM_SWITCH_SIZE][NUM_RR_SEQ_SIZE];

uint16_t * ia_rr_pointer;
uint16_t * oa_rr_pointer;


uint8_t * device_voq_all;
uint8_t * host_voq_all;
uint32_t * VoQCount;

uint32_t * device_granted_input;
uint32_t * host_granted_input;
uint32_t * device_granted_output;
uint32_t * host_granted_output;
uint16_t * req_map;

__shared__ int sh_switch_size;
__shared__ int sh_scale_factor;
__shared__ int sh_num_rr_seq;



int _switch_size;
int _scale_factor;
int _num_rr_seq;
int _num_iterations = 1;
int _num_cuda_blk = 4;
int _num_cuda_thread;



extern int eth_socket_init (int    argc, char **argv);
extern void receive_req(int sw_size);



static void CheckCudaErrorAux (const char *, unsigned, const char *, cudaError_t);
#define CUDA_CHECK_RETURN(value) CheckCudaErrorAux(__FILE__,__LINE__, #value, value)

static void CheckCudaErrorAux (const char *file, unsigned line, const char *statement, cudaError_t err)
{
	if (err == cudaSuccess)
		return;
	std::cerr << statement<<" returned " << cudaGetErrorString(err) << "("<<err<< ") at "<<file<<":"<<line << std::endl;
	exit (1);
}

void verify_grant()
{

	for (int i=0; i<_switch_size; i++)
		for (int j=1; j<_switch_size; j++)
		{
			short i1, i2, o1, o2;
			i1 = host_granted_input[i];
			i2 = host_granted_input[(i+j)%_switch_size];
			if (( i1== i2) & (i1 != -1 || i2 != -1))
			{
				printf("SW ERROR Duplicated granted_input[%d]:%d granted_input[%d]:%d \n", i, i1, (i+j)%_switch_size, i2);
			}

			o1 = host_granted_output[i];
			o2 = host_granted_output[(i+j)%_switch_size];
			if (( o1== o2) & (o1 != -1 || o2 != -1))
			{
				printf("SW ERROR Duplicated granted_output[%d]:%d granted_output[%d]:%d \n", i, o1, (i+j)%_switch_size, o2);
			}
		}
}


void copy_host_grant_reset_dev_grant()
{
	CUDA_CHECK_RETURN(cudaMemcpy(host_granted_input, device_granted_input, sizeof(uint32_t)*_switch_size, cudaMemcpyDeviceToHost));
	CUDA_CHECK_RETURN(cudaMemcpy(host_granted_output, device_granted_output, sizeof(uint32_t)*_switch_size, cudaMemcpyDeviceToHost));
	cudaMemset (device_granted_input, 0x0, sizeof(uint16_t)*_switch_size);
	cudaMemset (device_granted_output, 0x0, sizeof(uint16_t)*_switch_size);
}

void reset_voq(int in_idx, int out_idx)
{
	host_voq_all[in_idx*_switch_size+out_idx] = 0;
}

void update_voq()
{

	for (int i=0; i<_switch_size; i++)
	{
		if (host_granted_output[i] == 0)
			continue;

		if (VoQCount[i*_switch_size+host_granted_output[i]]==0)
			printf("SW ERROR input:%d output:%d count:%d !!!!!\n", i, host_granted_output[i], VoQCount[i*_switch_size+host_granted_output[i]]);
		VoQCount[i*_switch_size+host_granted_output[i]]--;

		if (VoQCount[i*_switch_size+host_granted_output[i]] == 0)
		{
			reset_voq(i, host_granted_output[i]);
		}
	}
}

void reset_req()
{
	cudaMemset (req_map, 0, sizeof(uint16_t)*_switch_size*_switch_size);
}

__device__ void print_req_device (int idx, uint16_t * req_map )
{
	//printf ("Output %d \n", idx);

	for (int i=0; i<sh_switch_size; i++)
	{
		if (req_map[i*sh_switch_size+idx] == 0)
			continue;
		printf ("Req(%d->%d):%d ",i, idx, req_map[i*sh_switch_size+idx]);
	}
	printf ("\n");
}

__global__ void print_req_global(uint16_t * req_map)
{
	int out_idx = blockIdx.x*blockDim.x+threadIdx.x;;
    print_req_device (out_idx, req_map);
}

__device__ void print_voq_device (int idx, uint8_t * voq )
{
	for (int i=0; i<sh_switch_size; i++)
	{
		printf("(%d->%d):%d ", idx, i, voq[i]);
	}
	printf(" \n");
}

__global__ void print_voq_global(uint8_t * voq)
{
	int in_idx = blockIdx.x*blockDim.x+threadIdx.x;;
    print_voq_device (in_idx, voq);
}

void print_req(uint16_t * req_map)
{
	print_req_global<<<_num_cuda_blk,_num_cuda_thread>>>(req_map);
}

__device__ void device_send_request(int in_idx,  uint8_t * voq_map, int ts, uint16_t * req_map, uint32_t* granted_input, uint16_t * rr_ptr)
{
	int rr_start = rr_ptr[in_idx*sh_num_rr_seq+ts%sh_num_rr_seq];

	for (int i=0; i<sh_switch_size; i++)
	{
		int out_idx = (rr_start+i)%sh_switch_size;
		if (voq_map[in_idx*sh_switch_size+out_idx] == 0)
			continue;

		req_map[in_idx*sh_switch_size+out_idx] = 1;
		return;
	}
}

__global__ void cuda_send_request(uint8_t * voq_map, int ts, uint32_t * granted_input, uint32_t * granted_output,uint16_t * req_map, uint16_t * rr_ptr)
{
	int input_idx = blockIdx.x*blockDim.x+threadIdx.x;
	//printf("Send Request for input:%04d \n", input_idx);

	if (granted_output[input_idx] != 0)
	{
		printf("already granted for input:%d, ouput:%d \n", input_idx, granted_output[input_idx]);
		return;
	}
    device_send_request(input_idx, voq_map, ts,req_map, granted_input, rr_ptr);
}

__device__ void device_send_grant(int out_idx, int ts, uint16_t * req_map, uint32_t * granted_input, uint32_t * granted_output, uint16_t * rr_ptr)
{
	int rr_start = rr_ptr[out_idx*sh_num_rr_seq+ts%sh_num_rr_seq];

	for (int i=0; i<sh_switch_size; i++)
	{
		int in_idx = (rr_start+i)%sh_switch_size;
		if (req_map[in_idx*sh_switch_size+out_idx] == 0)
			continue;

		granted_output [in_idx] = out_idx;
		granted_input [out_idx] = in_idx;

		return;
	}
}
__global__ void cuda_send_grant (int ts, uint16_t * req_map, uint32_t * granted_input, uint32_t * granted_output, uint16_t * rr_ptr)
{
	int output_idx = blockIdx.x*blockDim.x+threadIdx.x;
	device_send_grant(output_idx, ts, req_map, granted_input, granted_output, rr_ptr);
}

__global__ void cuda_cleanup_for_new_iter ()
{
	;
}

void print_voq(uint8_t * voq)
{
	for (int i=0; i<_switch_size; i++)
	{
		printf ("  VoQ for input %d: ", i);

		for (int j=0; j<_switch_size; j++)
		{
			printf ("0x%02x ", voq[i*_switch_size+j]);
		}
		printf ("\n");
	}
}

__device__ void init_rr_pointer_device (int idx, uint16_t * i_rr_ptr, uint16_t * o_rr_ptr )
{
	int off_set = idx*sh_num_rr_seq;
	for (int i=0; i<sh_num_rr_seq; i++)
	{
		i_rr_ptr[off_set+i] = (i+idx)%sh_switch_size;
		o_rr_ptr[off_set+i] = (sh_switch_size+i-idx)%sh_switch_size;
		//printf ("idx:%d i:%d, ,off_set:%d, i_rr:%d, o_rr:%d \n",idx, i, off_set, i_rr_ptr[idx*sh_switch_size+i], o_rr_ptr[idx*sh_switch_size+i]);
	}
}

__global__ void init_rr_pointer(uint16_t * i_rr_ptr, uint16_t * o_rr_ptr )
{
	int idx = blockIdx.x*blockDim.x+threadIdx.x;;
	init_rr_pointer_device (idx, i_rr_ptr, o_rr_ptr );
}

__device__ void print_rr_pointer_device (int idx, uint16_t * i_rr_ptr)
{
	//printf ("idx:%d - %d %d %d %d %d %d ...\n", idx, i_rr_ptr->pointer[idx][0], i_rr_ptr->pointer[idx][1], i_rr_ptr->pointer[idx][2], i_rr_ptr->pointer[idx][3], i_rr_ptr->pointer[idx][4], i_rr_ptr->pointer[idx][5]);
}

__global__ void print_rr_pointer(uint16_t * i_rr_ptr)
{
	int idx = blockIdx.x*blockDim.x+threadIdx.x;;
	print_rr_pointer_device (idx, i_rr_ptr);
}

void printDevProp(cudaDeviceProp devProp)
{
    printf("Major revision number:         %d\n",  devProp.major);
    printf("Minor revision number:         %d\n",  devProp.minor);
    printf("Name:                          %s\n",  devProp.name);
    printf("Total global memory:           %lu\n",  devProp.totalGlobalMem);
    printf("Total shared memory per block: %lu\n",  devProp.sharedMemPerBlock);
    printf("Total registers per block:     %d\n",  devProp.regsPerBlock);
    printf("Warp size:                     %d\n",  devProp.warpSize);
    printf("Maximum memory pitch:          %lu\n",  devProp.memPitch);
    printf("Maximum threads per block:     %d\n",  devProp.maxThreadsPerBlock);
    for (int i = 0; i < 3; ++i)
    printf("Maximum dimension %d of block:  %d\n", i, devProp.maxThreadsDim[i]);
    for (int i = 0; i < 3; ++i)
    printf("Maximum dimension %d of grid:   %d\n", i, devProp.maxGridSize[i]);
    printf("Clock rate:                    %d\n",  devProp.clockRate);
    printf("Total constant memory:         %lu\n",  devProp.totalConstMem);
    printf("Texture alignment:             %lu\n",  devProp.textureAlignment);
    printf("Concurrent copy and execution: %s\n",  (devProp.deviceOverlap ? "Yes" : "No"));
    printf("Number of multiprocessors:     %d\n",  devProp.multiProcessorCount);
    printf("Kernel execution timeout:      %s\n",  (devProp.kernelExecTimeoutEnabled ? "Yes" : "No"));
    return;
}

int pefrom_scheduling (int ts)
{
	CUDA_CHECK_RETURN(cudaMemcpy(device_voq_all, host_voq_all, sizeof(uint8_t)*_switch_size*_switch_size,cudaMemcpyHostToDevice));

	for (int iter=0; iter<_num_iterations; iter++)
	{
    	reset_req();
    	//print_req(req_map);
    	cuda_send_request<<<_num_cuda_blk,_num_cuda_thread>>>(device_voq_all, ts, device_granted_input, device_granted_output, req_map, ia_rr_pointer);
    	//cudaDeviceSynchronize();
		//print_req(req_map);
		cuda_send_grant <<<_num_cuda_blk,_num_cuda_thread>>>(ts, req_map, device_granted_input, device_granted_output, oa_rr_pointer);
		cudaDeviceSynchronize();
		cuda_cleanup_for_new_iter <<<_num_cuda_blk,_num_cuda_thread>>>();

	}
	copy_host_grant_reset_dev_grant();

	return 0;
}

void copyMsgVoQToDevice (msgRequest_t * req)
{
	int in_idx = req->s_pfwi_id-1;
    CUDA_CHECK_RETURN(cudaMemcpy((void *)&req->voq_info, (void *)&device_voq_all[in_idx*_switch_size], sizeof(uint8_t)*_switch_size, cudaMemcpyHostToDevice));
	return;
}



void generate_packet(int load)
{
	int random_port;
	int toss;
	for (int i=0; i<_switch_size; i++)
	{
		toss = rand()%100;
		if (toss < load)
		{
			random_port = rand()%_switch_size;
			if (VoQCount[i*_switch_size+random_port] == 0)
			{
				int idx = random_port;
				if (host_voq_all[i*_switch_size+idx])
				{
					printf("SW Error idx:%d, count:%d, voq:0x%x \n",random_port, VoQCount[i*_switch_size+random_port], host_voq_all[i*_switch_size+idx]);
				}
				host_voq_all[i*_switch_size+idx] =  1;
			}
			if (VoQCount[i*_switch_size+random_port] < NUM_VOQ_BUFFER_SIZE)
			{
				VoQCount[i*_switch_size+random_port] ++;
			}
		//printf("Gen Packet %d->%d \n",i, random_port);
		}
	}
}

__global__ void init_shared_value(int _switch_size, int _num_rr_seq)
{
	sh_switch_size = _switch_size;
	sh_num_rr_seq = _num_rr_seq;
	//printf("Init shared value sh_switch_size:%d, sh_block_size:%d, sh_num_rr_seq:%d, sh_num_req_per_uint32:%d \n", sh_switch_size, sh_block_size, sh_num_rr_seq, sh_num_req_per_uint32);
}

__global__ void print_shared_value()
{
	printf("shared value sh_switch_size:%d, sh_num_rr_seq:%d \n", sh_switch_size, sh_num_rr_seq);
}

void init_scheduler()
{
	CUDA_CHECK_RETURN(cudaDeviceReset());
	_scale_factor = 2;
	_num_rr_seq = _switch_size*_scale_factor;
	printf("Init Values scale_factor:%d, num_rr_seq:%d \n", _scale_factor, _num_rr_seq );

	init_shared_value<<<_num_cuda_blk,_num_cuda_thread>>>(_switch_size, _num_rr_seq);
	CUDA_CHECK_RETURN(cudaMalloc((void **) &device_voq_all, sizeof(uint8_t)*_switch_size*_switch_size));
	CUDA_CHECK_RETURN(cudaMemset ((void *) device_voq_all, 0, sizeof(uint8_t)*_switch_size*_switch_size));

	if (host_voq_all)
		free(host_voq_all);
	host_voq_all = (uint8_t *) malloc (sizeof(uint8_t)*_switch_size*_switch_size);
	memset(host_voq_all,0x0, sizeof(uint8_t)*_switch_size*_switch_size);
	CUDA_CHECK_RETURN(cudaMalloc ((void **) &device_granted_input, sizeof(uint16_t)*_switch_size));
	CUDA_CHECK_RETURN(cudaMemset((void*) device_granted_input, 0, sizeof(uint16_t)*_switch_size));

	CUDA_CHECK_RETURN(cudaMalloc ((void **) &device_granted_output, sizeof(uint16_t)*_switch_size));
	CUDA_CHECK_RETURN(cudaMemset((void*) device_granted_output, 0, sizeof(uint16_t)*_switch_size));

	if (host_granted_input)
		free(host_granted_input);
	host_granted_input = (uint32_t *) malloc (sizeof(uint32_t)*_switch_size);
	memset(host_granted_input,0, sizeof(uint32_t)*_switch_size);

	if (host_granted_output)
		free(host_granted_output);
	host_granted_output = (uint32_t *) malloc (sizeof(uint32_t)*_switch_size);
	memset(host_granted_output,0, sizeof(uint32_t)*_switch_size);


	CUDA_CHECK_RETURN(cudaMalloc ((void **) &req_map, sizeof(uint16_t)*_switch_size*_switch_size));

	CUDA_CHECK_RETURN(cudaMalloc ((void **) &ia_rr_pointer, sizeof(uint16_t)*_switch_size*_num_rr_seq));
	CUDA_CHECK_RETURN(cudaMalloc ((void **) &oa_rr_pointer, sizeof(uint16_t)*_switch_size*_num_rr_seq));
	init_rr_pointer<<<_num_cuda_blk,_num_cuda_thread>>>(ia_rr_pointer, oa_rr_pointer );

	if (VoQCount)
		free(VoQCount);
	VoQCount = (uint32_t *) malloc (sizeof(uint32_t)*_switch_size*_switch_size);
	memset(VoQCount,0x0, sizeof(uint32_t)*_switch_size*_switch_size);

}

void * start_receive_req_thread(void * arg)
{
	int * sw_size = (int *) (arg);
	receive_req(*sw_size);
	return 0;

}

int main(int argc, char * argv[])
{


    int devCount = 0;
    cudaGetDeviceCount(&devCount);
    printf("CUDA Device Query...\n");
    printf("There are %d CUDA devices.\n", devCount);

    if (devCount == 0)
	{
		std::cout<<"devCount : " << devCount << "  --> No GPU installed " <<std::endl;
		exit(1);
	}
    //cudaSetDevice(0);

    // Iterate through devices

    for (int i = 0; i < devCount; ++i)
    {
        // Get device properties
        printf("\nCUDA  #%d \n", i);
        cudaDeviceProp devProp;
        cudaGetDeviceProperties(&devProp, i);
        printDevProp(devProp);
    }

    cudaSetDevice(1);

	_num_cuda_thread = _switch_size/_num_cuda_blk;

#ifdef SIM
	FILE * SimOutFile;
	FILE * SimOutFile2;

    int _duration = 1000;
    int _load = 90;
    int _ts_th = -1;
	int _q_th = -1;
	int _aware = 1;

	uint64_t sum_delay, measure_count, max_delay, min_delay, measure_start;

	char fname[60];
	char fname2[60];

	sprintf(fname,"sim_result_scheduling_time_block_%d.txt", _num_cuda_blk);
	SimOutFile = fopen(fname,"w");
	printf("open fname %s\n", fname);

	for (_load = 50; _load<51; _load ++ )
	for (_switch_size = 8; _switch_size<2000; _switch_size ++ )

	{

		if (_load%10 != 0 || _switch_size%_num_cuda_blk != 0 || _switch_size%8 != 0)
		{
			continue;
		}

		sprintf(fname2,"sim_result_load_%d_switch_%d_block_%d.txt", _load, _switch_size, _num_cuda_blk);
		SimOutFile2 = fopen(fname2,"w");
		printf("open fname2 %s\n", fname2);

		sum_delay=0;
		measure_count = 0;
		measure_start = 0;
		max_delay = 0;
		min_delay = 1000000;
		_num_cuda_thread = _switch_size/_num_cuda_blk;
		init_scheduler();

		for (int i=0; i<_duration; i++)
		{
			// process request
			if (i==0)
				measure_start = 0;

			int _time_slot = i;

			generate_packet(_load);

			clock_t t1 = clock();

			pefrom_scheduling ( _time_slot);

			clock_t t2 = clock();

			clock_t diff = t2-t1;
			int schedule_time_usec = diff*1000000/CLOCKS_PER_SEC;
			//printf("Delta t2-t1: %d \n \n", schedule_time_usec);

			if (i==100)
				measure_start = 1;
			if (measure_start)
			{
				if (diff > max_delay)
					max_delay = diff;
				if (diff < min_delay)
					min_delay = diff;

				sum_delay+= diff;
				measure_count++;
				fprintf(SimOutFile2, "delay %d\n", diff);

			}

			update_voq();  // This operation is excluded for processing time because this happens in linecards
			//cudaDeviceSynchronize();

		}
		fprintf(SimOutFile, "switch_size %d load %d iteration %d max_delay %lu min_delay %lu sum_delay %lu measure_count %lu\n",
				_switch_size,_load, _num_iterations, max_delay, min_delay, sum_delay, measure_count);
		printf("switch_size %d load %d iteration %d max_delay %lu min_delay %lu sum_delay %lu measure_count %lu\n",
				_switch_size,_load, _num_iterations, max_delay, min_delay, sum_delay, measure_count);
		fclose(SimOutFile2);

	}
	fclose(SimOutFile);
	printf("close %s %d\n", fname, SimOutFile);
	return 0;
#else
	if (argc < 3)
	{
		std::cout<<" argc " << argc<< "-- exit " <<std::endl;
		std::cout<<"More Arguments required - switch_size, iteration" <<argc<<std::endl;
		exit(1);
	}


	_switch_size = atoi(argv[1]);
	_num_iterations = atoi(argv[2]);


	init_scheduler();

	pthread_t       SynchProc_threadID, ReqProc_threadID;


	// create ReqProc_threadID
	if (pthread_create(&ReqProc_threadID, NULL, start_receive_req_thread, & _switch_size) != 0)
	{
		printf("ReqProc Failed [%s, %d, %s]\n", __FILE__, __LINE__, __FUNCTION__);
	} else
	{
		printf("ReqProc is started [%s, %d, %s]\n", __FILE__, __LINE__, __FUNCTION__);
	}

/*
	// SynchProc_thread
	if (pthread_create(&SynchProc_threadID, NULL, start_receive_synch_thread, & _switch_size) != 0)
	{
		printf("SynchProc Failed [%s, %d, %s]\n", __FILE__, __LINE__, __FUNCTION__);
	} else
	{
		printf("SynchProc is started [%s, %d, %s]\n", __FILE__, __LINE__, __FUNCTION__);
	}
*/


	return 0;

#endif

}








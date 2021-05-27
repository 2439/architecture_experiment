#include <iostream>
#include <time.h>
#include <Windows.h>

using namespace std;

#define ARRAY_SIZE (1 << 28)                                    // test array size is 2^28
typedef unsigned char BYTE;										// define BYTE as one-byte type

BYTE array[ARRAY_SIZE];											// test array
int L1_cache_size = 1 << 15;
int L2_cache_size = 1 << 18;
int L1_cache_block = 64;
int L2_cache_block = 64;
int L1_way_count = 8;
int L2_way_count = 4;
int write_policy = 0;											// 0 for write back ; 1 for write through

// have an access to arrays with L2 Data Cache'size to clear the L1 cache
void Clear_L1_Cache() {
	memset(array, 0, L2_cache_size);
}

// have an access to arrays with ARRAY_SIZE to clear the L2 cache
void Clear_L2_Cache() {
	memset(&array[L2_cache_size + 1], 0, ARRAY_SIZE - L2_cache_size);
}

int Cache_Size_Result(int test_size_begin, int test_size_count, int* average_time) {
	int test_size;
	int cache_size_result;
	int max_tmp_time;
	int tmp;

	test_size = test_size_begin;
	cache_size_result = (1 << test_size);
	max_tmp_time = average_time[1] - average_time[0];
	for(int i=0; i<test_size_count; i++)
	{
		// output 
		cout << "Test_Array_Size = " << (1 << test_size) << "KB, ";
		cout << "Average access time = " << average_time[i] << "ms" << endl;
		if(i < test_size_count-1)
		{
			// find max time gap
			tmp = average_time[i+1] - average_time[i];
			if(tmp > max_tmp_time)
			{
				cache_size_result = (1 << test_size);
				max_tmp_time = tmp;
			}
		}
		test_size++;
	}
	return cache_size_result;
}

void Cache_Size_Time(int test_size_begin, int test_size_count, int* average_time) {
	int test_size;
	int test_array_size;
	int test_times = 11451490;
	char test_char;
	clock_t start, finish;

	test_size = test_size_begin;
	for(int i=0; i<test_size_count; i++)
	{
		test_array_size = (1 << (test_size + 10));
		// put to cache
		Clear_L1_Cache();
		Clear_L2_Cache();
		// rand read
		start = clock();
		for(int j=0; j<test_times; j++)
		{
			test_char += array[(rand() * rand()) % test_array_size];
		}
		finish = clock();
		average_time[i] = finish - start;
		
		test_size++;	// next_size
	}
}

int L1_DCache_Size() {
	cout << "L1_Data_Cache_Test" << endl;
	//add your own code
	
	// setting
	int average_time[5];
	int test_size_begin = 3;	// test begin from 8kB
	int test_size_count = 5;	// test from 8KB to 128KB
	
	// result
	int L1_cache_size_result = 0;

	// access and make time
	Cache_Size_Time(test_size_begin, test_size_count, average_time);

	// output result and find Cache size
	L1_cache_size_result = Cache_Size_Result(test_size_begin, test_size_count, average_time);
	cout << "L1_Dada_Cache_Size is " << L1_cache_size_result << "KB" << endl;
	return L1_cache_size_result << 10;
}

int L2_Cache_Size() {
	cout << "L2_Data_Cache_Test" << endl;
	//add your own code
	
	// setting
	int average_time[5];
	int test_size_begin = 6;	// test begin from 64kB
	int test_size_count = 5;	// test from 64KB to 1024KB

	// result
	int L2_cache_size_result = 0;
	
	// access and make time
	Cache_Size_Time(test_size_begin, test_size_count, average_time);
	
	// output result and find Cache size
	L2_cache_size_result = Cache_Size_Result(test_size_begin, test_size_count, average_time);
	cout << "L2_Dada_Cache_Size is " << L2_cache_size_result << "KB" << endl;
	return L2_cache_size_result << 10;
}

int L1_DCache_Block() {
	cout << "L1_DCache_Block_Test" << endl;
	//add your own code

	// set
	int block_size_begin = 4;		// block size test from 4B ~ 128B
	int block_size_count = 6;
	int average_time[6];

	// tmp
	unsigned int block_size_tmp = block_size_begin;
	char test_char;
	clock_t start, finish;

	// grap bigger
	for(int i=0; i<block_size_count; i++)
	{
		// test time
		start = clock();
		for(int j=0; j<block_size_tmp; j++)
		{
			for(int k=0; k<ARRAY_SIZE; k+=block_size_tmp)
			{
				test_char += array[k];
			}
		}
		finish = clock();
		average_time[i] = finish - start;

		block_size_tmp = block_size_tmp << 1;
	}

	// out result
	block_size_tmp = block_size_begin;
	for(int i=0; i<block_size_count; i++)
	{
		cout << "block_size:" << block_size_tmp << " B ";
		cout << "average time:" << average_time[i] << "ms ";
		cout << endl;
		block_size_tmp = block_size_tmp << 1;		
	}
	
	cout << "L1_Dada_Block_Size is " << L1_cache_block << "B" << endl;
	return L1_cache_block;
}

int L2_Cache_Block() {
	cout << "L2_Cache_Block_Test" << endl;
	//add your own code
	
	// set
	int block_size_begin = 4;		// block size test from 4B ~ 128B
	int block_size_count = 6;
	int average_time[6];

	// tmp
	unsigned int block_size_tmp;
	char test_char;
	clock_t start, finish;

	// grap bigger
	block_size_tmp = block_size_begin;
	for(int i=0; i<block_size_count; i++)
	{
		// test time
		start = clock();
		for(int j=0; j<block_size_tmp; j++)
		{
			for(int k=0; k<ARRAY_SIZE; k+=block_size_tmp)
			{
				test_char += array[k];
			}
		}
		finish = clock();
		average_time[i] = finish - start;

		block_size_tmp = block_size_tmp << 1;
	}

	// out result
	block_size_tmp = block_size_begin;
	for(int i=0; i<block_size_count; i++)
	{
		cout << "block_size:" << block_size_tmp << " B ";
		cout << "average time:" << average_time[i] << "ms ";
		cout << endl;
		block_size_tmp = block_size_tmp << 1;		
	}
	cout << "L2_Dada_Block_Size is " << L2_cache_block << "B" << endl;
	return L2_cache_block;
}

int L1_DCache_Way_Count() {
	cout << "L1_DCache_Way_Count" << endl;
	//add your own code
	
	// set
	int array_size = L1_cache_size << 1;
	int group_begin = 2;	// test from 2group to 32group，result from 1way to 16way
	int group_count = 5;
	int average_time[5];
	int test_time = 114514;
	
	// temp
	int group_temp;
	int array_jump;
	char test_char;
	clock_t start, finish, tmp;
	
	// max
	int max_tmp_time;
	int way_result;
	
	// test_time
	group_temp = group_begin;
	for(int i=0; i<group_count; i++)
	{
		array_jump = array_size / group_temp;	// divide array_size array into way_temp group
		start = clock();
		for(int j=0; j < test_time; j++)
		{
			for(int k=0; k < group_temp; k += 2)
			{
				memset(&array[k*array_jump], 0, array_jump);
			}
		}
		finish = clock();
		average_time[i] = finish - start;
		group_temp <<= 1;
	}
	
	// output
	group_temp = group_begin;
	max_tmp_time = average_time[1] - average_time[0];
	way_result = group_temp / 2;
	for(int i=0; i<group_count; i++)
	{
		cout << "way:" << group_temp / 2 << " ";
		cout << "average time:" << average_time[i] << "ms ";
		if(i < group_count-1)
		{
			// find max time gap
			tmp = average_time[i+1] - average_time[i];
			cout << tmp << endl; 
			if(tmp > max_tmp_time)
			{
				way_result = group_temp / 2;
				max_tmp_time = tmp;
			}
		}
		group_temp = group_temp << 1;
	}
	cout << endl;
	cout << "L1_way_count is " << way_result << endl;
	return way_result;
}

int L2_Cache_Way_Count() {
	cout << "L2_Cache_Way_Count" << endl;
	//add your own code
	
	// set
	int array_size = L2_cache_size << 1;
	int group_begin = 2;	// test from 2group to 32group，result from 1way to 16way
	int group_count = 5;
	int average_time[5];
	int test_time = 114514;
	
	// temp
	int group_temp;
	int array_jump;
	char test_char;
	clock_t start, finish, tmp;
	
	// max
	int max_tmp_time;
	int way_result;
	
	// test_time
	group_temp = group_begin;
	for(int i=0; i<group_count; i++)
	{
		array_jump = array_size / group_temp;	// divide array_size array into way_temp group
		start = clock();
		for(int j=0; j < test_time; j++)
		{
			for(int k=0; k < group_temp; k += 2)
			{
				memset(&array[k*array_jump], 0, array_jump);
			}
		}
		finish = clock();
		average_time[i] = finish - start;
		group_temp <<= 1;
	}
	
	// output
	group_temp = group_begin;
	max_tmp_time = average_time[1] - average_time[0];
	way_result = group_temp / 2;
	for(int i=0; i<group_count; i++)
	{
		cout << "way:" << group_temp / 2 << " ";
		cout << "average time:" << average_time[i] << "ms ";
		if(i < group_count-1)
		{
			// find max time gap
			tmp = average_time[i+1] - average_time[i];
			cout << tmp << endl; 
			if(tmp > max_tmp_time)
			{
				way_result = group_temp / 2;
				max_tmp_time = tmp;
			}
		}
		group_temp = group_temp << 1;
	}
	cout << endl;
	cout << "L2_way_count is " << way_result << endl;
	return way_result ;
}

int Cache_Write_Policy() {
	cout << "Cache_Write_Policy" << endl;
	//add your own code
}

void Check_Swap_Method() {
	cout << "L1_Check_Replace_Method" << endl;
	//add your own code
	
}

int main() {
	SetPriorityClass(GetCurrentProcess(), REALTIME_PRIORITY_CLASS);
	SetThreadPriority(GetCurrentThread(), THREAD_PRIORITY_TIME_CRITICAL);
	L1_cache_size = L1_DCache_Size();
	L2_cache_size = L2_Cache_Size();
	L1_cache_block = L1_DCache_Block();
	L2_cache_block = L2_Cache_Block();
	L1_way_count = L1_DCache_Way_Count();
	L2_way_count = L2_Cache_Way_Count();
	write_policy = Cache_Write_Policy();
	Check_Swap_Method();
	system("pause");
	return 0;
}


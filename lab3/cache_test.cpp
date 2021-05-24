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

int L1_DCache_Size() {
	cout << "L1_Data_Cache_Test" << endl;
	//add your own code
	
	// setting
	float average_time[5];
	int test_size_begin = 3;	// test begin from 8kB
	int test_size_count = 5;	// test from 8KB to 128KB
	int test_times = 114514900;
		
	// tmp
	clock_t start;
	clock_t finish;
	int test_size; 		
	int test_array_size;
	char test_char;
	// result
	int L1_cache_size_result = 0;
	clock_t max_tmp_time;
	clock_t tmp; 
	
	test_size = test_size_begin;
	for(int i=0; i<test_size_count; i++)	// 8kB~128KB
	{
		Clear_L1_Cache();
		test_array_size = (1 << (test_size + 10));
		// set rand read
		// rand read
		start = clock();
		for(int j=0; j<test_times; j++)
		{
			test_char = array[rand() % (test_array_size-(1<<(test_size+9)))];	//?????????????????
//			test_char = array[rand() % test_array_size];
		}
		finish = clock();
		average_time[i] = finish - start;
		
		test_size++;	// next_size
	}
	
	// output result and find Cache size
	test_size = test_size_begin;
	L1_cache_size_result = (1 << test_size);
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
				L1_cache_size_result = (1 << test_size);
				max_tmp_time = tmp;
			}
		}
		test_size++;
	}
	cout << "L1_Dada_Cache_Size is " << L1_cache_size_result << "KB" << endl;
}

int L2_Cache_Size() {
	cout << "L2_Data_Cache_Test" << endl;
	//add your own code
	
	float average_time[5];
	int test_size_begin = 6;	// test begin from 8kB
	int test_size_count = 5;	// test from 8KB to 128KB
	int test_times = 114514900;
		
	// tmp
	clock_t start;
	clock_t finish;
	int test_size; 		
	int test_array_size;
	char test_char;
	// result
	int L2_cache_size_result = 0;
	clock_t max_tmp_time;
	clock_t tmp; 
	
	test_size = test_size_begin;
	for(int i=0; i<test_size_count; i++)	// 8kB~128KB
	{
		Clear_L2_Cache();
		Clear_L1_Cache();
		test_array_size = (1 << (test_size + 10));
		// set rand read
		// rand read
		start = clock();
		for(int j=0; j<test_times; j++)
		{
//			test_char = array[rand() % (test_array_size-(1<<(test_size+9)))];	//?????????????????
			test_char = array[rand() % test_array_size];
		}
		finish = clock();
		average_time[i] = finish - start;
		
		test_size++;	// next_size
	}
	
	// output result and find Cache size
	test_size = test_size_begin;
	L2_cache_size_result = (1 << test_size);
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
				L2_cache_size_result = (1 << test_size);
				max_tmp_time = tmp;
			}
		}
		test_size++;
	}
	cout << "L2_Dada_Cache_Size is " << L2_cache_size_result << "KB" << endl;
}

int L1_DCache_Block() {
	cout << "L1_DCache_Block_Test" << endl;
	//add your own code
}

int L2_Cache_Block() {
	cout << "L2_Cache_Block_Test" << endl;
	//add your own code
}

int L1_DCache_Way_Count() {
	cout << "L1_DCache_Way_Count" << endl;
	//add your own code
}

int L2_Cache_Way_Count() {
	cout << "L2_Cache_Way_Count" << endl;
	//add your own code
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


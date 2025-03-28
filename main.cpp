//#undef DEBUG
//#undef PARALLEL
#define PARALLEL
//#define DEBUG
#include <iostream>
#include <algorithm>
#include <vector>
#include <iterator>
#include <ctime>
#include <iomanip>
#include <cstdlib>
extern "C" {
#include "Zeitmessung.h"
#ifdef PARALLEL
#include <omp.h>
#endif // PARALLEL
}

using namespace std;

const unsigned AveragingLimit = 100;
const unsigned P2mergeThreads = 4;

template <typename sorttype>
inline void simplemerge(sorttype A[], size_t A_length, sorttype B[], size_t B_length, sorttype Result[])
{
    size_t i = 0;      //Für A
    size_t j = 0;      //Für B
    size_t k = 0;      //Für Result

    while(i < A_length && j < B_length)
    {
        if(A[i] < B[j])
            {Result[k++] = A[i++];}

        else
            {Result[k++] = B[j++];}
    }
    
    while(i < A_length)
        {Result[k++] = A[i++];}

    while(j < B_length)
        {Result[k++] = B[j++];}
}

//equalabove
template <typename sorttype>
inline size_t binsearch(sorttype data[], size_t numitems, sorttype value)
{
    sorttype* low = data;
    sorttype* high = data + numitems - 1;
    size_t result = numitems;

    while (low <= high)
    {
        sorttype* mid = low + (high - low) / 2;
        if (*mid < value)
            {low = mid + 1;}
        else
        {
            result = mid - data;
            high = mid - 1;
        }
    }
    return result;
}

//equalbelow
template <typename sorttype>
inline size_t binsearch2(sorttype data[], size_t numitems, sorttype value)
{
    sorttype* low = data;
    sorttype* high = data + numitems - 1;
    size_t result = numitems;

    while (low <= high)
    {
        sorttype* mid = low + (high - low) / 2;
        if (*mid < value) 
            {low = mid + 1;}
        else
        {
            result = mid - data;
            high = mid - 1;
        }
    }
    return result;
}

template <typename sorttype>
size_t equalBelow(sorttype x, sorttype Array[], size_t sizeArray)
{
    size_t erg = 0;
    for(size_t i = 0; i < sizeArray; i++)
        {if(Array[i] < x)     {erg = i + 1;}}
    
    return erg;
}

template <typename sorttype>
size_t equalAbove(sorttype x, sorttype Array[], size_t sizeArray)
{
    size_t erg = sizeArray;
    for(int i = sizeArray - 1; i >= 0; i--)
        {if(Array[i] > x)     {erg = size_t(i);}}

    return erg;
}

template <typename sorttype>
inline void pmerge(sorttype A[], size_t A_length, sorttype B[], size_t B_length, sorttype Result[])
{
    #ifdef PARALLEL
   	#pragma omp parallel for
	#endif // PARALLEL
    for(size_t i = 0; i < A_length; i++)
        {Result[equalBelow(A[i], B, B_length) + i] = A[i];}


    #ifdef PARALLEL
   	#pragma omp parallel for
	#endif // PARALLEL
    for(size_t j = 0; j < B_length; j++)
        {Result[equalAbove(B[j], A, A_length) + j] = B[j];}
}

template <typename sorttype>
void rang(size_t& start, size_t& end, sorttype* arr1, size_t arr1_len, sorttype* arr2, size_t arr2_len)
{
    sorttype first = arr1[0];
    sorttype last = arr1[arr1_len - 1];
    //start = equalAbove(first, arr2, arr2_len);
    start = binsearch(arr2, arr2_len, first);
    
    //end = equalBelow(last, arr2, arr2_len);
    end = binsearch2(arr2, arr2_len, last);
}

template <typename sorttype>
inline void p2merge(sorttype A[], size_t A_length, sorttype B[], size_t B_length, sorttype Result[])
{
    //stand 17.01.2025
    if(A_length + B_length < 100)
        {simplemerge(A, A_length, B, B_length, Result);}

    else if(A_length + B_length >= 100)
    {
        sorttype* A_blocks[P2mergeThreads];
        size_t A_block_size = A_length / P2mergeThreads;
        size_t A_rest = A_length - ((P2mergeThreads - 1) * A_block_size);
        size_t A_block_sizes[P2mergeThreads];

        for (size_t i = 0; i < P2mergeThreads - 1; i++)            {A_block_sizes[i] = A_block_size;}
        A_block_sizes[P2mergeThreads - 1] = A_rest;

        size_t A_start_index = 0;
        for(size_t i = 0; i < P2mergeThreads; i++)
        {
            sorttype* ptr = new sorttype[A_block_sizes[i]];
            for(size_t j = 0; j < A_block_sizes[i]; j++)
                {*(ptr + j) = A[A_start_index + j];}
            
            A_blocks[i] = ptr;
            ptr = nullptr;
            A_start_index = A_start_index + A_block_sizes[i];
        }

        sorttype* B_blocks[P2mergeThreads];
        size_t B_block_sizes[P2mergeThreads];
        size_t B_block_starts[P2mergeThreads];
        size_t B_block_ends[P2mergeThreads];

        for(size_t i = 0; i < P2mergeThreads; i++)
        {
            size_t temp_B_start;
            size_t temp_B_end;
            rang(temp_B_start, temp_B_end, A_blocks[i], A_block_sizes[i], B, B_length);
            B_block_sizes[i] = temp_B_end - temp_B_start;
            B_block_starts[i] = temp_B_start;
            B_block_ends[i] = temp_B_end;
        }

        size_t B_start_index = 0;
        for(size_t i = 0; i < P2mergeThreads; i++)
        {
            sorttype* ptr = new sorttype[B_block_sizes[i]];
            for(size_t j = 0; j < B_block_sizes[i]; j++)
                {*(ptr + j) = B[B_start_index + j];}
            
            B_blocks[i] = ptr;
            ptr = nullptr;
            B_start_index = B_start_index + B_block_sizes[i];
        }
        
        sorttype* sorted_blocks[(2 * P2mergeThreads) + 1];
        size_t sorted_blocks_sizes[(2 * P2mergeThreads) + 1];

        
        for(size_t i = 0; i < P2mergeThreads; i++)
        {
            sorted_blocks_sizes[i] = A_block_sizes[i] + B_block_sizes[i];
            sorted_blocks[i] = new sorttype[sorted_blocks_sizes[i]];
        }
        
        size_t Result_index = 0;

        #ifdef PARALLEL
   	    #pragma omp parallel for
	    #endif // PARALLEL
        for(size_t i = 0; i < P2mergeThreads; i++)          // Parallelisierung des Merges der A- und B-Blöche
        {
            simplemerge(A_blocks[i], A_block_sizes[i], B_blocks[i], B_block_sizes[i], sorted_blocks[i]);
            Result_index = Result_index + A_block_sizes[i] + B_block_sizes[i];
        }

        for(size_t i = 0; i < P2mergeThreads; i++)
        {
            delete[] A_blocks[i];
            delete[] B_blocks[i];
        }

        vector<sorttype> B_remaining;
        size_t index = 0;

        for(size_t i = 0; i < P2mergeThreads; i++)
        {
            size_t temp_B_start = B_block_starts[i];
            size_t temp_B_end = B_block_ends[i];

            while (index < temp_B_start)
                {B_remaining.push_back(B[index++]);}

            index = temp_B_end;
        }

        while (index < B_length)
            {B_remaining.push_back(B[index++]);}
        
        sorted_blocks_sizes[P2mergeThreads] = B_remaining.size();
        sorttype* ptr = new sorttype[B_remaining.size()];
        
        for(size_t i = 0; i < B_remaining.size(); i++)
            {*(ptr + i) = B_remaining[i];}

        sorted_blocks[P2mergeThreads] = ptr;
        ptr = nullptr;

        size_t counter = 0;
        size_t start_quantity_arrays = P2mergeThreads + 1;

        for(size_t i = counter; i < start_quantity_arrays + counter - 1; i = i + 2)
        {
            sorted_blocks_sizes[start_quantity_arrays + counter] = sorted_blocks_sizes[i] + sorted_blocks_sizes[i + 1];
            sorted_blocks[start_quantity_arrays + counter] = new sorttype[sorted_blocks_sizes[start_quantity_arrays + counter]];
            simplemerge(sorted_blocks[i], sorted_blocks_sizes[i], sorted_blocks[i + 1], sorted_blocks_sizes[i + 1], sorted_blocks[start_quantity_arrays + counter]);
            counter++;        
        }

        for(size_t i = 0; i < sorted_blocks_sizes[2 * P2mergeThreads]; i++)
            {Result[i] = *(sorted_blocks[2 * P2mergeThreads] + i);}

        for(size_t i = 0; i <= 2*P2mergeThreads; i++)
            {delete[] sorted_blocks[i];}
    }
}


template <typename sorttype>
sorttype* mergesort(sorttype Data[], size_t NumItems)
{
    if (NumItems == 1)
    {
        sorttype* result = new sorttype[NumItems];
        for(size_t i = 0; i < NumItems; i++)
            {result[i] = Data[i];}

        return result;
    }

    else
    {
        size_t sizeOfArrays[2] = {NumItems / 2, NumItems - (NumItems / 2)};
        sorttype* pointers[2] = {Data, (Data + sizeOfArrays[0])};
        sorttype* sorted_parts[2];

        //#ifdef PARALLEL
   	    //#pragma omp parallel for
	    //#endif // PARALLEL
        for(size_t i = 0; i < 2; i++)
            {sorted_parts[i] = mergesort(pointers[i], sizeOfArrays[i]);}

        sorttype* result = new sorttype[NumItems];
        //simplemerge(sorted_parts[0], sizeOfArrays[0], sorted_parts[1], sizeOfArrays[1], result);
        //pmerge(sorted_parts[0], sizeOfArrays[0], sorted_parts[1], sizeOfArrays[1], result);
        p2merge(sorted_parts[0], sizeOfArrays[0], sorted_parts[1], sizeOfArrays[1], result);

        return result;
    }
}

template <typename sorttype>
sorttype* getData(size_t Amount)
{
    sorttype* Data = new sorttype[Amount];
    for (size_t i=0; i<Amount; i++)
    {
        Data[i]=sorttype(rand());
    }
    return Data;
}

template <typename sorttype>
void checkSorting(sorttype Data[], size_t NumItems)
{
#ifdef DEBUG
    for (size_t i=0; i<NumItems-1; i++)
    {
        cout << Data[i] << endl ;
    }
    cout << endl;
#endif // DEBUG
    for (size_t i=0; i<NumItems-1; i++)
    {
        if (Data[i]>Data[i+1])
        {
            cout << "." ;
            exit(9);
        }
    }
}



int main()
{
    srand(time(nullptr));
    const size_t datasizeMax = 200000001;

    for (size_t datasize=10; datasize<datasizeMax; datasize*=2)
    {
        t_timevalue CumulativeDuration = 0;
        t_timevalue MesTime =0;
        unsigned cycle;
        for(cycle=0; cycle<AveragingLimit && CumulativeDuration < 1e9; cycle++)
        {
            unsigned* Data = getData<unsigned>(datasize);
            unsigned* Result=nullptr;
            t_stopwatch MyStopwatch;
            startStopwatch(&MyStopwatch);
            Result=mergesort<unsigned>(Data, datasize);
            MesTime=stopStopwatch(&MyStopwatch);
            checkSorting<unsigned>(Result, datasize);
            delete[] Result;
            delete[] Data;
            CumulativeDuration+=MesTime;
        }
        cout << setw(12) << datasize << " "
             << setw(12) << CumulativeDuration/1000000.0/cycle << "  "
             << setw(12) << cycle << "  "
             << endl;
    }
    return 0;
}
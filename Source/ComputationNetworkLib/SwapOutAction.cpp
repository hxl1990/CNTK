//
// Copyright (c) Microsoft. All rights reserved.
// Licensed under the MIT license. See LICENSE.md file in the project root for full license information.
//

#define _CRT_SECURE_NO_WARNINGS // "secure" CRT not available on all platforms  --add this at the top of all CPP files that give "function or variable may be unsafe" warnings

#include "SwapOutAction.h"
#include <iostream>
#include <string>

#ifndef CPUONLY
	#include <cuda_runtime.h>
#endif


namespace Microsoft { namespace MSR { namespace CNTK {

using std::cout;
using std::endl;

template <typename ElemType>
SwapOutAction<ElemType>::SwapOutAction(Matrix<ElemType> *GPUbuffer)
{
        this->m_bufferCPU = NULL;
        this->m_bufferGPU = GPUbuffer;
        cudaStream_t stream;
        CUDA_CALL(cudaStreamCreate(&stream));
        this->m_streamAsync = stream;
        this->m_rows = this->m_bufferGPU->GetNumRows();
        this->m_cols = this->m_bufferGPU->GetNumCols();
        this->m_bytes = this->m_rows*this->m_cols*sizeof(ElemType);

        // do we already have a pinned, that is page-locked buffer?
        if (!this->m_bufferCPU){ allocatePinnedBuffer(); }
}

template <typename ElemType>
SwapOutAction<ElemType>::~SwapOutAction(){ ReleaseMemory(); }

template <typename ElemType>
void SwapOutAction<ElemType>::BeginAction()
{
    // perform the actual asynchronous copy
    CUDA_CALL(cudaMemcpyAsync(this->m_bufferCPU, this->m_bufferGPU->Data(), this->m_bytes, cudaMemcpyDefault, this->m_streamAsync));
}

template <typename ElemType>
void SwapOutAction<ElemType>::EndAction()
{
    CUDA_CALL(cudaStreamSynchronize(m_streamAsync));
    this->m_rows = this->m_bufferGPU->GetNumRows();
    this->m_cols = this->m_bufferGPU->GetNumCols();
}


template <typename ElemType>
void SwapOutAction<ElemType>::allocatePinnedBuffer()
{
    //cudaHostAllocPortable preservse the page-lock even across threads
    CUDA_CALL(cudaHostAlloc(&(this->m_bufferCPU), this->m_bytes, cudaHostAllocPortable));
}

template <typename ElemType>
void SwapOutAction<ElemType>::ReleaseMemory(){ CUDA_CALL(cudaFreeHost(this->m_bufferCPU)); }

}}}


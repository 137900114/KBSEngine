#pragma once
#include "d3dcommon.h"
#include "CopyBatch.h"
#include "TransitionBatch.h"
#include "DescriptorAllocator.h"

enum TEXTURE_FORMAT {
	//TEXTURE_FORMAT_RGB,
	TEXTURE_FORMAT_FLOAT4,
	TEXTURE_FORMAT_FLOAT2,
	TEXTURE_FORMAT_RGBA,
	TEXTURE_FORMAT_RG ,
	TEXTURE_FORMAT_FLOAT,
	TEXTURE_FORMAT_DEPTH_STENCIL
};

enum TEXTURE_TYPE {
	TEXTURE_TYPE_2D,
	TEXTURE_TYPE_2DCUBE
};

enum TEXTURE_FLAG {
	TEXTURE_FLAG_NONE = 0,
	TEXTURE_FLAG_ALLOW_RENDER_TARGET = 1,
	TEXTURE_FLAG_ALLOW_DEPTH_STENCIL = 1 << 1,
	TEXTURE_FLAG_ALLOW_UNORDERED_ACCESS = 1 << 2
};

//
class Texture {
public:
	Texture(size_t width,size_t height,TEXTURE_FORMAT format,
		TEXTURE_FLAG flag = TEXTURE_FLAG_NONE,D3D12_RESOURCE_STATES initState = D3D12_RESOURCE_STATE_COMMON,
		D3D12_CLEAR_VALUE* clearValue = nullptr);

	Texture(size_t width, size_t height, size_t mipnum, TEXTURE_FORMAT format,
		TEXTURE_FLAG flag = TEXTURE_FLAG_NONE, D3D12_RESOURCE_STATES initState = D3D12_RESOURCE_STATE_COMMON,
		D3D12_CLEAR_VALUE* clearValue = nullptr);

	Texture(size_t width, size_t height, TEXTURE_FORMAT format,
		TEXTURE_TYPE type,TEXTURE_FLAG flag = TEXTURE_FLAG_NONE,
		D3D12_RESOURCE_STATES initState = D3D12_RESOURCE_STATE_COMMON,
		D3D12_CLEAR_VALUE* cv = nullptr);

	Texture(size_t width, size_t height, size_t mipnum, TEXTURE_FORMAT format,
		TEXTURE_TYPE type, TEXTURE_FLAG flag = TEXTURE_FLAG_NONE,
		D3D12_RESOURCE_STATES initState = D3D12_RESOURCE_STATE_COMMON,
		D3D12_CLEAR_VALUE* cv = nullptr);

	Texture(size_t width,size_t height,TEXTURE_FORMAT format,
		void** data,TEXTURE_FLAG flag  = TEXTURE_FLAG_NONE,
		D3D12_RESOURCE_STATES initState = D3D12_RESOURCE_STATE_COMMON,
		UploadBatch* batch = nullptr);

	Texture(size_t width, size_t height, size_t mipLevel, TEXTURE_FORMAT format,
		void** data, TEXTURE_FLAG flag = TEXTURE_FLAG_ALLOW_UNORDERED_ACCESS,
		D3D12_RESOURCE_STATES initState = D3D12_RESOURCE_STATE_COMMON);

	Texture(size_t width, size_t height, TEXTURE_FORMAT format,
		TEXTURE_TYPE type, 
		void** original_buffer,
		size_t original_buffer_num,
		D3D12_SUBRESOURCE_DATA* sub_res,
		size_t sub_res_num,
		D3D12_RESOURCE_STATES initState = D3D12_RESOURCE_STATE_COMMON,
		TEXTURE_FLAG flag = TEXTURE_FLAG_NONE
		, UploadBatch* batch = nullptr);



	ID3D12Resource* GetResource() {
		if (!isValid) return nullptr;
		return mRes.Get();
	}
	size_t GetWidth() { return width; }
	size_t GetHeight() { return height; }

	TEXTURE_TYPE GetType() { return type; }
	DXGI_FORMAT GetFormat() { return format; }
	bool IsValid() { return isValid; }

	void CreateShaderResourceView(Descriptor descriptor,D3D12_SHADER_RESOURCE_VIEW_DESC* srv = nullptr);
	D3D12_CPU_DESCRIPTOR_HANDLE GetShaderResourceViewCPU() {
		return mSRV.cpuHandle;
	}
	D3D12_GPU_DESCRIPTOR_HANDLE GetShaderResourceViewGPU() {
		return mSRV.gpuHandle;
	}

	void CreateRenderTargetView(Descriptor descriptor,D3D12_RENDER_TARGET_VIEW_DESC* rtv = nullptr);
	D3D12_CPU_DESCRIPTOR_HANDLE GetRenderTargetViewCPU() { return mRTV.cpuHandle; }
	D3D12_GPU_DESCRIPTOR_HANDLE GetRenderTargetViewGPU() { return mRTV.gpuHandle; }

	void CreateDepthStencilView(Descriptor descriptor,D3D12_DEPTH_STENCIL_VIEW_DESC* dsv = nullptr);
	D3D12_CPU_DESCRIPTOR_HANDLE GetDepthStencilViewCPU() { return mDSV.cpuHandle; }
	D3D12_GPU_DESCRIPTOR_HANDLE GetDepthStencilViewGPU() { return mDSV.gpuHandle; }

	void CreateUnorderedAccessView(Descriptor descriptor,D3D12_UNORDERED_ACCESS_VIEW_DESC* uav = nullptr,size_t mip = 0);
	D3D12_CPU_DESCRIPTOR_HANDLE GetUnorderedAccessViewCPU(size_t mip = 0) { return mip < mipnum ? mUAV[mip].cpuHandle : D3D12_CPU_DESCRIPTOR_HANDLE{0}; }
	D3D12_GPU_DESCRIPTOR_HANDLE GetUnorderedAccessViewGPU(size_t mip = 0) { return mip < mipnum ? mUAV[mip].gpuHandle : D3D12_GPU_DESCRIPTOR_HANDLE{0}; }

	D3D12_RESOURCE_STATES StateTransition(D3D12_RESOURCE_STATES nextState,TransitionBatch* batch = nullptr);
	D3D12_RESOURCE_STATES GetResourceState() { return currState; }
protected:
	TEXTURE_TYPE type;
	TEXTURE_FLAG flag;
	DXGI_FORMAT  format;
	size_t width, height,mipnum;

	ComPtr<ID3D12Resource> mRes;
	D3D12_RESOURCE_STATES currState;
	bool isValid;

	Descriptor mSRV;
	Descriptor mRTV;
	Descriptor mDSV;
	Descriptor mUAV[32];
};
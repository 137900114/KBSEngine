#include "PostProcessRenderPass.h"
#include "ComputeCommand.h"
#include "graphic.h"
#include "MathFunctions.h"


bool PostProcessRenderPass::Initialize(UploadBatch* batch) {
	size_t winHeight = gGraphic.GetScreenHeight(), winWidth = gGraphic.GetScreenWidth();
	mTex = std::make_unique<Texture>(winWidth,winHeight,TEXTURE_FORMAT_FLOAT4,TEXTURE_FLAG_ALLOW_UNORDERED_ACCESS,
		D3D12_RESOURCE_STATE_COPY_SOURCE);

	mHeap = std::make_unique<DescriptorHeap>(heapDefaultCapacity);
	mTex->CreateUnorderedAccessView(mHeap->Allocate());
	mTex->CreateShaderResourceView(mHeap->Allocate());
	return true;
}

void PostProcessRenderPass::RegisterPostProcessPass(PostProcessPass* pass) {
	if (!pass->Initialize(this)) {
		return;
	}
	size_t priority = pass->GetPriority();
	if (auto res = PPPQueue.find(priority);res != PPPQueue.end()) {
		res->second.push_back(pass);
		return;
	}
	PPPQueue[priority] = std::vector<PostProcessPass*>{ pass };
}

void PostProcessRenderPass::PostProcess(ID3D12Resource* renderTarget) {
	if (PPPQueue.empty()) {
		return;
	}

	{
		ComputeCommand cc = ComputeCommand::Begin();
		cc.ResourceTrasition(renderTarget, D3D12_RESOURCE_STATE_RENDER_TARGET, D3D12_RESOURCE_STATE_COPY_SOURCE);
		//cc.ResourceTrasition(mTex->GetResource(), D3D12_RESOURCE_STATE_COPY_SOURCE, D3D12_RESOURCE_STATE_COPY_DEST);
		mTex->StateTransition(D3D12_RESOURCE_STATE_COPY_DEST);
		cc.ResourceCopy(mTex->GetResource(), renderTarget);
		//cc.ResourceTrasition(mTex->GetResource(), D3D12_RESOURCE_STATE_COPY_DEST, D3D12_RESOURCE_STATE_UNORDERED_ACCESS);
		cc.BindDescriptorHeap(mHeap->GetHeap());

		for (auto ppps : PPPQueue) {
			for (auto* ppp : ppps.second) {
				ppp->PostProcess(mTex.get(), &cc);
			}
		}

		//cc.ResourceTrasition(mTex->GetResource(), D3D12_RESOURCE_STATE_UNORDERED_ACCESS, D3D12_RESOURCE_STATE_COPY_SOURCE);
		mTex->StateTransition(D3D12_RESOURCE_STATE_COPY_SOURCE);
		cc.ResourceTrasition(renderTarget, D3D12_RESOURCE_STATE_COPY_SOURCE, D3D12_RESOURCE_STATE_COPY_DEST);
		cc.ResourceCopy(renderTarget, mTex->GetResource());
		cc.ResourceTrasition(renderTarget, D3D12_RESOURCE_STATE_COPY_DEST, D3D12_RESOURCE_STATE_RENDER_TARGET);

		cc.End();
	}
}

void PostProcessRenderPass::finalize() {
	for (auto ppps : PPPQueue) {
		for (auto* ppp : ppps.second) {
			ppp->finalize();
		}
	}
	PPPQueue.clear();
	mTex.release();
	mHeap.release();
}

Descriptor PostProcessRenderPass::AllocateDescriptor(size_t num) {
	{ return mHeap->Allocate(Game::imax(num, 1)); }
}
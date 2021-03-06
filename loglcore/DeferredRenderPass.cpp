#include "DeferredRenderPass.h"
#include "graphic.h"
#include "TextureManager.h"
#include "LightManager.h"

bool DeferredRenderPass::Initialize(UploadBatch* batch) {
	ID3D12Device* device = gGraphic.GetDevice();

	{
		Game::RootSignature rootSig(7, 1);
		rootSig[0].initAsConstantBuffer(0, 0);
		rootSig[1].initAsConstantBuffer(1, 0);
		rootSig[2].initAsDescriptorTable(0, 0, 1, D3D12_DESCRIPTOR_RANGE_TYPE_SRV);
		rootSig[3].initAsDescriptorTable(1, 0, 1, D3D12_DESCRIPTOR_RANGE_TYPE_SRV);
		rootSig[4].initAsDescriptorTable(2, 0, 1, D3D12_DESCRIPTOR_RANGE_TYPE_SRV);
		rootSig[5].initAsDescriptorTable(3, 0, 1, D3D12_DESCRIPTOR_RANGE_TYPE_SRV);
		rootSig[6].initAsDescriptorTable(4, 0, 1, D3D12_DESCRIPTOR_RANGE_TYPE_SRV);
		rootSig.InitializeSampler(0, CD3DX12_STATIC_SAMPLER_DESC(0));

		if (!gGraphic.CreateRootSignature(defPreproc, &rootSig)) {
			OUTPUT_DEBUG_STRING("DeferredRenderPass::Initialize fail to create root signature\n");
			return false;
		}
		std::vector<D3D12_INPUT_ELEMENT_DESC> layout = {
			{"POSITION",0,DXGI_FORMAT_R32G32B32_FLOAT,0,0,D3D12_INPUT_CLASSIFICATION_PER_VERTEX_DATA,0},
			{"NORMAL",0,DXGI_FORMAT_R32G32B32_FLOAT,0,12,D3D12_INPUT_CLASSIFICATION_PER_VERTEX_DATA,0},
			{"TEXCOORD",0,DXGI_FORMAT_R32G32_FLOAT,0,24,D3D12_INPUT_CLASSIFICATION_PER_VERTEX_DATA,0},
			{"TANGENT",0,DXGI_FORMAT_R32G32B32_FLOAT,0,32,D3D12_INPUT_CLASSIFICATION_PER_VERTEX_DATA,0},
		};

		Shader* shader = gShaderManager.loadShader(L"../shader/DeferredRPGBuffer.hlsl",
			"VS", "PS", defPreproc, layout, defPreproc, nullptr);
		if (shader == nullptr) {
			OUTPUT_DEBUG_STRING("DeferredRenderPass::Initialize fail to create preprocess shader\n");
			return false;
		}

		Game::GraphicPSO GBufferPSO;
		GBufferPSO.LazyBlendDepthRasterizeDefault();
		GBufferPSO.SetSampleMask(UINT_MAX);
		GBufferPSO.SetPrimitiveTopologyType(D3D12_PRIMITIVE_TOPOLOGY_TYPE_TRIANGLE);
		GBufferPSO.SetFlag(D3D12_PIPELINE_STATE_FLAG_NONE);
		DXGI_FORMAT rtvFormats[GBufferNum] = { DXGI_FORMAT_R32G32B32A32_FLOAT ,DXGI_FORMAT_R32G32B32A32_FLOAT
			,DXGI_FORMAT_R32G32B32A32_FLOAT,DXGI_FORMAT_R32G32B32A32_FLOAT };
		GBufferPSO.SetRenderTargetFormat(GBufferNum, rtvFormats);
		GBufferPSO.SetDepthStencilViewFomat(DXGI_FORMAT_D24_UNORM_S8_UINT);

		if (!gGraphic.CreatePipelineStateObject(shader, &GBufferPSO, defPreproc)) {
			OUTPUT_DEBUG_STRING("DeferredRenderPass::Initialize fail to create pso for preprocess\n");
			return false;
		}

		Game::RootSignature rootSigAnimated(8, 1);
		rootSigAnimated[0].initAsConstantBuffer(0, 0);
		rootSigAnimated[1].initAsConstantBuffer(1, 0);
		rootSigAnimated[2].initAsDescriptorTable(0, 0, 1, D3D12_DESCRIPTOR_RANGE_TYPE_SRV);
		rootSigAnimated[3].initAsDescriptorTable(1, 0, 1, D3D12_DESCRIPTOR_RANGE_TYPE_SRV);
		rootSigAnimated[4].initAsDescriptorTable(2, 0, 1, D3D12_DESCRIPTOR_RANGE_TYPE_SRV);
		rootSigAnimated[5].initAsDescriptorTable(3, 0, 1, D3D12_DESCRIPTOR_RANGE_TYPE_SRV);
		rootSigAnimated[6].initAsDescriptorTable(4, 0, 1, D3D12_DESCRIPTOR_RANGE_TYPE_SRV);
		rootSigAnimated[7].initAsShaderResource(5, 0);
		rootSigAnimated.InitializeSampler(0, CD3DX12_STATIC_SAMPLER_DESC(0));

		if (!gGraphic.CreateRootSignature(defSkinnedPreproc,&rootSigAnimated)) {
			OUTPUT_DEBUG_STRING("DeferredRenderPass::Initialize fail to create rootsignature for animated deferred render pass\n");
			return false;
		}
		std::vector<D3D12_INPUT_ELEMENT_DESC> skinnedLayout = {
			{"POSITION",0,DXGI_FORMAT_R32G32B32_FLOAT,0,0,D3D12_INPUT_CLASSIFICATION_PER_VERTEX_DATA,0},
			{"NORMAL",0,DXGI_FORMAT_R32G32B32_FLOAT,0,12,D3D12_INPUT_CLASSIFICATION_PER_VERTEX_DATA,0},
			{"TEXCOORD",0,DXGI_FORMAT_R32G32_FLOAT,0,24,D3D12_INPUT_CLASSIFICATION_PER_VERTEX_DATA,0},
			{"TANGENT",0,DXGI_FORMAT_R32G32B32_FLOAT,0,32,D3D12_INPUT_CLASSIFICATION_PER_VERTEX_DATA,0},
			{"TEXCOORD",1,DXGI_FORMAT_R32G32B32A32_UINT,0,44,D3D12_INPUT_CLASSIFICATION_PER_VERTEX_DATA,0},
			{"TEXCOORD",2,DXGI_FORMAT_R32G32B32A32_FLOAT,0,60,D3D12_INPUT_CLASSIFICATION_PER_VERTEX_DATA,0}
		};

		D3D_SHADER_MACRO macros[] = {
			{"USE_SKINNED_VERTEX",""},
			{NULL,NULL}
		};

		shader = gShaderManager.loadShader(L"../shader/DeferredRPGBuffer.hlsl", "VS", "PS", defSkinnedPreproc,
			skinnedLayout, defSkinnedPreproc,macros);

		if (shader == nullptr) {
			OUTPUT_DEBUG_STRING("DeferredRenderPass::Initialize fail to create shader for skinned gbuffer\n");
			return false;
		}

		Game::GraphicPSO animatedPso;
		animatedPso.LazyBlendDepthRasterizeDefault();
		animatedPso.SetSampleMask(UINT_MAX);
		animatedPso.SetPrimitiveTopologyType(D3D12_PRIMITIVE_TOPOLOGY_TYPE_TRIANGLE);
		animatedPso.SetFlag(D3D12_PIPELINE_STATE_FLAG_NONE);
		animatedPso.SetRenderTargetFormat(GBufferNum, rtvFormats);
		animatedPso.SetDepthStencilViewFomat(DXGI_FORMAT_D24_UNORM_S8_UINT);
		if (!gGraphic.CreatePipelineStateObject(shader,&animatedPso,defSkinnedPreproc)) {
			OUTPUT_DEBUG_STRING("DeferredRenderPass::Initialize fail to create pso for skinned gbuffer\n");
			return false;
		}

		descriptorHeap = std::make_unique<DescriptorHeap>(defaultDescriptorHeapSize);
		descriptorHeap->ClearUploadedDescriptors(D3D12_DESCRIPTOR_HEAP_TYPE_CBV_SRV_UAV);

		Descriptor desc = descriptorHeap->Allocate(GBufferNum, D3D12_DESCRIPTOR_HEAP_TYPE_CBV_SRV_UAV);

		D3D12_CLEAR_VALUE cv;
		cv.Format = DXGI_FORMAT_R32G32B32A32_FLOAT;
		memset(cv.Color, 0, sizeof(cv.Color));

		for (size_t i = 0; i != GBufferNum; i++) {
			GBuffer[i] = std::make_unique<Texture>(gGraphic.GetScreenWidth(), gGraphic.GetScreenHeight(),
				TEXTURE_FORMAT_FLOAT4, TEXTURE_FLAG_ALLOW_RENDER_TARGET, D3D12_RESOURCE_STATE_COMMON, &cv);
			GBuffer[i]->CreateRenderTargetView(descriptorHeap->Allocate(1, D3D12_DESCRIPTOR_HEAP_TYPE_RTV));
			GBuffer[i]->CreateShaderResourceView(desc.Offset(D3D12_DESCRIPTOR_HEAP_TYPE_CBV_SRV_UAV, i));
		}


		allocatedConstantBuffers = 0;
		objConstants = std::make_unique<ConstantBuffer<ObjectPass>>(device, defaultConstantBufferSize);
	}

	{
		Game::Vector4 rect[] = {
			{-1.,-1.,0.,1.},
			{ 1.,-1.,1.,1.},
			{ 1., 1.,1.,0.},
			{-1., 1.,0.,0.},
			{-1.,-1.,0.,1.},
			{ 1., 1.,1.,0.}
		};

		mImageVert = std::make_unique<StaticMesh<Game::Vector4>>(device, _countof(rect), rect, batch);

		Game::RootSignature rootSigShading(6, 2);
		rootSigShading[0].initAsConstantBuffer(0, 0);
		rootSigShading[1].initAsConstantBuffer(1, 0);
		rootSigShading[2].initAsDescriptorTable(0, 0, GBufferNum, D3D12_DESCRIPTOR_RANGE_TYPE_SRV);
		rootSigShading[3].initAsDescriptorTable(4, 0, 1, D3D12_DESCRIPTOR_RANGE_TYPE_SRV);
		rootSigShading[4].initAsDescriptorTable(5, 0, 1, D3D12_DESCRIPTOR_RANGE_TYPE_SRV);
		rootSigShading[5].initAsDescriptorTable(6, 0, 1, D3D12_DESCRIPTOR_RANGE_TYPE_SRV);
		D3D12_SAMPLER_DESC sd;
		sd.AddressV = D3D12_TEXTURE_ADDRESS_MODE_MIRROR;
		rootSigShading.InitializeSampler(0, CD3DX12_STATIC_SAMPLER_DESC(0));
		rootSigShading.InitializeSampler(1, CD3DX12_STATIC_SAMPLER_DESC(1));

		if (!gGraphic.CreateRootSignature(defShading, &rootSigShading)) {
			OUTPUT_DEBUG_STRING("fail to create root signature for deferred render pass\n");
			return false;
		}

		std::vector<D3D12_INPUT_ELEMENT_DESC> layoutShading = {
			{"POSITION",0,DXGI_FORMAT_R32G32_FLOAT,0,0,D3D12_INPUT_CLASSIFICATION_PER_VERTEX_DATA,0},
			{"TEXCOORD",0,DXGI_FORMAT_R32G32_FLOAT,0,8,D3D12_INPUT_CLASSIFICATION_PER_VERTEX_DATA,0}
		};
		D3D_SHADER_MACRO macros[] = {
			{"MAX_LIGHT_STRUCT_NUM",getShaderMaxLightStructNumStr},
			{nullptr,nullptr}
		};

		Shader* shader = gShaderManager.loadShader(L"../shader/DeferredRPShading.hlsl", "VS", "PS",
			defShading, layoutShading, defShading, macros);

		Game::GraphicPSO GBufferShadingPSO;

		GBufferShadingPSO.LazyBlendDepthRasterizeDefault();
		CD3DX12_DEPTH_STENCIL_DESC ds(D3D12_DEFAULT);
		ds.DepthWriteMask = D3D12_DEPTH_WRITE_MASK_ZERO;
		ds.DepthFunc = D3D12_COMPARISON_FUNC_ALWAYS;
		GBufferShadingPSO.SetDepthStencilState(ds);

		CD3DX12_RASTERIZER_DESC rd(D3D12_DEFAULT);
		rd.CullMode = D3D12_CULL_MODE_NONE;
		GBufferShadingPSO.SetRasterizerState(rd);

		GBufferShadingPSO.SetSampleMask(UINT_MAX);
		GBufferShadingPSO.SetPrimitiveTopologyType(D3D12_PRIMITIVE_TOPOLOGY_TYPE_TRIANGLE);
		GBufferShadingPSO.SetFlag(D3D12_PIPELINE_STATE_FLAG_NONE);
		GBufferShadingPSO.SetRenderTargetFormat(gGraphic.GetRenderTargetFormat());
		GBufferShadingPSO.SetDepthStencilViewFomat(gGraphic.GetBackBufferDepthFormat());

		if (!gGraphic.CreatePipelineStateObject(shader, &GBufferShadingPSO, defShading)) {
			OUTPUT_DEBUG_STRING("fail to create pso for deferred render pass");
			return false;
		}
		lutTex = gTextureManager.loadTexture(lutMapPath, L"_pbr_rendering_util_lut",
			true, batch);
		if (lutTex == nullptr) {
			OUTPUT_DEBUG_STRING("fail to load lut table\n");
			return false;
		}
		Descriptor lutDesc = descriptorHeap->Allocate(1);
		lutTex->CreateShaderResourceView(lutDesc);

		shadingPass = std::make_unique<DeferredShadingPass>(this);
		gGraphic.RegisterRenderPass(shadingPass.get());
	}

	return true;
}

void DeferredRenderPass::Render(Graphic* graphic, RENDER_PASS_LAYER layer) {
	if (layer != RENDER_PASS_LAYER_OPAQUE) return;
	if (objQueue.empty() && animatedObjQueue.empty()) return;
	graphic->BindPSOAndRootSignature(defPreproc, defPreproc);
	activated = true;
	
	D3D12_CPU_DESCRIPTOR_HANDLE rtvHandles[GBufferNum];
	for (size_t i = 0; i != GBufferNum;i++) {
		rtvHandles[i] = GBuffer[i]->GetRenderTargetViewCPU();
	}

	TransitionBatch tbatch = TransitionBatch::Begin();
	D3D12_RESOURCE_STATES resLastState;
	for (size_t i = 0; i != GBufferNum;i++) {
		resLastState = GBuffer[i]->StateTransition(D3D12_RESOURCE_STATE_RENDER_TARGET, &tbatch);
	}
	tbatch.End();

	Game::Vector4 ClearValue = Game::Vector4();
	graphic->BindRenderTarget(rtvHandles, { 0 }, GBufferNum, true,
		ClearValue.raw, nullptr, nullptr);
	graphic->BindDescriptorHeap(descriptorHeap->GetHeap());
	graphic->BindMainCameraPass(1);
	

	for (auto& ele : objQueue) {
		graphic->BindConstantBuffer(objConstants->GetADDR(ele.objectID), 0);
		graphic->BindDescriptorHandle(ele.normalMap, 2);
		graphic->BindDescriptorHandle(ele.diffuseMap, 3);
		graphic->BindDescriptorHandle(ele.roughnessMap, 4);
		graphic->BindDescriptorHandle(ele.metallicMap, 5);
		graphic->BindDescriptorHandle(ele.emissionMap, 6);
		if (ele.ibv == nullptr) {
			graphic->Draw(ele.vbv, ele.start, ele.num);
		}
		else{
			graphic->Draw(ele.vbv, ele.ibv, ele.start, ele.num);
		}
		
	}

	graphic->BindPSOAndRootSignature(defSkinnedPreproc, defSkinnedPreproc);
	for (auto& item : animatedObjQueue) {
		graphic->BindConstantBuffer(objConstants->GetADDR(item.objEle.objectID), 0);
		graphic->BindDescriptorHandle(item.objEle.normalMap, 2);
		graphic->BindDescriptorHandle(item.objEle.diffuseMap, 3);
		graphic->BindDescriptorHandle(item.objEle.roughnessMap, 4);
		graphic->BindDescriptorHandle(item.objEle.metallicMap, 5);
		graphic->BindDescriptorHandle(item.objEle.emissionMap, 6);
		graphic->BindShaderResource(item.boneTransform, 7);
		if (item.objEle.ibv == nullptr) {
			graphic->Draw(item.objEle.vbv, item.objEle.start, item.objEle.num);
		}
		else {
			graphic->Draw(item.objEle.vbv, item.objEle.ibv, item.objEle.start, item.objEle.num);
		}
	}
	
	tbatch = TransitionBatch::Begin();
	for (size_t i = 0; i != GBufferNum; i++) {
		GBuffer[i]->StateTransition(resLastState, &tbatch);
	}
	tbatch.End();

	graphic->BindCurrentBackBufferAsRenderTarget();

	/*SkyboxRenderPass* sbrp = gGraphic.GetRenderPass<SkyboxRenderPass>();
	Texture* irrMap = sbrp->GetIrradianceMap();
	D3D12_GPU_DESCRIPTOR_HANDLE irrDescHandle = descriptorHeap->UploadDescriptors(D3D12_DESCRIPTOR_HEAP_TYPE_CBV_SRV_UAV,
		irrMap->GetShaderResourceViewCPU()).gpuHandle;
	Texture* specMap = sbrp->GetSpecularIBLMap();
	D3D12_GPU_DESCRIPTOR_HANDLE specDescHandle = descriptorHeap->UploadDescriptors(D3D12_DESCRIPTOR_HEAP_TYPE_CBV_SRV_UAV,
		specMap->GetShaderResourceViewCPU()).gpuHandle;


	graphic->BindPSOAndRootSignature(defShading, defShading);
	gLightManager.BindLightPass2ConstantBuffer(0);
	graphic->BindMainCameraPass(1);
	graphic->BindDescriptorHandle(GBuffer[0]->GetShaderResourceViewGPU(),2);
	graphic->BindDescriptorHandle(irrDescHandle, 3);
	graphic->BindDescriptorHandle(specDescHandle, 4);
	graphic->BindDescriptorHandle(lutTex->GetShaderResourceViewGPU(), 5);

	graphic->Draw(mImageVert->GetVBV(), 0, mImageVert->GetVertexNum());

	descriptorHeap->ClearUploadedDescriptors(D3D12_DESCRIPTOR_HEAP_TYPE_CBV_SRV_UAV);*/
	objQueue.clear();
	animatedObjQueue.clear();
}

void DeferredRenderPass::DeferredShadingPass::Render(Graphic* graphic,RENDER_PASS_LAYER layer) {
	if (layer != RENDER_PASS_LAYER_OPAQUE || !drp->activated) return;
	SkyboxRenderPass* sbrp = gGraphic.GetRenderPass<SkyboxRenderPass>();
	Texture* irrMap = sbrp->GetIrradianceMap();
	D3D12_GPU_DESCRIPTOR_HANDLE irrDescHandle = drp->descriptorHeap->UploadDescriptors(D3D12_DESCRIPTOR_HEAP_TYPE_CBV_SRV_UAV,
		irrMap->GetShaderResourceViewCPU()).gpuHandle;
	Texture* specMap = sbrp->GetSpecularIBLMap();
	D3D12_GPU_DESCRIPTOR_HANDLE specDescHandle = drp->descriptorHeap->UploadDescriptors(D3D12_DESCRIPTOR_HEAP_TYPE_CBV_SRV_UAV,
		specMap->GetShaderResourceViewCPU()).gpuHandle;


	graphic->BindPSOAndRootSignature(drp->defShading, drp->defShading);
	gLightManager.BindLightPass2ConstantBuffer(0);
	graphic->BindMainCameraPass(1);
	graphic->BindDescriptorHandle(drp->GBuffer[0]->GetShaderResourceViewGPU(), 2);
	graphic->BindDescriptorHandle(irrDescHandle, 3);
	graphic->BindDescriptorHandle(specDescHandle, 4);
	graphic->BindDescriptorHandle(drp->lutTex->GetShaderResourceViewGPU(), 5);

	graphic->Draw(drp->mImageVert->GetVBV(), 0, drp->mImageVert->GetVertexNum());

	drp->descriptorHeap->ClearUploadedDescriptors(D3D12_DESCRIPTOR_HEAP_TYPE_CBV_SRV_UAV);
	drp->activated = false;
}

void DeferredRenderPass::finalize() {

}

DeferredRenderPassID  DeferredRenderPass::AllocateObjectPass() {
	if (!availableConstantBuffers.empty()) {
		DeferredRenderPassID rv = availableConstantBuffers[availableConstantBuffers.size() - 1];
		availableConstantBuffers.pop_back();
		return rv;
	}
	else if (allocatedConstantBuffers < defaultConstantBufferSize) {
		DeferredRenderPassID id = allocatedConstantBuffers++;
		return id;
	}
	return -1;
}


void DeferredRenderPass::DeallocateObjectPass(DeferredRenderPassID& objId) {
	if (defaultConstantBufferSize > objId) {
		availableConstantBuffers.push_back(objId);
		objId = -1;
	}
}

ObjectPass* DeferredRenderPass::GetObjectPass(DeferredRenderPassID id) {
	if (id >= defaultConstantBufferSize) return nullptr;
	return objConstants->GetBufferPtr(id);
}

void DeferredRenderPass::DrawObject(D3D12_VERTEX_BUFFER_VIEW* vbv, D3D12_INDEX_BUFFER_VIEW* ibv,
	size_t start, size_t num, DeferredRenderPassID id, DeferredRenderPassTexture* tex) {
	if (id < 0 || id >= defaultConstantBufferSize) {return;}
	
	Texture* white = gTextureManager.getWhiteTexture();
	D3D12_GPU_DESCRIPTOR_HANDLE whiteHandle{0};
	if (tex == nullptr || tex->diffuse == nullptr  || tex->metallic == nullptr
		 || tex->roughness == nullptr) {
		whiteHandle = descriptorHeap->UploadDescriptors(
			D3D12_DESCRIPTOR_HEAP_TYPE_CBV_SRV_UAV, 
			white->GetShaderResourceViewCPU()
		).gpuHandle;
	}

	D3D12_GPU_DESCRIPTOR_HANDLE normalHandle{0};
	if (tex == nullptr || tex->normal == nullptr) {
		normalHandle = descriptorHeap->UploadDescriptors(
			D3D12_DESCRIPTOR_HEAP_TYPE_CBV_SRV_UAV,
			gTextureManager.getNormalMapDefaultTexture()->GetShaderResourceViewCPU()
		).gpuHandle;
	}

	D3D12_GPU_DESCRIPTOR_HANDLE emissionHandle{ 0 };
	if (tex == nullptr || tex->emission == nullptr) {
		emissionHandle = descriptorHeap->UploadDescriptors(D3D12_DESCRIPTOR_HEAP_TYPE_CBV_SRV_UAV,
			gTextureManager.getBlackTexture()->GetShaderResourceViewCPU()
		).gpuHandle;
	}
	
	ObjectElement oe;
	oe.ibv = ibv;
	oe.vbv = vbv;
	oe.start = start;
	oe.num = num;
	oe.objectID = id; 
	auto uploadTexture2CurrentHeap = [&](Texture* tex, D3D12_GPU_DESCRIPTOR_HANDLE spare) {
		if (tex == nullptr) return spare;
		if (tex->GetShaderResourceViewCPU().ptr == 0) {
			tex->CreateShaderResourceView(gDescriptorAllocator.AllocateDescriptor());
		}
		return descriptorHeap->UploadDescriptors(D3D12_DESCRIPTOR_HEAP_TYPE_CBV_SRV_UAV, tex->GetShaderResourceViewCPU()).gpuHandle;
	};
	if (tex != nullptr) {
		oe.diffuseMap = uploadTexture2CurrentHeap(tex->diffuse, whiteHandle);
		oe.normalMap = uploadTexture2CurrentHeap(tex->normal, normalHandle);
		oe.roughnessMap = uploadTexture2CurrentHeap(tex->roughness, whiteHandle);
		oe.metallicMap = uploadTexture2CurrentHeap(tex->metallic, whiteHandle);
		oe.emissionMap = uploadTexture2CurrentHeap(tex->emission, emissionHandle);
	}
	else {
		oe.diffuseMap = whiteHandle;
		oe.normalMap = normalHandle;
		oe.roughnessMap = whiteHandle;
		oe.metallicMap = whiteHandle;		
		oe.emissionMap = emissionHandle;
	}
	objQueue.push_back(oe);
}

void DeferredRenderPass::DrawSkinnedObject(D3D12_VERTEX_BUFFER_VIEW* vbv,D3D12_INDEX_BUFFER_VIEW* ibv,
	size_t start,size_t num,DeferredRenderPassID id,DeferredRenderPassTexture* tex,
	D3D12_GPU_VIRTUAL_ADDRESS boneTransform) {
	if (id < 0 || id >= defaultConstantBufferSize) { return; }

	Texture* white = gTextureManager.getWhiteTexture();
	D3D12_GPU_DESCRIPTOR_HANDLE whiteHandle{ 0 };
	if (tex == nullptr || tex->diffuse == nullptr || tex->metallic == nullptr
		|| tex->roughness == nullptr) {
		whiteHandle = descriptorHeap->UploadDescriptors(
			D3D12_DESCRIPTOR_HEAP_TYPE_CBV_SRV_UAV,
			white->GetShaderResourceViewCPU()
		).gpuHandle;
	}

	D3D12_GPU_DESCRIPTOR_HANDLE normalHandle{ 0 };
	if (tex == nullptr || tex->normal == nullptr) {
		normalHandle = descriptorHeap->UploadDescriptors(
			D3D12_DESCRIPTOR_HEAP_TYPE_CBV_SRV_UAV,
			gTextureManager.getNormalMapDefaultTexture()->GetShaderResourceViewCPU()
		).gpuHandle;
	}

	D3D12_GPU_DESCRIPTOR_HANDLE emissionHandle{ 0 };
	if (tex == nullptr || tex->emission == nullptr) {
		emissionHandle = descriptorHeap->UploadDescriptors(D3D12_DESCRIPTOR_HEAP_TYPE_CBV_SRV_UAV,
			gTextureManager.getBlackTexture()->GetShaderResourceViewCPU()
		).gpuHandle;
	}

	ObjectElement oe;
	oe.ibv = ibv;
	oe.vbv = vbv;
	oe.start = start;
	oe.num = num;
	oe.objectID = id;
	auto uploadTexture2CurrentHeap = [&](Texture* tex, D3D12_GPU_DESCRIPTOR_HANDLE spare) {
		if (tex == nullptr) return spare;
		if (tex->GetShaderResourceViewCPU().ptr == 0) {
			tex->CreateShaderResourceView(gDescriptorAllocator.AllocateDescriptor());
		}
		return descriptorHeap->UploadDescriptors(D3D12_DESCRIPTOR_HEAP_TYPE_CBV_SRV_UAV, tex->GetShaderResourceViewCPU()).gpuHandle;
	};
	if (tex != nullptr) {
		oe.diffuseMap = uploadTexture2CurrentHeap(tex->diffuse, whiteHandle);
		oe.normalMap = uploadTexture2CurrentHeap(tex->normal, normalHandle);
		oe.roughnessMap = uploadTexture2CurrentHeap(tex->roughness, whiteHandle);
		oe.metallicMap = uploadTexture2CurrentHeap(tex->metallic, whiteHandle);
		oe.emissionMap = uploadTexture2CurrentHeap(tex->emission, emissionHandle);
	}
	else {
		oe.diffuseMap = whiteHandle;
		oe.normalMap = normalHandle;
		oe.roughnessMap = whiteHandle;
		oe.metallicMap = whiteHandle;
		oe.emissionMap = emissionHandle;
	}

	AnimatedObjectElement item;
	item.objEle = oe;
	item.boneTransform = boneTransform;

	animatedObjQueue.push_back(item);
}
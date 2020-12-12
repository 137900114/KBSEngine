#include "SpriteRenderPass.h"
#include "graphic.h"

bool SpriteRenderPass::Initialize(UploadBatch* batch) {
	Game::RootSignature RootSig(3, 1);
	RootSig[0].initAsShaderResource(1,0);
	RootSig[1].initAsConstantBuffer(1, 0);
	RootSig[2].initAsDescriptorTable(0, 0, 1, D3D12_DESCRIPTOR_RANGE_TYPE_SRV);
	RootSig.InitializeSampler(0, CD3DX12_STATIC_SAMPLER_DESC(0));

	if (!gGraphic.CreateRootSignature(L"Sprite",&RootSig)) {
		return false;
	}

	std::vector<D3D12_INPUT_ELEMENT_DESC> inputLayout = {
		{"POSITION",0,DXGI_FORMAT_R32G32_FLOAT,0,0,D3D12_INPUT_CLASSIFICATION_PER_VERTEX_DATA,0},
		{"TEXCOORD",0,DXGI_FORMAT_R32G32_FLOAT,0,8,D3D12_INPUT_CLASSIFICATION_PER_VERTEX_DATA,0}
	};

	spriteShader = gShaderManager.loadShader(L"../shader/DrawSprite.hlsl", "VS", "PS", L"Sprite",inputLayout,L"DrawSprite");
	if (spriteShader == nullptr) {
		return false;
	}
	
	Game::GraphicPSORP Pso;
	Pso.LazyBlendDepthRasterizeDefault();
	CD3DX12_RASTERIZER_DESC rsDesc(D3D12_DEFAULT);
	rsDesc.CullMode = D3D12_CULL_MODE_NONE;
	Pso.SetRasterizerState(rsDesc);

	Pso.SetFlag(D3D12_PIPELINE_STATE_FLAG_NONE);
	Pso.SetNodeMask(0);
	Pso.SetPrimitiveTopologyType(D3D12_PRIMITIVE_TOPOLOGY_TYPE_TRIANGLE);
	

	if (!gGraphic.CreatePipelineStateObject(spriteShader, &Pso)) {
		return false;
	}

	float square[] = {
		 .5, .5, 1., 1.,
		-.5, .5, 0., 1.,
		 .5,-.5, 1., 0.,
		-.5,-.5, 0., 0.,
		 .5,-.5, 1., 0.,
		-.5, .5, 0., 1.
	};
	if (batch != nullptr) {
		mVBuffer = batch->UploadBuffer(sizeof(square), square);
		if (mVBuffer == nullptr) return false;
	}
	else {
		UploadBatch mbatch = UploadBatch::Begin();
		mVBuffer = mbatch.UploadBuffer(sizeof(square), square);
		mbatch.End();

		if (mVBuffer == nullptr) return false;
	}
	mVbv.BufferLocation = mVBuffer->GetGPUVirtualAddress();
	mVbv.StrideInBytes  = sizeof(float) * 4;
	mVbv.SizeInBytes    = sizeof(square);


	ID3D12Device* mDevice = gGraphic.GetDevice();

	heap = std::make_unique<DescriptorHeap>();
	allocatedBufferSize = 0;
	instanceBufferSize  = 2048;
	spriteInstances = std::make_unique<ConstantBuffer<SpriteInstanceConstant>>(mDevice, instanceBufferSize, true);

	viewCenter    = Game::Vector2(0., 0.);
	float whRatio = gGraphic.GetHeightWidthRatio();
	viewScale     = Game::Vector2(whRatio, 1.);
	spriteViewConstant = std::make_unique<ConstantBuffer<SpriteViewConstant>>(mDevice);
	UpdateViewConstant();

	return true;
}

void SpriteRenderPass::UpdateViewConstant() {
	Game::Mat4x4 viewMat = Game::Mat4x4(
		1. / viewScale.x   ,0.							,- viewCenter.x / viewScale.x,0.,
		0.				   ,1. / viewScale.y			,-viewCenter.y / viewScale.y ,0.,
		0.				   ,0.							,1.							 ,0.,
		0.				   ,0.							,0.							 ,1.
	);

	spriteViewConstant->GetBufferPtr()->view = viewMat.T();
}

void SpriteRenderPass::Render(Graphic* graphic,RENDER_PASS_LAYER layer) {
	std::vector<SpriteGroup>* targetGroup;
	if (layer == RENDER_PASS_LAYER_OPAQUE) {
		targetGroup = &renderGroupOpaque;
	}
	else if (layer == RENDER_PASS_LAYER_TRANSPARENT) {
		targetGroup = &renderGroupTransparent;
	}
	else {return;}

	if (targetGroup->size() == 0) {
		return;
	}

	graphic->BindShader(spriteShader);
	ID3D12DescriptorHeap* heaps[] = {heap->GetHeap()};
	graphic->BindDescriptorHeap(heaps,_countof(heaps));
	graphic->BindConstantBuffer(spriteViewConstant->GetADDR(), 1);

	for (auto item : *targetGroup) {
		graphic->BindShaderResource(spriteInstances->GetADDR(item.startInstance),0);
		graphic->BindDescriptorHandle(item.texture->GetShaderResourceViewGPU(), 2);

		graphic->DrawInstance(&mVbv, 0, 6, item.instanceNum);
	}
	targetGroup->clear();
	allocatedBufferSize = 0;
}

void SpriteRenderPass::finalize() {
	spriteInstances.release();
	spriteViewConstant.release();
	registedTextures.clear();
}

SpriteID SpriteRenderPass::RegisterTexture(Texture* tex, D3D12_SHADER_RESOURCE_VIEW_DESC* srv) {
	Descriptor desc = heap->Allocate();
	tex->CreateShaderResourceView(desc, srv);
	registedTextures.push_back(tex);
	
	return registedTextures.size() - 1;
}

void SpriteRenderPass::DrawSprite(size_t num, SpriteData* data, SpriteID sprite) {
	SpriteGroup group = AllocateGroupConstant(num, data, sprite,true);
	if (group.instanceNum == 0) return;
	renderGroupOpaque.push_back(group);
}

void SpriteRenderPass::DrawSpriteTransparent(size_t num, SpriteData* data, SpriteID sprite) {
	SpriteGroup group = AllocateGroupConstant(num, data, sprite, false);
	if (group.instanceNum == 0) return;
	renderGroupTransparent.push_back(group);
}

SpriteRenderPass::SpriteGroup SpriteRenderPass::AllocateGroupConstant(size_t num, SpriteData* data, SpriteID sprite,bool adjustAlpha) {
	SpriteGroup group;
	if (num + allocatedBufferSize > instanceBufferSize) { 
		group.instanceNum = 0;
		return group; 
	}

	if (sprite >= registedTextures.size()) {
		group.instanceNum = 0;
		return group;
	}

	Texture* tex = registedTextures[sprite];

	group.startInstance = allocatedBufferSize;
	group.instanceNum = num;

	group.texture = tex;

	SpriteInstanceConstant* constant = spriteInstances->GetBufferPtr(group.startInstance);
	for (size_t i = 0; i != num; i++) {
		constant[i].color = data[i].Color;
		if(adjustAlpha)
			constant[i].color.w = 1.f;

		Game::Mat4x4 trans = Game::MatrixScale(data[i].Scale.x, data[i].Scale.y, 1.);
		trans = Game::mul(Game::MatrixRotateZ(data[i].rotation), trans);
		trans = Game::mul(Game::Mat4x4(
			1.,0.,data[i].Position.x,0.,
			0.,1.,data[i].Position.y,0.,
			0.,0.,data[i].Position.z,0.,
			0.,0.,0.				,1.
		), trans);

		constant[i].world = trans.T();
	}

	allocatedBufferSize += num;
	return group;
}
#include "../loglcore/d3dcommon.h"
#include "../loglcore/Mesh.h"
#include "../loglcore/ShaderDataStruct.h"

#include "../loglcore/RenderPass.h"
#include "../loglcore/Shader.h"
#include "../loglcore/ConstantBuffer.h"


#include "../loglcore/Complex.h"
#include "../loglcore/Texture.h"

class FFTWave;

class FFTWaveRenderPass : public RenderPass {
public:

	virtual size_t GetPriority() { return 10; }

	virtual bool   Initialize(UploadBatch* batch = nullptr) override;
	virtual void   Render(Graphic* graphic, RENDER_PASS_LAYER layer) override;

	virtual void   finalize() override;

	void Attach(FFTWave* wave) {
		this->wave = wave;
	}

	void SetWorldTransform(Game::Vector3 position,Game::Vector3 rotation,Game::Vector3 scale);
	LightPass* GetLightPass() { return lightPass->GetBufferPtr(); }

	void UpdateTime(float time) {
		mGenConstant->GetBufferPtr()->time = time;
	}
private:
	FFTWave* wave;
	Shader* fftWaveShader;

	struct FFTWaveObjectPass {
		Game::Mat4x4 world;
		Game::Mat4x4 transInvWorld;
	};
	std::unique_ptr<ConstantBuffer<FFTWaveObjectPass>> objectPass;
	std::unique_ptr<ConstantBuffer<LightPass>> lightPass;

	struct FFTWaveGenerateConstant {
		float time;
		Game::Vector3 trash1;
		float length;
		Game::Vector3 trash2;
	};
	std::unique_ptr<ConstantBuffer<FFTWaveGenerateConstant>> mGenConstant;
	std::unique_ptr<DescriptorHeap> mHeap;
};

class FFTWave {
	friend class FFTWaveRenderPass;
public:
	bool Initialize(float width, float height);
	void Update(float deltaTime);

	void SetPosition(Game::Vector3 position) { this->position = position; transformUpdated = true; }
	void SetRotation(Game::Vector3 rotation) { this->rotation = rotation; transformUpdated = true;}
	void SetScale(Game::Vector3 scale) { this->scale = scale; transformUpdated = true;}

	FFTWaveRenderPass* GetRenderPass() { return mRenderPass.get(); }

	void SetWaveHeight(float height) { this->height = height; }
	void SetWaveWind(Game::Vector2 wind){
		windDir = Game::normalize(wind);
		windSpeed = Game::length(wind);
	}
	void SetWaveLength(float length) {
		this->length = length;
	}
private:

	float phillips(uint32_t gridm,uint32_t gridn);
	Complex h0(uint32_t gridm,uint32_t gridn);
	Complex ht(uint32_t gridm,uint32_t gridn,float time);

	static constexpr size_t rowNum = 64;
	std::unique_ptr<DynamicMesh<MeshVertex>> mMesh;
	std::unique_ptr<FFTWaveRenderPass> mRenderPass;

	float currentTime;

	Game::Vector3 position;
	Game::Vector3 rotation;
	Game::Vector3 scale;
	bool transformUpdated = false;

	Game::Vector2 windDir;
	float windSpeed;
	float height;
	float length;

	//Game::Vector2 RandomMap[rowNum][rowNum];
	//Complex Hmap[rowNum][rowNum];
	//Complex HConjmap[rowNum][rowNum];

	std::unique_ptr<Texture> HmapTex, HConjmapTex;
	std::unique_ptr<Texture> HeightMap, GradientX, GradientZ;
};
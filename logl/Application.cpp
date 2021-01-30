#include "Application.h"
#include "../loglcore/graphic.h"
#include "../loglcore/CopyBatch.h"
#include "../loglcore/Mesh.h"
#include "../loglcore/Shader.h"
#include "../loglcore/GeometryGenerator.h"
#include "../loglcore/ConstantBuffer.h"
#include "../loglcore/TextureManager.h"
#include "../loglcore/DescriptorAllocator.h"

#include "../loglcore/SpriteRenderPass.h"
#include "../loglcore/PhongRenderPass.h"

#include "../loglcore/ModelManager.h"

#include "../loglcore/RenderObject.h"
#include "../loglcore/LightManager.h"

#include "../loglcore/AudioClipManager.h"

#include  "../loglcore/GenerateMipmapBatch.h"

#include "InputBuffer.h"
#include "Timer.h"
#include "FPSCamera.h"
#include "logl.h"

struct BoxConstantBuffer {

	Game::Mat4x4 world;
	Game::Mat4x4 transInvWorld;
};

Texture* face;
std::unique_ptr<StaticMesh<MeshVertexNormal>> planeMesh;
std::unique_ptr<StaticMesh<MeshVertexNormal>> sphereMesh;

PhongRenderPass* prp;
DeferredRenderPass* drp;

std::unique_ptr<RenderObject> fro,spro1,spro2;

std::vector<Game::Vector3> bullets;
FPSCamera camera;

AudioClip* audio;

LightSource* mainLight,* pointLight;

#include "Config.h"

bool Application::initialize() {
	
	drp = gGraphic.GetRenderPass<DeferredRenderPass>();

	{
		UploadBatch up = UploadBatch::Begin();
		{
			auto[v, i] = GeometryGenerator::Square(20., 20., GEOMETRY_FLAG_NONE);
			planeMesh = std::make_unique<StaticMesh<MeshVertexNormal>>(gGraphic.GetDevice(),
				i.size(),
				i.data(),
				v.size() / getVertexStrideByFloat<MeshVertexNormal>(),
				reinterpret_cast<MeshVertexNormal*>(v.data()),
				&up);
				
				Texture* fdiff = gTextureManager.loadTexture(L"../asserts/brickwall.jpg", 5, L"wall",
					true);
				Texture* fnormal = gTextureManager.loadTexture(L"../asserts/brickwall_normal.jpg", 5,
					L"wall_normal", true);

				fdiff->CreateShaderResourceView(gDescriptorAllocator.AllocateDescriptor());
				fnormal->CreateShaderResourceView(gDescriptorAllocator.AllocateDescriptor());
				SubMeshMaterial mat;
				mat.diffuse = Game::Vector3(1., 1., 1.);
				mat.roughness = .3;
				mat.metallic = .4;

				mat.textures[SUBMESH_MATERIAL_TYPE_BUMP] = fnormal;
				mat.textures[SUBMESH_MATERIAL_TYPE_DIFFUSE] = fdiff;

				mat.matTransformScale = Game::Vector2(20., 20.);

				fro = std::make_unique<RenderObject>(planeMesh->GetMesh(), mat, Game::Vector3(0., -2., 5.), Game::Vector3(), Game::Vector3(1., 1., 1.), "floor");
		}
		{
			auto[v, i] = GeometryGenerator::Sphere(1., 20, GEOMETRY_FLAG_NONE);
			sphereMesh = std::make_unique<StaticMesh<MeshVertexNormal>>(
					gGraphic.GetDevice(), i.size(), i.data(),
					v.size() / getVertexStrideByFloat<MeshVertexNormal>(),
					reinterpret_cast<MeshVertexNormal*>(v.data()),
					&up);
			Texture* rust_roughness = gTextureManager.loadTexture(L"../asserts/pbr/rust_textures/roughness.png", L"rr", true, &up);
			Texture* rust_metallic	= gTextureManager.loadTexture(L"../asserts/pbr/rust_textures/metallic.png",L"rm", true, &up);
			Texture* rust_normal	= gTextureManager.loadTexture(L"../asserts/pbr/rust_textures/normal.png", L"rn", true, &up);
			Texture* rust_diffuse	= gTextureManager.loadTexture(L"../asserts/pbr/rust_textures/color.png", L"rd", true, &up);

			rust_roughness->CreateShaderResourceView(gDescriptorAllocator.AllocateDescriptor());
			rust_metallic->CreateShaderResourceView(gDescriptorAllocator.AllocateDescriptor());
			rust_normal->CreateShaderResourceView(gDescriptorAllocator.AllocateDescriptor());
			rust_diffuse->CreateShaderResourceView(gDescriptorAllocator.AllocateDescriptor());

			SubMeshMaterial  m1,m2;
			m1.diffuse = Game::ConstColor::White;
			m1.metallic = .3f;
			m1.roughness = .1f;
			
			m2.diffuse = Game::ConstColor::White;
			m2.metallic = 1.f;
			m2.roughness = .5f;
			m2.textures[SUBMESH_MATERIAL_TYPE_BUMP] = rust_normal;
			m2.textures[SUBMESH_MATERIAL_TYPE_DIFFUSE] = rust_diffuse;
			m2.textures[SUBMESH_MATERIAL_TYPE_METALNESS] = rust_metallic;
			m2.textures[SUBEMSH_MATERIAL_TYPE_ROUGHNESS] = rust_roughness;
			
			spro1 = std::make_unique<RenderObject>(sphereMesh->GetMesh(),m1, Game::Vector3( 1., -1.5, 3.), Game::Vector3(), Game::Vector3(.2, .2, .2));
			spro2 = std::make_unique<RenderObject>(sphereMesh->GetMesh(),m2, Game::Vector3(-1., -1.5, 3.), Game::Vector3(), Game::Vector3(.2, .2, .2));
		}
		
		up.End();
	}

	gLightManager.SetAmbientLight(Game::Vector3(.4, .4, .4));

	mainLight = gLightManager.GetMainLightData();
	mainLight->SetLightDirection(Game::Vector3(0., -1., 1.));
	
	pointLight = gLightManager.AllocateLightSource(SHADER_LIGHT_TYPE_POINT);
	pointLight->SetLightIntensity(Game::Vector3(50., 50., 50.));
	pointLight->SetLightPosition(Game::Vector3(0., -1., 3.));
	pointLight->SetLightFallout(0., 5.);

	auto l2 = gLightManager.AllocateLightSource(SHADER_LIGHT_TYPE_POINT);
	l2->SetLightIntensity(Game::Vector3(5., 4., 3.));
	l2->SetLightPosition(Game::Vector3(3., -1.5, 5.));
	l2->SetLightFallout(0., 5.);

	camera.attach(gGraphic.GetMainCamera());

	return true;
}

Game::Vector2 mousePos;

void Application::update() {

	if (gInput.KeyDown(InputBuffer::MOUSE_LEFT)) {
		mousePos = gInput.MousePosition();
	}
	if (gInput.KeyHold(InputBuffer::MOUSE_LEFT)) {
		Game::Vector2 currPos = gInput.MousePosition();
		Game::Vector2 dif = currPos - mousePos;

		camera.rotateX(dif.x * gTimer.DeltaTime() * 30.);
		camera.rotateY(dif.y * gTimer.DeltaTime() * 30.);

		mousePos = currPos;

		constexpr float speed = 1.;
		if (gInput.KeyHold(InputBuffer::W)) {
			camera.walk(speed * gTimer.DeltaTime());
		}
		else if (gInput.KeyHold(InputBuffer::S)) {
			camera.walk(-speed * gTimer.DeltaTime());
		}
		else if (gInput.KeyHold(InputBuffer::A)) {
			camera.strafe(-speed * gTimer.DeltaTime());
		}
		else if (gInput.KeyHold(InputBuffer::D)) {
			camera.strafe(speed * gTimer.DeltaTime());
		}
	}

	if (gInput.KeyDown(InputBuffer::ESCAPE)) {
		ApplicationQuit();
	}

	static float li = 1.;
	if (gInput.KeyHold(InputBuffer::Q)) {
		li = fmax(li - 1e-2,0.);
	}
	else if (gInput.KeyHold(InputBuffer::E)) {
		li = fmin(li + 1e-2,40.);
	}

	mainLight->SetLightIntensity(Game::Vector3(.3, .3, .3) * li);

	fro->Render(drp);
	spro1->Render(drp);
	spro2->Render(drp);
}

void Application::finalize() {}

void Application::Quit() {}
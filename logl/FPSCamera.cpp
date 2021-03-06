#include "FPSCamera.h"

void FPSCamera::walk(float distance) {
	this->Position += this->lookAt * distance;
	update();
}
void FPSCamera::strafe(float distance) {
	this->Position += this->biAxis * distance;
	update();
}

void FPSCamera::rotateX(float angle) {
	phi = phi - angle;
	updateAxis();
	update();
}

void FPSCamera::rotateY(float angle) {
	theta = clamp(90., -90., theta + angle);
	updateAxis();
	update();
}

void FPSCamera::updateAxis() {
	float sin_phi = sin(phi * PI / 180.), cos_phi = cos(phi* PI / 180.);
	float sin_theta = sin(theta * PI / 180.), cos_theta = cos(theta * PI / 180.);

	lookAt = Game::Vector3(cos_theta * sin_phi, sin_theta, cos_theta * cos_phi);

	biAxis = normalize(cross(Game::Vector3(0., 1., 0.), lookAt));
	up = normalize(cross(lookAt, biAxis));
}

void FPSCamera::attach(Camera* camera) {
	if (camera == nullptr) return;
	Position = camera->getPosition();
	Game::Mat4x4 Rotation = Game::MatrixRotation(camera->getRotation());
	lookAt = Game::normalize(Game::Vector3(Rotation.a[0][2], Rotation.a[1][2], Rotation.a[2][2]));
	biAxis = Game::normalize(Game::cross(Game::Vector3(0., 1., 0.), lookAt));
	up = Game::normalize(Game::cross(lookAt, biAxis));

	updateAngle();

	this->camera = camera;
}

void FPSCamera::attach(Game::Vector3 position,Game::Vector3 rotation) {
	this->camera = nullptr;
	Position = position;
	Game::Mat4x4 Rotation = Game::MatrixRotation(rotation);
	lookAt = Game::normalize(Game::Vector3(Rotation.a[0][2], Rotation.a[1][2], Rotation.a[2][2]));
	biAxis = Game::normalize(Game::cross(Game::Vector3(0., 1., 0.), lookAt));
	up = Game::normalize(Game::cross(lookAt, biAxis));

	updateAngle();
}

void FPSCamera::look(Game::Vector3 Position) {
	if (Game::length(Position - this->Position) < 1e-7)
		return;
	lookAt = Game::normalize(Position - this->Position);
	biAxis = Game::normalize(Game::cross(Game::Vector3(0., 1., 0.), lookAt));
	up = Game::normalize(Game::cross(lookAt, biAxis));

	updateAngle();
	update();
}

void FPSCamera::updateAngle() {
	float sintheta = lookAt[1], costheta = up[1],
		sinphi = -biAxis[2], cosphi = biAxis[0];

	this->theta = get_angle(sintheta, costheta,false);
	if (theta > 180.) {
		this->theta -= 360.;
	}
	this->phi = get_angle(sinphi, cosphi,false);
}

void FPSCamera::update() {
	Game::Mat4x4 lookAtMat = Game::Mat4x4(
		biAxis[0], up[0], lookAt[0], Position[0],
		biAxis[1], up[1], lookAt[1], Position[1],
		biAxis[2], up[2], lookAt[2], Position[2],
		0, 0, 0, 1
	);
	if (camera == nullptr) return;
	camera->SetTransform(lookAtMat);
}

Game::Mat4x4 FPSCamera::GetLookAtMat() {
	return Game::Mat4x4(
		biAxis[0], up[0], lookAt[0], Position[0],
		biAxis[1], up[1], lookAt[1], Position[1],
		biAxis[2], up[2], lookAt[2], Position[2],
		0, 0, 0, 1
	);
}

void FPSCamera::setPosition(Game::Vector3 pos) {
	this->Position = pos;
	update();
}
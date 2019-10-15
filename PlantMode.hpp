#pragma once

#include "Mode.hpp"

#include "BoneAnimation.hpp"
#include "GL.hpp"
#include "Scene.hpp"

#include <SDL.h>
#include <glm/glm.hpp>

#include <vector>
#include <list>

// The 'PlantMode' mode shows off some bone animations:

struct PlantMode : public Mode {
	PlantMode();
	virtual ~PlantMode();

	virtual bool handle_event(SDL_Event const &evt, glm::uvec2 const &window_size) override;
	virtual void update(float elapsed) override;
	virtual void draw(glm::uvec2 const &drawable_size) override;

	//controls:
	bool mouse_captured = false;
	bool forward = false;
	bool backward = false;
	bool left = false;
	bool right = false;
	bool w = false;
	bool s = false;
	bool a = false;
	bool d = false;
	bool direct1 = true;
	bool direct2 = true;
	bool direct3 = true;
	bool direct4 = true;
	bool won = false;
	bool fail = false;

	//scene:
	Scene scene;
	Scene::Drawable *plant = nullptr;
	Scene::Drawable *plant1 = nullptr;
	Scene::Drawable *plant2 = nullptr;
	Scene::Drawable *plant3 = nullptr;
	Scene::Drawable *plant4 = nullptr;
	Scene::Drawable *my_son = nullptr;
	Scene::Camera *camera = nullptr;
	float camera_radius = 10.0f;
	float camera_azimuth = glm::radians(60.0f);
	float camera_elevation = glm::radians(45.0f);

	std::vector< BoneAnimationPlayer > plant_animations;
	std::vector< BoneAnimationPlayer > my_son_animations;

	float wind_acc = 0.0f;
};

#include "PlantMode.hpp"
#include "DrawLines.hpp"
#include "LitColorTextureProgram.hpp"
#include "BoneLitColorTextureProgram.hpp"
#include "Load.hpp"
#include "Mesh.hpp"
#include "Scene.hpp"
#include "gl_errors.hpp"
#include "data_path.hpp"
#include "load_save_png.hpp"
#include "Sprite.hpp"
#include "Mesh.hpp"
#include "DrawSprites.hpp"
#include "collide.hpp"
#include <glm/gtc/type_ptr.hpp>
#include <glm/gtc/matrix_transform.hpp>

#include <iostream>
#include <fstream>
#include <map>
#include <cstddef>
#include <random>
#include <unordered_map>

Mesh const *plant_tile = nullptr;
std::shared_ptr< Mode > plant_mode;

Load< SpriteAtlas > trade_font_atlas(LoadTagDefault, []() -> SpriteAtlas const * {
	return new SpriteAtlas(data_path("trade-font"));
});


Load< MeshBuffer > plant_meshes(LoadTagDefault, [](){
	auto ret = new MeshBuffer(data_path("plant.pnct"));
	plant_tile = &ret->lookup("Tile");
	return ret;
});

Load< MeshBuffer > my_son_meshes(LoadTagDefault, [](){
	auto ret = new MeshBuffer(data_path("my_son.pnct"));
	return ret;
});

Load< GLuint > plant_meshes_for_lit_color_texture_program(LoadTagDefault, [](){
	return new GLuint(plant_meshes->make_vao_for_program(lit_color_texture_program->program));
});

Load< GLuint > my_son_meshes_for_lit_color_texture_program(LoadTagDefault, [](){
	return new GLuint(my_son_meshes->make_vao_for_program(lit_color_texture_program->program));
});

BoneAnimation::Animation const *plant_banim_wind = nullptr;
BoneAnimation::Animation const *plant_banim_walk = nullptr;
BoneAnimation::Animation const *plant_banim_crash = nullptr;
BoneAnimation::Animation const *my_son_banim_walk = nullptr;

Load< BoneAnimation > plant_banims(LoadTagDefault, [](){
	auto ret = new BoneAnimation(data_path("plant.banims"));
	plant_banim_wind = &(ret->lookup("Wind"));
	plant_banim_walk = &(ret->lookup("Walk"));
	plant_banim_crash = &(ret->lookup("Crash"));
	return ret;
});

Load< BoneAnimation > my_son_banims(LoadTagDefault, [](){
	auto ret = new BoneAnimation(data_path("my_son.banims"));
	my_son_banim_walk = &(ret->lookup("Walk"));
	return ret;
});


Load< GLuint > plant_banims_for_bone_lit_color_texture_program(LoadTagDefault, [](){
	return new GLuint(plant_banims->make_vao_for_program(bone_lit_color_texture_program->program));
});

Load< GLuint > my_son_banims_for_bone_lit_color_texture_program(LoadTagDefault, [](){
	return new GLuint(my_son_banims->make_vao_for_program(bone_lit_color_texture_program->program));
});

PlantMode::PlantMode() {
	//Make a scene from scratch using the plant prop and the tile mesh:
	{ //make a tile floor:
		Scene::Drawable::Pipeline tile_info;
		tile_info = lit_color_texture_program_pipeline;
		tile_info.vao = *plant_meshes_for_lit_color_texture_program;
		tile_info.start = plant_tile->start;
		tile_info.count = plant_tile->count;
		// my_son_tile_info = lit_color_texture_program_pipeline;
		// my_son_tile_info.vao = *my_son_meshes_for_lit_color_texture_program;
		// my_son_tile_info.start = my_son_tile_info->start;
		// my_son_tile_info.count = my_son_tile_info->count;

		for (int32_t x = -5; x <= 5; ++x) {
			for (int32_t y = -5; y <= 5; ++y) {
				scene.transforms.emplace_back();
				Scene::Transform *transform = &scene.transforms.back();
				transform->name = "Tile-" + std::to_string(x) + "," + std::to_string(y); //<-- no reason to do this, we don't have scene debugger or anything
				transform->position = glm::vec3(2.0f*x, 2.0f*y, 0.0f);

				scene.drawables.emplace_back(transform);
				Scene::Drawable *tile = &scene.drawables.back();
				tile->pipeline = tile_info;
			}
		}
	}

	// add a goal
	{

		Scene::Drawable::Pipeline my_son_info;
		my_son_info = bone_lit_color_texture_program_pipeline;
		my_son_info.vao = *my_son_banims_for_bone_lit_color_texture_program;
		my_son_info.start = my_son_banims->mesh.start;
		my_son_info.count = my_son_banims->mesh.count;

		my_son_animations.reserve(1);
		my_son_animations.emplace_back(*my_son_banims, *my_son_banim_walk, BoneAnimationPlayer::Loop, 0.0f);						
		BoneAnimationPlayer *my_son_player = &my_son_animations.back();
		my_son_info.set_uniforms = [my_son_player](){
			my_son_player->set_uniform(bone_lit_color_texture_program->BONES_mat4x3_array);
		};
		scene.transforms.emplace_back();
		Scene::Transform *transform = &scene.transforms.back();
		transform->position.x = 10.0f;
		transform->position.y = 11.5f;
		scene.drawables.emplace_back(transform);
		Scene::Drawable *my_son = &scene.drawables.back();
		my_son->pipeline = my_son_info;
		this->my_son = my_son;
	}

	{ //put some plants around the edge:
		Scene::Drawable::Pipeline plant_info;
		plant_info = bone_lit_color_texture_program_pipeline;
     
		plant_info.vao = *plant_banims_for_bone_lit_color_texture_program;
		plant_info.start = plant_banims->mesh.start;
		plant_info.count = plant_banims->mesh.count;

		plant_animations.reserve(5);
		for (int32_t x = -2; x <= 2; ++x) {
			if (x != 0 && x != 3) {
				plant_animations.emplace_back(*plant_banims, *plant_banim_walk, BoneAnimationPlayer::Loop,0.0f);
				// plant_animations.emplace_back(*plant_banims, *plant_banim_wind, BoneAnimationPlayer::Once);
				plant_animations.back().position = 1.0f;
			}else if(x == 3){
				plant_animations.emplace_back(*plant_banims, *plant_banim_crash, BoneAnimationPlayer::Once);
				plant_animations.back().position = 1.0f;
			}
			else {
				plant_animations.emplace_back(*plant_banims, *plant_banim_walk, BoneAnimationPlayer::Loop, 0.0f);
			}

			BoneAnimationPlayer *player = &plant_animations.back();
			
		
			plant_info.set_uniforms = [player](){
				player->set_uniform(bone_lit_color_texture_program->BONES_mat4x3_array);
			};

			scene.transforms.emplace_back();
			Scene::Transform *transform = &scene.transforms.back();
			if ( x== 0) {
				transform->position.x = - 10.0f;
				transform->position.y = - 10.0f;
			}else if( x == -2){
				transform->position.x = - 6.0f;
				transform->position.y = - 6.5f;
			}else if( x == -1){
				transform->position.x =   1.0f;
				transform->position.y =   0.5f;
			}else if( x == 1){
				transform->position.x = 9.0f;
				transform->position.y = 8.0f;
			}else{
				transform->position.x = 5.0f;
				transform->position.y = 8.5f;
			}
			// transform->position.x = x * 3.0f;

            scene.drawables.emplace_back(transform);
			Scene::Drawable *plant = &scene.drawables.back();
			plant->pipeline = plant_info;
			
			if (x == 0) this->plant = plant;
			if (x == -2) this->plant1 = plant;
			if (x == -1) this->plant2 = plant;
			if (x == 1) this->plant3 = plant;
			if (x == 2) this->plant4 = plant;
			
		};
		assert(plant_animations.size() == 5);
	}

	{ //make a camera:
		scene.transforms.emplace_back();
		Scene::Transform *transform = &scene.transforms.back();
		transform->position = glm::vec3(0.0f, -10.0f, 3.0f);
		transform->rotation = glm::quat_cast(glm::mat3(glm::lookAt(
			transform->position,
			glm::vec3(0.0f, 0.0f, 0.0f),
			glm::vec3(0.0f, 0.0f, 1.0f)
		)));
		scene.cameras.emplace_back(transform);
		camera = &scene.cameras.back();
		camera->near = 0.01f;
		camera->fovy = glm::radians(45.0f);
	}

}

PlantMode::~PlantMode() {
}

bool PlantMode::handle_event(SDL_Event const &evt, glm::uvec2 const &window_size) {
	//ignore any keys that are the result of automatic key repeat:
	if (evt.type == SDL_KEYDOWN && evt.key.repeat) {
		return false;
	}

	if (evt.type == SDL_KEYDOWN && evt.key.keysym.scancode == SDL_SCANCODE_UP) {
		forward = true;
		return true;
	}
	if (evt.type == SDL_KEYUP && evt.key.keysym.scancode == SDL_SCANCODE_UP) {
		forward = false;
		return true;
	}
	if (evt.type == SDL_KEYDOWN && evt.key.keysym.scancode == SDL_SCANCODE_DOWN) {
		backward = true;
		return true;
	}
	if (evt.type == SDL_KEYUP && evt.key.keysym.scancode == SDL_SCANCODE_DOWN) {
		backward = false;
		return true;
	}
	if (evt.type == SDL_KEYDOWN && evt.key.keysym.sym == SDLK_LEFT) {
		left = true;
		return true;
	}
	if (evt.type == SDL_KEYUP && evt.key.keysym.sym == SDLK_LEFT) {
		left = false;
		return true;
	}
	if (evt.type == SDL_KEYDOWN && evt.key.keysym.sym == SDLK_RIGHT) {
		right = true;
		return true;
	}
	if (evt.type == SDL_KEYUP && evt.key.keysym.sym == SDLK_RIGHT) {
		right = false;
		return true;
	}


	if(evt.type == SDL_KEYDOWN && evt.key.keysym.sym == SDLK_RIGHT){
		d = true;
		return true;
	}
	if(evt.type == SDL_KEYUP && evt.key.keysym.sym == SDLK_RIGHT){
		d = false;
		return true;
	}


	if (evt.type == SDL_MOUSEBUTTONDOWN) {
		if (!mouse_captured) {
			SDL_SetRelativeMouseMode(SDL_TRUE);
			mouse_captured = true;
		}
	}
	if (evt.type == SDL_MOUSEBUTTONUP) {
		if (mouse_captured) {
			SDL_SetRelativeMouseMode(SDL_FALSE);
			mouse_captured = false;
		}
	}
	if (evt.type == SDL_MOUSEMOTION) {
		if (mouse_captured) {
			float yaw = evt.motion.xrel / float(window_size.y) * camera->fovy;
			float pitch = -evt.motion.yrel / float(window_size.y) * camera->fovy;

			//update camera angles:
			camera_elevation = glm::clamp(camera_elevation + pitch, glm::radians(10.0f), glm::radians(80.0f));
			camera_azimuth = camera_azimuth + yaw;
		}
	}

	return false;
}

void PlantMode::update(float elapsed) {

	{
        //move plant1
		float step1 = 0.0f;
		if(direct1){
			step1 = elapsed * 4.0f;
		}else{
			step1 = -elapsed * 4.0f;
		}
		plant1->transform->position.y += step1;
		plant_animations[0].position += step1 / 1.88803f;
		plant_animations[0].position -= std::floor(plant_animations[0].position);
		if(plant1->transform->position.y > 3.5 * 4.0f){
			direct1 = false;
		}else if(plant1->transform->position.y < -3.5 * 4.0f){
			direct1 = true;
		}

		//move plant2
		float step2 = 0.0f;
		if(direct2){
			step2 = elapsed * 3.8f;
		}else{
			step2 = -elapsed * 3.8f;
		}
		plant2->transform->position.x += step2;
		plant_animations[1].position += step2 / 1.88803f;
		plant_animations[1].position -= std::floor(plant_animations[1].position);
		if(plant2->transform->position.x > 3.5 * 4.0f){
			direct2 = false;
		}else if(plant2->transform->position.x < -3.5 * 4.0f){
			direct2 = true;
		}

		//move plant3
		float step3 = 0.0f;
		if(direct3){
			step3 = elapsed * 2.4f;
		}else{
			step3 = -elapsed * 2.4f;
		}
		plant3->transform->position.y += step3;
		plant3->transform->position.x += step3;
		plant_animations[3].position += step3 / 1.88803f;
		plant_animations[3].position -= std::floor(plant_animations[3].position);
		if(plant3->transform->position.y > 3.5 * 4.0f){
			direct3 = false;
		}else if(plant3->transform->position.y < - 3.5 * 4.0f){
			direct3 = true;
		}		


		float step = 0.0f;
		if (forward){	
			step -= elapsed * 4.0f;
			plant->transform->position.y += step;
			plant_animations[2].position += step / 1.88803f;
			plant_animations[2].position -= std::floor(plant_animations[2].position);
			// my_son->transform->position.y += step;
			// my_son_animations[2].position += step / 1.88803f;
			// my_son_animations[2].position -= std::floor(plant_animations[2].position);
		}
		if (backward){
			step += elapsed * 4.0f;
			plant->transform->position.y += step;
			plant_animations[2].position += step / 1.88803f;
			plant_animations[2].position -= std::floor(plant_animations[2].position);
		}
		if (right){
			step -= elapsed * 4.0f;
			plant->transform->position.x += step;
			plant_animations[2].position += step / 1.88803f;
			plant_animations[2].position -= std::floor(plant_animations[2].position);
		}
		if (left){
			step += elapsed * 4.0f;
			plant->transform->position.x += step;
			plant_animations[2].position += step / 1.88803f;
			plant_animations[2].position -= std::floor(plant_animations[2].position);
		}

		if (glm::length(my_son->transform->make_local_to_world()[3] - plant->transform->make_local_to_world()[3]) < 1.0f) {
			won = true;
		}

		if (glm::length(plant1->transform->make_local_to_world()[3] - plant->transform->make_local_to_world()[3]) < 2.0f) {
			fail = true;
		}
		if (glm::length(plant2->transform->make_local_to_world()[3] - plant->transform->make_local_to_world()[3]) < 2.0f) {
			fail = true;
		}
		if (glm::length(plant3->transform->make_local_to_world()[3] - plant->transform->make_local_to_world()[3]) < 2.0f) {
			fail = true;
		}
		if (glm::length(plant4->transform->make_local_to_world()[3] - plant->transform->make_local_to_world()[3]) < 2.0f) {
			fail = true;
		}
		
	}

	float ce = std::cos(camera_elevation);
	float se = std::sin(camera_elevation);
	float ca = std::cos(camera_azimuth);
	float sa = std::sin(camera_azimuth);
	camera->transform->position = camera_radius * glm::vec3(ce * ca, ce * sa, se) + plant->transform->position;
	camera->transform->rotation =
		glm::quat_cast(glm::transpose(glm::mat3(glm::lookAt(
			camera->transform->position,
			plant->transform->position,
			glm::vec3(0.0f, 0.0f, 1.0f)
		))));
	
	static std::mt19937 mt(0xfeedf00d);
	wind_acc -= elapsed;
	if (wind_acc < 0.0f) {
		wind_acc = mt() / float(mt.max()) * 2.0f;
		uint32_t idx = mt() % plant_animations.size();
		if (idx != 2) {
			if (plant_animations[idx].done()) {
				plant_animations[idx].position = 0.0f;
			}
		}
	}

	for (auto &anim : plant_animations) {
		anim.update(elapsed);
	}
}

void PlantMode::draw(glm::uvec2 const &drawable_size) {
	//Draw scene:
	camera->aspect = drawable_size.x / float(drawable_size.y);

	glClearColor(0.0f, 0.0f, 0.0f, 0.0f);
	glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);

	//set up basic OpenGL state:
	glEnable(GL_DEPTH_TEST);
	glEnable(GL_BLEND);
	glBlendEquation(GL_FUNC_ADD);
	glBlendFunc(GL_SRC_ALPHA, GL_ONE_MINUS_SRC_ALPHA);
	glEnable(GL_CULL_FACE);
	glCullFace(GL_BACK);
    DrawSprites draw(*trade_font_atlas, glm::vec2(0,0), glm::vec2(320, 200), drawable_size, DrawSprites::AlignPixelPerfect);

	if (won) {
		std::string text = "Finished!";
		glm::vec2 min, max;
		draw.get_text_extents(text, glm::vec2(0.0f, 0.0f), 2.0f, &min, &max);
		float x = std::round(160.0f - (0.5f * (max.x + min.x)));
		draw.draw_text(text, glm::vec2(x, 100.0f), 2.0f, glm::u8vec4(0x00,0x00,0x00,0xff));
		draw.draw_text(text, glm::vec2(x, 101.0f), 2.0f, glm::u8vec4(0xff,0xff,0xff,0xff));
	}

	if (fail) {
		std::string text = "Failed!";
		glm::vec2 min, max;
		draw.get_text_extents(text, glm::vec2(0.0f, 0.0f), 2.0f, &min, &max);
		float x = std::round(160.0f - (0.5f * (max.x + min.x)));
		draw.draw_text(text, glm::vec2(x, 100.0f), 2.0f, glm::u8vec4(0x00,0x00,0x00,0xff));
		draw.draw_text(text, glm::vec2(x, 101.0f), 2.0f, glm::u8vec4(0xff,0xff,0xff,0xff));
	}

	scene.draw(*camera);

	GL_ERRORS();
}

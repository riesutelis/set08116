#include <glm\glm.hpp>
#include <graphics_framework.h>

using namespace std;
using namespace graphics_framework;
using namespace glm;


class turbo_mesh : public mesh
{
	// Parent for hierarchies
	turbo_mesh* _parent = nullptr;


public:
	turbo_mesh() : mesh() {};
	turbo_mesh(geometry &geom) : mesh(geom) {};
	turbo_mesh(geometry &geom, material &mat) : mesh(geom, mat) {};
	turbo_mesh(const turbo_mesh &other) = default;



	// Gets the parent pointer
	turbo_mesh* get_parent() { return _parent; }
	// Gets the parent pointer
	void set_parent(turbo_mesh* parent) { _parent = parent; }

	// Gets the transform matrix for an object is a hierarchy
	glm::mat4 get_hierarchical_transform_matrix()
	{
		glm::mat4 M = get_transform().get_transform_matrix();
		turbo_mesh *current = this;
		while (current->get_parent() != nullptr)
		{
			current = current->get_parent();
			M = current->get_transform().get_transform_matrix() * M;
		}
		return M;
	}

	// Gets the normal matrix for an object is a hierarchy
	glm::mat3 get_hierarchical_normal_matrix()
	{
		glm::mat3 N = get_transform().get_normal_matrix();
		turbo_mesh *current = this;
		while (current->get_parent() != nullptr)
		{
			current = current->get_parent();
			N = current->get_transform().get_normal_matrix() * N;
		}
		return N;
	}


};


// Effects
effect eff;
effect portal_eff;
effect shadow_eff;
effect colour_eff;
effect sky_eff;
effect mask_eff;

// Object containers
map<string, turbo_mesh> meshes;
map<string, texture> texs;
map<string, texture> normal_maps;
vector<shadow_map> shadows;

// Variables used for player controls
// Portal wobble
bool portal_wobble = false;
// Hue offset for colour correction (0 to 360 degrees)
float hue = 0;
// Saturation offset for colour correction
float saturation = 0;
// Luma (brightness) offset for colour correction
float luma = 0;

// Cameras
enum cam_choice { free0, target0 };
target_camera target_cam;
free_camera free_cam;
cam_choice cam_select;

// Cursor position
double cursor_x;
double cursor_y;

// Lights
directional_light light;
vector<point_light> points;
vector<spot_light> spots;

// Skybox
mesh skybox;
cubemap cube_map;

// Portals
pair<turbo_mesh, turbo_mesh> portals;
map<string, turbo_mesh> portal_meshes;
vec3 portal1_normal;
vec3 portal2_normal;
float dist_to_p1;
float dist_to_p2;

// Screen quad for postprocessing
geometry screen_quad;

// FBO texture
GLuint colour_tex;
// FBO depth-stencil buffer
GLuint depth_stencil_buffer;
// FBO
GLuint frame;

// Masking textures
texture current_mask;
map<string, texture> masks;

// Menu controls
enum menu_choice { main_menu, colour_menu, portal1_menu, portal2_menu };
menu_choice menu = main_menu;
bool show_menu = false;

default_random_engine ran;
// Time accumulators
float dev_dx = 0.0f;
float dev_dy = 0.0f;

// Some constants
vec4 white = vec4(1.0f, 1.0f, 1.0f, 1.0f);
vec4 black = vec4(0.0f, 0.0f, 0.0f, 1.0f);





bool initialise()
{
	// Set input mode - hide the cursor
	glfwSetInputMode(renderer::get_window(), GLFW_CURSOR, GLFW_CURSOR_DISABLED);
	// Capture initial mouse position
	glfwGetCursorPos(renderer::get_window(), &cursor_x, &cursor_y);
	return true;
}


// Calculates PV part of the MVP matrix depending on the camera curently selected
mat4 calculatePV()
{
	switch (cam_select)
	{
	case free0:
		return free_cam.get_projection() * free_cam.get_view();
		break;
	case target0:
		return target_cam.get_projection() * target_cam.get_view();
		break;
	default:
		cout << "No case for camera enum selected (calculatePV)" << endl;
		break;
	}
	return mat4();
}


// Returns eye position based on selected camera
vec3 eye_pos()
{
	switch (cam_select)
	{
	case free0:
		return free_cam.get_position();
		break;
	case target0:
		return target_cam.get_position();
		break;
	default:
		cout << "No case for camera enum selected (calculatePV)" << endl;
		break;
	}
	return vec3();
}


// Use keyboard to move the camera - WSAD for xz and space, left control for y, mouse to rotate
void moveFreeCamera(float delta_time)
{
	float speed = 0.3f;
	float mouse_sensitivity = 2.0;

	vec3 fw = free_cam.get_forward();
	fw.y = 0.0f;
	fw = normalize(fw);
	vec3 left = vec3(rotate(mat4(1), half_pi<float>(), vec3(0.0f, 1.0f, 0.0f)) * vec4(fw, 1.0));

	if (glfwGetKey(renderer::get_window(), GLFW_KEY_W))
		free_cam.set_position(free_cam.get_position() + fw * speed);
	if (glfwGetKey(renderer::get_window(), GLFW_KEY_S))
		free_cam.set_position(free_cam.get_position() + fw * -speed);
	if (glfwGetKey(renderer::get_window(), GLFW_KEY_A))
		free_cam.set_position(free_cam.get_position() + left * speed);
	if (glfwGetKey(renderer::get_window(), GLFW_KEY_D))
		free_cam.set_position(free_cam.get_position() + left * -speed);
	if (glfwGetKey(renderer::get_window(), GLFW_KEY_SPACE))
		free_cam.set_position(free_cam.get_position() + vec3(0.0f, 1.0f, 0.0f) * speed);
	if (glfwGetKey(renderer::get_window(), GLFW_KEY_LEFT_CONTROL))
		free_cam.set_position(free_cam.get_position() + vec3(0.0f, 1.0f, 0.0f) * -speed);

	static double ratio_width = quarter_pi<float>() / static_cast<float>(renderer::get_screen_width());
	static double ratio_height = (quarter_pi<float>() * (1.0f / static_cast<float>(renderer::get_screen_width())));

	double prev_x = cursor_x;
	double prev_y = cursor_y;

	glfwGetCursorPos(renderer::get_window(), &cursor_x, &cursor_y);

	double delta_x = cursor_x - prev_x;
	double delta_y = cursor_y - prev_y;

	float theta_y = delta_x * ratio_width * mouse_sensitivity;
	float theta_x = delta_y * ratio_height * mouse_sensitivity;

	free_cam.rotate(theta_y, -theta_x);
}


// Draws a stencil mask for a single mesh
void draw_stencil_mask(turbo_mesh m, int layer)
{
	// Enables stencil testing
	glEnable(GL_STENCIL_TEST);
	// Disable colour mask
	glColorMask(GL_FALSE, GL_FALSE, GL_FALSE, GL_FALSE);

	// Put layer into stencil buffer where depth test passes
	glStencilFunc(GL_ALWAYS, layer, 0xFF);
	glStencilOp(GL_KEEP, GL_KEEP, GL_REPLACE);

	// Enables writing to all bits of the stencil buffer
	glStencilMask(0xFF);

	// Binds shadow_eff because it only calculates position information for objects
	renderer::bind(shadow_eff);
	mat4 MVP = calculatePV() * m.get_hierarchical_transform_matrix();
	glUniformMatrix4fv(shadow_eff.get_uniform_location("MVP"), 1, GL_FALSE, value_ptr(MVP));
	// Draw to stencil buffer regardless of facing
	glDisable(GL_CULL_FACE);
	renderer::render(m);
	glEnable(GL_CULL_FACE);

	// Set colour writing on
	glColorMask(GL_TRUE, GL_TRUE, GL_TRUE, GL_TRUE);

	// Disable stencil buffer editing on all bits
	glStencilMask(0x00);
}


// Renders the meshes stored in the 'meshes' map using the main effect 'eff'
void render_scene(mat4 lightProjectionMat)
{
	mat4 M;
	mat4 PV = calculatePV();
	renderer::bind(eff);
	for (auto &e : meshes)
	{
		turbo_mesh m = e.second;
		M = m.get_hierarchical_transform_matrix();

		// Calculate MVP
		auto MVP = PV * M;

		// Pass uniforms to shaders
		glUniformMatrix4fv(eff.get_uniform_location("MVP"), 1, GL_FALSE, value_ptr(MVP));
		glUniformMatrix4fv(eff.get_uniform_location("M"), 1, GL_FALSE, value_ptr(M));
		glUniformMatrix3fv(eff.get_uniform_location("N"), 1, GL_FALSE, value_ptr(m.get_hierarchical_normal_matrix()));
		mat4 lMVP = lightProjectionMat * shadows[1].get_view() * M;
		glUniformMatrix4fv(eff.get_uniform_location("lMVP"), 1, GL_FALSE, value_ptr(lMVP));
		renderer::bind(m.get_material(), "mat");
		renderer::bind(light, "light");
		renderer::bind(points, "points");
		renderer::bind(spots, "spots");

		// If no texture assigned, assign default
		if (texs[e.first].get_id() != 0)
			renderer::bind(texs[e.first], 0);
		else
			renderer::bind(texs["check_1"], 0);

		// If no normal map assigned tell shader to not do normal mapping
		if (normal_maps[e.first].get_id() != 0)
		{
			renderer::bind(normal_maps[e.first], 2);
			glUniform1i(eff.get_uniform_location("normal_map"), 2);
			glUniform1f(eff.get_uniform_location("map_norms"), 1.0);
		}
		else
			glUniform1f(eff.get_uniform_location("map_norms"), -1.0);

		glUniform1i(eff.get_uniform_location("pn"), points.size());
		glUniform1i(eff.get_uniform_location("sn"), spots.size());
		glUniform1i(eff.get_uniform_location("tex"), 0);
		glUniform3fv(eff.get_uniform_location("eye_pos"), 1, value_ptr(eye_pos()));
		renderer::bind(shadows[1].buffer->get_depth(), 1);
		glUniform1i(eff.get_uniform_location("shadow_map"), 1);
		renderer::render(m);
	}
}


// Renders the meshes stored in the 'meshes' map using the main effect 'eff'
void render_portal(mat4 offsetMatrix, mat4 lightProjectionMat, vec3 portal_pos, vec3 other_portal_normal, vec3 other_portal_pos, vec3 portal_normal)
{
	if (portal_wobble)
	{
		uniform_real_distribution<float> dist(-0.005f, 0.005f);
		offsetMatrix = offsetMatrix * rotate(mat4(1.0), dist(ran), vec3(0.0, 1.0, 0.0));
		offsetMatrix = offsetMatrix * rotate(mat4(1.0), dist(ran), vec3(1.0, 0.0, 0.0));
		offsetMatrix = offsetMatrix * rotate(mat4(1.0), dist(ran), vec3(0.0, 0.0, 1.0));
	}

	mat4 PV = calculatePV() * offsetMatrix;

	skybox.get_transform().position = other_portal_pos;
	glDisable(GL_DEPTH_TEST);
	glDepthMask(GL_FALSE);
	renderer::bind(sky_eff);
	mat4 M = skybox.get_transform().get_transform_matrix();
	mat4 MVP = PV * M;
	glUniformMatrix4fv(sky_eff.get_uniform_location("MVP"), 1, GL_FALSE, value_ptr(MVP));
	renderer::bind(cube_map, 0);
	glUniform1i(sky_eff.get_uniform_location("cubemap"), 0);
	renderer::render(skybox);
	glEnable(GL_DEPTH_TEST);
	glDepthMask(GL_TRUE);
	


	renderer::bind(portal_eff);
	for (auto &e : meshes)
	{
		turbo_mesh m = e.second;
		M = m.get_hierarchical_transform_matrix();

		// Calculate MVP using M that is transformed to be seen through the portal
		auto MVP = PV * M;

		// Pass uniforms to shaders
		glUniformMatrix4fv(portal_eff.get_uniform_location("MVP"), 1, GL_FALSE, value_ptr(MVP));
		glUniformMatrix4fv(portal_eff.get_uniform_location("M"), 1, GL_FALSE, value_ptr(M));
		glUniformMatrix3fv(portal_eff.get_uniform_location("N"), 1, GL_FALSE, value_ptr(m.get_hierarchical_normal_matrix()));
		mat4 lMVP = lightProjectionMat * shadows[1].get_view() * M;
		glUniformMatrix4fv(portal_eff.get_uniform_location("lMVP"), 1, GL_FALSE, value_ptr(lMVP));
		renderer::bind(m.get_material(), "mat");
		renderer::bind(light, "light");
		renderer::bind(points, "points");
		renderer::bind(spots, "spots");

		// If no texture assigned, assign default
		if (texs[e.first].get_id() != 0)
			renderer::bind(texs[e.first], 0);
		else
			renderer::bind(texs["check_1"], 0);

		// If no normal map assigned tell shader to not do normal mapping
		if (normal_maps[e.first].get_id() != 0)
		{
			renderer::bind(normal_maps[e.first], 2);
			glUniform1i(portal_eff.get_uniform_location("normal_map"), 2);
			glUniform1f(portal_eff.get_uniform_location("map_norms"), 1.0);
		}
		else
			glUniform1f(portal_eff.get_uniform_location("map_norms"), -1.0);

		glUniform1i(portal_eff.get_uniform_location("pn"), points.size());
		glUniform1i(portal_eff.get_uniform_location("sn"), spots.size());
		glUniform1i(portal_eff.get_uniform_location("tex"), 0);
		glUniform3fv(portal_eff.get_uniform_location("eye_pos"), 1, value_ptr(eye_pos()));
		renderer::bind(shadows[1].buffer->get_depth(), 1);
		glUniform1i(portal_eff.get_uniform_location("shadow_map"), 1);
		glUniform3fv(portal_eff.get_uniform_location("portal_pos"), 1, value_ptr(portal_pos));
		glUniform3fv(portal_eff.get_uniform_location("portal_normal"), 1, value_ptr(portal_normal));
		glUniform3fv(portal_eff.get_uniform_location("other_portal_normal"), 1, value_ptr(other_portal_normal));
		glUniformMatrix4fv(portal_eff.get_uniform_location("offset"), 1, GL_FALSE, value_ptr(inverse(offsetMatrix)));

		renderer::render(m);
	}
}




bool load_content()
{
	// Create a frame buffer
	{
		// RGBA8 2D texture, D24S8 depth/stencil texture
		glGenTextures(1, &colour_tex);
		glBindTexture(GL_TEXTURE_2D, colour_tex);
		glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_S, GL_REPEAT);
		glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_T, GL_REPEAT);
		glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_LINEAR);
		glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_LINEAR_MIPMAP_LINEAR);
		// NULL means reserve texture memory, but texels are undefined
		glTexImage2D(GL_TEXTURE_2D, 0, GL_RGBA8, renderer::get_screen_width(), renderer::get_screen_height(), 0, GL_BGRA, GL_UNSIGNED_BYTE, NULL);
		// Reserve memory for other mipmaps levels
		glGenerateMipmapEXT(GL_TEXTURE_2D);
		//-------------------------
		glGenFramebuffersEXT(1, &frame);
		glBindFramebufferEXT(GL_FRAMEBUFFER_EXT, frame);
		// Attach 2D texture to this FBO
		glFramebufferTexture2DEXT(GL_FRAMEBUFFER_EXT, GL_COLOR_ATTACHMENT0_EXT, GL_TEXTURE_2D, colour_tex, 0);
		//-------------------------
		// Generate the depth-stencil buffer
		glGenRenderbuffersEXT(1, &depth_stencil_buffer);
		glBindRenderbufferEXT(GL_RENDERBUFFER_EXT, depth_stencil_buffer);
		glRenderbufferStorageEXT(GL_RENDERBUFFER_EXT, GL_DEPTH24_STENCIL8_EXT, renderer::get_screen_width(), renderer::get_screen_height());
		//-------------------------
		// Attach depth buffer to FBO
		glFramebufferRenderbufferEXT(GL_FRAMEBUFFER_EXT, GL_DEPTH_ATTACHMENT_EXT, GL_RENDERBUFFER_EXT, depth_stencil_buffer);
		// Also attach as a stencil
		glFramebufferRenderbufferEXT(GL_FRAMEBUFFER_EXT, GL_STENCIL_ATTACHMENT_EXT, GL_RENDERBUFFER_EXT, depth_stencil_buffer);
		//-------------------------
		// Does the GPU support current FBO configuration?
		GLenum status;
		status = glCheckFramebufferStatusEXT(GL_FRAMEBUFFER_EXT);
		switch (status)
		{
		case GL_FRAMEBUFFER_COMPLETE_EXT:
			cout << "Frame buffer initialized" << endl;
			break;
		default:
			cout << "Frame buffer not complete" << endl;
		}
	}


	// Seting up the screen quad
	{
		vector<vec3> positions{ vec3(-1.0f, -1.0f, 0.0f), vec3(1.0f, -1.0f, 0.0f), vec3(-1.0f, 1.0f, 0.0f),	vec3(1.0f, 1.0f, 0.0f) };
		vector<vec2> tex_coords{ vec2(0.0, 0.0), vec2(1.0f, 0.0f), vec2(0.0f, 1.0f), vec2(1.0f, 1.0f) };
		screen_quad.set_type(GL_TRIANGLE_STRIP);
		screen_quad.add_buffer(positions, BUFFER_INDEXES::POSITION_BUFFER);
		screen_quad.add_buffer(tex_coords, BUFFER_INDEXES::TEXTURE_COORDS_0);
	}


	// Setting up the portals
	{
		portals.first = turbo_mesh(geometry_builder::create_disk(40, vec2(4.0f, 2.5f)));
		portals.first.get_transform().position = vec3(-10.0, 4.0, 10.0);
		portals.second = turbo_mesh(geometry_builder::create_disk(40, vec2(4.0f, 2.5f)));
		portals.second.get_transform().position = vec3(15.0, 4.0, -15.0);
		portals.second.get_transform().orientation = vec3(0.0, quarter_pi<float>() / -2.0f, 0.0);

		// Portal meshes are required to correctly draw portals on the stencil buffer
		portal_meshes["portal1"] = turbo_mesh(geometry_builder::create_disk(40, vec2(6.0f, 3.0f)));
		portal_meshes["portal1"].get_transform().orientation = vec3(half_pi<float>(), 0.0, half_pi<float>());
		portal_meshes["portal1"].set_parent(&portals.first);
		portal_meshes["portal2"] = turbo_mesh(geometry_builder::create_disk(40, vec2(6.0f, 3.0f)));
		portal_meshes["portal2"].get_transform().orientation = vec3(half_pi<float>(), 0.0, half_pi<float>());
		portal_meshes["portal2"].set_parent(&portals.second);
	}


	// Materials
	material whitePlastic = material(black, white, white, 25.0f);
	material whitePlasticNoShine = material(black, white, vec4(0.0f, 0.0f, 0.0f, 1.0f), 1.0f);
	material whiteCopper = material(black, white, vec4(0.5f, 0.75, 0.6f, 0.0f), 55.0f);


	// Set up skybox
	skybox = mesh(geometry_builder::create_box());
	skybox.get_transform().scale = vec3(-100, -100, -100);
	skybox.get_transform().rotate(rotate(mat4(1), pi<float>(), vec3(0.0f, 0.0f, 1.0f)));
	skybox.get_transform().rotate(rotate(mat4(1), half_pi<float>(), vec3(0.0f, 1.0f, 0.0f)));
	array<string, 6> filenames = { "textures/skybox/posx.jpg", "textures/skybox/negx.jpg", "textures/skybox/posy.jpg",
									"textures/skybox/negy.jpg", "textures/skybox/posz.jpg", "textures/skybox/negz.jpg" };
	cube_map = cubemap(filenames);


	// Load normal maps, meshes and textures
	{
		// Load normal maps
		{
			normal_maps["wall0"] = texture("textures/CeramicBrick_normalmap_M.jpg");
			normal_maps["deviceFrameBottom"] = texture("textures/Copper_A_normalmap_M.png");
			normal_maps["deviceFrameRight"] = normal_maps["deviceFrameBottom"];
			normal_maps["deviceFrameLeft"] = normal_maps["deviceFrameBottom"];
			normal_maps["deviceFrameTop"] = normal_maps["deviceFrameBottom"];
			normal_maps["deviceArmHorizontal"] = normal_maps["deviceFrameBottom"];
			normal_maps["deviceArmVertical"] = normal_maps["deviceFrameBottom"];
			normal_maps["deviceRing"] = normal_maps["deviceFrameBottom"];
		}


		// Load meshes
		{
			meshes["floor"] = turbo_mesh(geometry_builder::create_plane());
			meshes["floor"].get_transform().position = vec3(0.0f, 0.0f, 0.0f);
			meshes["floor"].set_material(whitePlasticNoShine);

			meshes["arch0"] = turbo_mesh(geometry("models/arch.obj"));
			meshes["arch0"].get_transform().position = vec3(-19.0f, 5.0f, -1.0f);
			meshes["arch0"].get_transform().orientation = vec3(0.0f, half_pi<float>(), 0.0f);
			meshes["arch0"].set_material(whitePlastic);

			meshes["lamppost0"] = turbo_mesh(geometry("models/lamp.obj"));
			meshes["lamppost0"].get_transform().position = vec3(25.0f, 0.0f, 18.0f);
			meshes["lamppost0"].get_transform().scale = vec3(0.05f, 0.05f, 0.05f);
			meshes["lamppost0"].set_material(whitePlastic);

			meshes["lamppost1"] = turbo_mesh(geometry("models/lamp.obj"));
			meshes["lamppost1"].get_transform().position = vec3(25.0f, 0.0f, 0.0f);
			meshes["lamppost1"].get_transform().scale = vec3(0.05f, 0.05f, 0.05f);
			meshes["lamppost1"].set_material(whitePlastic);

			meshes["wall0"] = turbo_mesh(geometry_builder::create_box(vec3(2.0f, 12.0f, 60.0f)));
			meshes["wall0"].get_transform().position = vec3(-20.0f, 6.0f, 0.0f);
			meshes["wall0"].set_material(whitePlastic);

			meshes["wall1"] = turbo_mesh(geometry_builder::create_box(vec3(60.0f, 12.0f, 2.0f)));
			meshes["wall1"].get_transform().position = vec3(10.0f, 6.0f, -30.0f);
			meshes["wall1"].set_material(whitePlasticNoShine);

			meshes["spotlight0"] = turbo_mesh(geometry("models/street lamp.obj"));
			meshes["spotlight0"].get_transform().position = vec3(-18.5f, 0.0f, 5.0f);
			meshes["spotlight0"].get_transform().scale = vec3(0.1f, 0.1f, 0.1f);

			meshes["flashlight0"] = turbo_mesh(geometry("models/Flashlight.obj"));
			meshes["flashlight0"].get_transform().position = vec3(0.0, 0.0f, 0.25f);
			meshes["flashlight0"].get_transform().scale = vec3(0.2f, 0.2f, 0.2f);
			meshes["flashlight0"].get_transform().orientation = vec3(0.0f, pi<float>(), 0.0f);

			// Child to deviceFrameBottom
			meshes["deviceArmVertical"] = turbo_mesh(geometry_builder::create_box(vec3(0.29f, 7.0f, 0.1f)));
			meshes["deviceArmVertical"].get_transform().position = vec3(0.0f, 3.75f, 0.0f);
			meshes["deviceArmVertical"].set_material(whiteCopper);
			meshes["deviceArmVertical"].set_parent(&meshes["deviceFrameBottom"]);

			// Child to deviceFrameBottom
			meshes["deviceArmHorizontal"] = turbo_mesh(geometry_builder::create_box(vec3(20.0f, 0.3f, 0.1f)));
			meshes["deviceArmHorizontal"].get_transform().position = vec3(0.0f, 3.5f, 0.0f);
			meshes["deviceArmHorizontal"].set_material(whiteCopper);
			meshes["deviceArmHorizontal"].set_parent(&meshes["deviceFrameBottom"]);

			// Child to deviceArmVertical
			meshes["deviceRing"] = turbo_mesh(geometry_builder::create_torus(32, 20, 0.2f, 1.2f));
			meshes["deviceRing"].get_transform().position = vec3(0.0f, 0.0f, 0.0f);
			meshes["deviceRing"].get_transform().orientation = vec3(half_pi<float>(), 0.0f, 0.0f);
			meshes["deviceRing"].set_material(whiteCopper);
			meshes["deviceRing"].set_parent(&meshes["deviceArmVertical"]);

			// Child to deviceFrameBottom
			meshes["deviceFrameTop"] = turbo_mesh(geometry_builder::create_box(vec3(20.0f, 0.5f, 0.5f)));
			meshes["deviceFrameTop"].get_transform().position = vec3(0.0f, 7.5f, 0.0f);
			meshes["deviceFrameTop"].set_material(whiteCopper);
			meshes["deviceFrameTop"].set_parent(&meshes["deviceFrameBottom"]);

			// Child to deviceFrameBottom
			meshes["deviceFrameLeft"] = turbo_mesh(geometry_builder::create_box(vec3(0.5f, 8.0f, 0.5f)));
			meshes["deviceFrameLeft"].get_transform().position = vec3(-10.25f, 3.75f, 0.0f);
			meshes["deviceFrameLeft"].set_material(whiteCopper);
			meshes["deviceFrameLeft"].set_parent(&meshes["deviceFrameBottom"]);

			// Child to deviceFrameBottom
			meshes["deviceFrameRight"] = turbo_mesh(geometry_builder::create_box(vec3(0.5f, 8.0f, 0.5f)));
			meshes["deviceFrameRight"].get_transform().position = vec3(10.25f, 3.75f, 0.0f);
			meshes["deviceFrameRight"].set_material(whiteCopper);
			meshes["deviceFrameRight"].set_parent(&meshes["deviceFrameBottom"]);

			// Top dog of the hierarchy tree
			meshes["deviceFrameBottom"] = turbo_mesh(geometry_builder::create_box(vec3(20.0f, 0.5f, 0.5f)));
			meshes["deviceFrameBottom"].get_transform().position = vec3(0.0f, 0.25f, -24.0f);
			meshes["deviceFrameBottom"].set_material(whiteCopper);
		}


		// Load textures
		{
			texs["check_1"] = texture("textures/check_1.png", true, true);
			texs["floor"] = texture("textures/Asphalt.jpg", true, true);
			texs["arch0"] = texture("textures/concrete.jpg", true, true);
			texs["lamppost0"] = texture("textures/st-metal.jpg", true, true);
			texs["lamppost1"] = texs["lamppost0"];
			texs["wall0"] = texture("textures/CeramicBrick_albedo_M.jpg", true, true);
			texs["wall1"] = texture("textures/map-8.jpg", true, true);
			texs["spotlight0"] = texs["lamppost0"];
			texs["deviceFrameBottom"] = texture("textures/Copper_A_albedo_M.png", true, true);
			texs["deviceFrameRight"] = texs["deviceFrameBottom"];
			texs["deviceFrameLeft"] = texs["deviceFrameBottom"];
			texs["deviceFrameTop"] = texs["deviceFrameBottom"];
			texs["deviceArmHorizontal"] = texs["deviceFrameBottom"];
			texs["deviceArmVertical"] = texs["deviceFrameBottom"];
			texs["deviceRing"] = texs["deviceFrameBottom"];

			masks["mainMenu"] = texture("textures/MenuMain.png", true, true);
			masks["portalMenu"] = texture("textures/MenuPortal.png", true, true);
			masks["colourMenu"] = texture("textures/MenuColour.png", true, true);
			masks["helpMenu"] = texture("textures/MenuHelp.png", true, true);
			current_mask = masks["mainMenu"];
		}
	}


	// Load lights
	light = directional_light(vec4(0.003f, 0.003f, 0.003f, 1.0f), vec4(0.2f, 0.1f, 0.2f, 1.0f), normalize(vec3(0.5f, -0.2f, 0.5f)));		// evening

	points.push_back(point_light(vec4(1.0f, 0.9f, 0.63f, 1.0f), vec3(25.0f, 11.5f, 18.0f), 0.0f, 0.01f, 0.01f));							// Lamppost0
	points.push_back(point_light(vec4(1.0f, 0.9f, 0.63f, 1.0f), vec3(25.0f, 11.5f, 0.0f), 0.0f, 0.01f, 0.01f));								// Lamppost1

	spots.push_back(spot_light(white, vec3(-16.5f, 14.3f, 5.0f), vec3(0.0f, -1.0f, 0.0f), 0.0f, 0.05f, 0.005f, 10.0f));						// spotlight0
	spots.push_back(spot_light(white, vec3(0.0f, 0.4f, -1.0f), vec3(0.0f, 0.0f, -1.0f), 0.0f, 0.05f, 0.0f, 10.0f));							// flashlight

	
	// Initialize shadow maps
	for (int i = 0; i < (spots.size() + points.size() * 6); i++)
	{
		shadows.push_back(shadow_map(renderer::get_screen_width(), renderer::get_screen_height()));
	}
	

	// Load in shaders
	{
		eff.add_shader("shaders/vert_shader.vert", GL_VERTEX_SHADER);
		vector<string> frag_shaders{ "shaders/top_shader.frag", "shaders/directional.frag", "shaders/spot.frag", "shaders/point.frag", "shaders/shadow_index.frag" };
		eff.add_shader(frag_shaders, GL_FRAGMENT_SHADER);

		portal_eff.add_shader("shaders/vert_shader.vert", GL_VERTEX_SHADER);
		vector<string> portal_frag_shaders{ "shaders/portal_top_shader.frag", "shaders/directional.frag", "shaders/spot.frag", "shaders/point.frag", "shaders/shadow_index.frag" };
		portal_eff.add_shader(portal_frag_shaders, GL_FRAGMENT_SHADER);

		shadow_eff.add_shader("shaders/shadow_depth.vert", GL_VERTEX_SHADER);

		colour_eff.add_shader("shaders/simple.vert", GL_VERTEX_SHADER);
		colour_eff.add_shader("shaders/colour_correction.frag", GL_FRAGMENT_SHADER);

		sky_eff.add_shader("shaders/skybox.vert", GL_VERTEX_SHADER);
		sky_eff.add_shader("shaders/skybox.frag", GL_FRAGMENT_SHADER);

		mask_eff.add_shader("shaders/simple.vert", GL_VERTEX_SHADER);
		mask_eff.add_shader("shaders/mask.frag", GL_FRAGMENT_SHADER);


		// Build effect
		eff.build();
		shadow_eff.build();
		portal_eff.build();
		colour_eff.build();
		sky_eff.build();
		mask_eff.build();
	}


	// Set target camera
	target_cam.set_position(vec3(0.0f, 1.0f, 50.0f));
	target_cam.set_target(vec3(0.0f, 0.0f, 0.0f));
	target_cam.set_projection(quarter_pi<float>() * 1.3f, renderer::get_screen_aspect(), 0.1f, 1000.0f);

	
	// Set free camera
	free_cam.set_position(vec3(30.0f, 1.0f, 50.0f));
	free_cam.set_target(vec3(0.0f, 0.0f, 0.0f));
	free_cam.set_projection(quarter_pi<float>() * 1.3f, renderer::get_screen_aspect(), 0.1f, 1000.0f);

	// Select starting camera
	cam_select = free0;
	return true;
}

bool update(float delta_time)
{

	// Assigns positions and directions of the lights to shadows
	for (int i = 0; i < spots.size(); i++)
	{
		shadows[i].light_position = spots[i].get_position();
		shadows[i].light_dir = spots[i].get_direction();
	}
	for (int i = spots.size(); i < shadows.size(); i++)
	{
		shadows[i].light_position = points[i].get_position();
		if (i % 6 == 0)
			shadows[i].light_dir = vec3(1.0f, 0.0f, 0.0f);
		else if (i % 6 == 1)
			shadows[i].light_dir = vec3(-1.0f, 0.0f, 0.0f);
		else if (i % 6 == 2)
			shadows[i].light_dir = vec3(0.0f, 1.0f, 0.0f);
		else if (i % 6 == 3)
			shadows[i].light_dir = vec3(0.0f, -1.0f, 0.0f);
		else if (i % 6 == 4)
			shadows[i].light_dir = vec3(0.0f, 0.0f, 1.0f);
		else if (i % 6 == 5)
			shadows[i].light_dir = vec3(0.0f, 0.0f, -1.0f);
	}


	// Key inputs
	{
		if (menu == colour_menu)
		{
			// Hue+
			if (glfwGetKey(renderer::get_window(), GLFW_KEY_KP_9))
			{
				hue += 15.0 * delta_time;
				if (hue > 360.0)
					hue = 0.0;
			}
			// Hue-
			if (glfwGetKey(renderer::get_window(), GLFW_KEY_KP_8))
			{
				hue -= 15.0 * delta_time;
				if (hue < -360.0)
					hue = 0.0;
			}
			// Saturation+
			if (glfwGetKey(renderer::get_window(), GLFW_KEY_KP_6))
			{
				saturation += 0.5 * delta_time;
				if (saturation > 1.0f)
					saturation = 1.0f;
			}
			// Saturation-
			if (glfwGetKey(renderer::get_window(), GLFW_KEY_KP_5))
			{
				saturation -= 0.5 * delta_time;
				if (saturation < -1.0f)
					saturation = -1.0f;
			}
			// Luma+
			if (glfwGetKey(renderer::get_window(), GLFW_KEY_KP_3))
			{
				luma += 0.5 * delta_time;
				if (luma > 1.0f)
					luma = 1.0f;
			}
			// Luma-
			if (glfwGetKey(renderer::get_window(), GLFW_KEY_KP_2))
			{
				luma -= 0.5 * delta_time;
				if (luma < -1.0f)
					luma = -1.0f;
			}
		}
		else if (menu == portal1_menu)
		{
			// Portal1 movement
			if (glfwGetKey(renderer::get_window(), GLFW_KEY_KP_8))
				portals.first.get_transform().translate(vec3(0.0f, 0.0f, -10.0f * delta_time));
			if (glfwGetKey(renderer::get_window(), GLFW_KEY_KP_2))
				portals.first.get_transform().translate(vec3(0.0f, 0.0f, 10.0f * delta_time));
			if (glfwGetKey(renderer::get_window(), GLFW_KEY_KP_4))
				portals.first.get_transform().translate(vec3(-10.0f * delta_time, 0.0f, 0.0f));
			if (glfwGetKey(renderer::get_window(), GLFW_KEY_KP_6))
				portals.first.get_transform().translate(vec3(10.0f * delta_time, 0.0f, 0.0f));
			if (glfwGetKey(renderer::get_window(), GLFW_KEY_KP_7))
				portals.first.get_transform().rotate(rotate(mat4(1.0f), 1.0f * delta_time, vec3(0.0f, 1.0f, 0.0f)));
			if (glfwGetKey(renderer::get_window(), GLFW_KEY_KP_9))
				portals.first.get_transform().rotate(rotate(mat4(1.0f), -1.0f * delta_time, vec3(0.0f, 1.0f, 0.0f)));
		}
		else if (menu == portal2_menu)
		{
			// Portal1 movement
			if (glfwGetKey(renderer::get_window(), GLFW_KEY_KP_8))
				portals.second.get_transform().translate(vec3(0.0f, 0.0f, -10.0f * delta_time));
			if (glfwGetKey(renderer::get_window(), GLFW_KEY_KP_2))
				portals.second.get_transform().translate(vec3(0.0f, 0.0f, 10.0f * delta_time));
			if (glfwGetKey(renderer::get_window(), GLFW_KEY_KP_4))
				portals.second.get_transform().translate(vec3(-10.0f * delta_time, 0.0f, 0.0f));
			if (glfwGetKey(renderer::get_window(), GLFW_KEY_KP_6))
				portals.second.get_transform().translate(vec3(10.0f * delta_time, 0.0f, 0.0f));
			if (glfwGetKey(renderer::get_window(), GLFW_KEY_KP_7))
				portals.second.get_transform().rotate(rotate(mat4(1.0f), 1.0f * delta_time, vec3(0.0f, 1.0f, 0.0f)));
			if (glfwGetKey(renderer::get_window(), GLFW_KEY_KP_9))
				portals.second.get_transform().rotate(rotate(mat4(1.0f), -1.0f * delta_time, vec3(0.0f, 1.0f, 0.0f)));
		}


		// Camera selection
		if (glfwGetKey(renderer::get_window(), GLFW_KEY_1))
			cam_select = target0;
		else if (glfwGetKey(renderer::get_window(), GLFW_KEY_2))
			cam_select = free0;


		// Camera movement delegated to methods
		if (cam_select == free0)
			moveFreeCamera(delta_time);
	}


	// Update portal normals
	portal1_normal = normalize(vec3(portal_meshes["portal1"].get_hierarchical_transform_matrix() * vec4(0.0, 1.0, 0.0, 0.0)));
	portal2_normal = normalize(vec3(portal_meshes["portal2"].get_hierarchical_transform_matrix() * vec4(0.0, 1.0, 0.0, 0.0)));


	// Movement trough portals
	{
		// Calculate the angle between portal normals on the y axis
		vec3 p1yproj = portal1_normal;
		p1yproj.y = 0.0f;
		p1yproj = normalize(p1yproj);
		vec3 p2yproj = portal2_normal;
		p2yproj.y = 0.0f;
		p2yproj = normalize(p2yproj);
		float angle;
		if (cross(p1yproj, p2yproj).y > 0.0f)
			angle = -acos(dot(p1yproj, p2yproj));
		else
			angle = acos(dot(p1yproj, p2yproj));

		// Calculate distance to the portals
		float new_dist_to_p1 = distance(free_cam.get_position(), portals.first.get_transform().position);
		float new_dist_to_p2 = distance(free_cam.get_position(), portals.second.get_transform().position);

		// Check if close enough to portals and if got closer since last update
		if (new_dist_to_p1 < 1.5f && (new_dist_to_p1 < dist_to_p1))
		{
			if (dot(portal1_normal, free_cam.get_position() - portals.first.get_transform().position) > 0)
				free_cam.set_position(free_cam.get_position() + portals.second.get_transform().position - portals.first.get_transform().position - portal2_normal / 2.0f);
			else
				free_cam.set_position(free_cam.get_position() + portals.second.get_transform().position - portals.first.get_transform().position + portal2_normal / 2.0f);
			free_cam.rotate(angle, 0.0f);
			new_dist_to_p2 = 0.0f;
		}
		else if (new_dist_to_p2 < 1.5f && (new_dist_to_p2 < dist_to_p2))
		{
			if (dot(portal2_normal, free_cam.get_position() - portals.second.get_transform().position) > 0)
				free_cam.set_position(free_cam.get_position() + portals.first.get_transform().position - portals.second.get_transform().position - portal1_normal / 2.0f);
			else
				free_cam.set_position(free_cam.get_position() + portals.first.get_transform().position - portals.second.get_transform().position + portal1_normal / 2.0f);
			new_dist_to_p1 = 0.0f;
			free_cam.rotate(-angle, 0.0f);
		}
		// Update the previous distance values
		dist_to_p1 = new_dist_to_p1;
		dist_to_p2 = new_dist_to_p1;
	}


	// Movement for the thing
	{
		uniform_real_distribution<float> dist(-0.4f, 0.4f);
		dev_dx += dist(ran);
		if (meshes["deviceArmVertical"].get_transform().position.x > 8.7f)
		{
			meshes["deviceArmVertical"].get_transform().position.x = 8.7f;
			dev_dx = 0.0f;
		}
		if (meshes["deviceArmVertical"].get_transform().position.x < -8.7f)
		{
			meshes["deviceArmVertical"].get_transform().position.x = -8.7f;
			dev_dx = 0.0f;
		}
		meshes["deviceArmVertical"].get_transform().translate(vec3(dev_dx * delta_time, 0.0f, 0.0f));

		dev_dy += dist(ran);
		if (meshes["deviceArmHorizontal"].get_transform().position.y > 5.5f)
		{
			meshes["deviceArmHorizontal"].get_transform().position.y = 5.5f;
			dev_dy = 0.0f;
		}
		if (meshes["deviceArmHorizontal"].get_transform().position.y < 1.5f)
		{
			meshes["deviceArmHorizontal"].get_transform().position.y = 1.5f;
			dev_dy = 0.0f;
		}
		meshes["deviceArmHorizontal"].get_transform().translate(vec3(0.0f, dev_dy * delta_time, 0.0f));

		meshes["deviceRing"].get_transform().position.y = meshes["deviceArmHorizontal"].get_transform().position.y - 3.75f;
	}


	// Update the camera
	switch (cam_select)
	{
	case free0:
		free_cam.update(delta_time);
		break;
	case target0:
		target_cam.update(delta_time);
		break;
	default:
		cout << "No case for camera enum selected (camera update)" << endl;
		break;
	}

	// Update skybox position
	skybox.get_transform().position = eye_pos();

	// Display frames per second in the console
	cout << "FPS: " << 1.0f / delta_time << endl;
	return true;
}

bool render()
{
	mat4 V;
	// Render the shadow map
	// Set render target to shadow map
	renderer::set_render_target(shadows[1]);
	// Clear depth buffer bit
	glClear(GL_DEPTH_BUFFER_BIT);
	// Set face cull mode to front
	glCullFace(GL_FRONT);
	// Create a projection matrix for the point of view of the light
	mat4 lightProjectionMat = perspective<float>(90.0f, renderer::get_screen_aspect(), 0.1f, 1000.f);
	// Bind shadow shader
	renderer::bind(shadow_eff);
	V = shadows[1].get_view();

	// Render the meshes
	for (auto &e : meshes)
	{
		turbo_mesh m = e.second;
		// Create MVP matrix
		auto M = m.get_hierarchical_transform_matrix();
		mat4 MVP = lightProjectionMat * V * M;
		glUniformMatrix4fv(shadow_eff.get_uniform_location("MVP"), 1, GL_FALSE, value_ptr(MVP));
		renderer::render(m);
	}
	glCullFace(GL_BACK);

	
	// Set the render target to frame (frame buffer object)
	glBindFramebufferEXT(GL_FRAMEBUFFER_EXT, frame);
	glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT | GL_STENCIL_BUFFER_BIT);


	// Render skybox
	glDisable(GL_DEPTH_TEST);
	glDepthMask(GL_FALSE);
	renderer::bind(sky_eff);
	mat4 M = skybox.get_transform().get_transform_matrix();
	mat4 MVP = calculatePV() * M;
	glUniformMatrix4fv(sky_eff.get_uniform_location("MVP"), 1, GL_FALSE, value_ptr(MVP));
	renderer::bind(cube_map, 0);
	glUniform1i(sky_eff.get_uniform_location("cubemap"), 0);
	renderer::render(skybox);
	glEnable(GL_DEPTH_TEST);
	glDepthMask(GL_TRUE);


	render_scene(lightProjectionMat);


	// Mark out portals in the stencil buffer
	{
		// Enables stencil buffer editing 
		glStencilMask(0xFF);
		// Clears the stencil buffer
		glClear(GL_STENCIL_BUFFER_BIT);
		draw_stencil_mask(portal_meshes["portal1"], 1);
		draw_stencil_mask(portal_meshes["portal2"], 2);
		// Disables stencil buffer editing 
		glStencilMask(0x00);
		// Clears the depth buffer
		glClear(GL_DEPTH_BUFFER_BIT);
	}

	
	// Render portals
	{
		// Render image through first portal
		glStencilFunc(GL_EQUAL, 1, 0xFF);
		mat4 offset = portals.first.get_transform().get_transform_matrix() * inverse(portals.second.get_transform().get_transform_matrix());
		render_portal(offset, lightProjectionMat, portals.first.get_transform().position, portal2_normal, portals.second.get_transform().position, portal1_normal);
		// Render image through second portal
		glStencilFunc(GL_EQUAL, 2, 0xFF);
		offset = portals.second.get_transform().get_transform_matrix() * inverse(portals.first.get_transform().get_transform_matrix());
		render_portal(offset, lightProjectionMat, portals.second.get_transform().position, portal1_normal, portals.first.get_transform().position, portal2_normal);
		// Disable stencil testing
		glDisable(GL_STENCIL_TEST);
	}
	

	// Postprocessing
	// Colour correction
	{
		renderer::bind(colour_eff);
		glUniformMatrix4fv(colour_eff.get_uniform_location("MVP"), 1, GL_FALSE, value_ptr(mat4(1.0)));
		glBindTexture(GL_TEXTURE_2D, colour_tex);
		glUniform1i(colour_eff.get_uniform_location("tex"), colour_tex);
		glUniform1f(colour_eff.get_uniform_location("hue_offset"), hue);
		glUniform1f(colour_eff.get_uniform_location("saturation"), saturation);
		glUniform1f(colour_eff.get_uniform_location("brightness"), luma);
		renderer::render(screen_quad);
	}
	
	
	// Masking
	// Set render target to screen
	renderer::set_render_target();
	renderer::bind(mask_eff);
	glUniformMatrix4fv(mask_eff.get_uniform_location("MVP"), 1, GL_FALSE, value_ptr(mat4(1.0)));
	glActiveTexture(GL_TEXTURE0);
	glBindTexture(GL_TEXTURE_2D, colour_tex);
	glUniform1i(colour_eff.get_uniform_location("tex"), 0);
	// Set active texture
	glActiveTexture(GL_TEXTURE0 + 1);
	if (show_menu)
	{
		glBindTexture(current_mask.get_type(), current_mask.get_id());
		glUniform1i(mask_eff.get_uniform_location("alpha_map"), 1);
	}
	else
	{
		glBindTexture(masks["helpMenu"].get_type(), masks["helpMenu"].get_id());
		glUniform1i(mask_eff.get_uniform_location("alpha_map"), 1);
	}
	renderer::render(screen_quad);
	

	return true;
}


void key_callback(GLFWwindow* window, int key, int scancode, int action, int mods)
{
	// Whether or not to show menu
	if (key == GLFW_KEY_F1 && action == GLFW_RELEASE)
		show_menu = !show_menu;


	if (menu != main_menu)
	{
		if (key == GLFW_KEY_KP_0 && action == GLFW_RELEASE)
		{
			menu = main_menu;
			current_mask = masks["mainMenu"];
		}
	}
	else if (menu == main_menu)
	{
		if (key == GLFW_KEY_KP_1 && action == GLFW_RELEASE)
		{
			menu = colour_menu;
			current_mask = masks["colourMenu"];
		}
		else if (key == GLFW_KEY_KP_2 && action == GLFW_RELEASE)
		{
			menu = portal1_menu;
			current_mask = masks["portalMenu"];
		}
		else if (key == GLFW_KEY_KP_3 && action == GLFW_RELEASE)
		{
			menu = portal2_menu;
			current_mask = masks["portalMenu"];
		}

		// Enable/disable portal wobble
		if (key == GLFW_KEY_KP_0 && action == GLFW_RELEASE)
			portal_wobble = !portal_wobble;
	}
}


void main()
{
	// Create application
	app application("Graphics Coursework");
	// Set initialise, load content, update and render methods
	application.set_initialise(initialise);
	application.set_load_content(load_content);
	application.set_update(update);
	application.set_render(render);
	application.set_keyboard_callback(key_callback);
	// Run application
	application.run();
}
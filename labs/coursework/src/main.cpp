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






geometry geom;
effect eff;
effect portal_eff;
effect shadow_eff;
map<string, turbo_mesh> meshes;
map<string, texture> texs;
map<string, texture> normal_maps;
vector<shadow_map> shadows;

enum cam_choice { free0, target0 };
target_camera target_cam;
free_camera free_cam;
cam_choice cam_select;

double cursor_x;
double cursor_y;

directional_light light;
vector<point_light> points;
vector<spot_light> spots;

pair<mesh, mesh> portals;
frame_buffer first_portal_pass;
map<string, turbo_mesh> meshes1;



default_random_engine ran;
// Time accumulator
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

	first_portal_pass = frame_buffer(renderer::get_screen_width(), renderer::get_screen_height());
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
	float speed = 0.4f;
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
void draw_stencil_mask(mesh m, int layer)
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
	mat4 M = m.get_transform().get_transform_matrix();;
	mat4 MVP = calculatePV() * M;
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

		// Calculate MVP using M that is transformed to be seen through the portal
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
void render_portal(mat4 offsetMatrix, mat4 lightProjectionMat, vec3 portal_pos, vec3 portal_normal)
{
	mat4 M;
	mat4 PV = calculatePV() * offsetMatrix;
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
	//	glUniform3fv(portal_eff.get_uniform_location("portal_pos"), 1, value_ptr(vec3(PV * vec4(portal_pos, 1.0))));
	//	glUniform3fv(portal_eff.get_uniform_location("portal_normal"), 1, value_ptr(vec3(PV * vec4(portal_normal, 1.0))));
		glUniform3fv(portal_eff.get_uniform_location("portal_pos"), 1, value_ptr(vec3(PV * portals.first.get_transform().get_transform_matrix() * vec4(portal_pos, 1.0))));
		glUniform3fv(portal_eff.get_uniform_location("portal_normal"), 1, value_ptr(vec3(PV * portals.first.get_transform().get_transform_matrix() * vec4(portal_normal, 1.0))));

		renderer::render(m);
	}
}




bool load_content()
{
	// setting up the portals
	portals.first = mesh(geometry_builder::create_disk(40, vec2(4.0f, 2.5f)));
	portals.first.get_transform().position = vec3(-10.0, 4.0, 10.0);
	//portals.first.get_transform().orientation = vec3(quarter_pi<float>() * 3.5, 0.0, half_pi<float>());
	portals.second = mesh(geometry_builder::create_disk(40, vec2(4.0f, 2.5f)));
	portals.second.get_transform().position = vec3(15.0, 4.0, -15.0);
	//portals.second.get_transform().orientation = vec3(half_pi<float>(), 0.0, half_pi<float>());

	// debug stuff
	meshes1["portal1"] = turbo_mesh(geometry_builder::create_disk(40, vec2(6.0f, 3.0f)));
	meshes1["portal1"].get_transform().position = vec3(-10.0, 4.0, 10.0);
	meshes1["portal1"].get_transform().orientation = vec3(half_pi<float>(), 0.0, half_pi<float>());
	meshes1["portal2"] = turbo_mesh(geometry_builder::create_disk(40, vec2(6.0f, 3.0f)));
	meshes1["portal2"].get_transform().position = vec3(15.0, 4.0, -15.0);
	meshes1["portal2"].get_transform().orientation = vec3(half_pi<float>(), 0.0, half_pi<float>());






	// Materials
	material whitePlastic = material(black, white, white, 25.0f);
	material whitePlasticNoShine = material(black, white, vec4(0.0f, 0.0f, 0.0f, 1.0f), 1.0f);
	material whiteCopper = material(black, white, vec4(0.5f, 0.75, 0.6f, 0.0f), 55.0f);


	// Load normal maps ---------------------------------------------------------------------------------------------------------------------------------------
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
	//---------------------------------------------------------------------------------------------------------------------------------------------------------


	// Load meshes #############################################################################################################################################

	//meshes["box"] = mesh(geometry_builder::create_box(vec3(5.0f, 5.0f, 5.0f)));				// Box for various debuging purposes
	//meshes["box"].get_transform().position = vec3(8.0f, 5.0f, 2.0f);
	//meshes["box"].set_material(whiteCopper);
	//normal_maps["box"] = texture("textures/brick_normalmap.jpg");
	//
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
	//##########################################################################################################################################################




	// Load textures -------------------------------------------------------------------------------------------------------------------------------------------
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
	}
	//----------------------------------------------------------------------------------------------------------------------------------------------------------




	// Load lights ############################################################################################################################################
	{
		light = directional_light(vec4(0.003f, 0.003f, 0.003f, 1.0f), vec4(0.2f, 0.08f, 0.06f, 1.0f), normalize(vec3(0.6f, -1.0f, 0.3f)));		// evening
	//	light = directional_light(vec4(0.0f, 0.0f, 0.0f, 1.0f), vec4(0.0f, 0.0f, 0.0f, 1.0f), normalize(vec3(0.6f, -1.0f, 0.3f)));				// No directional for debugging
		light = directional_light(vec4(1.0f, 1.0f, 1.0f, 1.0f), vec4(0.0f, 0.0f, 0.0f, 1.0f), normalize(vec3(0.6f, -1.0f, 0.3f)));				// full ambient for debugging

		points.push_back(point_light(vec4(1.0f, 0.9f, 0.63f, 1.0f), vec3(25.0f, 11.5f, 18.0f), 0.0f, 0.01f, 0.01f));							// Lamppost0
		points.push_back(point_light(vec4(1.0f, 0.9f, 0.63f, 1.0f), vec3(25.0f, 11.5f, 0.0f), 0.0f, 0.01f, 0.01f));								// Lamppost1

		spots.push_back(spot_light(white, vec3(-16.5f, 14.3f, 5.0f), vec3(0.0f, -1.0f, 0.0f), 0.0f, 0.05f, 0.005f, 10.0f));						// spotlight0
		shadows.push_back(shadow_map(renderer::get_screen_width(), renderer::get_screen_height()));
		spots.push_back(spot_light(white, vec3(0.0f, 0.4f, -1.0f), vec3(0.0f, 0.0f, -1.0f), 0.0f, 0.05f, 0.0f, 10.0f));							// flashlight
		shadows.push_back(shadow_map(renderer::get_screen_width(), renderer::get_screen_height()));
	}
	//#########################################################################################################################################################
	
	
	
	
	// Colours
	vector<vec4> colours{ vec4(1.0f, 0.0f, 0.0f, 1.0f), vec4(1.0f, 0.0f, 0.0f, 1.0f), vec4(1.0f, 0.0f, 0.0f, 1.0f) };

	// Load in shaders
	eff.add_shader("shaders/vert_shader.vert", GL_VERTEX_SHADER);
	vector<string> frag_shaders{ "shaders/top_shader.frag", "shaders/directional.frag", "shaders/spot.frag", "shaders/point.frag", "shaders/shadow_index.frag" };
	eff.add_shader(frag_shaders, GL_FRAGMENT_SHADER);

	portal_eff.add_shader("shaders/vert_shader.vert", GL_VERTEX_SHADER);
	vector<string> portal_frag_shaders{ "shaders/portal_top_shader.frag", "shaders/directional.frag", "shaders/spot.frag", "shaders/point.frag", "shaders/shadow_index.frag" };
	portal_eff.add_shader(portal_frag_shaders, GL_FRAGMENT_SHADER);

	shadow_eff.add_shader("shaders/shadow_depth.vert", GL_VERTEX_SHADER);

	// Build effect
	eff.build();
	shadow_eff.build();
	portal_eff.build();
	
	// Set target camera
	target_cam.set_position(vec3(0.0f, 1.0f, 50.0f));
	target_cam.set_target(vec3(0.0f, 0.0f, 0.0f));
	target_cam.set_projection(quarter_pi<float>() * 1.3f, renderer::get_screen_aspect(), 0.1f, 1000.0f);

	
	// Set free camera
	free_cam.set_position(vec3(30.0f, 1.0f, 50.0f));
//	free_cam.set_position(vec3(10.0f, 1.0f, 20.0f));
	free_cam.set_target(vec3(0.0f, 0.0f, 0.0f));
//	free_cam.set_target(vec3(-10.0, 4.0, 10.0));
	free_cam.set_projection(quarter_pi<float>() * 1.3f, renderer::get_screen_aspect(), 0.1f, 1000.0f);

	// Select starting camera
	cam_select = free0;
	return true;
}

bool update(float delta_time)
{

	// Assigns positions and directions of the lights to shadows
	for (int i = 0; i < shadows.size(); i++)
	{
		shadows[i].light_position = spots[i].get_position();
		shadows[i].light_dir = spots[i].get_direction();
	}





	// Camera selection
	if (glfwGetKey(renderer::get_window(), GLFW_KEY_1))
		cam_select = target0;
	else if (glfwGetKey(renderer::get_window(), GLFW_KEY_2))
		cam_select = free0;

	// Camera movement delegated to methods
	if (cam_select == free0)
		moveFreeCamera(delta_time);





	// Movement for the thing----------------------------------------------------------------------------------------------------------------------------------
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
	//---------------------------------------------------------------------------------------------------------------------------------------------------------





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


	// Display frames per second in the console
	cout << "FPS: " << 1.0f / delta_time << endl;
	return true;
}

bool render()
{
	// Render the shadow map ##############################################################################################################

	// Set render target to shadow map
	renderer::set_render_target(shadows[1]);
	// Clear depth buffer bit
	glClear(GL_DEPTH_BUFFER_BIT);
	// Set face cull mode to front
	glCullFace(GL_FRONT);

	// Create a projection matrix for the poin of view of the light
	mat4 lightProjectionMat = perspective<float>(90.0f, renderer::get_screen_aspect(), 0.1f, 1000.f);

	// Bind shadow shader
	renderer::bind(shadow_eff);
	auto V = shadows[1].get_view();

	// Render the meshes
	for (auto &e : meshes) {
		turbo_mesh m = e.second;
		// Create MVP matrix
		auto M = m.get_hierarchical_transform_matrix();
		mat4 MVP = lightProjectionMat * V * M;
		glUniformMatrix4fv(shadow_eff.get_uniform_location("MVP"), 1, GL_FALSE, value_ptr(MVP));
		renderer::render(m);
	}
	glCullFace(GL_BACK);

	
	//####################################################################################################################################




	// Render the scene 
	renderer::set_render_target();
	render_scene(lightProjectionMat);




	// Mark out portals in the stencil buffer //////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
	// Enables stencil buffer editing 
	glStencilMask(0xFF);
	// Clears the stencil buffer
	glClear(GL_STENCIL_BUFFER_BIT);
	draw_stencil_mask(meshes1["portal1"], 1);
	draw_stencil_mask(meshes1["portal2"], 2);
	// Disables stencil buffer editing 
	glStencilMask(0x00);

	// Clears the depth buffer
	glClear(GL_DEPTH_BUFFER_BIT);
	///////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////




	renderer::set_render_target();
	// Render image through first portal
	glStencilFunc(GL_EQUAL, 1, 0xFF);
	mat4 offset = inverse(portals.second.get_transform().get_transform_matrix()) * portals.first.get_transform().get_transform_matrix();
	render_portal(offset, lightProjectionMat, portals.first.get_transform().position, vec3(0.0f, 1.0f, 0.0f) * meshes1["portal1"].get_transform().orientation);

	// Render image through second portal
	glStencilFunc(GL_EQUAL, 2, 0xFF);
	offset = inverse(portals.first.get_transform().get_transform_matrix()) * portals.second.get_transform().get_transform_matrix();
	render_portal(offset, lightProjectionMat, portals.second.get_transform().position, vec3(0.0f, 1.0f, 0.0f) * meshes1["portal2"].get_transform().orientation);


	// Disable stencil testing
	glDisable(GL_STENCIL_TEST);
	
	return true;
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
	// Run application
	application.run();
}
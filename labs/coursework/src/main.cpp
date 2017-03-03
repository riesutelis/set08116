#include <glm\glm.hpp>
#include <graphics_framework.h>

using namespace std;
using namespace graphics_framework;
using namespace glm;

geometry geom;
effect eff;
effect shadow_eff;
map<string, mesh> meshes;
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




bool load_content()
{

	// Materials
	material whitePlastic = material(black, white, white, 25.0f);
	material whitePlasticNoShine = material(black, white, vec4(0.0f, 0.0f, 0.0f, 1.0f), 1.0f);
	material whiteCopper = material(black, white, vec4(0.5f, 0.75, 0.6f, 0.0f), 55.0f);


	// Load normal maps ---------------------------------------------------------------------------------------------------------------------------------------
	normal_maps["wall0"] = texture("textures/CeramicBrick_normalmap_M.jpg");
	normal_maps["deviceFrameBottom"] = texture("textures/Copper_A_normalmap_M.png");
	normal_maps["deviceFrameRight"] = texture("textures/Copper_A_normalmap_M.png");
	normal_maps["deviceFrameLeft"] = texture("textures/Copper_A_normalmap_M.png");
	normal_maps["deviceFrameTop"] = texture("textures/Copper_A_normalmap_M.png");
	normal_maps["deviceArmHorizontal"] = texture("textures/Copper_A_normalmap_M.png");
	normal_maps["deviceArmVertical"] = texture("textures/Copper_A_normalmap_M.png");
	normal_maps["deviceRing"] = texture("textures/Copper_A_normalmap_M.png");
	//---------------------------------------------------------------------------------------------------------------------------------------------------------


	// Load meshes #############################################################################################################################################

	//meshes["box"] = mesh(geometry_builder::create_box(vec3(5.0f, 5.0f, 5.0f)));
	//meshes["box"].get_transform().position = vec3(8.0f, 5.0f, 2.0f);
	//meshes["box"].set_material(whiteCopper);
	//normal_maps["box"] = texture("textures/brick_normalmap.jpg");
	//

	meshes["floor"] = mesh(geometry_builder::create_plane());
	meshes["floor"].get_transform().position = vec3(0.0f, 0.0f, 0.0f);
	meshes["floor"].set_material(whitePlasticNoShine);

	meshes["arch0"] = mesh(geometry("models/arch.obj"));
	meshes["arch0"].get_transform().position = vec3(-19.0f, 5.0f, -1.0f);
	meshes["arch0"].get_transform().orientation = vec3(0.0f, half_pi<float>(), 0.0f);
	meshes["arch0"].set_material(whitePlastic);

	meshes["lamppost0"] = mesh(geometry("models/lamp.obj"));
	meshes["lamppost0"].get_transform().position = vec3(25.0f, 0.0f, 18.0f);
	meshes["lamppost0"].get_transform().scale = vec3(0.05f, 0.05f, 0.05f);
	meshes["lamppost0"].set_material(whitePlastic);

	meshes["lamppost1"] = mesh(geometry("models/lamp.obj"));
	meshes["lamppost1"].get_transform().position = vec3(25.0f, 0.0f, 0.0f);
	meshes["lamppost1"].get_transform().scale = vec3(0.05f, 0.05f, 0.05f);
	meshes["lamppost1"].set_material(whitePlastic);

	meshes["wall0"] = mesh(geometry_builder::create_box(vec3(2.0f, 12.0f, 60.0f)));
	meshes["wall0"].get_transform().position = vec3(-20.0f, 6.0f, 0.0f);
	meshes["wall0"].set_material(whitePlastic);

	meshes["wall1"] = mesh(geometry_builder::create_box(vec3(60.0f, 12.0f, 2.0f)));
	meshes["wall1"].get_transform().position = vec3(10.0f, 6.0f, -30.0f);
	meshes["wall1"].set_material(whitePlasticNoShine);

	meshes["spotlight0"] = mesh(geometry("models/street lamp.obj"));
	meshes["spotlight0"].get_transform().position = vec3(-18.5f, 0.0f, 5.0f);
	meshes["spotlight0"].get_transform().scale = vec3(0.1f, 0.1f, 0.1f);
	
	meshes["flashlight0"] = mesh(geometry("models/Flashlight.obj"));
	meshes["flashlight0"].get_transform().position = vec3(0.0, 0.0f, 0.25f);
	meshes["flashlight0"].get_transform().scale = vec3(0.2f, 0.2f, 0.2f);
	meshes["flashlight0"].get_transform().orientation = vec3(0.0f, pi<float>(), 0.0f);

	// Child to deviceFrameBottom
	meshes["deviceArmVertical"] = mesh(geometry_builder::create_box(vec3(0.29f, 7.0f, 0.1f)));
	meshes["deviceArmVertical"].get_transform().position = vec3(0.0f, 3.75f, 0.0f);
	meshes["deviceArmVertical"].set_material(whiteCopper);
	meshes["deviceArmVertical"].set_parent(&meshes["deviceFrameBottom"]);

	// Child to deviceFrameBottom
	meshes["deviceArmHorizontal"] = mesh(geometry_builder::create_box(vec3(20.0f, 0.3f, 0.1f)));
	meshes["deviceArmHorizontal"].get_transform().position = vec3(0.0f, 3.5f, 0.0f);
	meshes["deviceArmHorizontal"].set_material(whiteCopper);
	meshes["deviceArmHorizontal"].set_parent(&meshes["deviceFrameBottom"]);
	
	// Child to deviceArmVertical
	meshes["deviceRing"] = mesh(geometry_builder::create_torus(32, 20, 0.2f, 1.2f));
	meshes["deviceRing"].get_transform().position = vec3(0.0f, 0.0f, 0.0f);
	meshes["deviceRing"].get_transform().orientation = vec3(half_pi<float>(), 0.0f, 0.0f);
	meshes["deviceRing"].set_material(whiteCopper);
	meshes["deviceRing"].set_parent(&meshes["deviceArmVertical"]);
	
	// Child to deviceFrameBottom
	meshes["deviceFrameTop"] = mesh(geometry_builder::create_box(vec3(20.0f, 0.5f, 0.5f)));
	meshes["deviceFrameTop"].get_transform().position = vec3(0.0f, 7.5f, 0.0f);
	meshes["deviceFrameTop"].set_material(whiteCopper);
	meshes["deviceFrameTop"].set_parent(&meshes["deviceFrameBottom"]);

	// Child to deviceFrameBottom
	meshes["deviceFrameLeft"] = mesh(geometry_builder::create_box(vec3(0.5f, 8.0f, 0.5f)));
	meshes["deviceFrameLeft"].get_transform().position = vec3(-10.25f, 3.75f, 0.0f);
	meshes["deviceFrameLeft"].set_material(whiteCopper);
	meshes["deviceFrameLeft"].set_parent(&meshes["deviceFrameBottom"]);

	// Child to deviceFrameBottom
	meshes["deviceFrameRight"] = mesh(geometry_builder::create_box(vec3(0.5f, 8.0f, 0.5f)));
	meshes["deviceFrameRight"].get_transform().position = vec3(10.25f, 3.75f, 0.0f);
	meshes["deviceFrameRight"].set_material(whiteCopper);
	meshes["deviceFrameRight"].set_parent(&meshes["deviceFrameBottom"]);

	// Top dog of the hierarchy tree
	meshes["deviceFrameBottom"] = mesh(geometry_builder::create_box(vec3(20.0f, 0.5f, 0.5f)));
	meshes["deviceFrameBottom"].get_transform().position = vec3(0.0f, 0.25f, -24.0f);
	meshes["deviceFrameBottom"].set_material(whiteCopper);

	//##########################################################################################################################################################





	// Load textures -------------------------------------------------------------------------------------------------------------------------------------------
	texs["check_1"] = texture("textures/check_1.png", true, true);
	texs["floor"] = texture("textures/Asphalt.jpg", true, true);
	texs["arch0"] = texture("textures/concrete.jpg", true, true);
	texs["lamppost0"] = texture("textures/st-metal.jpg", true, true);
	texs["lamppost1"] = texture("textures/st-metal.jpg", true, true);
	texs["wall0"] = texture("textures/CeramicBrick_albedo_M.jpg", true, true);
	texs["wall1"] = texture("textures/map-8.jpg", true, true);
	texs["spotlight0"] = texture("textures/st-metal.jpg", true, true);
	texs["deviceFrameBottom"] = texture("textures/Copper_A_albedo_M.png", true, true);
	texs["deviceFrameRight"] = texture("textures/Copper_A_albedo_M.png", true, true);
	texs["deviceFrameLeft"] = texture("textures/Copper_A_albedo_M.png", true, true);
	texs["deviceFrameTop"] = texture("textures/Copper_A_albedo_M.png", true, true);
	texs["deviceArmHorizontal"] = texture("textures/Copper_A_albedo_M.png", true, true);
	texs["deviceArmVertical"] = texture("textures/Copper_A_albedo_M.png", true, true);
	texs["deviceRing"] = texture("textures/Copper_A_albedo_M.png", true, true);

	//----------------------------------------------------------------------------------------------------------------------------------------------------------




	// Load lights ############################################################################################################################################
	light = directional_light(vec4(0.003f, 0.003f, 0.003f, 1.0f), vec4(0.2f, 0.08f, 0.06f, 1.0f), normalize(vec3(0.6f, -1.0f, 0.3f)));		// evening
//	light = directional_light(vec4(0.0f, 0.0f, 0.0f, 1.0f), vec4(0.0f, 0.0f, 0.0f, 1.0f), normalize(vec3(0.6f, -1.0f, 0.3f)));				// No directional
//	light = directional_light(vec4(1.0f, 1.0f, 1.0f, 1.0f), vec4(0.0f, 0.0f, 0.0f, 1.0f), normalize(vec3(0.6f, -1.0f, 0.3f)));				// full ambient for debugging

	points.push_back(point_light(vec4(1.0f, 0.9f, 0.63f, 1.0f), vec3(25.0f, 11.5f, 18.0f), 0.0f, 0.01f, 0.01f));							// Lamppost0
	points.push_back(point_light(vec4(1.0f, 0.9f, 0.63f, 1.0f), vec3(25.0f, 11.5f, 0.0f), 0.0f, 0.01f, 0.01f));								// Lamppost1

	spots.push_back(spot_light(white, vec3(-16.5f, 14.3f, 5.0f), vec3(0.0f, -1.0f, 0.0f), 0.0f, 0.05f, 0.005f, 10.0f));						// spotlight0
	shadows.push_back(shadow_map(renderer::get_screen_width(), renderer::get_screen_height()));
	spots.push_back(spot_light(white, vec3(0.0f, 0.4f, -1.0f), vec3(0.0f, 0.0f, -1.0f), 0.0f, 0.05f, 0.0f, 10.0f));							// flashlight
	shadows.push_back(shadow_map(renderer::get_screen_width(), renderer::get_screen_height()));
	
	//#########################################################################################################################################################
	
	
	
	
	// Colours
	vector<vec4> colours{ vec4(1.0f, 0.0f, 0.0f, 1.0f), vec4(1.0f, 0.0f, 0.0f, 1.0f), vec4(1.0f, 0.0f, 0.0f, 1.0f) };

	// Load in shaders
	eff.add_shader("shaders/vert_shader.vert", GL_VERTEX_SHADER);
	vector<string> frag_shaders{ "shaders/top_shader.frag", "shaders/directional.frag", "shaders/spot.frag", "shaders/point.frag", "shaders/shadow_index.frag" };
	eff.add_shader(frag_shaders, GL_FRAGMENT_SHADER);

	shadow_eff.add_shader("shaders/shadow_spot.vert", GL_VERTEX_SHADER);
	shadow_eff.add_shader("shaders/shadow_spot.frag", GL_FRAGMENT_SHADER);

	// Build effect
	eff.build();
	shadow_eff.build();

	// Set camera properties
	target_cam.set_position(vec3(0.0f, 1.0f, 50.0f));
	target_cam.set_target(vec3(0.0f, 0.0f, 0.0f));
	target_cam.set_projection(quarter_pi<float>() * 1.3f, renderer::get_screen_aspect(), 0.1f, 1000.0f);

	free_cam.set_position(vec3(30.0f, 1.0f, 50.0f));
	free_cam.set_target(vec3(0.0f, 0.0f, 0.0f));
	free_cam.set_projection(quarter_pi<float>() * 1.3f, renderer::get_screen_aspect(), 0.1f, 1000.0f);

	cam_select = free0;
	return true;
}


bool update(float delta_time)
{

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
	// *********************************

	
	mat4 LightProjectionMat = perspective<float>(90.0f, renderer::get_screen_aspect(), 0.1f, 1000.f);

	// Bind shader
	renderer::bind(shadow_eff);
	auto V = shadows[1].get_view();
	// Render meshes
	for (auto &e : meshes) {
		auto m = e.second;
		// Create MVP matrix
		auto M = m.get_hierarchical_transform_matrix();
		// *********************************
		// View matrix taken from shadow map
		// *********************************
		auto MVP = LightProjectionMat * V * M;
		// Set MVP matrix uniform
		glUniformMatrix4fv(shadow_eff.get_uniform_location("MVP"), 1, GL_FALSE, value_ptr(MVP));
		// Render mesh
		renderer::render(m);
	}
	// *********************************
	// Set render target back to the screen
	renderer::set_render_target();
	// Set face cull mode to back
	glCullFace(GL_BACK);
	// *********************************

	// Bind shader
	renderer::bind(eff);


	//####################################################################################################################################



	// Render the scene
	mat4 M;
	for (auto &e : meshes)
	{
		mesh m = e.second;

		renderer::bind(eff);

	//	M = m.get_transform().get_transform_matrix();

		M = m.get_hierarchical_transform_matrix();



		auto MVP = calculatePV() * M;
	//	auto MVP = LightProjectionMat * shadows[1].get_view() * M;
		
		glUniformMatrix4fv(eff.get_uniform_location("MVP"), 1, GL_FALSE, value_ptr(MVP));
		glUniformMatrix4fv(eff.get_uniform_location("M"), 1, GL_FALSE, value_ptr(M));
		glUniformMatrix3fv(eff.get_uniform_location("N"), 1, GL_FALSE, value_ptr(m.get_hierarchical_normal_matrix()));
		mat4 lMVP = LightProjectionMat * shadows[1].get_view() * M;
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
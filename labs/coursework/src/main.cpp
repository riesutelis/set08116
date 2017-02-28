#include <glm\glm.hpp>
#include <graphics_framework.h>

using namespace std;
using namespace graphics_framework;
using namespace glm;

geometry geom;
effect eff;
map<string, mesh> meshes;
map<string, texture> texs;

enum cam_choice { free0, target0 };
target_camera target_cam;
free_camera free_cam;
cam_choice cam_select;

double cursor_x;
double cursor_y;

directional_light light;
vector<point_light> points;
vector<spot_light> spots;


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
	float speed = 0.6f;
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
	material whitePlastic = material(vec4(0.0f, 0.0f, 0.0f, 1.0f), vec4(1.0f, 1.0f, 1.0f, 1.0f), vec4(1.0f, 1.0f, 1.0f, 1.0f), 25.0f);

	// Load meshes
	meshes["floor"] = mesh(geometry_builder::create_plane());
	meshes["floor"].get_transform().position = vec3(0.0f, 0.0f, 0.0f);
	meshes["floor"].set_material(whitePlastic);

	meshes["arch0"] = mesh(geometry("models/arch.obj"));
	meshes["arch0"].get_transform().position = vec3(0.0f, 5.0f, 0.0f);
	meshes["arch0"].set_material(whitePlastic);

	meshes["lamppost0"] = mesh(geometry("models/lamp.obj"));
	meshes["lamppost0"].get_transform().position = vec3(25.0f, 0.0f, 12.0f);
	meshes["lamppost0"].get_transform().scale = vec3(0.05f, 0.05f, 0.05f);
	meshes["lamppost0"].set_material(whitePlastic);


	// Load lights
	light = directional_light(vec4(0.1f, 0.1f, 0.1f, 1.0f), vec4(0.5f, 0.4f, 0.4f, 1.0f), normalize(vec3(0.6f, -1.0f, 0.3f)));
	points.push_back(point_light(vec4(1.0f, 0.9f, 0.5f, 1.0f), vec3(25.0f, 11.5f, 12.0f), 1.0f, 0.0f, 0.0f));
	
	
	// Load textures
	texs["check_1"] = texture("textures/check_1.png");

	// Colours
	vector<vec4> colours{ vec4(1.0f, 0.0f, 0.0f, 1.0f), vec4(1.0f, 0.0f, 0.0f, 1.0f), vec4(1.0f, 0.0f, 0.0f, 1.0f) };

	// Load in shaders
//	eff.add_shader("shaders/basic.vert", GL_VERTEX_SHADER);
//	eff.add_shader("shaders/basic.frag", GL_FRAGMENT_SHADER);
	eff.add_shader("shaders/vert_shader.vert", GL_VERTEX_SHADER);
	vector<string> frag_shaders{ "shaders/top_shader.frag", "shaders/directional.frag", "shaders/spot.frag", "shaders/point.frag" };
	eff.add_shader(frag_shaders, GL_FRAGMENT_SHADER);

	// Build effect
	eff.build();

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
	if (glfwGetKey(renderer::get_window(), GLFW_KEY_1))
		cam_select = target0;
	else if (glfwGetKey(renderer::get_window(), GLFW_KEY_2))
		cam_select = free0;

	// Camera movement delegated to methods
	if (cam_select == free0)
		moveFreeCamera(delta_time);



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

	mat4 M;
	for (auto &e : meshes)
	{
		mesh m = e.second;

		renderer::bind(eff);

		M = m.get_transform().get_transform_matrix();
		auto MVP = calculatePV() * M;

		glUniformMatrix4fv(eff.get_uniform_location("MVP"), 1, GL_FALSE, value_ptr(MVP));
		glUniformMatrix4fv(eff.get_uniform_location("M"), 1, GL_FALSE, value_ptr(M));
		glUniformMatrix3fv(eff.get_uniform_location("N"), 1, GL_FALSE, value_ptr(m.get_transform().get_normal_matrix()));
		renderer::bind(m.get_material(), "mat");
		renderer::bind(light, "light");
		renderer::bind(points, "points");
		renderer::bind(spots, "spots");
		renderer::bind(texs["check_1"], 0);
		glUniform1i(eff.get_uniform_location("tex"), 0);
		glUniform3fv(eff.get_uniform_location("eye_pos"), 1, value_ptr(eye_pos()));
		renderer::render(m);
	}


	return true;
}

void main()
{
	// Create application
	app application("Graphics Coursework");
	// Set load content, update and render methods
	application.set_initialise(initialise);
	application.set_load_content(load_content);
	application.set_update(update);
	application.set_render(render);
	// Run application
	application.run();
}
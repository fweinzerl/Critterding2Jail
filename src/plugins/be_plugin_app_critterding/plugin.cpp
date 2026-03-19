#include "plugin.h"
#include "kernel/be_lib_handler.h"
#include "food_system.h"
#include "body_system.h"
#include "species_system.h"
// #include "critter_system.h"
#include "vision_system.h"
#include "control_panel.h"
#include "life_stats_panel.h"
#include "population_controller.h"
#include "critter_exchanger.h"
#include "keybind_config.h"
#include "commands.h"
// #include "plugins/be_plugin_bullet/be_entity_transform.h" // FIXME work this away
#include "plugins/be_plugin_opengl_modern/be_entity_camera.h"
#include "opengl_setup.h"
#include <iostream>
#include <cstdlib>

// 	void Scene::construct()
// 	{
// 		// LOAD QT APP FIXME THIS DOESN'T NEED TO GET ONTO THE TREE
// 			pluginManager()->load( "qt6", "src/plugins/be_plugin_qt6", "be_plugin_qt6" );
// 			auto spawner = addChild( "spawner", "QApplicationSpawner" );
// 			auto t_parent_to_add_to = spawner->getChildCustom( this );
// 			removeChild( spawner );
// 		
// 		pluginManager()->load( "app_admin_window", "src/plugins/be_plugin_app_admin_window", "be_plugin_app_admin_window" );
// 		pluginManager()->load( "app_sysmon", "src/plugins/be_plugin_app_sysmon", "be_plugin_app_sysmon" );
// 
// 		// t_parent_to_add_to->addChild( "Admin App", "AdminWindow" );
// 		// t_parent_to_add_to->addChild( "sysmon", "SystemMonitor" );
// 
// 		auto sdl_window = addChild("Critterding", new Critterding())->getChild("GLWindow");
// 		sdl_window->set("on_close_destroy_entity", this);
// 	}
	
	void Scene::construct()
	{
		setName( "Critterding" );
		std::cout << "Scene::construct()" << std::endl;

		// LOAD QT APP FIXME THIS DOESN'T NEED TO GET ONTO THE TREE
		pluginManager()->load( "qt6", "src/plugins/be_plugin_qt6", "be_plugin_qt6" );

			auto spawner = addChild( "spawner", "QApplicationSpawner" );
			// auto t_parent_to_add_to = spawner->getChildCustom( parent() );
			auto t_parent_to_add_to = spawner->getChildCustom( this );
			removeChild( spawner );

		pluginManager()->load( "app_admin_window", "src/plugins/be_plugin_app_admin_window", "be_plugin_app_admin_window" );
		pluginManager()->load( "app_sysmon", "src/plugins/be_plugin_app_sysmon", "be_plugin_app_sysmon" );

		// settings
			auto settings = addChild( "settings", new BEntity() );
			m_eat_transfer_energy = settings->addChild( "eat_energy_transfer", new BEntity_float() );
			m_eat_transfer_energy->set( 100.0f );
			m_eat_passive_fraction = settings->addChild( "eat_passive_fraction", new BEntity_float() );
			m_eat_passive_fraction->set( 0.25f );

		pluginManager()->load( "system", "src/plugins/be_plugin_system", "be_plugin_system" );
		pluginManager()->load( "opengl", "src/plugins/be_plugin_opengl_modern", "be_plugin_opengl_modern" );
		pluginManager()->load( "bullet", "src/plugins/be_plugin_bullet", "be_plugin_bullet" );
		pluginManager()->load( "brainz", "src/plugins/be_plugin_brainz", "be_plugin_brainz" );
		pluginManager()->load( "qwt", "src/plugins/be_plugin_qwt", "be_plugin_qwt" ); // FIXME

		if ( !pluginManager()->load( "glfw", "src/plugins/be_plugin_glfw", "be_plugin_glfw" ) )
		{
			// pluginManager()->load( "sdl", "src/plugins/be_plugin_sdl", "be_plugin_sdl" );
		}

		// // TIMER
		// 	addChild( "timer", "BTimer" );

		// RANDOM NUMBER GENERATOR
			auto rng = addChild( "random_number_generator", "Brng" );

		// SEED
			// FIXME FIX TIMER
			// auto startTimeMs = topParent()->getChild("sys", 1)->getChild( "timer",1 )->?? ;
			rng->set( "seed", Bint( 111 ) );
		
		// BULLET PHYSICS
			m_physics_world = addChild( "physicsworld", "PhysicsWorld" );
			m_physics_world_collisions = m_physics_world->getChild( "collisions", 1 );
			// m_physics_world->setFps(60);

		// RAYCAST
			m_bullet_raycast = m_physics_world->addChild( "raycaster", "Bullet_Raycast" );
			auto source = m_bullet_raycast->getChild( "source", 1 );
			m_raycast_source_x = source->getChild( "x", 1 );
			m_raycast_source_y = source->getChild( "y", 1 );
			m_raycast_source_z = source->getChild( "z", 1 );
			auto target = m_bullet_raycast->getChild( "target", 1 );
			m_raycast_target_x = target->getChild( "x", 1 );
			m_raycast_target_y = target->getChild( "y", 1 );
			m_raycast_target_z = target->getChild( "z", 1 );
			addChild( "external_raycaster", new BEntity_external() )->set( m_bullet_raycast );
			
		// MOUSE PICKER
			auto mousepicker = m_physics_world->addChild( "mousepicker", "Bullet_MousePicker" );
			addChild( "external_mousepicker", new BEntity_external() )->set( mousepicker );
		
		// CONNECT RAYCAST AND MOUSEPICKER VALUES
			auto ray_source = m_bullet_raycast->getChild("source", 1);
			auto mouse_source = mousepicker->getChild("source", 1);
			ray_source->getChild("x")->connectServerServer( mouse_source->getChild("x") );
			ray_source->getChild("y")->connectServerServer( mouse_source->getChild("y") );
			ray_source->getChild("z")->connectServerServer( mouse_source->getChild("z") );
			auto ray_target = m_bullet_raycast->getChild("target", 1);
			auto mouse_target = mousepicker->getChild("target", 1);
			ray_target->getChild("x")->connectServerServer( mouse_target->getChild("x") );
			ray_target->getChild("y")->connectServerServer( mouse_target->getChild("y") );
			ray_target->getChild("z")->connectServerServer( mouse_target->getChild("z") );

		// SDL & OPENGL
			auto glwindow = addChild( "GLWindow", "GLWindow" );
			glwindow->set("title", "Critterding 2");
			glwindow->set("on_close_destroy_entity", this);
			// glwindow->setFps(60);
			glwindow->addChild("OpenGL_Setup", "OpenGL_Setup");
			
			m_win_width = glwindow->getChild( "width", 1 );
			m_win_height = glwindow->getChild( "height", 1 );
			m_mouse_x = glwindow->getChild( "mouse_x", 1 );
			m_mouse_y = glwindow->getChild( "mouse_y", 1 );

		// SDL SWAPBUFFER, making sure this runs right after sdl_window and it's children are done processing
			auto swap_buffers = addChild("GLSwapBuffers", "GLSwapBuffers");
			swap_buffers->set("set_glwindow", glwindow);
			swap_buffers->setFps(50);

		// GRAPHICS MODELSYSTEM
			auto t_graphicsModelSystem = glwindow->addChild("GraphicsModelSystem", "GraphicsModelSystem");
			t_graphicsModelSystem->setFps(50);

		// CAMERA
			// m_camera = new BCamera();
			// t_graphicsModelSystem->addChild("Camera", m_camera);
			auto c = t_graphicsModelSystem->addChild("Camera", "Camera");
			m_camera = dynamic_cast<BCamera*>(c);
			
			
			auto transform = m_camera->getChild( "transform", 1 );

			// transform->getChild( "position_x" )->connectServerServer( m_raycast_source_x );
			// transform->getChild( "position_y" )->connectServerServer( m_raycast_source_y );
			// transform->getChild( "position_z" )->connectServerServer( m_raycast_source_z );

			transform->getChild( "position_x" )->set( 0.0f );
			transform->getChild( "position_y" )->set( 156.0f );
			transform->getChild( "position_z" )->set( -100.0f );
			transform->getChild( "rotation_euler_x" )->set( -0.5f * 3.141592f );
			transform->getChild( "rotation_euler_y" )->set( 0.0f );
			transform->getChild( "rotation_euler_z" )->set( 0.0f );
			// transform->getChild( "position_x" )->set( 0.0f );
			// transform->getChild( "position_y" )->set( -15.0f );
			// transform->getChild( "position_z" )->set( -88.0f );
			// transform->getChild( "rotation_euler_x" )->set( 0.0f );
			// transform->getChild( "rotation_euler_y" )->set( 0.0f );
			// transform->getChild( "rotation_euler_z" )->set( 0.0f );

		// KEYBIND CONFIG + MOVEMENT SETTINGS
			const auto keybind_config = load_cd_keybind_config();
			m_camera_turbo_multiplier = keybind_config.turbo_multiplier;
			m_camera_sensitivity_move = m_camera->getChild("sensitivity_move", 1);
			m_camera_move_base_sensitivity = m_camera_sensitivity_move->get_float();
			m_camera_turbo_active = m_camera->addChild("turbo_active", new BEntity_bool());
			m_camera_turbo_active->set(false);
			auto movement_settings = m_camera->getChild("movement", 1)->getChild("settings", 1);
			movement_settings->getChild("forward_horizontal_only", 1)->set(keybind_config.forward_horizontal_only);
			movement_settings->getChild("allow_roll_for_movement", 1)->set(keybind_config.allow_roll_for_movement);

		// COMMANDS
			auto commands = glwindow->addChild( "commands", new BEntity() );
			auto toggleFullscreen = commands->addChild( "toggleFullscreen", new cmd_toggleFullscreen() );
			auto launchAdminWindow = commands->addChild( "launchAdminWindow", new cmd_launchAdminWindow() );
			auto launchControlPanel = commands->addChild( "launchControlPanel", new cmd_launchControlPanel() );
			auto launchLifeStatsPanel = commands->addChild( "launchLifeStatsPanel", new cmd_launchLifeStatsPanel() );
			auto launchSystemMonitor = commands->addChild( "launchSystemMonitor", new cmd_launchSystemMonitor() );
			auto launchSelectionWindow = commands->addChild( "launchSelectionWindow", new cmd_launchSelectionWindow() );
			auto mousePickBody = commands->addChild( "mousePickBody", new cmd_mousePickBody() );
			auto mouseUnpickBody = commands->addChild( "mouseUnpickBody", new cmd_mouseUnpickBody() );

		// BINDINGS
			auto bindings = glwindow->getChild( "bindings", 1 );
			auto bind_trigger = [bindings](const std::string& key, BEntity* target)
			{
				auto binding = bindings->addChild(key.c_str(), new BEntity_trigger());
				binding->connectServerServer(target);
			};
			auto bind_hold = [bindings](const std::string& key, BEntity* target)
			{
				auto binding = bindings->addChild(key.c_str(), new BEntity_bool());
				binding->connectServerServer(target);
			};
			auto bind_keydown_toggle = [bindings](const std::string& key, BEntity* target, bool initial)
			{
				std::string node_name("key_down_");
				node_name += key;
				auto binding = bindings->addChild(node_name.c_str(), new BEntity_bool());
				binding->set(initial);
				binding->connectServerServer(target);
			};

			for (const auto& key : get_cd_action_keys(keybind_config, "ui.open_admin_window", {"f1"}))
				bind_trigger(key, launchAdminWindow);
			for (const auto& key : get_cd_action_keys(keybind_config, "ui.open_control_panel", {"f2"}))
				bind_trigger(key, launchControlPanel);
			for (const auto& key : get_cd_action_keys(keybind_config, "ui.open_system_monitor", {"f3"}))
				bind_trigger(key, launchSystemMonitor);
			for (const auto& key : get_cd_action_keys(keybind_config, "ui.open_life_stats_panel", {"f4"}))
				bind_trigger(key, launchLifeStatsPanel);

			auto render_active = t_graphicsModelSystem->getChild("active", 1);
			if (render_active)
			{
				for (const auto& key : get_cd_action_keys(keybind_config, "render.toggle", {"f10"}))
					bind_keydown_toggle(key, render_active, true);
			}
			auto fullscreen = glwindow->getChild("fullscreen", 1);
			if (fullscreen)
			{
				for (const auto& key : get_cd_action_keys(keybind_config, "window.toggle_fullscreen", {"f11"}))
					bind_keydown_toggle(key, fullscreen, false);
			}
			auto vsync = glwindow->getChild("vsync", 1);
			if (vsync)
			{
				for (const auto& key : get_cd_action_keys(keybind_config, "window.toggle_vsync", {"f12"}))
					bind_keydown_toggle(key, vsync, false);
			}

			auto binding_mouse_2 = bindings->addChild( "mousebutton_down_2", new BEntity_trigger() );  // FIXME CONNECT TO bool under std_window
			binding_mouse_2->connectServerServer( launchSelectionWindow );

			auto binding_mouse_3_down = bindings->addChild( "mousebutton_down_1", new BEntity_trigger() );  // FIXME CONNECT TO bool under std_window
			binding_mouse_3_down->connectServerServer( mousePickBody );
			auto binding_mouse_3_up = bindings->addChild( "mousebutton_up_1", new BEntity_trigger() );  // FIXME CONNECT TO bool under std_window
			binding_mouse_3_up->connectServerServer( mouseUnpickBody );
			
			auto movement = m_camera->getChild("movement", 1);
			auto forward = movement->getChild( "forward", 1 );
			auto backward = movement->getChild( "backward", 1 );
			auto left = movement->getChild( "left", 1 );
			auto right = movement->getChild( "right", 1 );
			auto up = movement->getChild( "up", 1 );
			auto down = movement->getChild( "down", 1 );
			for (const auto& key : get_cd_action_keys(keybind_config, "camera.move_forward", {"up"}))
				bind_hold(key, forward);
			for (const auto& key : get_cd_action_keys(keybind_config, "camera.move_backward", {"down"}))
				bind_hold(key, backward);
			for (const auto& key : get_cd_action_keys(keybind_config, "camera.move_left", {"left"}))
				bind_hold(key, left);
			for (const auto& key : get_cd_action_keys(keybind_config, "camera.move_right", {"right"}))
				bind_hold(key, right);
			for (const auto& key : get_cd_action_keys(keybind_config, "camera.move_up", {"home"}))
				bind_hold(key, up);
			for (const auto& key : get_cd_action_keys(keybind_config, "camera.move_down", {"end"}))
				bind_hold(key, down);

			auto looking = m_camera->getChild("looking", 1);
			auto look_left = looking->getChild( "left", 1 );
			auto look_right = looking->getChild( "right", 1 );
			auto look_up = looking->getChild( "up", 1 );
			auto look_down = looking->getChild( "down", 1 );
			auto look_roll_left = looking->getChild( "roll_left", 1 );
			auto look_roll_right = looking->getChild( "roll_right", 1 );
			for (const auto& key : get_cd_action_keys(keybind_config, "camera.look_left", {"[4]"}))
				bind_hold(key, look_left);
			for (const auto& key : get_cd_action_keys(keybind_config, "camera.look_right", {"[6]"}))
				bind_hold(key, look_right);
			for (const auto& key : get_cd_action_keys(keybind_config, "camera.look_up", {"[2]"}))
				bind_hold(key, look_up);
			for (const auto& key : get_cd_action_keys(keybind_config, "camera.look_down", {"[8]"}))
				bind_hold(key, look_down);
			for (const auto& key : get_cd_action_keys(keybind_config, "camera.roll_left", {"[1]"}))
				bind_hold(key, look_roll_left);
			for (const auto& key : get_cd_action_keys(keybind_config, "camera.roll_right", {"[3]"}))
				bind_hold(key, look_roll_right);
			for (const auto& key : get_cd_action_keys(keybind_config, "camera.turbo", {"shift"}))
				bind_hold(key, m_camera_turbo_active);

		// "SHADERS" hack for now
			auto shaders = t_graphicsModelSystem->addChild( "shaders", "entity" );
			auto u_vec4_color = shaders->addChild( "u_Color", "ShaderUniformVec4" );
			auto u_i14_textureSample = shaders->addChild( "u_textureSample", "ShaderUniformI1" );
			shaders->addChild( "e_scale_x", "float" )->set(0.0f);
			shaders->addChild( "e_scale_y", "float" )->set(0.0f);
			shaders->addChild( "e_scale_z", "float" )->set(0.0f);

// 		// LIGHT
// 		{
// 			auto light = t_graphicsModelSystem->addChild( "light", "GLLight" );
// 			
// 			light->getChild( "model_ambient_r", 1 )->set( 0.5f );
// 			light->getChild( "model_ambient_g", 1 )->set( 0.5f );
// 			light->getChild( "model_ambient_b", 1 )->set( 0.5f );
// 			light->getChild( "model_ambient_a", 1 )->set( 0.0f );
// 			
// 			light->getChild( "color_ambient_r", 1 )->set( 0.5f );
// 			light->getChild( "color_ambient_g", 1 )->set( 0.5f );
// 			light->getChild( "color_ambient_b", 1 )->set( 0.5f );
// 			light->getChild( "color_ambient_a", 1 )->set( 0.0f );
// 			
// 			light->getChild( "color_diffuse_r", 1 )->set( 0.4f );
// 			light->getChild( "color_diffuse_g", 1 )->set( 0.4f );
// 			light->getChild( "color_diffuse_b", 1 )->set( 0.4f );
// 			light->getChild( "color_diffuse_a", 1 )->set( 0.0f );
// 
// 			light->getChild( "color_specular_r", 1 )->set( 0.2f );
// 			light->getChild( "color_specular_g", 1 )->set( 0.2f );
// 			light->getChild( "color_specular_b", 1 )->set( 0.2f );
// 			light->getChild( "color_specular_a", 1 )->set( 0.0f );
// 		}

		// MAP
		{
			auto map = addChild( "map", new BEntity() );
			// const char* map_location = "../share/modules/cube-floor-100-1.obj";
			const char* map_location = "../share/modules/easy.obj";
			// const char* map_location = "../share/modules/medium.obj";
			// const char* map_location = "../share/modules/hard.obj";

			// PHYSICS
				auto physics_entity = m_physics_world->addChild( "physics_entity_map", "PhysicsEntity" );
				physics_entity->getChild( "filename", 1 )->set( map_location );
				map->addChild( "external_physics", new BEntity_external() )->set(physics_entity);

				auto physics_weight = physics_entity->addChild("weight", new BEntity_float_property());
					physics_weight->set( 0.0f );

				auto physics_scale_x = physics_entity->addChild("scale_x", new BEntity_float_propertyNew());
				auto physics_scale_y = physics_entity->addChild("scale_y", new BEntity_float_propertyNew());
				auto physics_scale_z = physics_entity->addChild("scale_z", new BEntity_float_propertyNew());

			// GRAPHICS
				auto t_graphicsModel = t_graphicsModelSystem->addChild("graphics_entity_map", "GraphicsModel");
				t_graphicsModel->addChild( "filename", new BEntity_string_property() )->set( map_location );
				auto graphics_transform = t_graphicsModel->addChild("transform", "Transform");
				map->addChild( "external_graphics", new BEntity_external() )->set(t_graphicsModel);

			// CONNECT AND POSITION
				auto physics_entity_transform = physics_entity->getChild("transform", 1);
				if ( physics_entity_transform )
				{
					physics_entity_transform->getChild("position_x", 1)->set( 0.0f );
					physics_entity_transform->getChild("position_y", 1)->set( -20.0f );
					physics_entity_transform->getChild("position_z", 1)->set( -100.0f );

					physics_scale_x->connectServerServer( graphics_transform->getChild("scale_x", 1) );
					physics_scale_y->connectServerServer( graphics_transform->getChild("scale_y", 1) );
					physics_scale_z->connectServerServer( graphics_transform->getChild("scale_z", 1) );

					physics_scale_x->set( 8.0f );
					physics_scale_y->set( 1.0f );
					physics_scale_z->set( 10.0f );

					// CONNECT
					physics_entity_transform->connectServerServer( graphics_transform );
					physics_entity_transform->onUpdate();
				}
		}

		// SKY DOME
			auto t_graphicsModelSkyDome = t_graphicsModelSystem->addChild("GraphicsModel_SkyDome", "GraphicsModel");
			t_graphicsModelSkyDome->set("filename", "../share/modules/skydome3.obj");

		// DEPTHMAP VIEWER
			t_graphicsModelSystem->addChild("DepthMapViewer", "DepthMapViewer");

		// VISION SYSTEM
			auto vision_system = addChild( "vision_system", "CdVisionSystem" );
  			// addChild("GLSwapBuffers", "GLSwapBuffers")->set("set_glwindow", glwindow);

		// CRITTER SYSTEM
			auto critter_system = addChild( "critter_system", new CdCritterSystem() );
			m_critter_unit_container = critter_system->getChild( "unit_container", 1 );

		// REGISTER UNIT CONTAINER IN VISION SYSTEM
			vision_system->set("register_container", m_critter_unit_container);

		// FOOD SYSTEM
			auto food_system = addChild( "food_system", new CdFoodSystem() );
			m_food_unit_container = food_system->getChild( "unit_container", 1 );

		// POPULATION CONTROLLER
			auto population_controller = addChild( "CdPopulationController", new CdPopulationController() );
			population_controller->set( "register_critter_container", m_critter_unit_container );
			population_controller->set( "register_food_container", m_food_unit_container );

		// CRITTER EXCHANGER
			addChild( "CdCritterExchanger", new CdCritterExchanger() );

	}

	void Scene::process()
	{
		if (m_camera_sensitivity_move && m_camera_turbo_active)
		{
			const auto turbo_factor = m_camera_turbo_active->get_bool() ? m_camera_turbo_multiplier : 1.0f;
			m_camera_sensitivity_move->set(m_camera_move_base_sensitivity * turbo_factor);
		}

		// CHECK PHYSICS COLLISIONS
			for_all_children_of( m_physics_world_collisions )
			{
				auto e1 = (*child)->getChild( "entity1", 1 )->get_reference();
				auto e2 = (*child)->getChild( "entity2", 1 )->get_reference();
				// std::cout << "collision e1: " << e1->name() << "(" << e1->id() << ")" << std::endl;
				// std::cout << "collision e2: " << e2->name() << "(" << e2->id() << ")" << std::endl;

				// EAT: TRANSFER ENERGY FROM FOOD TO CRITTER
				// Energy scales with eat output: passive_base at eat=0, full at eat=1
					auto critter = findCritter( e1, e2 );
					if ( critter )
					{
						auto food = findFood( e1, e2 );
						if ( food )
						{
							auto eat = critter->getChild("motor_neurons", 1)->getChild("eat", 1);
							float eat_value = eat->get_float();
							if ( eat_value < 0.0f ) eat_value = 0.0f;
							if ( eat_value > 1.0f ) eat_value = 1.0f;

							float passive = m_eat_passive_fraction->get_float();
							float scale = passive + (1.0f - passive) * eat_value;
							float transfer = m_eat_transfer_energy->get_float() * scale;

							auto critter_energy = critter->getChild("energy", 1);
							auto food_energy = food->getChild("energy", 1);

							if ( food_energy->get_float() < transfer )
							{
								critter_energy->set( critter_energy->get_float() + food_energy->get_float() );
								food_energy->set( 0.0f );
							}
							else
							{
								critter_energy->set( critter_energy->get_float() + transfer );
								food_energy->set( food_energy->get_float() - transfer );
							}
						}
					}
			}

		// FIXME CONNECT THESE UP
		// CAST RAY FROM MOUSE
			auto camera_position = m_camera->m_transform->m_transform.getOrigin();
			m_raycast_source_x->set( camera_position.x() );
			m_raycast_source_y->set( camera_position.y() );
			m_raycast_source_z->set( camera_position.z() );

			btVector3 rayDirection = m_camera->getScreenDirection( m_win_width->get_int(), m_win_height->get_int(), m_mouse_x->get_int(), m_mouse_y->get_int() );
			m_raycast_target_x->set( rayDirection.x() );
			m_raycast_target_y->set( rayDirection.y() );
			m_raycast_target_z->set( rayDirection.z() );
			
			m_bullet_raycast->process();
	}

	BEntity* Scene::findCritter( BEntity* e1, BEntity* e2 )
	{
		for_all_children_of2( m_critter_unit_container )
		{
			
			auto critter = dynamic_cast<CdCritter*>( *child2 );
			if ( critter )
			{
				if ( critter->m_bodyparts_shortcut == 0 )
				{
					std::cerr << "ERROR: scene::findCritter: missing required bodyparts shortcut" << std::endl;
					std::exit(1);
				}

				for_all_children_of3( critter->m_bodyparts_shortcut )
				{
					if ( (*child3)->get_reference() == e1 || (*child3)->get_reference() == e2 )
					{
						return (*child2);
					}
				}
			}
		}

		return 0;
	}

	// FIXME like findCritter above
	BEntity* Scene::findFood( BEntity* e1, BEntity* e2 )
	{
		BEntity* food_bp( e1 );
		if ( e2->name() == "physics_entity_food" )
		{
			food_bp = e2;
		}
		for_all_children_of2( m_food_unit_container )
		{
			if ( (*child2)->getChild( "external_physics", 1 )->get_reference() == food_bp )
			{
				return (*child2);
			}
		}
		return 0;
	}
	
	
// ---- FACTORIES
	enum CLASS
	{
		  PLUGIN_INFO
		, SCENE
		, CRITTERDING
		, CD_CONTROL_PANEL
		, CD_LIFE_STATS_PANEL
		, CD_POPULATION_CONTROL
		, CD_CRITTER_EXCHANGER
		, CD_CRITTER_SYSTEM
		, CD_SPECIES_SYSTEM
		, CD_VISION_SYSTEM
		, CD_CRITTER
		, CD_FOOD_SYSTEM
		, CD_FOOD
		, CD_BODY_SYSTEM
		, CD_BODY // FIXME REMOVE LEGACY
		, CD_BODY_PLAN
		, CD_BODY_FIXED1
		, OPENGL_SETUP
	};

	extern "C" BEntity* create( BEntity* parent, const Buint type )
	{
		// PLUGIN DESCRIPTION ENTITY
			if ( type == PLUGIN_INFO )
			{
				BClassesHelper i;
					i.addClass( parent, CLASS::SCENE, "Scene" );
					i.addClass( parent, CLASS::CRITTERDING, "Critterding" );
					i.addClass( parent, CLASS::CD_CONTROL_PANEL, "CdControlPanel" );
					i.addClass( parent, CLASS::CD_LIFE_STATS_PANEL, "CdLifeStatsPanel" );
					i.addClass( parent, CLASS::CD_POPULATION_CONTROL, "CdPopulationController" );
					i.addClass( parent, CLASS::CD_CRITTER_EXCHANGER, "CdCritterExchanger" );
					i.addClass( parent, CLASS::CD_CRITTER_SYSTEM, "CdCritterSystem" );
					i.addClass( parent, CLASS::CD_SPECIES_SYSTEM, "CdSpeciesSystem" );
					i.addClass( parent, CLASS::CD_VISION_SYSTEM, "CdVisionSystem" );
					i.addClass( parent, CLASS::CD_CRITTER, "CdCritter" );
					i.addClass( parent, CLASS::CD_FOOD_SYSTEM, "CdFoodSystem" );
					i.addClass( parent, CLASS::CD_FOOD, "CdFood" );
					i.addClass( parent, CLASS::CD_BODY_SYSTEM, "CdBodySystem" );
					i.addClass( parent, CLASS::CD_BODY, "CdBody" );
					i.addClass( parent, CLASS::CD_BODY_PLAN, "CdBodyPlan" );
					i.addClass( parent, CLASS::CD_BODY_FIXED1, "BodyFixed1" );
					i.addClass( parent, CLASS::OPENGL_SETUP, "OpenGL_Setup" );
				return 0;
			}

		// ENTITIES
			else
			{
				BEntity* i(0);

				if ( type == CLASS::SCENE )
					i = new Scene();
				else if ( type == CLASS::CRITTERDING )
					i = new Scene();
				else if ( type == CLASS::CD_CONTROL_PANEL )
					i = new CdControlPanel();
				else if ( type == CLASS::CD_LIFE_STATS_PANEL )
					i = new CdLifeStatsPanel();
				else if ( type == CLASS::CD_POPULATION_CONTROL )
					i = new CdPopulationController();
				else if ( type == CLASS::CD_CRITTER_EXCHANGER )
					i = new CdCritterExchanger();
				else if ( type == CLASS::CD_CRITTER_SYSTEM )
					i = new CdCritterSystem();
				else if ( type == CLASS::CD_SPECIES_SYSTEM )
					i = new CdSpeciesSystem();
				else if ( type == CLASS::CD_VISION_SYSTEM )
					i = new CdVisionSystem();
				else if ( type == CLASS::CD_CRITTER )
					i = new CdCritter();
				else if ( type == CLASS::CD_FOOD_SYSTEM )
					i = new CdFoodSystem();
				else if ( type == CLASS::CD_FOOD )
					i = new CdFood();
				else if ( type == CLASS::CD_BODY_SYSTEM )
					i = new BodySystem();
				else if ( type == CLASS::CD_BODY )
					i = new BEntity();
				else if ( type == CLASS::CD_BODY_PLAN )
					i = new CdBodyPlan();
				else if ( type == CLASS::CD_BODY_FIXED1 )
					i = new CdBodyPlan();
				else if ( type == CLASS::OPENGL_SETUP )
					i = new OpenGL_Setup();

				return i;
			}
	}

	extern "C" void destroy( BEntity* p )
	{
		delete p;
	}
	

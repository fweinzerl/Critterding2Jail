#include "critter_system.h"
#include "kernel/be_entity_core_types.h"
#include "body_runtime_access.h"
// #include "species_system.h"
#include "plugins/be_plugin_bullet/be_entity_mousepicker.h"
#include <cmath>
#include <limits>
// #include <iostream>

namespace
{
	unsigned int find_vision_retina_size_or_die(BEntity* critter_system)
	{
		auto bin = critter_system->topParent()->getChild("bin", 1);
		if ( bin == 0 )
		{
			std::cerr << "ERROR: critter_system: missing bin root while reading vision retina size" << std::endl;
			std::exit(1);
		}
		auto app = bin->getChild("Critterding", 1);
		if ( app == 0 )
		{
			std::cerr << "ERROR: critter_system: missing Critterding root while reading vision retina size" << std::endl;
			std::exit(1);
		}
		auto vision_system = app->getChild("vision_system", 1);
		if ( vision_system == 0 )
		{
			std::cerr << "ERROR: critter_system: missing vision_system while reading retina size" << std::endl;
			std::exit(1);
		}
		auto retina_size = vision_system->getChild("retina_size", 1);
		if ( retina_size == 0 || retina_size->get_uint() == 0 )
		{
			std::cerr << "ERROR: critter_system: invalid vision retina_size" << std::endl;
			std::exit(1);
		}
		return retina_size->get_uint();
	}
}

	void CdCritterSystem::refreshBodyShortcuts(CdCritter* critter)
	{
		if ( critter == 0 )
		{
			return;
		}

		critter->m_body_root_shortcut = cd_body_runtime::find_body_root_from_critter( critter );
		if ( critter->m_body_root_shortcut == 0 )
		{
			critter->m_constraints_shortcut = 0;
			critter->m_bodyparts_shortcut = 0;
			critter->m_physics_component_shortcut = 0;
			critter->m_transform_shortcut = 0;
			return;
		}

		critter->m_constraints_shortcut = cd_body_runtime::find_constraints_from_body_root( critter->m_body_root_shortcut );
		critter->m_bodyparts_shortcut = critter->m_body_root_shortcut->getChild( "bodyparts", 1 );
		critter->m_physics_component_shortcut = cd_body_runtime::find_primary_bodypart_physics_from_critter( critter );
		critter->m_transform_shortcut = 0;
		if ( critter->m_physics_component_shortcut )
		{
			critter->m_transform_shortcut = critter->m_physics_component_shortcut->getChild( "transform", 1 );
		}
	}
 
	void CdCritterSystem::construct()
	{
		m_command_buffer = getCommandBuffer();
		auto settings = addChild( "settings", new BEntity() );

		// BRAINZ SYSTEM
			m_brain_system = addChild( "brain_system", "BrainSystem" );

		// BODY SYSTEM
			auto body_system = addChild( "body_system", "CdBodySystem" );
			m_body_system_unit_container = body_system->getChild("unit_container", 1);
			// m_body_system = addChild( "body_system", "CdBodySystem" );

		// // SO MOVE THIS UP THE TREE, HAVE A set("register" entity*) in visionsystem, then register it in plugin.cpp
		// // VISION SYSTEM
		// 	auto vision_system = addChild( "vision_system", "CdVisionSystem" );
		// 	addChild("SDLSwapBuffers", "SDLSwapBuffers");

		// // SPECIES SYSTEM
		// 	m_species_system = new CdSpeciesSystem();
		// 	addChild( "species_system", m_species_system );

		// UNIT CONTAINER
			m_unit_container = addChild( "unit_container", new BEntity() );
		
		// // REGISTER UNIT CONTAINER IN VISION SYSTEM
		// 	vision_system->set("register_container", m_unit_container);
		
		// SETTINGS
		m_minimum_number_of_units = settings->addChild( "minimum_number_of_units", new BEntity_uint() );
		m_maximum_age = settings->addChild( "maximum_age", new BEntity_uint() );
		m_intitial_energy = settings->addChild( "intitial_energy", new BEntity_float() );
		m_procreate_minimum_energy = settings->addChild( "procreate_minimum_energy ", new BEntity_float() );
		
		auto dropzone = settings->addChild( "dropzone", new BEntity() );
		m_dropzone_position_x = dropzone->addChild( "position_x", new BEntity_float() );
		m_dropzone_position_y = dropzone->addChild( "position_y", new BEntity_float() );
		m_dropzone_position_z = dropzone->addChild( "position_z", new BEntity_float() );
		m_dropzone_size_x = dropzone->addChild( "size_x", new BEntity_float() );
		m_dropzone_size_y = dropzone->addChild( "size_y", new BEntity_float() );
		m_dropzone_size_z = dropzone->addChild( "size_z", new BEntity_float() );
		
		m_minimum_number_of_units->set( Buint(20) );
		m_intitial_energy->set( Bfloat(1500.0f) );
		m_procreate_minimum_energy->set( Bfloat(2501.0f) );
		m_maximum_age->set( Buint(18000) );
		m_dropzone_position_x->set( Bfloat(-100.0f) );
		m_dropzone_position_y->set( Bfloat(-18.0f) );
		m_dropzone_position_z->set( Bfloat(-170.0f) );
		m_dropzone_size_x->set( Bfloat(200.0f) );
		m_dropzone_size_y->set( Bfloat(1.5f) );
		m_dropzone_size_z->set( Bfloat(140.0f) );

		// m_minimum_number_of_units->set( Buint(1) );
		// m_intitial_energy->set( Bfloat(1500.0f) );
		// m_procreate_minimum_energy->set( Bfloat(2501.0f) );
		// m_maximum_age->set( Buint(16000000) );
		// m_dropzone_position_x->set( Bfloat(-90.0f) );
		// m_dropzone_position_y->set( Bfloat(-18.0f) );
		// m_dropzone_position_z->set( Bfloat(-190.0f) );
		// m_dropzone_size_x->set( Bfloat(180.0f) );
		// m_dropzone_size_y->set( Bfloat(1.0f) );
		// m_dropzone_size_z->set( Bfloat(180.0f) );
		
			m_insert_frame_interval = settings->addChild( "insert_frame_interval", new BEntity_uint() );
			m_insert_frame_interval->set( (Buint)20 );

			m_copy_random_position = settings->addChild( "copy_random_position", new BEntity_bool() );
			m_copy_random_position->set( false );

			auto stats = settings->addChild( "stats", new BEntity() );
			m_stats_births_total = stats->addChild( "births_total", new BEntity_uint() );
			m_stats_deaths_total = stats->addChild( "deaths_total", new BEntity_uint() );
			m_stats_energy_total = stats->addChild( "energy_total", new BEntity_float() );
			m_stats_births_total->set( Buint(0) );
			m_stats_deaths_total->set( Buint(0) );
			m_stats_energy_total->set( Bfloat(0.0f) );
			m_learning_enabled = settings->addChild( "learning_enabled", new BEntity_bool() );
			m_learning_episode_ticks = settings->addChild( "learning_episode_ticks", new BEntity_uint() );
			m_learning_reward_energy_weight = settings->addChild( "learning_reward_energy_weight", new BEntity_float() );
			m_learning_reward_green_weight = settings->addChild( "learning_reward_green_weight", new BEntity_float() );
			m_learning_reward_tick_cost = settings->addChild( "learning_reward_tick_cost", new BEntity_float() );
			m_learning_explore_mutation_chance = settings->addChild( "learning_explore_mutation_chance", new BEntity_uint() );
			m_learning_enabled->set( true );
			m_learning_episode_ticks->set( Buint(300) );
			m_learning_reward_energy_weight->set( Bfloat(0.02f) );
			m_learning_reward_green_weight->set( Bfloat(0.40f) );
			m_learning_reward_tick_cost->set( Bfloat(0.01f) );
			m_learning_explore_mutation_chance->set( Buint(5) );
				m_stats_learning_avg_episode_reward = stats->addChild( "learning_avg_episode_reward", new BEntity_float() );
				m_stats_learning_mutations_total = stats->addChild( "learning_mutations_total", new BEntity_uint() );
				m_stats_learning_avg_episode_reward->set( Bfloat(0.0f) );
				m_stats_learning_mutations_total->set( Buint(0) );
				m_eat_active_cost = settings->addChild( "eat_active_cost", new BEntity_float() );
				m_eat_active_cost->set( Bfloat(0.5f) );
		
		m_mouse_picker = 0;
		auto ext = parent()->getChild("external_mousepicker", 1);
		if ( ext )
			m_mouse_picker = dynamic_cast<BMousePicker*>( ext->get_reference() );
		m_rng = parent()->getChild( "random_number_generator", 1 );

		// CPG: load config from the same body plan file used by body_system
		{
			auto body_sys = getChild("body_system", 1);
			if ( body_sys )
			{
				auto settings = body_sys->getChild("settings", 1);
				if ( settings )
				{
					auto bpf = settings->getChild("body_plan_file", 1);
					if ( bpf )
					{
						m_cpg_system.loadConfig( bpf->get_string() );
					}
				}
			}
		}

		m_collisions = 0;
	}
	
	void CdCritterSystem::process()
	{
		// // GIVE SPECIES SPECIES, FIXME this is a fix for a manually loaded critter
		// 	// pick the last entity from m_unit_container
		// 	auto it = m_unit_container->children().rbegin();
		// 	if ( it != m_unit_container->children().rend() )
		// 	{
		// 		// if it doesn't have a species_reference, create a new species
		// 		auto species_reference = (*it)->getChild("species_reference", 1);
		// 		if ( !species_reference )
		// 		{
		// 			// species
		// 			m_species_system->addNewSpecies( (*it) );
		// 		}
		// 	}
		
		// AGE ALL UNITS WITH A DAY, COUNT UP ALL ENERGY FROM UNITS (FIXME ACCOUNT FOR CRITTERS, MAKE GLOBAL VARIABLE FOR TOTAL ENERGY)
			float total_energy_in_entities(0.0f);
			for_all_children_of( m_unit_container )
			{
				auto critter_unit = dynamic_cast<CdCritter*>( *child );
				if ( critter_unit )
				{
					critter_unit->setAge( 1+critter_unit->age() );

					// eat activation cost: penalize keeping eat output high
					auto eat = critter_unit->getChild("motor_neurons", 1)->getChild("eat", 1);
					if ( eat )
					{
						float eat_value = eat->get_float();
						if ( eat_value > 0.0f )
						{
							if ( eat_value > 1.0f ) eat_value = 1.0f;
							critter_unit->setEnergy( critter_unit->energy() - m_eat_active_cost->get_float() * eat_value );
						}
					}

					total_energy_in_entities += critter_unit->energy();
					// CPG: fixed speed=1, turn=0 (brain control comes later)
					m_cpg_system.update( critter_unit, critter_unit->m_cpg_phase, critter_unit->m_cpg_params, 1.0f, 0.0f );
					// critter_unit->m_always_firing_input->onUpdate();
				}
			}
			m_stats_energy_total->set( total_energy_in_entities );
			if ( !m_cpg_system.enabled() )
				updateLifetimeLearning();

		// INSERT NEW RANDOM CRITTER, only check every 100 frames
			if ( m_minimum_number_of_units->get_uint() > 0 && (++m_framecount == m_insert_frame_interval->get_uint() || m_insert_frame_interval->get_uint() == 0 ) )
			{
				// std::cout << "new random critter" << std::endl;
				m_framecount = 0;
				if ( m_unit_container->numChildren() < m_minimum_number_of_units->get_uint() )
				{
					auto cmd_insert = m_command_buffer->addChild( "pass_command", new BEntity_reference() );
					cmd_insert->set(this);
					auto command = cmd_insert->addChild( "command", new BEntity_string() );
					command->set("insert_critter");

					// PREVENT FURTHER ACTIONS IN THIS FRAME
						return;
				}
			}

		// DIE FROM OLD AGE OR ENERGY DEPLETION
		{
			for_all_children_of2( m_unit_container )
			{
				auto critter_unit = dynamic_cast<CdCritter*>( *child2 );
				if ( critter_unit )
				{
					// reached max age or energy is depleted
					if ( critter_unit->age() >= m_maximum_age->get_uint() || critter_unit->energy() <= 0.0f )
					{
						removeCritter( *child2 );
						
						// PREVENT FURTHER ACTIONS IN THIS FRAME
							return; 
					}
				}
			}
		}

		// PROCREATE
// 		if ( 1 == 1 )
		{
			static Buint t_highest(0);
			
			for_all_children_of2( m_unit_container )
			{
				auto critter_unit = dynamic_cast<CdCritter*>( *child2 );
				if ( critter_unit )
				{
					// energy is enough
					if ( critter_unit->energy() >= m_procreate_minimum_energy->get_float() )
					{
						auto procreate = critter_unit->getChild( "motor_neurons", 1)->getChild( "procreate", 1);
						// CPG mode: auto-procreate when energy is sufficient
						if ( m_cpg_system.enabled() )
							procreate->set(1.0f);

						// FIXME SHOULD THIS BE IsFiring? TURNS OUT THIS ARE FLOATS and not motor neurons
						if ( procreate->get_float() != 0.0f )
						// if ( dynamic_cast<BNeuron*>(procreate)->m_firing->get_bool() )
						// if ( procreate->get_reference()->getChild("firing", 1)->get_bool() )
						{
							procreate->set( 0.0f );
							critter_unit->setEnergy( critter_unit->energy() / 2 );

							auto cmd_procreate = m_command_buffer->addChild( "pass_command", new BEntity_reference() );
							cmd_procreate->set(this);
							auto command = cmd_procreate->addChild( "command", new BEntity_string() );
							command->set("procreate_critter");
							command->addChild( "entity", new BEntity_reference() )->set( critter_unit );
							
							// std::cout << "COPYING CRITTER: " << critter_unit->id() << " done" << std::endl << std::endl;
							// PREVENT FURTHER ACTIONS IN THIS FRAME
								return; 
						}
					}
				}
			}
		}
	}

	// bool CdCritterSystem::set( const char* value )
	bool CdCritterSystem::set( const Bstring& id, BEntity* value )
	{
			if ( id == std::string("insert_critter") )
			{
						auto critter_unit = new CdCritter();
						m_unit_container->addChild( "critter_unit", critter_unit );
						critter_unit->setEnergy( m_intitial_energy->get_float() );
						if ( m_cpg_system.enabled() )
							critter_unit->m_cpg_params = m_cpg_system.defaultParams();
						resetLearningState( critter_unit );
						m_stats_births_total->set( m_stats_births_total->get_uint() + 1 );

					// BODY
						// auto newBody = body_unit_system->addChild( "body", new BBody() );
						auto newBody = m_body_system_unit_container->addChild( "body", new BEntity() );
						auto body_root = newBody->addChild( "body", "CdBodyPlan" );
						
						// auto fixed_1 = body_unit_system->addChild( "body_fixed1", "BodyFixed1" );
						
						// auto fixed_1 = newBody->addChild( "body_fixed1", new BEntity() );

						// CdBodyPlanBuilder m;
						// m.make( fixed_1 );
						
							
						// REFERENCE TO EXTERNAL CHILD
							critter_unit->addChild( "external_body", new BEntity_external() )->set( newBody );
							refreshBodyShortcuts( critter_unit );

					// BRAIN (skipped when CPG drives locomotion)
					if ( !m_cpg_system.enabled() )
					{
						critter_unit->m_brain = m_brain_system->getChild( "unit_container", 1)->addChild( "brain", "Brain" );

						// OUTPUTS
						auto outputs = critter_unit->m_brain->getChild( "outputs", 1 );
						auto constraints = critter_unit->m_constraints_shortcut;
						auto constraints_ref = outputs->addChild( "bullet_constraints", new BEntity_reference() );
						constraints_ref->set( constraints );

						// motor neurons
						auto motor_neurons = critter_unit->addChild( "motor_neurons", new BEntity() );
						motor_neurons->addChild( "eat", new BEntity_float() );
						motor_neurons->addChild( "procreate", new BEntity_float() );
						auto motor_neurons_ref = outputs->addChild( "motor_neurons_ref", new BEntity_reference() );
						motor_neurons_ref->set( motor_neurons );

						// INPUTS
						auto inputs = critter_unit->m_brain->getChild( "inputs", 1 );

						// constraint angles as sensory input
						for_all_children_of3( constraints )
						{
							auto constraint_angle_input = inputs->addChild( "constraint_angle", new BEntity_float() );
							auto angle = (*child3)->get_reference()->getChild("angle", 1);
							if ( angle )
							{
								if ( constraint_angle_input )
								{
									angle->connectServerServer( constraint_angle_input );
								}
								else
								{
									std::cout << "error: constraint_angle not found" << std::endl;
								}
							}
							else
							{
								std::cout << "error: angle not found" << std::endl;
							}
						}

						// VISION
						const unsigned int retinasize = find_vision_retina_size_or_die(this);
						do_times( retinasize*retinasize )
						{
							inputs->addChild( "vision_value_R", new BEntity_float() );
							inputs->addChild( "vision_value_G", new BEntity_float() );
							inputs->addChild( "vision_value_B", new BEntity_float() );
							inputs->addChild( "vision_value_A", new BEntity_float() );
						}

						critter_unit->m_brain = m_brain_system->getChildCustom( critter_unit->m_brain, "new" );
						critter_unit->addChild( "external_brain", new BEntity_external() )->set( critter_unit->m_brain );
						critter_unit->m_brain_inputs = critter_unit->m_brain->getChild( "inputs", 1 );
					}
					else
					{
						// CPG mode: motor neurons without brain
						auto motor_neurons = critter_unit->addChild( "motor_neurons", new BEntity() );
						motor_neurons->addChild( "eat", new BEntity_float() );
						motor_neurons->addChild( "procreate", new BEntity_float() );
					}

			return true;
		}

			if ( id == std::string("procreate_critter") )
			{
			auto critter_unit = dynamic_cast<CdCritter*>( value->getChild("entity", 1)->get_reference() );
			// std::cout << "ad: " << critter_unit->getChild( "adam_distance", 1 )->get_uint() << " total:" << m_unit_container->numChildren()+1 << "(h: " << t_highest << ")" << ": " << critter_unit->id() << std::endl;
			std::cout << "ad: " << critter_unit->getChild( "adam_distance", 1 )->get_uint() << " total:" << m_unit_container->numChildren()+1 << ": " << critter_unit->id() << std::endl;

			// critter_unit->setEnergy( critter_unit->energy() / 2 );
			// critter_unit->setAge( Buint(0) );
			
			// COPY CRITTER
				auto critter_new = dynamic_cast<CdCritter*>( m_entityCopy.copyEntity( critter_unit ) );
				critter_new->getChild( "age", 1 )->set( Buint(0) );
				refreshBodyShortcuts( critter_new );
				resetLearningState( critter_new );
				// critter_new->getChild( "energy", 1 )->set( Buint(0) );

			// CHANGE POSITION to above parent
			if ( !m_copy_random_position->get_bool() )
			{
				float spawn_offset_x(0.0f);
				float spawn_offset_z(0.0f);
				if ( m_rng )
				{
					m_rng->set( "min", Bint(-250) );
					m_rng->set( "max", Bint(250) );
					spawn_offset_x = 0.01f * m_rng->get_int();
					m_rng->set( "min", Bint(-250) );
					m_rng->set( "max", Bint(250) );
					spawn_offset_z = 0.01f * m_rng->get_int();
					if ( spawn_offset_x > -0.3f && spawn_offset_x < 0.3f &&
					     spawn_offset_z > -0.3f && spawn_offset_z < 0.3f )
					{
						spawn_offset_x = 1.5f;
						spawn_offset_z = -1.5f;
					}
				}

				auto bodyparts_old = critter_unit->m_bodyparts_shortcut;
				if ( bodyparts_old == 0 )
				{
					refreshBodyShortcuts( critter_unit );
					bodyparts_old = critter_unit->m_bodyparts_shortcut;
				}
				if ( critter_new->m_bodyparts_shortcut == 0 )
				{
					refreshBodyShortcuts( critter_new );
				}
				if ( bodyparts_old == 0 || critter_new->m_bodyparts_shortcut == 0 )
				{
					std::cerr << "ERROR: procreate_critter: missing bodyparts shortcut in source or copy" << std::endl;
					std::exit(1);
				}
				
				const auto& children_old = bodyparts_old->children();
				const auto& children_new = critter_new->m_bodyparts_shortcut->children();
				if ( children_old.size() != children_new.size() )
				{
					std::cerr << "ERROR: procreate_critter: bodyparts size mismatch source=" << children_old.size()
					          << " copy=" << children_new.size() << std::endl;
					std::exit(1);
				}
				auto old_child = children_old.begin();

				for_all_children_of3( critter_new->m_bodyparts_shortcut )
				{
					if ( old_child == children_old.end() )
					{
						std::cerr << "ERROR: procreate_critter: source bodyparts iterator exhausted early" << std::endl;
						std::exit(1);
					}
					auto t = (*child3)->get_reference()->getChild( "transform", 1 );
					auto oldt = (*old_child)->get_reference()->getChild( "transform", 1 );
					if ( t )
					{
						// std::cout << "changing position" << std::endl;
						t->set("position_x", oldt->get_float("position_x") + spawn_offset_x);
						t->set("position_y", oldt->get_float("position_y") + 0.75f);
						t->set("position_z", oldt->get_float("position_z") + spawn_offset_z);
					}
					old_child++;
				}
			}

				if ( m_cpg_system.enabled() )
				{
					// CPG mode: inherit and mutate CPG params
					critter_new->m_cpg_params = critter_unit->m_cpg_params;
					m_cpg_system.mutate( critter_new->m_cpg_params, m_rng );
					auto ad = critter_new->getChild( "adam_distance", 1 );
					ad->set( ad->get_uint() + 1 );
				}
				else
				{
					// Brain mode: mutate brain
					BEntity* brain_new = 0;
					for_all_children_of3( critter_new )
					{
						if ( (*child3)->name() == "external_brain" )
						{
							if ( (*child3)->get_reference()->name() == "brain" )
							{
								brain_new = (*child3)->get_reference();
							}
						}
					}
					if ( m_brain_system->set( "mutate", brain_new ) )
					{
						auto ad = critter_new->getChild( "adam_distance", 1 );
						ad->set( ad->get_uint() + 1 );
					}
				}
				m_stats_births_total->set( m_stats_births_total->get_uint() + 1 );

			return true;
		}

		// if ( id == std::string("migrate_critter") )
		// {
		// 	return true;
		// }
		return false;
	}

	void CdCritterSystem::ensureLearningShortcuts(CdCritter* critter)
	{
		if ( critter->m_learning_episode_tick_entity != 0 )
		{
			return;
		}

		auto learning = critter->getChild( "learning", 1 );
		critter->m_learning_episode_tick_entity = learning->getChild( "episode_tick", 1 );
		critter->m_learning_episode_reward_entity = learning->getChild( "episode_reward", 1 );
		critter->m_learning_best_episode_reward_entity = learning->getChild( "best_episode_reward", 1 );
		critter->m_learning_last_reward_entity = learning->getChild( "last_reward", 1 );
		critter->m_learning_last_green_entity = learning->getChild( "last_green", 1 );
	}

	void CdCritterSystem::resetLearningState(CdCritter* critter)
	{
		ensureLearningShortcuts( critter );
		critter->m_learning_initialized = false;
		critter->m_learning_episode_tick = 0;
		critter->m_learning_episode_reward = 0.0f;
		critter->m_learning_best_episode_reward = -std::numeric_limits<float>::max();
		critter->m_learning_previous_energy = critter->energy();
		critter->m_learning_previous_green = 0.0f;
		critter->m_learning_episode_tick_entity->set( Buint(0) );
		critter->m_learning_episode_reward_entity->set( Bfloat(0.0f) );
		critter->m_learning_best_episode_reward_entity->set( Bfloat(0.0f) );
		critter->m_learning_last_reward_entity->set( Bfloat(0.0f) );
		critter->m_learning_last_green_entity->set( Bfloat(0.0f) );
	}

		float CdCritterSystem::readVisionGreenSum(CdCritter* critter)
	{
		if ( critter->m_brain_inputs == 0 )
		{
			critter->m_brain_inputs = critter->getChild( "external_brain", 1 )->get_reference()->getChild( "inputs", 1 );
		}
		if ( critter->m_brain_vision_input_start == 0 )
		{
			const auto& inputs = critter->m_brain_inputs->children();
			for ( unsigned int i = 0; i < inputs.size(); ++i )
			{
				if ( inputs[i]->name() == "vision_value_R" )
				{
					critter->m_brain_vision_input_start = inputs[i];
					critter->m_brain_vision_input_start_index = i;
					break;
				}
			}
		}
		if ( critter->m_brain_vision_input_start == 0 )
		{
			return 0.0f;
		}

		const auto& inputs = critter->m_brain_inputs->children();
		const auto start = critter->m_brain_vision_input_start_index;
		const auto green_start = start + 1;
		const auto retina_size = find_vision_retina_size_or_die(this);
		const auto pixels = retina_size * retina_size;
		float green_sum(0.0f);
		for ( unsigned int i = 0; i < pixels; ++i )
		{
			const auto index = green_start + (i * 4);
			if ( index >= inputs.size() )
			{
				break;
			}
			green_sum += inputs[index]->get_float();
		}
		return green_sum;
	}

	bool CdCritterSystem::mutateBrainSlightly(CdCritter* critter)
	{
		// TODO: Temporary duplicate mutation path for stable lifetime learning; unify with BrainSystem mutation profiles.
		if ( m_rng == 0 || critter->m_brain == 0 )
		{
			return false;
		}

		auto neurons = critter->m_brain->getChild( "neurons", 1 );
		if ( !neurons || neurons->numChildren() == 0 )
		{
			return false;
		}

		m_rng->set( "min", 0 );
		m_rng->set( "max", Bint(neurons->numChildren()) - 1 );
		auto neuron = neurons->children()[ m_rng->get_int() ];
		if ( !neuron )
		{
			return false;
		}

		m_rng->set( "min", -50 );
		m_rng->set( "max", 50 );
		const auto delta = 0.001f * m_rng->get_int();

		m_rng->set( "min", 0 );
		m_rng->set( "max", 2 );
		const auto mode = m_rng->get_int();

		if ( mode == 0 )
		{
			auto target = neuron->getChild( "firingWeight", 1 );
			if ( !target )
			{
				return false;
			}
			target->set( target->get_float() + delta );
			return true;
		}

		if ( mode == 1 )
		{
			auto target = neuron->getChild( "firingThreshold", 1 );
			if ( !target )
			{
				return false;
			}
			target->set( target->get_float() + delta );
			return true;
		}

		auto synapses = neuron->getChild( "synapses", 1 );
		if ( !synapses || synapses->numChildren() == 0 )
		{
			auto target = neuron->getChild( "firingWeight", 1 );
			if ( !target )
			{
				return false;
			}
			target->set( target->get_float() + delta );
			return true;
		}

		m_rng->set( "min", 0 );
		m_rng->set( "max", Bint(synapses->numChildren()) - 1 );
		auto synapse = synapses->children()[ m_rng->get_int() ];
		if ( !synapse )
		{
			return false;
		}
		auto weight = synapse->getChild( "weight", 1 );
		if ( !weight )
		{
			return false;
		}

		weight->set( weight->get_float() + delta );
		return true;
	}

	void CdCritterSystem::updateLifetimeLearning()
	{
		if ( !m_learning_enabled->get_bool() )
		{
			return;
		}

		const auto episode_ticks = std::max( 1u, m_learning_episode_ticks->get_uint() );
		const auto reward_energy_weight = m_learning_reward_energy_weight->get_float();
		const auto reward_green_weight = m_learning_reward_green_weight->get_float();
		const auto reward_tick_cost = m_learning_reward_tick_cost->get_float();
		const auto explore_mutation_chance = m_learning_explore_mutation_chance->get_uint();
		float completed_episode_reward_sum(0.0f);
		unsigned int completed_episodes(0);

		for_all_children_of( m_unit_container )
		{
			auto critter = dynamic_cast<CdCritter*>( *child );
			if ( !critter )
			{
				continue;
			}

			ensureLearningShortcuts( critter );
			const auto current_energy = critter->energy();
			const auto current_green = readVisionGreenSum( critter );
			if ( !critter->m_learning_initialized )
			{
				critter->m_learning_previous_energy = current_energy;
				critter->m_learning_previous_green = current_green;
				critter->m_learning_initialized = true;
			}

			const auto delta_energy = current_energy - critter->m_learning_previous_energy;
			const auto delta_green = current_green - critter->m_learning_previous_green;
			float effective_delta_green(delta_green);
			if ( delta_energy > 0.0f && effective_delta_green < 0.0f )
			{
				effective_delta_green = 0.0f;
			}
			const auto reward = (reward_energy_weight * delta_energy) + (reward_green_weight * effective_delta_green) - reward_tick_cost;
			critter->m_learning_previous_energy = current_energy;
			critter->m_learning_previous_green = current_green;
			critter->m_learning_episode_reward += reward;
			++critter->m_learning_episode_tick;
			critter->m_learning_last_reward_entity->set( Bfloat(reward) );
			critter->m_learning_last_green_entity->set( Bfloat(current_green) );
			critter->m_learning_episode_reward_entity->set( Bfloat(critter->m_learning_episode_reward) );
			critter->m_learning_episode_tick_entity->set( Buint(critter->m_learning_episode_tick) );

			if ( critter->m_learning_episode_tick < episode_ticks )
			{
				continue;
			}

			const auto episode_reward = critter->m_learning_episode_reward;
			completed_episode_reward_sum += episode_reward;
			++completed_episodes;
			if ( critter->m_brain == 0 )
			{
				auto external_brain = critter->getChild( "external_brain", 1 );
				if ( external_brain )
				{
					critter->m_brain = external_brain->get_reference();
				}
			}
			bool should_mutate(false);
			if ( episode_reward >= critter->m_learning_best_episode_reward )
			{
				critter->m_learning_best_episode_reward = episode_reward;
			}
			else
			{
				should_mutate = true;
			}

			if ( !should_mutate && m_rng && explore_mutation_chance > 0 )
			{
				m_rng->set( "min", 1 );
				m_rng->set( "max", 100 );
				should_mutate = ((Buint)m_rng->get_int() <= explore_mutation_chance);
			}

			if ( should_mutate && mutateBrainSlightly( critter ) )
			{
				m_stats_learning_mutations_total->set( m_stats_learning_mutations_total->get_uint() + 1 );
			}

			critter->m_learning_episode_reward = 0.0f;
			critter->m_learning_episode_tick = 0;
			critter->m_learning_episode_reward_entity->set( Bfloat(0.0f) );
			critter->m_learning_episode_tick_entity->set( Buint(0) );
			critter->m_learning_best_episode_reward_entity->set( Bfloat(critter->m_learning_best_episode_reward) );
		}

		if ( completed_episodes > 0 )
		{
			const auto avg = completed_episode_reward_sum / completed_episodes;
			const auto prev = m_stats_learning_avg_episode_reward->get_float();
			m_stats_learning_avg_episode_reward->set( Bfloat((prev * 0.9f) + (avg * 0.1f)) );
		}
	}

		void CdCritterSystem::removeCritter( BEntity* entity, bool force_direct_deletion )
		{
			m_stats_deaths_total->set( m_stats_deaths_total->get_uint() + 1 );

				auto critter = dynamic_cast<CdCritter*>( entity );
				if ( critter )
			{
				// shortcut 
					if ( critter->m_bodyparts_shortcut == 0 )
					{
						refreshBodyShortcuts( critter );
					}

				// COLLISIONS
					while ( removeFromCollisions( critter->m_bodyparts_shortcut ) ) {;}
					
				// MOUSEPICKER, loop all bodyparts
					if ( m_mouse_picker )
					{
						for_all_children_of3( critter->m_bodyparts_shortcut )
						{
							m_mouse_picker->removeGrabbedEntity( (*child3)->get_reference() );
						}
					}
				
			}

		// // SPECIES
		// 	m_species_system->removeFromSpecies( entity );

		// ACTUAL REMOVAL
			if ( force_direct_deletion )
			{
				m_unit_container->removeChild( entity );
			}
			else
			{
				auto cmd_rm = m_command_buffer->addChild( "remove", new BEntity_reference() );
				cmd_rm->set( entity );
			}
	}

	bool CdCritterSystem::removeFromCollisions( BEntity* to_remove_list )
	{
		if ( m_collisions == 0 )
		{
			m_collisions = parent()->getChild("physicsworld", 1)->getChild("collisions", 1);
		}

		for_all_children_of( m_collisions )
		{
			auto e1 = (*child)->getChild( "entity1", 1 )->get_reference();
			auto e2 = (*child)->getChild( "entity2", 1 )->get_reference();
			
			// check if e1 is a bodypart belonging to to_remove
			for_all_children_of2( to_remove_list )
			{
				if ( e1 == (*child2)->get_reference() || e2 == (*child2)->get_reference() )
				{
					// std::cout << "removing critter" << std::endl;
					m_collisions->removeChild( *child );
					// std::cout << "removing critter done" << std::endl;
					return true;
				}
			}
		}
		return false;
	}

	void CdCritter::construct()
	{
		m_age = addChild( "age", new BEntity_uint() );
		m_energy = addChild( "energy", new BEntity_float() );
		addChild( "adam_distance", new BEntity_uint() )->set( Buint(0) );
		
		m_brain_inputs = 0;
		m_brain_vision_input_start = 0;
		m_brain_vision_input_start_index = 0;
		m_learning_initialized = false;
		m_learning_episode_tick = 0;
		m_learning_episode_reward = 0.0f;
		m_learning_best_episode_reward = -std::numeric_limits<float>::max();
		m_learning_previous_energy = 0.0f;
		m_learning_previous_green = 0.0f;
		auto learning = addChild( "learning", new BEntity() );
		m_learning_episode_tick_entity = learning->addChild( "episode_tick", new BEntity_uint() );
		m_learning_episode_reward_entity = learning->addChild( "episode_reward", new BEntity_float() );
		m_learning_best_episode_reward_entity = learning->addChild( "best_episode_reward", new BEntity_float() );
		m_learning_last_reward_entity = learning->addChild( "last_reward", new BEntity_float() );
		m_learning_last_green_entity = learning->addChild( "last_green", new BEntity_float() );
		m_learning_episode_tick_entity->set( Buint(0) );
		m_learning_episode_reward_entity->set( Bfloat(0.0f) );
		m_learning_best_episode_reward_entity->set( Bfloat(0.0f) );
		m_learning_last_reward_entity->set( Bfloat(0.0f) );
		m_learning_last_green_entity->set( Bfloat(0.0f) );
		m_transform_shortcut = 0;
		m_body_root_shortcut = 0;
		m_constraints_shortcut = 0;
		m_physics_component_shortcut = 0;
		m_bodyparts_shortcut = 0;
	}
	
	
	
	

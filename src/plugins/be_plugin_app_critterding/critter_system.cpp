#include "critter_system.h"
#include "food_system.h"
#include "body_plan_config.h"
#include "kernel/be_entity_core_types.h"
#include "body_runtime_access.h"
// #include "species_system.h"
#include "plugins/be_plugin_bullet/be_entity_mousepicker.h"
#include <cmath>
#include <limits>
#include <iostream>

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

		// EGG CONTAINER
			m_egg_container = addChild( "egg_container", new BEntity() );
		
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
		
		m_minimum_number_of_units->set( Buint(4) );
		m_intitial_energy->set( Bfloat(1500.0f) );
		m_procreate_minimum_energy->set( Bfloat(5001.0f) );
		m_maximum_age->set( Buint(0) );
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
				m_eat_active_cost->set( Bfloat(0.35f) );
				m_egg_incubation_ticks = settings->addChild( "egg_incubation_ticks", new BEntity_uint() );
				m_egg_incubation_ticks->set( Buint(500) );
		
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
		m_food_unit_container = 0;
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
		
		// COLLECT FOOD POSITIONS FOR SCENT FIELD
			if ( m_food_unit_container == 0 )
			{
				auto food_system = parent()->getChild("food_system", 1);
				if ( food_system )
					m_food_unit_container = food_system->getChild("unit_container", 1);
			}
			m_food_positions.clear();
			if ( m_food_unit_container )
			{
				const auto& fchildren = m_food_unit_container->children();
				for ( auto it = fchildren.begin(); it != fchildren.end(); ++it )
				{
					auto phys_ext = (*it)->getChild("external_physics", 1);
					if ( phys_ext && phys_ext->get_reference() )
					{
						auto t = phys_ext->get_reference()->getChild("transform", 1);
						if ( t )
							m_food_positions.push_back({ t->get_float("position_x"), t->get_float("position_z") });
					}
				}
			}

		// AGE ALL UNITS WITH A DAY, COUNT UP ALL ENERGY FROM UNITS (FIXME ACCOUNT FOR CRITTERS, MAKE GLOBAL VARIABLE FOR TOTAL ENERGY)
			float total_energy_in_entities(0.0f);
			for_all_children_of( m_unit_container )
			{
				auto critter_unit = dynamic_cast<CdCritter*>( *child );
				if ( critter_unit )
				{
					critter_unit->setAge( 1+critter_unit->age() );

					// eat: auto-eat in CPG mode, penalize active eat otherwise
					auto eat = critter_unit->getChild("motor_neurons", 1)->getChild("eat", 1);
					if ( eat )
					{
						if ( m_cpg_system.enabled() )
							eat->set(1.0f);
						float eat_value = eat->get_float();
						if ( eat_value > 0.0f )
						{
							if ( eat_value > 1.0f ) eat_value = 1.0f;
							critter_unit->setEnergy( critter_unit->energy() - m_eat_active_cost->get_float() * eat_value );
						}
					}

					total_energy_in_entities += critter_unit->energy();

					// scent field at critter position
					if ( critter_unit->m_transform_shortcut && !m_food_positions.empty() )
					{
						const float cx = critter_unit->m_transform_shortcut->get_float("position_x");
						const float cz = critter_unit->m_transform_shortcut->get_float("position_z");
						auto scent = compute_scent(cx, cz, m_food_positions, 1.0f);
						critter_unit->m_scent_field = scent.field;
						critter_unit->m_scent_grad_x = scent.grad_x;
						critter_unit->m_scent_grad_z = scent.grad_z;
					}

					// modulation network: scent gradient rotated into body-local frame -> CPG params
					if ( m_cpg_system.enabled() )
					{
						float in_left_right = 0.0f;
						float in_fwd_back   = 0.0f;
						if ( critter_unit->m_transform_shortcut )
						{
							const float gx = critter_unit->m_scent_grad_x;
							const float gz = critter_unit->m_scent_grad_z;
							const float gmag = std::sqrt(gx * gx + gz * gz);
							if ( gmag > 1e-6f )
							{
								const float yaw = critter_unit->m_transform_shortcut->get_float("rotation_euler_y");
								const float c = std::cos(yaw);
								const float s = std::sin(yaw);
								// world -> body: R(-yaw) around Y
								const float local_x = ( c * gx - s * gz) / gmag; // body right (+) / left (-)
								const float local_z = ( s * gx + c * gz) / gmag; // body forward axis
								in_left_right = local_x;
								in_fwd_back   = local_z;
							}
						}
						mod_net_forward( critter_unit->m_genome.mod_net, in_left_right, in_fwd_back, critter_unit->m_scent_field, critter_unit->m_genome.cpg_params );
					}

					// CPG drives the constraints from the just-computed params
					m_cpg_system.update( critter_unit, critter_unit->m_genome.cpg_phase, critter_unit->m_genome.cpg_params );

					// Hebbian: accumulate per-tick product (δ · I · O) into weight_accum.
					// Apply every ~20 CPG cycles, or when plugin.cpp flags an eat event
					// (it already deposited the eat reward into accum with the pre-eat
					// I/O snapshot). On eat, skip this tick's delta — it's the post-eat
					// cliff and would otherwise dominate one accumulator entry.
					if ( m_cpg_system.enabled() )
					{
						auto& net = critter_unit->m_genome.mod_net;
						if ( net.pending_eat_apply )
						{
							mod_net_hebbian_apply( net );
							net.pending_eat_apply = false;
							net.ticks_since_apply = 0;
							net.prev_field = critter_unit->m_scent_field;
							net.prev_field_valid = true;
						}
						else
						{
							if ( net.prev_field_valid )
								mod_net_hebbian_accumulate( net, critter_unit->m_scent_field - net.prev_field );
							net.prev_field = critter_unit->m_scent_field;
							net.prev_field_valid = true;
							net.ticks_since_apply++;

							constexpr float TWO_PI = 6.28318530717958647692f;
							const float freq = critter_unit->m_genome.cpg_params.frequency > 0.001f
							                       ? critter_unit->m_genome.cpg_params.frequency
							                       : 0.001f;
							const unsigned int interval = (unsigned int)(20.0f * TWO_PI / freq);
							if ( net.ticks_since_apply >= interval )
							{
								mod_net_hebbian_apply( net );
								net.ticks_since_apply = 0;
							}
						}
					}
				}
			}
			m_stats_energy_total->set( total_energy_in_entities );
			if ( !m_cpg_system.enabled() )
				updateLifetimeLearning();

		// HATCH EGGS
			for_all_children_of2( m_egg_container )
			{
				auto egg = static_cast<CdEgg*>( *child2 );
				if ( egg->m_ticks_remaining == 0 )
				{
					auto cmd_hatch = m_command_buffer->addChild( "pass_command", new BEntity_reference() );
					cmd_hatch->set(this);
					auto command = cmd_hatch->addChild( "command", new BEntity_string() );
					command->set("hatch_egg");
					command->addChild( "entity", new BEntity_reference() )->set( egg );
					return;
				}
				egg->m_ticks_remaining--;
			}

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
					// reached max age (0 = no limit) or energy is depleted
					if ( (m_maximum_age->get_uint() > 0 && critter_unit->age() >= m_maximum_age->get_uint()) || critter_unit->energy() <= 0.0f )
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
			// static Buint t_highest(0);
			
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

	// Shared spawn path for fresh critters (inherited == nullptr, uses CPG
	// defaults) and hatchlings (inherited points to the egg's genome).
	// Handles genome, body construction, body-plan override (CPG mode) and
	// brain/motor wiring. Caller is responsible for anything positional
	// (relocation, adam_distance bookkeeping) after the spawn.
	CdCritter* CdCritterSystem::spawnCritter(float initial_energy, const CritterGenome* inherited)
	{
		auto critter_unit = new CdCritter();
		m_unit_container->addChild( "critter_unit", critter_unit );
		critter_unit->setEnergy( initial_energy );

		if ( m_cpg_system.enabled() )
		{
			if ( inherited )
			{
				critter_unit->m_genome = *inherited;
			}
			else
			{
				critter_unit->m_genome.cpg_params = m_cpg_system.defaultParams();
				critter_unit->m_genome.body_params = m_cpg_system.defaultBodyParams();
				mod_net_init_from_defaults( critter_unit->m_genome.mod_net, m_cpg_system.defaultParams() );
			}
			// hatchling always starts its oscillator fresh
			critter_unit->m_genome.cpg_phase = 0.0f;
		}
		resetLearningState( critter_unit );
		m_stats_births_total->set( m_stats_births_total->get_uint() + 1 );

		// BODY — apply override iff we're in CPG mode with evolved params
		BodyPlanConfig child_body_cfg;
		if ( m_cpg_system.enabled() && inherited )
		{
			m_cpg_system.expandBodyParams( critter_unit->m_genome.body_params, child_body_cfg );
			cd_body_plan_set_override( &child_body_cfg );
		}
		auto newBody = m_body_system_unit_container->addChild( "body", new BEntity() );
		newBody->addChild( "body", "CdBodyPlan" );
		if ( m_cpg_system.enabled() && inherited )
			cd_body_plan_set_override( nullptr );

		critter_unit->addChild( "external_body", new BEntity_external() )->set( newBody );
		refreshBodyShortcuts( critter_unit );

		// BRAIN (skipped when CPG drives locomotion) or CPG motor neurons
		if ( !m_cpg_system.enabled() )
		{
			critter_unit->m_brain = m_brain_system->getChild( "unit_container", 1)->addChild( "brain", "Brain" );

			auto outputs = critter_unit->m_brain->getChild( "outputs", 1 );
			auto constraints = critter_unit->m_constraints_shortcut;
			auto constraints_ref = outputs->addChild( "bullet_constraints", new BEntity_reference() );
			constraints_ref->set( constraints );

			auto motor_neurons = critter_unit->addChild( "motor_neurons", new BEntity() );
			motor_neurons->addChild( "eat", new BEntity_float() );
			motor_neurons->addChild( "procreate", new BEntity_float() );
			auto motor_neurons_ref = outputs->addChild( "motor_neurons_ref", new BEntity_reference() );
			motor_neurons_ref->set( motor_neurons );

			auto inputs = critter_unit->m_brain->getChild( "inputs", 1 );
			for_all_children_of3( constraints )
			{
				auto constraint_angle_input = inputs->addChild( "constraint_angle", new BEntity_float() );
				auto angle = (*child3)->get_reference()->getChild("angle", 1);
				if ( angle && constraint_angle_input )
					angle->connectServerServer( constraint_angle_input );
			}

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
			auto motor_neurons = critter_unit->addChild( "motor_neurons", new BEntity() );
			motor_neurons->addChild( "eat", new BEntity_float() );
			motor_neurons->addChild( "procreate", new BEntity_float() );
		}

		return critter_unit;
	}

	// bool CdCritterSystem::set( const char* value )
	bool CdCritterSystem::set( const Bstring& id, BEntity* value )
	{
			if ( id == std::string("insert_critter") )
			{
				spawnCritter( m_intitial_energy->get_float(), nullptr );
				return true;
			}

			if ( id == std::string("procreate_critter") )
			{
			auto critter_unit = dynamic_cast<CdCritter*>( value->getChild("entity", 1)->get_reference() );

			// get parent position and heading
			if ( critter_unit->m_transform_shortcut == 0 )
				refreshBodyShortcuts( critter_unit );
			if ( critter_unit->m_transform_shortcut == 0 )
			{
				std::cerr << "ERROR: procreate_critter: no transform on parent" << std::endl;
				return false;
			}
			auto parent_t = critter_unit->m_transform_shortcut;
			float parent_heading = parent_t->get_float("rotation_euler_y");

			// place egg behind parent
			// forward = (sin(heading), 0, -cos(heading)), behind = negated
			float behind_offset = 1.5f;
			auto egg = new CdEgg();
			egg->m_pos_x = parent_t->get_float("position_x") - std::sin(parent_heading) * behind_offset;
			egg->m_pos_y = -19.8f; // above ground, will fall and settle
			egg->m_pos_z = parent_t->get_float("position_z") + std::cos(parent_heading) * behind_offset;
			egg->m_ticks_remaining = m_egg_incubation_ticks->get_uint();
			egg->m_energy = critter_unit->energy(); // parent already halved its energy

			if ( m_cpg_system.enabled() )
			{
				// inherit entire genome, then reset transient learning state and mutate
				egg->m_genome = critter_unit->m_genome;
				egg->m_genome.mod_net.prev_field = 0.0f;
				egg->m_genome.mod_net.prev_field_valid = false;
				egg->m_genome.mod_net.ticks_since_apply = 0;
				egg->m_genome.mod_net.pending_eat_apply = false;
				for ( int j = 0; j < ModulationNetwork::N_OUTPUTS; ++j )
					for ( int i = 0; i < ModulationNetwork::N_INPUTS; ++i )
						egg->m_genome.mod_net.weight_accum[j][i] = 0.0f;
				mod_net_mutate( egg->m_genome.mod_net, m_rng );
				m_cpg_system.mutateBody( egg->m_genome.body_params, m_rng );
			}

			m_egg_container->addChild( "egg", egg );
			return true;
		}

			if ( id == std::string("hatch_egg") )
			{
			auto egg = dynamic_cast<CdEgg*>( value->getChild("entity", 1)->get_reference() );
			if ( !egg )
			{
				std::cerr << "ERROR: hatch_egg: invalid egg reference" << std::endl;
				return false;
			}

			// create critter from egg genetics
			auto critter_unit = spawnCritter( egg->m_energy, &egg->m_genome );

			// relocate critter from default spawn to egg's current physics position
			if ( critter_unit->m_transform_shortcut && critter_unit->m_bodyparts_shortcut )
			{
				// read egg's actual physics position (after falling/settling)
				float egg_x = egg->m_pos_x;
				float egg_y = egg->m_pos_y;
				float egg_z = egg->m_pos_z;
				auto egg_phys_ext = egg->getChild("external_physics", 1);
				if ( egg_phys_ext && egg_phys_ext->get_reference() )
				{
					auto egg_t = egg_phys_ext->get_reference()->getChild("transform", 1);
					if ( egg_t )
					{
						egg_x = egg_t->get_float("position_x");
						egg_y = egg_t->get_float("position_y");
						egg_z = egg_t->get_float("position_z");
					}
				}

				// measure how far the body extends below center
				float cx = critter_unit->m_transform_shortcut->get_float("position_x");
				float cy = critter_unit->m_transform_shortcut->get_float("position_y");
				float cz = critter_unit->m_transform_shortcut->get_float("position_z");
				float lowest_bottom = cy;
				{
					const auto& parts = critter_unit->m_bodyparts_shortcut->children();
					for ( auto it = parts.begin(); it != parts.end(); ++it )
					{
						auto part = (*it)->get_reference();
						auto t = part->getChild( "transform", 1 );
						auto sy = part->getChild( "scale_y", 1 );
						if ( t && sy )
						{
							float bottom = t->get_float("position_y") - sy->get_float();
							if ( bottom < lowest_bottom )
								lowest_bottom = bottom;
						}
					}
				}
				float body_hang = cy - lowest_bottom;

				float dx = egg_x - cx;
				float dy = egg_y - cy + body_hang + 0.3f; // lift so lowest part clears egg
				float dz = egg_z - cz;

				for_all_children_of3( critter_unit->m_bodyparts_shortcut )
				{
					auto t = (*child3)->get_reference()->getChild( "transform", 1 );
					if ( t )
					{
						t->set("position_x", t->get_float("position_x") + dx);
						t->set("position_y", t->get_float("position_y") + dy);
						t->set("position_z", t->get_float("position_z") + dz);
					}
				}
			}

			// set adam_distance from parent
			if ( m_cpg_system.enabled() )
			{
				auto ad = critter_unit->getChild( "adam_distance", 1 );
				ad->set( ad->get_uint() + 1 );
			}

			// remove egg (physics + graphics + entity)
			auto egg_physics = egg->getChild("external_physics", 1);
			if ( egg_physics )
			{
				auto physicsworld = egg_physics->get_reference()->parent();
				physicsworld->removeChild( egg_physics->get_reference() );
			}
			auto egg_graphics = egg->getChild("external_graphics", 1);
			if ( egg_graphics )
			{
				auto graphics_parent = egg_graphics->get_reference()->parent();
				graphics_parent->removeChild( egg_graphics->get_reference() );
			}
			m_egg_container->removeChild( egg );

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

	void CdEgg::construct()
	{
		// small static physics cube
		auto physicsworld = parent()->parent()->parent()->getChild("physicsworld", 1);
		if ( !physicsworld )
		{
			std::cerr << "ERROR: CdEgg: no physicsworld found" << std::endl;
			std::exit(1);
		}

		auto egg_physics = physicsworld->addChild( "egg", "PhysicsEntity_Cube" );
		egg_physics->addChild( "weight", new BEntity_float_property() )->set( 0.5f );
		egg_physics->addChild( "scale_x", new BEntity_float_property() )->set( 0.6f );
		egg_physics->addChild( "scale_y", new BEntity_float_property() )->set( 0.6f );
		egg_physics->addChild( "scale_z", new BEntity_float_property() )->set( 0.6f );

		auto physics_transform = egg_physics->getChild( "transform", 1 );
		physics_transform->getChild("position_x", 1)->set( m_pos_x );
		physics_transform->getChild("position_y", 1)->set( m_pos_y );
		physics_transform->getChild("position_z", 1)->set( m_pos_z );

		addChild( "external_physics", new BEntity_external() )->set( egg_physics );

		// graphics
		auto graphicsmodelsystem = topParent()->getChild("bin", 1)->getChild("Critterding", 1)->getChild("GLWindow", 1)->getChild("GraphicsModelSystem", 1);
		if ( graphicsmodelsystem )
		{
			auto graphics_entity_egg = graphicsmodelsystem->getChild( "graphics_entity_egg", 1 );
			BEntity* graphics_transform_egg(0);
			if ( !graphics_entity_egg )
			{
				graphics_entity_egg = graphicsmodelsystem->addChild("graphics_entity_egg", "GraphicsModel");
				graphics_entity_egg->getChild( "pre_scale_x", 1 )->set( 0.6f );
				graphics_entity_egg->getChild( "pre_scale_y", 1 )->set( 0.6f );
				graphics_entity_egg->getChild( "pre_scale_z", 1 )->set( 0.6f );
				graphics_entity_egg->set("filename", "../share/modules/cube-egg.obj");
				graphics_transform_egg = graphics_entity_egg->addChild("transform", "Transform");
			}
			else
			{
				graphics_transform_egg = graphics_entity_egg->addChild("transform", "Transform");
			}
			addChild( "external_graphics", new BEntity_external() )->set( graphics_transform_egg );
			physics_transform->connectServerServer(graphics_transform_egg, true);
		}
	}


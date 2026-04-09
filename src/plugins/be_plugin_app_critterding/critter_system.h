#pragma once

#include "kernel/be_entity_interface.h"
#include "kernel/be_entity_ops_copy.h"
#include "cpg_system.h"
#include "scent_field.h"
#include <limits>
#include <vector>

	class CdSpeciesSystem;
	class BeRigidBody;
	class BMousePicker;
	class CdCritter;

	//  SYSTEM
		class CdCritterSystem : public BEntity
		{
			public:
				CdCritterSystem() : m_framecount(1) { setProcessing(); };
				const char* class_id() const { return "CdCritterSystem"; }
				virtual ~CdCritterSystem() {};

				void construct();
				void process();
				void removeCritter( BEntity* entity, bool force_direct_deletion=false );
				// bool set( const char* value );
				virtual bool set( const Bstring& id, BEntity* value );

				private:
					void refreshBodyShortcuts(CdCritter* critter);
					void updateLifetimeLearning();
					void ensureLearningShortcuts(CdCritter* critter);
					void resetLearningState(CdCritter* critter);
					float readVisionGreenSum(CdCritter* critter);
					bool mutateBrainSlightly(CdCritter* critter);

				BMousePicker* m_mouse_picker;
				BEntity* m_unit_container;
				bool removeFromCollisions( BEntity* to_remove );
				BEntity* m_collisions;
				BEntityCopy m_entityCopy;
				// BEntityLoad m_entityLoad;
				BEntity* m_command_buffer;
				BEntity* m_rng;

				BEntity* m_insert_frame_interval;
				unsigned int m_framecount;

				// SETTINGS
				BEntity* m_minimum_number_of_units;
				BEntity* m_intitial_energy;
				BEntity* m_procreate_minimum_energy;
				BEntity* m_maximum_age;
				BEntity* m_dropzone_position_x;
				BEntity* m_dropzone_position_y;
				BEntity* m_dropzone_position_z;
				BEntity* m_dropzone_size_x;
				BEntity* m_dropzone_size_y;
				BEntity* m_dropzone_size_z;
				BEntity* m_copy_random_position;
				BEntity* m_stats_births_total;
				BEntity* m_stats_deaths_total;
				BEntity* m_stats_energy_total;
				BEntity* m_learning_enabled;
				BEntity* m_learning_episode_ticks;
				BEntity* m_learning_reward_energy_weight;
				BEntity* m_learning_reward_green_weight;
					BEntity* m_learning_reward_tick_cost;
					BEntity* m_learning_explore_mutation_chance;
					BEntity* m_stats_learning_avg_episode_reward;
					BEntity* m_stats_learning_mutations_total;
				BEntity* m_eat_active_cost;

				CdSpeciesSystem* m_species_system;
				BEntity* m_body_system_unit_container;
				BEntity* m_brain_system;
				CpgSystem m_cpg_system;

				// scent field
				BEntity* m_food_unit_container;
				std::vector<FoodPos> m_food_positions; // reused each tick to avoid allocation

				// eggs
				BEntity* m_egg_container;
				BEntity* m_egg_incubation_ticks;
		};

	//  UNIT
		class CdCritter : public BEntity
		{
			public:
					CdCritter()
						: m_brain(0), m_brain_inputs(0), m_brain_vision_input_start(0), m_brain_vision_input_start_index(0),
						  m_learning_initialized(false), m_learning_episode_tick(0), m_learning_episode_reward(0.0f),
						  m_learning_best_episode_reward(0.0f), m_learning_previous_energy(0.0f), m_learning_previous_green(0.0f),
						  m_learning_episode_tick_entity(0), m_learning_episode_reward_entity(0),
					  m_learning_best_episode_reward_entity(0), m_learning_last_reward_entity(0), m_learning_last_green_entity(0),
					  m_body_root_shortcut(0), m_constraints_shortcut(0), m_transform_shortcut(0), m_physics_component_shortcut(0), m_bodyparts_shortcut(0),
					  m_cpg_phase(0.0f), m_cpg_params{0.0f, 0.0f,0.0f, 0.0f,0.0f, 0.5f,0.0f, 0.0f, 0.0f}, m_body_params{0,0,0, 0,0,0, 0,0,0},
					  m_scent_field(0.0f), m_scent_grad_x(0.0f), m_scent_grad_z(0.0f),
					  m_age(0), m_energy(0), m_species(0)
				{
				};
				const char* class_id() const { return "CdCritter"; }
				virtual ~CdCritter() {};
				void construct();

				Buint age() { return m_age->get_uint(); }
				Bfloat energy() { return m_energy->get_float(); }
				void setAge( Buint age ) { m_age->set( age ); }
				void setEnergy( Bfloat energy ) { m_energy->set( energy ); }
				void setSpecies( BEntity* species ) { m_species->set( species ); }
				BEntity* m_brain;
					BEntity* m_brain_inputs;
					BEntity* m_brain_vision_input_start;
					unsigned int m_brain_vision_input_start_index;
					bool m_learning_initialized;
				unsigned int m_learning_episode_tick;
				float m_learning_episode_reward;
				float m_learning_best_episode_reward;
				float m_learning_previous_energy;
				float m_learning_previous_green;
				BEntity* m_learning_episode_tick_entity;
				BEntity* m_learning_episode_reward_entity;
				BEntity* m_learning_best_episode_reward_entity;
				BEntity* m_learning_last_reward_entity;
				BEntity* m_learning_last_green_entity;

				// performance shortcuts
				BEntity* m_body_root_shortcut;
				BEntity* m_constraints_shortcut;
				BEntity* m_transform_shortcut;
				BEntity* m_physics_component_shortcut;
				BEntity* m_bodyparts_shortcut;
				float m_cpg_phase;
				CpgEvolvableParams m_cpg_params;
				BodyEvolvableParams m_body_params; // unused for now — infrastructure for body evolution

				// scent field at current position
				float m_scent_field;
				float m_scent_grad_x;
				float m_scent_grad_z;
				// BEntity* m_always_firing_input;
		private:
				// PROPERTIES
				BEntity* m_age;
				BEntity* m_energy;
				BEntity* m_species;
		};

	// EGG
		class CdEgg : public BEntity
		{
			public:
				CdEgg() : m_ticks_remaining(0), m_energy(0.0f) {};
				const char* class_id() const { return "CdEgg"; }
				virtual ~CdEgg() {};
				void construct();

				unsigned int m_ticks_remaining;
				float m_pos_x, m_pos_y, m_pos_z;
				float m_energy;
				CpgEvolvableParams m_cpg_params;
				BodyEvolvableParams m_body_params;
		};

#include "body_system.h"
#include "kernel/be_entity_core_types.h"
#include <cctype>
#include <cstdlib>
#include <fstream>
#include <iterator>
#include <locale>
#include <sstream>
#include <string>
#include <vector>
#include <iostream>

namespace
{
	struct BodyPlanPart
	{
		std::string name;
		float offset_x;
		float offset_y;
		float offset_z;
		float scale_x;
		float scale_y;
		float scale_z;
		bool has_friction_override;
		float friction_override;
	};

	struct BodyPlanHinge
	{
		std::string name;
		unsigned int part_a;
		unsigned int part_b;
		float local_a_x;
		float local_a_y;
		float local_a_z;
		float local_a_pitch;
		float local_b_x;
		float local_b_y;
		float local_b_z;
		float local_b_pitch;
		float limit_low;
		float limit_high;
		float softness;
		float biasfactor;
		float relaxationfactor;
		bool bidirectional;
	};

	struct BodyPlanConfig
	{
		std::string physics_bodypart_class;
		std::string graphics_model_name;
		std::string graphics_mesh_file;
		bool bodypart_use_density;
		float bodypart_density;
		float bodypart_friction;
		float bodypart_restitution;
		bool bodypart_wants_deactivation;
		std::vector<BodyPlanPart> parts;
		std::vector<BodyPlanHinge> hinges;
	};

	BodyPlanConfig g_verified_body_plan;

	void fatal_body_plan_error( const std::string& message );

	bool read_file_text( const std::string& path, std::string& out_text )
	{
		std::ifstream file(path.c_str(), std::ios::in);
		if ( !file.is_open() )
		{
			return false;
		}
		out_text.assign(std::istreambuf_iterator<char>(file), std::istreambuf_iterator<char>());
		return true;
	}

	bool load_body_plan_file_text( const std::string& path, std::string& out_text )
	{
		if ( read_file_text(path, out_text) )
		{
			return true;
		}
		if ( read_file_text(std::string("../") + path, out_text) )
		{
			return true;
		}
		if ( read_file_text(std::string("../../") + path, out_text) )
		{
			return true;
		}
		return false;
	}

	bool json_find_field_value( const std::string& text, const std::string& key, std::string& out_value )
	{
		std::string pattern("\"");
		pattern += key;
		pattern += "\"";
		const auto key_pos = text.find(pattern);
		if ( key_pos == std::string::npos )
		{
			return false;
		}
		const auto colon_pos = text.find(':', key_pos + pattern.size());
		if ( colon_pos == std::string::npos )
		{
			return false;
		}
		auto value_start = colon_pos + 1;
		while ( value_start < text.size() && std::isspace(static_cast<unsigned char>(text[value_start])) )
		{
			++value_start;
		}
		if ( value_start >= text.size() )
		{
			return false;
		}

		auto value_end = value_start;
		if ( text[value_start] == '"' )
		{
			++value_end;
			while ( value_end < text.size() && text[value_end] != '"' )
			{
				++value_end;
			}
			if ( value_end < text.size() )
			{
				++value_end;
			}
		}
		else
		{
			while ( value_end < text.size() )
			{
				const auto c = text[value_end];
				if ( c == ',' || c == '}' || c == '\n' || c == '\r' )
				{
					break;
				}
				++value_end;
			}
		}
		out_value = text.substr(value_start, value_end - value_start);
		return true;
	}

	bool json_get_required_number( const std::string& text, const std::string& key, float& target )
	{
		std::string raw;
		if ( !json_find_field_value(text, key, raw) )
		{
			return false;
		}
		std::istringstream stream(raw);
		stream.imbue(std::locale::classic());
		stream >> target;
		if ( stream.fail() )
		{
			return false;
		}
		stream >> std::ws;
		return stream.eof();
	}

	bool json_get_optional_number( const std::string& text, const std::string& key, float& target )
	{
		std::string raw;
		if ( !json_find_field_value(text, key, raw) )
		{
			return false;
		}
		std::istringstream stream(raw);
		stream.imbue(std::locale::classic());
		stream >> target;
		if ( stream.fail() )
		{
			fatal_body_plan_error(std::string("invalid numeric value for optional key '") + key + "'");
		}
		stream >> std::ws;
		if ( !stream.eof() )
		{
			fatal_body_plan_error(std::string("invalid trailing data for optional key '") + key + "'");
		}
		return true;
	}

	bool json_get_required_uint( const std::string& text, const std::string& key, unsigned int& target )
	{
		float number(0.0f);
		if ( !json_get_required_number(text, key, number) )
		{
			return false;
		}
		if ( number < 0.0f )
		{
			return false;
		}
		target = static_cast<unsigned int>(number);
		return true;
	}

	bool json_get_required_string( const std::string& text, const std::string& key, std::string& target )
	{
		std::string raw;
		if ( !json_find_field_value(text, key, raw) )
		{
			return false;
		}
		if ( raw.size() < 2 || raw.front() != '"' || raw.back() != '"' )
		{
			return false;
		}
		target = raw.substr(1, raw.size() - 2);
		return true;
	}

	bool json_get_required_bool( const std::string& text, const std::string& key, bool& target )
	{
		std::string raw;
		if ( !json_find_field_value(text, key, raw) )
		{
			return false;
		}
		if ( raw.find("true") != std::string::npos )
		{
			target = true;
			return true;
		}
		if ( raw.find("false") != std::string::npos )
		{
			target = false;
			return true;
		}
		return false;
	}

	void fatal_body_plan_error( const std::string& message )
	{
		std::cerr << "ERROR: body plan config: " << message << std::endl;
		std::exit(1);
	}

	bool load_body_plan_config( const std::string& path, BodyPlanConfig& cfg )
	{
		std::string text;
		if ( !load_body_plan_file_text(path, text) )
		{
			return false;
		}

		unsigned int part_count(0);
		unsigned int hinge_count(0);
		if ( !json_get_required_string(text, "physics_bodypart_class", cfg.physics_bodypart_class) ) return false;
		if ( !json_get_required_string(text, "graphics_model_name", cfg.graphics_model_name) ) return false;
		if ( !json_get_required_string(text, "graphics_mesh_file", cfg.graphics_mesh_file) ) return false;
		if ( !json_get_required_bool(text, "bodypart_use_density", cfg.bodypart_use_density) ) return false;
		if ( !json_get_required_number(text, "bodypart_density", cfg.bodypart_density) ) return false;
		if ( !json_get_required_number(text, "bodypart_friction", cfg.bodypart_friction) ) return false;
		if ( !json_get_required_number(text, "bodypart_restitution", cfg.bodypart_restitution) ) return false;
		if ( !json_get_required_bool(text, "bodypart_wants_deactivation", cfg.bodypart_wants_deactivation) ) return false;
		if ( !json_get_required_uint(text, "part_count", part_count) ) return false;
		if ( !json_get_required_uint(text, "hinge_count", hinge_count) ) return false;
		if ( part_count == 0 ) return false;

		cfg.parts.clear();
		cfg.hinges.clear();
		cfg.parts.reserve(part_count);
		cfg.hinges.reserve(hinge_count);

		for ( unsigned int i = 0; i < part_count; ++i )
		{
			BodyPlanPart part;
			const auto id = std::to_string(i);
			if ( !json_get_required_string(text, "part_" + id + "_name", part.name) ) return false;
			if ( !json_get_required_number(text, "part_" + id + "_offset_x", part.offset_x) ) return false;
			if ( !json_get_required_number(text, "part_" + id + "_offset_y", part.offset_y) ) return false;
			if ( !json_get_required_number(text, "part_" + id + "_offset_z", part.offset_z) ) return false;
			if ( !json_get_required_number(text, "part_" + id + "_scale_x", part.scale_x) ) return false;
			if ( !json_get_required_number(text, "part_" + id + "_scale_y", part.scale_y) ) return false;
			if ( !json_get_required_number(text, "part_" + id + "_scale_z", part.scale_z) ) return false;
			part.has_friction_override = json_get_optional_number(text, "part_" + id + "_friction", part.friction_override);
			cfg.parts.push_back(part);
		}

		for ( unsigned int i = 0; i < hinge_count; ++i )
		{
			BodyPlanHinge hinge;
			const auto id = std::to_string(i);
			if ( !json_get_required_string(text, "hinge_" + id + "_name", hinge.name) ) return false;
			if ( !json_get_required_uint(text, "hinge_" + id + "_part_a", hinge.part_a) ) return false;
			if ( !json_get_required_uint(text, "hinge_" + id + "_part_b", hinge.part_b) ) return false;
			if ( !json_get_required_number(text, "hinge_" + id + "_local_a_x", hinge.local_a_x) ) return false;
			if ( !json_get_required_number(text, "hinge_" + id + "_local_a_y", hinge.local_a_y) ) return false;
			if ( !json_get_required_number(text, "hinge_" + id + "_local_a_z", hinge.local_a_z) ) return false;
			if ( !json_get_required_number(text, "hinge_" + id + "_local_a_pitch", hinge.local_a_pitch) ) return false;
			if ( !json_get_required_number(text, "hinge_" + id + "_local_b_x", hinge.local_b_x) ) return false;
			if ( !json_get_required_number(text, "hinge_" + id + "_local_b_y", hinge.local_b_y) ) return false;
			if ( !json_get_required_number(text, "hinge_" + id + "_local_b_z", hinge.local_b_z) ) return false;
			if ( !json_get_required_number(text, "hinge_" + id + "_local_b_pitch", hinge.local_b_pitch) ) return false;
			if ( !json_get_required_number(text, "hinge_" + id + "_limit_low", hinge.limit_low) ) return false;
			if ( !json_get_required_number(text, "hinge_" + id + "_limit_high", hinge.limit_high) ) return false;
			if ( !json_get_required_number(text, "hinge_" + id + "_softness", hinge.softness) ) return false;
			if ( !json_get_required_number(text, "hinge_" + id + "_biasfactor", hinge.biasfactor) ) return false;
			if ( !json_get_required_number(text, "hinge_" + id + "_relaxationfactor", hinge.relaxationfactor) ) return false;
			if ( !json_get_required_bool(text, "hinge_" + id + "_bidirectional", hinge.bidirectional) ) return false;
			if ( hinge.part_a >= part_count || hinge.part_b >= part_count ) return false;
			cfg.hinges.push_back(hinge);
		}

		return true;
	}

	void verify_and_cache_body_plan_or_die( const std::string& path )
	{
		BodyPlanConfig cfg;
		if ( !load_body_plan_config(path, cfg) )
		{
			fatal_body_plan_error(std::string("verifier failed for required file '") + path + "'");
		}
		g_verified_body_plan = cfg;
	}
}
 
	void BodySystem::construct()
	{
		auto settings = addChild( "settings", new BEntity() );
		m_unit_container = addChild( "unit_container", new BEntity() );

		// m_mutation_chance = settings->addChild( "mutation_chance", new BEntity_uint() );
		// m_mutationruns_min = settings->addChild( "mutationruns_min", new BEntity_uint() );
		// m_mutationruns_max = settings->addChild( "mutationruns_max", new BEntity_uint() );
		// m_bodypart_min = settings->addChild( "bodypart_min", new BEntity_uint() );
		// m_bodypart_max = settings->addChild( "bodypart_max", new BEntity_uint() );
		// m_head_min = settings->addChild( "head_min", new BEntity_uint() );
		// m_head_max = settings->addChild( "head_max", new BEntity_uint() );
		// m_eye_min = settings->addChild( "eye_min", new BEntity_uint() );
		// m_eye_max = settings->addChild( "eye_max", new BEntity_uint() );
		// m_eyes_connect_to_heads = settings->addChild( "eyes_connect_to_heads", new BEntity_bool() );
		// m_spike_min = settings->addChild( "spike_min", new BEntity_uint() );
		// m_spike_max = settings->addChild( "spike_max", new BEntity_uint() );
		// m_bodypart_density = settings->addChild( "bodypart_density", new BEntity_float() );
		m_bodypart_scale_x_min = settings->addChild( "bodypart_scale_x_min", new BEntity_float() );
		m_bodypart_scale_x_max = settings->addChild( "bodypart_scale_x_max", new BEntity_float() );
		m_bodypart_scale_y_min = settings->addChild( "bodypart_scale_y_min", new BEntity_float() );
		m_bodypart_scale_y_max = settings->addChild( "bodypart_scale_y_max", new BEntity_float() );
		m_bodypart_scale_z_min = settings->addChild( "bodypart_scale_z_min", new BEntity_float() );
		m_bodypart_scale_z_max = settings->addChild( "bodypart_scale_z_max", new BEntity_float() );
		m_bodypart_spacing = settings->addChild( "bodypart_spacing", new BEntity_float() );
		m_bodypart_friction = settings->addChild( "bodypart_friction", new BEntity_float() );
		m_bodypart_restitution = settings->addChild( "bodypart_restitution", new BEntity_float() );
		m_bodypart_density = settings->addChild( "bodypart_density", new BEntity_float() );
		m_body_plan_file = settings->addChild( "body_plan_file", new BEntity_string() );
		settings->addChild( "bodypart_use_density", new BEntity_bool() )->set( false );

		// auto mutation_weights = settings->addChild( "mutation_weights", new BEntity() );
		// m_mutationweight_bodypart_add = mutation_weights->addChild( "mutationweight_bodypart_add", new BEntity_uint() );
		// m_mutationweight_bodypart_remove = mutation_weights->addChild( "mutationweight_bodypart_remove", new BEntity_uint() );
		// m_mutationweight_bodypart_rescale = mutation_weights->addChild( "mutationweight_bodypart_rescale", new BEntity_uint() );
		// m_mutationweight_head_add = mutation_weights->addChild( "mutationweight_head_add", new BEntity_uint() );
		// m_mutationweight_head_remove = mutation_weights->addChild( "mutationweight_head_remove", new BEntity_uint() );
		// m_mutationweight_eye_add = mutation_weights->addChild( "mutationweight_eye_add", new BEntity_uint() );
		// m_mutationweight_eye_remove = mutation_weights->addChild( "mutationweight_eye_remove", new BEntity_uint() );
		// m_mutationweight_spike_add = mutation_weights->addChild( "mutationweight_spike_add", new BEntity_uint() );
		// m_mutationweight_spike_remove = mutation_weights->addChild( "mutationweight_spike_remove", new BEntity_uint() );
		// m_mutationweight_constraint_alter = mutation_weights->addChild( "mutationweight_constraint_alter", new BEntity_uint() );
		// m_mutationweight_constraint_change_angle = mutation_weights->addChild( "mutationweight_constraint_change_angle", new BEntity_uint() );
		
		// m_mutation_chance->set( Buint(20) );
		// m_mutationruns_min->set( Buint(2) );
		// m_mutationruns_max->set( Buint(10) );
		// m_bodypart_min->set( Buint( 1 ) );
		// m_bodypart_max->set( Buint( 10 ) );
		// m_head_min->set( Buint( 1 ) );
		// m_head_max->set( Buint( 1 ) );
		// m_eye_min->set( Buint(1) );
		// m_eye_max->set( Buint(1) );
		// m_eyes_connect_to_heads->set( true );
		// m_spike_min->set( Buint( 0 ) );
		// m_spike_max->set( Buint( 0 ) );
		// m_bodypart_density->set( 1.0f );
		m_bodypart_scale_x_min->set( 0.1f * 2 );
		m_bodypart_scale_x_max->set( 2.0f * 2 );
		m_bodypart_scale_y_min->set( 0.1f * 2 );
		m_bodypart_scale_y_max->set( 2.0f * 2 );
		m_bodypart_scale_z_min->set( 0.1f * 2 );
		m_bodypart_scale_z_max->set( 2.0f * 2 );
		m_bodypart_spacing->set( 0.07f );
		m_bodypart_friction->set( 0.95f );
		m_bodypart_restitution->set( 0.95f );
		m_bodypart_density->set( 100.0f );
		m_body_plan_file->set( "config/body_plan.mudskipper.json" );
		verify_and_cache_body_plan_or_die( m_body_plan_file->get_string() );
		m_bodypart_friction->set( g_verified_body_plan.bodypart_friction );
		m_bodypart_restitution->set( g_verified_body_plan.bodypart_restitution );
		m_bodypart_density->set( g_verified_body_plan.bodypart_density );
		settings->getChild( "bodypart_use_density", 1 )->set( g_verified_body_plan.bodypart_use_density );

		// m_mutationweight_bodypart_add->set( Buint(5) );
		// m_mutationweight_bodypart_remove->set( Buint(6) );
		// m_mutationweight_bodypart_rescale->set( Buint(10) );
		// m_mutationweight_head_add->set( Buint(5) );
		// m_mutationweight_head_remove->set( Buint(6) );
		// m_mutationweight_eye_add->set( Buint(5) );
		// m_mutationweight_eye_remove->set( Buint(6) );
		// m_mutationweight_spike_add->set( Buint(5) );
		// m_mutationweight_spike_remove->set( Buint(6) );
		// m_mutationweight_constraint_alter->set( Buint(10) );
		// m_mutationweight_constraint_change_angle->set( Buint(10) );

			// SKIPPED ONES FOR REFERENCE
// 				t_body_eye_rays_min = entityType_UINT
// 				t_body_eye_rays_max _maxentityType_UINT
	}

	void CdBodyPlanBuilder::make( BEntity* entity_parent )
	{
		auto critter_system = entity_parent->parent()->parent()->parent()->parent();
		entity_parent->addChild( "bodyparts", new BEntity() );
		auto t_constraints = entity_parent->addChild( "constraints", new BEntity() );
		
		auto critterding = entity_parent->topParent()->getChild("bin", 1)->getChild( "Critterding", 1 );
		if ( critterding )
		{
			// exit (0);
		}
		
		if ( m_rng == 0)
		{
			m_rng = critterding->getChild( "random_number_generator" ); // FIXME PREFETCH
		}

		auto physics_world = critter_system->parent()->getChild( "physicsworld", 1 );
		auto settings = entity_parent->parent()->parent()->parent()->getChild( "settings", 1 );
		const auto& plan = g_verified_body_plan;
		auto dropzone = critter_system->getChild( "settings", 1 )->getChild( "dropzone", 1 );

		// SPAWN BASE POSITION
			m_rng->set( "min", (Bint)0 );
			m_rng->set( "max", (Bint)dropzone->getChild( "size_x", 1 )->get_float() );
			const auto spawn_base_x = dropzone->getChild( "position_x", 1 )->get_float() + m_rng->get_int();

			m_rng->set( "max", (Bint)dropzone->getChild( "size_y", 1 )->get_float() );
			const auto spawn_base_y = dropzone->getChild( "position_y", 1 )->get_float() + m_rng->get_int();

			m_rng->set( "max", (Bint)dropzone->getChild( "size_z", 1 )->get_float() );
			const auto spawn_base_z = dropzone->getChild( "position_z", 1 )->get_float() + m_rng->get_int();

		std::vector<BEntity*> built_parts;
		built_parts.reserve(plan.parts.size());

		for ( const auto& part : plan.parts )
		{
			auto bodypart = constructBodypart(
				entity_parent,
				part.name.c_str(),
				physics_world,
				spawn_base_x + part.offset_x,
				spawn_base_y + part.offset_y,
				spawn_base_z + part.offset_z,
				part.scale_x,
				part.scale_y,
				part.scale_z
			);
			built_parts.push_back(bodypart);

			const auto part_friction = part.has_friction_override ? part.friction_override : plan.bodypart_friction;
			bodypart->set("friction", part_friction);
			bodypart->set("restitution", plan.bodypart_restitution);
		}

		for ( const auto& hinge : plan.hinges )
		{
			auto hinge_entity = physics_world->addChild( hinge.name.c_str(), "Constraint_Hinge" );
			auto bodyA_ref = hinge_entity->addChild( "bodyA", new BEntity_reference() );
			auto bodyB_ref = hinge_entity->addChild( "bodyB", new BEntity_reference() );
			bodyA_ref->set( built_parts[hinge.part_a] );
			bodyB_ref->set( built_parts[hinge.part_b] );

			auto t_a = hinge_entity->getChild( "localA", 1 );
			auto t_b = hinge_entity->getChild( "localB", 1 );
			t_a->set( "position_x", hinge.local_a_x );
			t_a->set( "position_y", hinge.local_a_y );
			t_a->set( "position_z", hinge.local_a_z );
			t_a->set( "pitch", hinge.local_a_pitch );
			t_b->set( "position_x", hinge.local_b_x );
			t_b->set( "position_y", hinge.local_b_y );
			t_b->set( "position_z", hinge.local_b_z );
			t_b->set( "pitch", hinge.local_b_pitch );

			hinge_entity->getChild( "limit_low", 1 )->set( hinge.limit_low );
			hinge_entity->getChild( "limit_high", 1 )->set( hinge.limit_high );
			hinge_entity->getChild( "softness", 1 )->set( hinge.softness );
			hinge_entity->getChild( "biasfactor", 1 )->set( hinge.biasfactor );
			hinge_entity->getChild( "relaxationfactor", 1 )->set( hinge.relaxationfactor );
			hinge_entity->getChild( "bidirectional", 1 )->set( hinge.bidirectional );
			hinge_entity->set( "create_hinge", true );

			t_constraints->addChild( "external_hinge", new BEntity_external() )->set( hinge_entity );
		}
	}
	
	void CdBodyPlan::construct()
	{
		CdBodyPlanBuilder m;
		m.make( this );
	}

	BEntity* CdBodyPlan::customCopy( BEntity* to_parent, BEntity* entity, std::map<BEntity*, BEntity*>& translation_map )
	{
		auto entity_new = to_parent->addChild( entity->name(), new CdBodyPlan() );
		
		// HACK SPECIAL CASE
		// LOOP ENTITY & ENTITY_NEW TO ADD THEM TO THE TRANSLATION_MAP
		translation_map.insert( std::make_pair( entity, entity_new ) );
		
		auto bodyparts = entity->getChild( "bodyparts", 1 );
		auto constraints = entity->getChild( "constraints", 1 );
		auto bodyparts_new = entity_new->getChild( "bodyparts", 1 );
		auto constraints_new = entity_new->getChild( "constraints", 1 );
		translation_map.insert( std::make_pair( bodyparts, bodyparts_new ) );
		translation_map.insert( std::make_pair( constraints, constraints_new ) );

		// TRANSLATION_MAP: BODYPARTS
		{
			const auto& children_vector_new = bodyparts_new->children();
			auto child_new = children_vector_new.begin();
			for_all_children_of( bodyparts )
			{
				if ( child_new != children_vector_new.end() )
				{
					translation_map.insert( std::make_pair( *child, *child_new ) );
					translation_map.insert( std::make_pair( (*child)->get_reference(), (*child_new)->get_reference() ) );
				}
				child_new++;
			}
		}
		
		// TRANSLATION_MAP: CONSTRAINTS
		{
			const auto& children_vector_new = constraints_new->children();
			auto child_new = children_vector_new.begin();
			for_all_children_of( constraints )
			{
				if ( child_new != children_vector_new.end() )
				{
					translation_map.insert( std::make_pair( *child, *child_new ) );
					translation_map.insert( std::make_pair( (*child)->get_reference(), (*child_new)->get_reference() ) );
					translation_map.insert( std::make_pair( (*child)->get_reference()->getChild("angle", 1), (*child_new)->get_reference()->getChild("angle", 1) ) );

					// do body a and b here
				}
				child_new++;
			}
		}
		return entity_new;
	}
	
	
	BEntity* CdBodyPlanBuilder::tergite2( BEntity* body, float central_bodypart_position_x, float central_bodypart_position_y, float central_bodypart_position_z, float central_bodypart_scale_x, float central_bodypart_scale_y, float central_bodypart_scale_z, float joint_bodypart_scale_x, float joint_bodypart_scale_y, float joint_bodypart_scale_z, float leg_bodypart_scale_x, float leg_bodypart_scale_y, float leg_bodypart_scale_z )
	{
		auto t_constraints = body->getChild( "constraints", 1 );

			// find physicsworld
		auto physics_world = body->parent()->parent()->parent()->parent()->parent()->getChild( "physicsworld", 1 );
		auto settings = body->parent()->parent()->parent()->getChild( "settings", 1 );

		if ( physics_world )
		{
			auto bodypart_spacing = settings->getChild( "bodypart_spacing", 1 )->get_float();
			
			float shift = (central_bodypart_scale_x/2) + (leg_bodypart_scale_x/2) + bodypart_spacing;
			
			// construct
			auto central_bodypart     = constructBodypart( body, "bodypart_central", physics_world, central_bodypart_position_x, central_bodypart_position_y, central_bodypart_position_z, central_bodypart_scale_x, central_bodypart_scale_y, central_bodypart_scale_z );
			auto joint_bodypart_right = constructBodypart( body, "joint_bodypart_right", physics_world, central_bodypart_position_x + shift, central_bodypart_position_y, central_bodypart_position_z, joint_bodypart_scale_x, joint_bodypart_scale_y, joint_bodypart_scale_z );
			auto joint_bodypart_left  = constructBodypart( body, "joint_bodypart_left", physics_world, central_bodypart_position_x - shift, central_bodypart_position_y, central_bodypart_position_z, joint_bodypart_scale_x, joint_bodypart_scale_y, joint_bodypart_scale_z );
			auto leg_bodypart_right = constructBodypart( body, "bodypart_right", physics_world, central_bodypart_position_x + joint_bodypart_scale_x + shift, central_bodypart_position_y, central_bodypart_position_z, leg_bodypart_scale_x, leg_bodypart_scale_y, leg_bodypart_scale_z );
			auto leg_bodypart_left  = constructBodypart( body, "bodypart_left", physics_world, central_bodypart_position_x - joint_bodypart_scale_x - shift, central_bodypart_position_y, central_bodypart_position_z, leg_bodypart_scale_x, leg_bodypart_scale_y, leg_bodypart_scale_z );

			auto friction = settings->getChild( "bodypart_friction", 1 )->get_float();
			central_bodypart->set("friction", friction);
			joint_bodypart_right->set("friction", friction);
			joint_bodypart_left->set("friction", friction);
			leg_bodypart_right->set("friction", friction);
			leg_bodypart_left->set("friction", friction);

			auto restitution = settings->getChild( "bodypart_restitution", 1 )->get_float();
			central_bodypart->set("restitution", restitution);
			joint_bodypart_right->set("restitution", restitution);
			joint_bodypart_left->set("restitution", restitution);
			leg_bodypart_right->set("restitution", restitution);
			leg_bodypart_left->set("restitution", restitution);

			// CONNECT RIGHT BODYPART
			{
				// HINGE INNER
				{
					// a hinge
					auto hinge_entity = physics_world->addChild( "hinge", "Constraint_Hinge" );

					// set A bodypart
					auto bodyA_ref = hinge_entity->addChild( "bodyA", new BEntity_reference() );
					bodyA_ref->set( central_bodypart );

					// set B bodypart
					auto bodyB_ref = hinge_entity->addChild( "bodyB", new BEntity_reference() );
					bodyB_ref->set( joint_bodypart_right );

					// get A & B  transforms from hinge
					auto t_a = hinge_entity->getChild( "localA", 1 );
					auto t_b = hinge_entity->getChild( "localB", 1 );

					// hinge position
					t_a->set( "position_x", +central_bodypart->get_float( "scale_x") / 2 + bodypart_spacing / 2 );
					t_b->set( "position_x", -joint_bodypart_right->get_float( "scale_x" ) / 2 - bodypart_spacing / 2 );
					t_a->set( "pitch", 1.57f );
					t_b->set( "pitch", 1.57f );
					
					// // hinge properties
					hinge_entity->getChild( "limit_low", 1 )->set( -0.5f );
					hinge_entity->getChild( "limit_high", 1 )->set( 0.5f );
					// hinge_entity->getChild( "softness", 1 )->set( 0.9f );
					// hinge_entity->getChild( "biasfactor", 1 )->set( 0.9f );
					// hinge_entity->getChild( "relaxationfactor", 1 )->set( 1.0f );
					hinge_entity->getChild( "bidirectional", 1 )->set( true );

					// pull create trigger
					hinge_entity->set( "create_hinge", true );
					
					// REFERENCE TO EXTERNAL CHILD
						t_constraints->addChild( "external_hinge", new BEntity_external() )->set( hinge_entity );
				}

				// HINGE OUTER
				{
					// a hinge
					auto hinge_entity = physics_world->addChild( "hinge", "Constraint_Hinge" );

					// set A bodypart
					auto bodyA_ref = hinge_entity->addChild( "bodyA", new BEntity_reference() );
					bodyA_ref->set( joint_bodypart_right );

					// set B bodypart
					auto bodyB_ref = hinge_entity->addChild( "bodyB", new BEntity_reference() );
					bodyB_ref->set( leg_bodypart_right );

					// get A & B  transforms from hinge
					auto t_a = hinge_entity->getChild( "localA", 1 );
					auto t_b = hinge_entity->getChild( "localB", 1 );

					// hinge position
					t_a->set( "position_x", +joint_bodypart_right->get_float( "scale_x") / 2 + bodypart_spacing / 2 );
					t_b->set( "position_x", -leg_bodypart_right->get_float( "scale_x" ) / 2 - bodypart_spacing / 2 );
					
					// // hinge properties
					// hinge_entity->getChild( "limit_low", 1 )->set( 0.4f );
					// hinge_entity->getChild( "limit_high", 1 )->set( 0.9f );
					// hinge_entity->getChild( "softness", 1 )->set( 0.9f );
					// hinge_entity->getChild( "biasfactor", 1 )->set( 0.9f );
					// hinge_entity->getChild( "relaxationfactor", 1 )->set( 1.0f );

					// pull create trigger
					hinge_entity->set( "create_hinge", true );
					
					// REFERENCE TO EXTERNAL CHILD
						t_constraints->addChild( "external_hinge", new BEntity_external() )->set( hinge_entity );
				}
			}

			// CONNECT LEFT BODYPART
			{
				// HINGE INNER
				{
					// a hinge
					auto hinge_entity = physics_world->addChild( "hinge", "Constraint_Hinge" );

					// set A bodypart
					auto bodyA_ref = hinge_entity->addChild( "bodyA", new BEntity_reference() );
					bodyA_ref->set( central_bodypart );

					// set B bodypart
					auto bodyB_ref = hinge_entity->addChild( "bodyB", new BEntity_reference() );
					bodyB_ref->set( joint_bodypart_left );

					// get A & B  transforms from hinge
					auto t_a = hinge_entity->getChild( "localA", 1 );
					auto t_b = hinge_entity->getChild( "localB", 1 );

					// hinge position, flip them to mirror (pitch)
					t_a->set( "position_x", -central_bodypart->get_float( "scale_x") / 2 - bodypart_spacing / 2 );
					t_b->set( "position_x", +joint_bodypart_left->get_float( "scale_x" ) / 2 + bodypart_spacing / 2 );
					// t_b->set( "pitch", 3.141593f );
					t_a->set( "pitch", 1.57f );
					t_b->set( "pitch", 1.57f );
					
					// // hinge properties
					hinge_entity->getChild( "limit_low", 1 )->set( -0.5f );
					hinge_entity->getChild( "limit_high", 1 )->set( 0.5f );
					// hinge_entity->getChild( "softness", 1 )->set( 0.9f );
					// hinge_entity->getChild( "biasfactor", 1 )->set( 0.9f );
					// hinge_entity->getChild( "relaxationfactor", 1 )->set( 1.0f );
					hinge_entity->getChild( "bidirectional", 1 )->set( true );

					// pull create trigger
					hinge_entity->set( "create_hinge", true );
					
					// REFERENCE TO EXTERNAL CHILD
						t_constraints->addChild( "external_hinge", new BEntity_external() )->set( hinge_entity );
				}

				// HINGE OUTER
				{
					// a hinge
					auto hinge_entity = physics_world->addChild( "hinge", "Constraint_Hinge" );

					// set A bodypart
					auto bodyA_ref = hinge_entity->addChild( "bodyA", new BEntity_reference() );
					bodyA_ref->set( joint_bodypart_left );

					// set B bodypart
					auto bodyB_ref = hinge_entity->addChild( "bodyB", new BEntity_reference() );
					bodyB_ref->set( leg_bodypart_left );

					// get A & B  transforms from hinge
					auto t_a = hinge_entity->getChild( "localA", 1 );
					auto t_b = hinge_entity->getChild( "localB", 1 );

					// hinge position, flip them to mirror (pitch)
					t_a->set( "position_x", -joint_bodypart_left->get_float( "scale_x") / 2 - bodypart_spacing / 2 );
					t_b->set( "position_x", +leg_bodypart_left->get_float( "scale_x" ) / 2 + bodypart_spacing / 2 );
					t_a->set( "pitch", 3.141593f );
					t_b->set( "pitch", 3.141593f );
					
					// // hinge properties
					// hinge_entity->getChild( "limit_low", 1 )->set( 0.4f );
					// hinge_entity->getChild( "limit_high", 1 )->set( 0.9f );
					// hinge_entity->getChild( "softness", 1 )->set( 0.9f );
					// hinge_entity->getChild( "biasfactor", 1 )->set( 0.9f );
					// hinge_entity->getChild( "relaxationfactor", 1 )->set( 1.0f );

					// pull create trigger
					hinge_entity->set( "create_hinge", true );
					
					// REFERENCE TO EXTERNAL CHILD
						t_constraints->addChild( "external_hinge", new BEntity_external() )->set( hinge_entity );
				}
			}
			
			return central_bodypart;
		}
		return 0;
	}
	
	BEntity* CdBodyPlanBuilder::tergite_simple(
		BEntity* body,
		float central_bodypart_position_x,
		float central_bodypart_position_y,
		float central_bodypart_position_z,
		float central_bodypart_scale_x,
		float central_bodypart_scale_y,
		float central_bodypart_scale_z,
		float leg_bodypart_scale_x,
		float leg_bodypart_scale_y,
		float leg_bodypart_scale_z )
	{
		auto t_constraints = body->getChild( "constraints", 1 );

			// find physicsworld
		auto physics_world = body->parent()->parent()->parent()->parent()->parent()->getChild( "physicsworld", 1 );
		auto settings = body->parent()->parent()->parent()->getChild( "settings", 1 );

		if ( physics_world )
		{
			auto bodypart_spacing = settings->getChild( "bodypart_spacing", 1 )->get_float();
			float shift = (central_bodypart_scale_x/2) + (leg_bodypart_scale_x/2) + bodypart_spacing;
			
			// construct
			auto central_bodypart     = constructBodypart( body, "bodypart_central", physics_world, central_bodypart_position_x, central_bodypart_position_y, central_bodypart_position_z, central_bodypart_scale_x, central_bodypart_scale_y, central_bodypart_scale_z );
			auto leg_bodypart_right = constructBodypart( body, "bodypart_right", physics_world, central_bodypart_position_x + shift, central_bodypart_position_y, central_bodypart_position_z, leg_bodypart_scale_x, leg_bodypart_scale_y, leg_bodypart_scale_z );
			auto leg_bodypart_left  = constructBodypart( body, "bodypart_left", physics_world, central_bodypart_position_x - shift, central_bodypart_position_y, central_bodypart_position_z, leg_bodypart_scale_x, leg_bodypart_scale_y, leg_bodypart_scale_z );

			auto friction = settings->getChild( "bodypart_friction", 1 )->get_float();
			central_bodypart->set("friction", friction);
			leg_bodypart_right->set("friction", friction);
			leg_bodypart_left->set("friction", friction);

			auto restitution = settings->getChild( "bodypart_restitution", 1 )->get_float();
			central_bodypart->set("restitution", restitution);
			leg_bodypart_right->set("restitution", restitution);
			leg_bodypart_left->set("restitution", restitution);

			// CONNECT RIGHT BODYPART
			{
				// a hinge
				auto hinge_entity = physics_world->addChild( "hinge", "Constraint_Hinge" );

				// set A bodypart
				auto bodyA_ref = hinge_entity->addChild( "bodyA", new BEntity_reference() );
				bodyA_ref->set( central_bodypart );

				// set B bodypart
				auto bodyB_ref = hinge_entity->addChild( "bodyB", new BEntity_reference() );
				bodyB_ref->set( leg_bodypart_right );

				// get A & B  transforms from hinge
				auto t_a = hinge_entity->getChild( "localA", 1 );
				auto t_b = hinge_entity->getChild( "localB", 1 );

				// hinge position
				t_a->set( "position_x", +central_bodypart->get_float( "scale_x") / 2 + bodypart_spacing / 2 );
				t_b->set( "position_x", -leg_bodypart_right->get_float( "scale_x" ) / 2 - bodypart_spacing / 2 );

				// pull create trigger
				hinge_entity->set( "create_hinge", true );
				
				// REFERENCE TO EXTERNAL CHILD
					t_constraints->addChild( "external_hinge", new BEntity_external() )->set( hinge_entity );
			}

			// CONNECT LEFT BODYPART
			{
				// a hinge
				auto hinge_entity = physics_world->addChild( "hinge", "Constraint_Hinge" );

				// set A bodypart
				auto bodyA_ref = hinge_entity->addChild( "bodyA", new BEntity_reference() );
				bodyA_ref->set( central_bodypart );

				// set B bodypart
				auto bodyB_ref = hinge_entity->addChild( "bodyB", new BEntity_reference() );
				bodyB_ref->set( leg_bodypart_left );

				// get A & B  transforms from hinge
				auto t_a = hinge_entity->getChild( "localA", 1 );
				auto t_b = hinge_entity->getChild( "localB", 1 );

				// hinge position, flip them to mirror (pitch)
				t_a->set( "position_x", -central_bodypart->get_float( "scale_x") / 2 - bodypart_spacing / 2 );
				t_a->set( "pitch", 3.141593f );
				
				t_b->set( "position_x", +leg_bodypart_right->get_float( "scale_x" ) / 2 + bodypart_spacing / 2 );
				t_b->set( "pitch", 3.141593f );

				// // hinge properties
				// hinge_entity->getChild( "limit_low", 1 )->set( 0.4f );
				// hinge_entity->getChild( "limit_high", 1 )->set( 0.9f );
				// hinge_entity->getChild( "softness", 1 )->set( 0.9f );
				// hinge_entity->getChild( "biasfactor", 1 )->set( 0.9f );
				// hinge_entity->getChild( "relaxationfactor", 1 )->set( 1.0f );

				// pull create trigger
				hinge_entity->set( "create_hinge", true );
				
				// REFERENCE TO EXTERNAL CHILD
					t_constraints->addChild( "external_hinge", new BEntity_external() )->set( hinge_entity );
			}
			
			return central_bodypart;
		}
		return 0;
	}

	BEntity* CdBodyPlanBuilder::constructBodypart( BEntity* body, const char* name, BEntity* physics_world, float pos_x, float pos_y, float pos_z, float scale_x, float scale_y, float scale_z )
	{
		auto t_bodyparts = body->getChild( "bodyparts", 1 );
		
		// PHYSICS
			auto new_bodypart = physics_world->addChild( name, g_verified_body_plan.physics_bodypart_class.c_str() );
			
			// WEIGHT
				float weight(1.0f);
				if ( g_verified_body_plan.bodypart_use_density )
				{
					weight = g_verified_body_plan.bodypart_density * (scale_x * scale_y * scale_z);
				}
				new_bodypart->addChild( "weight", new BEntity_float_property() )->set( weight ); // FIXME SETTING
			
			// DEACTIVATION
				new_bodypart->addChild( "wants_deactivation", new BEntity_bool_property() )->set( g_verified_body_plan.bodypart_wants_deactivation ); // FIXME SETTING

			// SCALE
				new_bodypart->addChild( "scale_x", new BEntity_float_property() )->set( scale_x ); // FIXME SETTING
				new_bodypart->addChild( "scale_y", new BEntity_float_property() )->set( scale_y ); // FIXME SETTING
				new_bodypart->addChild( "scale_z", new BEntity_float_property() )->set( scale_z ); // FIXME SETTING
				
			// POSTITION
				auto physics_transform = new_bodypart->getChild( "transform", 1 );
				physics_transform->getChild("position_x", 1)->set( pos_x );
				physics_transform->getChild("position_y", 1)->set( pos_y );
				physics_transform->getChild("position_z", 1)->set( pos_z );
			
			// REFERENCE TO EXTERNAL CHILD
				t_bodyparts->addChild( "external_bodypart_physics", new BEntity_external() )->set( new_bodypart );
				
		// GRAPHICS
			BEntity* graphics_transform(0);
			// auto graphicsmodelsystem = body->topParent()->getChild("Scene", 1)->getChild("GraphicsModelSystem");
			auto graphicsmodelsystem = body->topParent()->getChild("bin", 1)->getChild("Critterding", 1)->getChild("GLWindow", 1)->getChild("GraphicsModelSystem", 1);

			if ( graphicsmodelsystem )
			{
				// FIXME do the graphics entity upstairs in body_system, we're assuming we need a graphics entity for all anyway
				// LOAD MODEL IF NEEDED, ADD TRANSFORM
				auto graphics_entity_critter = graphicsmodelsystem->getChild( g_verified_body_plan.graphics_model_name.c_str(), 1 );
				if ( !graphics_entity_critter )
				{
					graphics_entity_critter = graphicsmodelsystem->addChild(g_verified_body_plan.graphics_model_name.c_str(), "GraphicsModel");
					graphics_transform = graphics_entity_critter->addChild("transform", "Transform");
					graphics_entity_critter->set("filename", g_verified_body_plan.graphics_mesh_file.c_str());
					// graphics_entity_critter->set("filename", "/projects/bengine-new/share/sandbox/modules/cube-critter.obj");
					
					
					// std::cout << $0 << std::endl;
				}
				else
				{
					graphics_transform = graphics_entity_critter->addChild("transform", "Transform");
				}

				graphics_transform->getChild( "scale_x", 1 )->set( scale_x );
				graphics_transform->getChild( "scale_y", 1 )->set( scale_y );
				graphics_transform->getChild( "scale_z", 1 )->set( scale_z );
				// graphics_transform->addChild( "scale_x", new BEntity_float )->set( scale_x );
				// graphics_transform->addChild( "scale_y", new BEntity_float )->set( scale_y );
				// graphics_transform->addChild( "scale_z", new BEntity_float )->set( scale_z );

				// REFERENCE TO EXTERNAL CHILD
					// auto external_reference = t_bodyparts->addChild( "_external_child", new BEntity_reference() );
					// external_reference->set( graphics_transform );
					t_bodyparts->addChild( "external_bodypart_graphics", new BEntity_external() )->set( graphics_transform );
			}
			physics_transform->connectServerServer(graphics_transform, true);

			return new_bodypart;
	}

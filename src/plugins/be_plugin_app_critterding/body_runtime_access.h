#pragma once

#include "kernel/be_entity_interface.h"

namespace cd_body_runtime
{
	inline BEntity* find_body_root(BEntity* body_container)
	{
		if ( body_container == 0 )
		{
			return 0;
		}
		if ( body_container->getChild("bodyparts", 1) )
		{
			return body_container;
		}
		for ( auto* child : body_container->children() )
		{
			if ( child && child->getChild("bodyparts", 1) )
			{
				return child;
			}
		}
		return 0;
	}

	inline BEntity* find_body_container_from_critter(BEntity* critter)
	{
		if ( critter == 0 )
		{
			return 0;
		}
		auto external_body = critter->getChild("external_body", 1);
		if ( external_body == 0 )
		{
			return 0;
		}
		return external_body->get_reference();
	}

	inline BEntity* find_body_root_from_critter(BEntity* critter)
	{
		return find_body_root(find_body_container_from_critter(critter));
	}

	inline BEntity* find_bodyparts_from_critter(BEntity* critter)
	{
		auto body_root = find_body_root_from_critter(critter);
		if ( body_root == 0 )
		{
			return 0;
		}
		return body_root->getChild("bodyparts", 1);
	}

	inline BEntity* find_constraints_from_body_root(BEntity* body_root)
	{
		if ( body_root == 0 )
		{
			return 0;
		}
		return body_root->getChild("constraints", 1);
	}

	inline BEntity* find_primary_bodypart_physics_from_critter(BEntity* critter)
	{
		auto bodyparts = find_bodyparts_from_critter(critter);
		if ( bodyparts == 0 )
		{
			return 0;
		}
		auto external_physics = bodyparts->getChild("external_bodypart_physics", 1);
		if ( external_physics == 0 )
		{
			return 0;
		}
		return external_physics->get_reference();
	}
}


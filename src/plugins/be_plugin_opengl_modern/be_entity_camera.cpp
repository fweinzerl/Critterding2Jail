#include "be_entity_camera.h"
#include "be_entity_graphics_model.h"
#include <GL/gl.h>
#include <cmath>
#include <glm/gtc/type_ptr.hpp>

// #include <iostream>

	BCamera::BCamera()
	: BEntity()
	, m_s_elapsed(0)
	, m_yaw_pitch_initialized(false)
	, m_yaw(0.0f)
	, m_pitch(0.0f)
	{
		setProcessing();
	}

	void BCamera::construct()
	{
		m_active = parent()->getChild( "active", 1 );

		m_fov_y = addChild( "fov_y", new BEntity_float() );
		m_fov_y->set( SIMD_PI / 4 );
		// m_fov_y->set( 45.0f );
		// m_fov_y->set( 90.0f );
		m_aspect_ratio = addChild( "aspect_ratio", new BEntity_float() );
		m_aspect_ratio->set( 1280.0f/800.0f );
		m_z_near = addChild( "z_near", new BEntity_float() );
		m_z_near->set( 0.1f );
		m_z_far = addChild( "z_far", new BEntity_float() );
		m_z_far->set( 10000.0f );
		m_sensitivity_move = addChild( "sensitivity_move", new BEntity_float() );
		m_sensitivity_move->set( 7.0f );
		m_sensitivity_look = addChild( "sensitivity_look", new BEntity_float() );
		m_sensitivity_look->set( 1.0f );
		// m_transform = addChild( "transform", "Bullet_Transform" );
		m_transform = new BBulletTransform();
		addChild( "transform", m_transform );
		m_base_transform = new BBulletTransform();
		addChild( "base_transform", m_base_transform );
		
		auto movement = addChild( "movement", new BEntity() );
		m_forward = movement->addChild( "forward", new BEntity_bool() );
		m_backward = movement->addChild( "backward", new BEntity_bool() );
		m_left = movement->addChild( "left", new BEntity_bool() );
		m_right = movement->addChild( "right", new BEntity_bool() );
		m_up = movement->addChild( "up", new BEntity_bool() );
		m_down = movement->addChild( "down", new BEntity_bool() );
		auto movement_settings = movement->addChild( "settings", new BEntity() );
		m_forward_horizontal_only = movement_settings->addChild( "forward_horizontal_only", new BEntity_bool() );
		m_forward_horizontal_only->set( false );
		m_allow_roll_for_movement = movement_settings->addChild( "allow_roll_for_movement", new BEntity_bool() );
		m_allow_roll_for_movement->set( true );

		auto looking = addChild( "looking", new BEntity() );
		m_look_left = looking->addChild( "left", new BEntity_bool() );
		m_look_right = looking->addChild( "right", new BEntity_bool() );
		m_look_up = looking->addChild( "up", new BEntity_bool() );
		m_look_down = looking->addChild( "down", new BEntity_bool() );
		m_look_roll_left = looking->addChild( "roll_left", new BEntity_bool() );
		m_look_roll_right = looking->addChild( "roll_right", new BEntity_bool() );
		
		// GL UNIFORMS
		// m_ProjectionMatrixID = glGetUniformLocation(  dynamic_cast<BGraphicsModelSystem*>( parent() )->m_effect->m_program.get()->handle(), "ProjectionMatrix_Camera" );
		// m_ViewMatrixID = glGetUniformLocation(  dynamic_cast<BGraphicsModelSystem*>( parent() )->m_effect->m_program.get()->handle(), "ViewMatrix_Camera" );
		m_ProjectionViewMatrixID = glGetUniformLocation(  dynamic_cast<BGraphicsModelSystem*>( parent() )->m_effect->m_program.get()->handle(), "ProjectionViewMatrix_Camera" );
		
		m_e_scale_x = 0;
		
		m_window_width = parent()->parent()->getChild("width", 1);
		m_window_height = parent()->parent()->getChild("height", 1);
		
		// glTransform t;
		// btTransform bt;
		// bt.setIdentity();
		// bt.getOpenGLMatrix( t.m_value );
  // 
		// glUniformMatrix4fv( m_ViewMatrixID, 1, GL_FALSE, t.m_value );		
	}

	// FIXME do this in the transform itself?
	void BCamera::process()
	{
		glViewport( 0, 0, m_window_width->get_int(), m_window_height->get_int() );
		glClearColor(0.0f, 0.0f, 0.0f, 0.0f);
		glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT | GL_STENCIL_BUFFER_BIT);

		if( m_active->get_bool() )
		{
			
			// Pick Shader
			// glUseProgram( dynamic_cast<BGraphicsModelSystem*>( parent() )->m_effect->m_program.get()->handle() );
			// glUseProgram( dynamic_cast<BGraphicsModelSystem*>( parent() )->m_effect_critter->m_program.get()->handle() );
			
			// std::cout << "BCamera::process()_modern " << m_aspect_ratio->get_float() << std::endl;
			
			if ( m_s_elapsed == 0 )
				m_s_elapsed = topParent()->getChild("sys", 1)->getChild("timer", 1)->getChild("s_elapsed", 1);

			// process movement
			const auto move_step = m_sensitivity_move->get_float() * m_s_elapsed->get_float();
			const bool forward_horizontal_only = m_forward_horizontal_only->get_bool();
			const bool allow_roll_for_movement = m_allow_roll_for_movement->get_bool();

			if ( forward_horizontal_only || !allow_roll_for_movement )
			{
				btVector3 forward(
					-m_transform->m_transform.getBasis()[0][2],
					-m_transform->m_transform.getBasis()[1][2],
					-m_transform->m_transform.getBasis()[2][2]
				);
				forward.setY( 0.0f );
				if ( forward.length2() > 0.000001f )
				{
					forward.normalize();
				}
				else
				{
					forward = btVector3( 0.0f, 0.0f, -1.0f );
				}

				auto right = forward.cross( btVector3( 0.0f, 1.0f, 0.0f ) );
				if ( right.length2() > 0.000001f )
				{
					right.normalize();
				}
				else
				{
					right = btVector3( 1.0f, 0.0f, 0.0f );
				}

				btVector3 delta( 0.0f, 0.0f, 0.0f );
				if ( m_forward->get_bool() ) delta += forward * move_step;
				if ( m_backward->get_bool() ) delta -= forward * move_step;
				if ( m_left->get_bool() ) delta -= right * move_step;
				if ( m_right->get_bool() ) delta += right * move_step;
				if ( m_up->get_bool() ) delta += btVector3( 0.0f, move_step, 0.0f );
				if ( m_down->get_bool() ) delta -= btVector3( 0.0f, move_step, 0.0f );

				if ( delta.length2() > 0.0f )
				{
					auto origin = m_transform->m_transform.getOrigin();
					origin += delta;
					m_transform->m_transform.setOrigin( origin );
				}
			}
			else
			{
				if ( m_forward->get_bool() )
				{
					m_transform->m_transform = m_transform->m_transform * btTransform(btQuaternion( 0.0f, 0.0f, 0.0f ), btVector3( 0.0f, 0.0f, -move_step ));
				}
				if ( m_backward->get_bool() )
				{
					m_transform->m_transform = m_transform->m_transform * btTransform(btQuaternion( 0.0f, 0.0f, 0.0f ), btVector3( 0.0f, 0.0f, move_step ));
				}
				if ( m_left->get_bool() )
				{
					m_transform->m_transform = m_transform->m_transform * btTransform(btQuaternion( 0.0f, 0.0f, 0.0f ), btVector3( -move_step, 0.0f, 0.f ));
				}
				if ( m_right->get_bool() )
				{
					m_transform->m_transform = m_transform->m_transform * btTransform(btQuaternion( 0.0f, 0.0f, 0.0f ), btVector3( move_step, 0.0f, 0.f ));
				}
				if ( m_up->get_bool() )
				{
					m_transform->m_transform = m_transform->m_transform * btTransform(btQuaternion( 0.0f, 0.0f, 0.0f ), btVector3( 0.0f, move_step, 0.f ));
				}
				if ( m_down->get_bool() )
				{
					m_transform->m_transform = m_transform->m_transform * btTransform(btQuaternion( 0.0f, 0.0f, 0.0f ), btVector3( 0.0f, -move_step, 0.f ));
				}
			}
			
			// process looking using yaw/pitch only (no roll tilt drift)
			if ( !m_yaw_pitch_initialized )
			{
				btVector3 initial_forward(
					-m_transform->m_transform.getBasis()[0][2],
					-m_transform->m_transform.getBasis()[1][2],
					-m_transform->m_transform.getBasis()[2][2]
				);
				if ( initial_forward.length2() > 0.000001f )
				{
					initial_forward.normalize();
					m_yaw = std::atan2( initial_forward.x(), -initial_forward.z() );
					m_pitch = std::asin( std::max( -1.0f, std::min( 1.0f, initial_forward.y() ) ) );
				}
				m_yaw_pitch_initialized = true;
			}

			const auto look_step = m_sensitivity_look->get_float() * m_s_elapsed->get_float();
			if ( m_look_left->get_bool() ) m_yaw -= look_step;
			if ( m_look_right->get_bool() ) m_yaw += look_step;
			if ( m_look_up->get_bool() ) m_pitch += look_step;
			if ( m_look_down->get_bool() ) m_pitch -= look_step;

			const float max_pitch = 1.553343f; // ~89 degrees
			if ( m_pitch > max_pitch ) m_pitch = max_pitch;
			if ( m_pitch < -max_pitch ) m_pitch = -max_pitch;

			const auto cp = std::cos( m_pitch );
			const auto sp = std::sin( m_pitch );
			const auto sy = std::sin( m_yaw );
			const auto cy = std::cos( m_yaw );

			btVector3 forward( sy * cp, sp, -cy * cp );
			if ( forward.length2() > 0.000001f )
			{
				forward.normalize();
			}
			else
			{
				forward = btVector3( 0.0f, 0.0f, -1.0f );
			}

			btVector3 world_up( 0.0f, 1.0f, 0.0f );
			btVector3 right = forward.cross( world_up );
			if ( right.length2() > 0.000001f )
			{
				right.normalize();
			}
			else
			{
				right = btVector3( 1.0f, 0.0f, 0.0f );
			}
			btVector3 up = right.cross( forward );
			up.normalize();

			btMatrix3x3 basis(
				right.x(), up.x(), -forward.x(),
				right.y(), up.y(), -forward.y(),
				right.z(), up.z(), -forward.z()
			);
			m_transform->m_transform.setBasis( basis );

			// PROJECTION MATRIX
				// perspective( m_fov_y->get_float(), m_aspect_ratio->get_float(), m_z_near->get_float(), m_z_far->get_float() );
				// glUniformMatrix4fv( m_ProjectionMatrixID, 1, GL_FALSE, m_projectionMatrix );

			// PROJECTION MATRIX GLM
				m_ProjectionMatrixGLM = glm::perspective( m_fov_y->get_float(), m_aspect_ratio->get_float(), m_z_near->get_float(), m_z_far->get_float() );
				// glUniformMatrix4fv(m_ProjectionMatrixID, 1, GL_FALSE, glm::value_ptr(m_ProjectionMatrixGLM));

			// // PROJECTION MATRIX GLM 2
			// 	perspective( m_fov_y->get_float(), m_aspect_ratio->get_float(), m_z_near->get_float(), m_z_far->get_float() );
			// 	m_ProjectionMatrixGLM = glm::make_mat4(m_projectionMatrix);

			// VIEW MATRIX
				// m_ViewMatrix = (m_base_transform->m_transform * m_transform->m_transform).inverse();
				// m_ViewMatrix.getOpenGLMatrix( m_viewMatrix );
				// glUniformMatrix4fv( m_ViewMatrixID, 1, GL_FALSE, m_viewMatrix );

			// VIEW MATRIX GLM
				m_ViewMatrix = (m_base_transform->m_transform * m_transform->m_transform).inverse();
				m_ViewMatrix.getOpenGLMatrix( m_viewMatrix );
				glm::mat4 viewMatrix = glm::make_mat4(m_viewMatrix);
				// glUniformMatrix4fv( m_ViewMatrixID, 1, GL_FALSE, glm::value_ptr(viewMatrix) );


	// 		// PROJECTIONVIEW MATRIX GLM
				// glm::mat4 pvMatrix = glm::mat4(1.0f);
				// pvMatrix = m_ProjectionMatrixGLM * theMatrix;
				glm::mat4 pvMatrix = m_ProjectionMatrixGLM * viewMatrix;
				glUniformMatrix4fv( m_ProjectionViewMatrixID, 1, GL_FALSE, glm::value_ptr(pvMatrix) );

			// FIXME HACK  RESET ALL THE SHADERS?, idea is we don't have to brute force a change to the scale uniform down the line, do it here
				if ( m_e_scale_x == 0 )
				{
					m_e_scale_x = parent()->getChild("shaders", 1)->getChild("e_scale_x", 1);
				}
				m_e_scale_x->set(0.0f);
		}
	}

	btVector3 BCamera::getScreenDirection(const int win_x, const int win_y, const int mouse_x, const int mouse_y)
	{
		btVector3 rayForward( -m_transform->m_transform.getBasis()[0][2], -m_transform->m_transform.getBasis()[1][2], -m_transform->m_transform.getBasis()[2][2]); 
		rayForward.normalize();
		rayForward *= m_z_far->get_float();

		btVector3 rayUp( m_transform->m_transform.getBasis()[0][1], m_transform->m_transform.getBasis()[1][1], m_transform->m_transform.getBasis()[2][1]); 
		btVector3 hor( rayForward.cross(rayUp) );
		hor.normalize();

		btVector3 vertical( hor.cross(rayForward) );
		vertical.normalize();

		const float tanfov( tanf( 0.5f * m_fov_y->get_float() ) );
		const float mult = 2.f * m_z_far->get_float() * tanfov;

		hor *= mult * (float)win_x / win_y;
		vertical *= mult;

		btVector3 rayTo = (m_transform->m_transform.getOrigin() + rayForward) - 0.5f * hor + 0.5f * vertical;

		rayTo += mouse_x * hor * 1.f / win_x;
		rayTo -= mouse_y * vertical * 1.f / win_y;

		return rayTo;
	}

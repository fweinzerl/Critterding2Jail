// void calcClipPlanes(glm::mat4& view, glm::mat4& projection)
// {
//     glm::mat4 viewProjectionMatrix = projection * view;
//     glm::vec4 row0 = glm::row(viewProjectionMatrix, 0);
//     glm::vec4 row1 = glm::row(viewProjectionMatrix, 1);
//     glm::vec4 row2 = glm::row(viewProjectionMatrix, 2);
//     glm::vec4 row3 = glm::row(viewProjectionMatrix, 3);
// 
//     g_leftClipPlane = row0 + row3;
//     g_rightClipPlane = row0 - row3;
//     g_bottomClipPlane = row1 + row3;
//     g_topClipPlane = row1 - row3;
//     g_nearClipPlane = row2 + row3;
//     g_farClipPlane = row2 - row3;
// }
// 
// bool isPointInsideViewFrustum(const glm::vec3& point)
// {
//     glm::vec4 p(point, 1.0);
// 
//     return glm::dot(p, g_leftClipPlane) >= 0 &&
//         glm::dot(p, g_rightClipPlane) <= 0 &&
//         glm::dot(p, g_bottomClipPlane) >= 0 &&
//         glm::dot(p, g_topClipPlane) <= 0 &&
//         glm::dot(p, g_nearClipPlane) >= 0 &&
//         glm::dot(p, g_farClipPlane) <= 0;
// }


#include "vision_system.h"
#include "plugins/be_plugin_opengl_modern/be_entity_graphics_model.h"
#include "plugins/be_plugin_bullet/be_entity_physics_entity.h"
#include "kernel/be_plugin_base_entity_types.h"
#include "critter_system.h"

	void CdVisionSystem::construct()
	{
		m_print = addChild("print", new BEntity_bool());
		// m_print->set( true );
		m_critter_sightrange = addChild("sight_range", new BEntity_float());
		m_critter_sightrange->set( 8.0f );
		m_update_every_n_ticks = addChild("update_every_n_ticks", new BEntity_uint());
		m_update_every_n_ticks->set( Buint(5) );
		m_tick_counter = 0;

		m_critter_containers = addChild("critter_containers", new BEntity());
		
		m_turn_180 = btTransform( btQuaternion( SIMD_PI, 0.0f, 0.0f ) );

		m_critter_retinasize = 6; // FIXME max 8?
		m_retinasperrow = 2048 / m_critter_retinasize;

		// vision retina allocation
		unsigned int items = 4 * 2048 * 2048;
		retina = (unsigned char*)malloc( items );
		memset( retina, 0, items );

		// generate namespace for the frame buffer, colorbuffer and depthbuffer
		glGenFramebuffersEXT(1, &fb);
		glGenTextures(1, &color_tex); 
		glGenRenderbuffersEXT(1, &depth_rb);

		//switch to our fbo so we can bind stuff to it
		glBindFramebufferEXT(GL_FRAMEBUFFER_EXT, fb);

		//create the colorbuffer texture and attach it to the frame buffer
		glBindTexture(GL_TEXTURE_2D, color_tex);
		glTexParameterf(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_LINEAR); 
		glTexImage2D(GL_TEXTURE_2D, 0, GL_RGBA8, 2048, 2048, 0, GL_RGBA, GL_INT, NULL);
		glFramebufferTexture2DEXT(GL_FRAMEBUFFER_EXT, GL_COLOR_ATTACHMENT0_EXT, GL_TEXTURE_2D, color_tex, 0); 
		
		// create a render buffer as our depthbuffer and attach it
		glBindRenderbufferEXT(GL_RENDERBUFFER_EXT, depth_rb);
		glRenderbufferStorageEXT(GL_RENDERBUFFER_EXT, GL_DEPTH_COMPONENT24, 2048, 2048);
		glFramebufferRenderbufferEXT(GL_FRAMEBUFFER_EXT, GL_DEPTH_ATTACHMENT_EXT, GL_RENDERBUFFER_EXT, depth_rb);

		// Go back to regular frame buffer rendering
		// glBindFramebufferEXT(GL_FRAMEBUFFER_EXT, 0);
		
		m_graphics_model_system = dynamic_cast<BGraphicsModelSystem*>( parent()->getChild("GLWindow", 1)->getChild("GraphicsModelSystem", 1) );
		m_ProjectionViewMatrixID = glGetUniformLocation(  m_graphics_model_system->m_effect_critter->m_program.get()->handle(), "ProjectionViewMatrix_Camera" );
		
		// shortcut to reset scale uniform, hackish
		m_e_scale_x = topParent()->getChild("bin", 1)->getChild("Critterding", 2)->getChild("GLWindow", 1)->getChild("GraphicsModelSystem", 1)->getChild("shaders", 1)->getChild("e_scale_x", 1);
		
		m_ProjectionMatrixGLM = glm::perspective( SIMD_PI / 4, 1280.0f/800.0f, 0.1f, m_critter_sightrange->get_float() );
		
		m_skyDome = 0;
		m_visible_models_cache_source_count = 0;
		
	} 

	void CdVisionSystem::process()
	{
		if ( m_critter_sightrange->get_float() > 0.0f )
		{
			// std::cout << "------------- vision system START" << std::endl;
			if ( !m_skyDome )
			{
				m_skyDome = m_graphics_model_system->getChild("GraphicsModel_SkyDome", 1);
			}
			const auto source_model_count = m_graphics_model_system->numChildren();
			if ( m_visible_models_cache.empty() || m_visible_models_cache_source_count != source_model_count )
			{
				m_visible_models_cache.clear();
				m_visible_models_cache_source_count = source_model_count;
				const auto& model_entities = m_graphics_model_system->children();
				for ( auto* entity : model_entities )
				{
					if ( entity != m_skyDome )
					{
						auto model = dynamic_cast<BGraphicsModel*>( entity );
						if ( model )
						{
							m_visible_models_cache.push_back( model );
						}
					}
				}
			}

			glUseProgram( m_graphics_model_system->m_effect_critter->m_program.get()->handle() );

			glBindFramebufferEXT(GL_FRAMEBUFFER_EXT, fb); 
			glClearColor (0.0f, 0.0f, 0.0f, 0.0f);
			glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT | GL_STENCIL_BUFFER_BIT);

			// DISABLE SHADOWS
			glActiveTexture(GL_TEXTURE1);
			glBindTexture(GL_TEXTURE_2D, 0);
			glActiveTexture(GL_TEXTURE0);

		// FIXME HACK RESET ALL THE SHADERS?, idea is we don't have to brute force a change to the scale uniform down the line, do it here
			m_e_scale_x->set( 0.0f );

			bool empty(true);
			unsigned int critter_counter = 0;
			unsigned int global_critter_index = 0;
			const auto update_every_n_ticks = std::max(1u, m_update_every_n_ticks->get_uint());
			const auto current_phase = m_tick_counter % update_every_n_ticks;
			std::vector<CdCritter*> rendered_critters;
			for_all_children_of2( m_critter_containers )
			{
				auto container = (*child2)->get_reference();
				for_all_children_of( container )
				{
					if ( (global_critter_index % update_every_n_ticks) != current_phase )
					{
						++global_critter_index;
						continue;
					}

					calcFramePos( critter_counter );

					empty = false;
					CdCritter* critter = dynamic_cast<CdCritter*>( (*child) );
					rendered_critters.push_back( critter );

					if ( critter->m_physics_component_shortcut == 0 )
					{
						critter->m_physics_component_shortcut = critter->getChild("external_body", 1)->get_reference()->getChild("body_fixed1", 1)->getChild("bodyparts", 1)->getChild("external_bodypart_physics", 1)->get_reference();
					}

					auto bodypart_to_attach_cam = dynamic_cast<BPhysicsEntity*>( critter->m_physics_component_shortcut )->getPhysicsComponent();
					
					if ( bodypart_to_attach_cam != 0 )
					{
						glViewport(framePosX, framePosY, m_critter_retinasize, m_critter_retinasize);

						// VIEW MATRIX GLM
							m_ViewMatrix = bodypart_to_attach_cam->getTransform() * m_turn_180;
							m_ViewMatrix.inverse().getOpenGLMatrix( m_buffer );
							m_viewMatrix = glm::make_mat4( m_buffer );
							// glUniformMatrix4fv( m_ViewMatrixID, 1, GL_FALSE, glm::value_ptr(theMatrix) );
						// PROJECTIONVIEW MATRIX GLM
							m_pvMatrix = m_ProjectionMatrixGLM * m_viewMatrix;
							glUniformMatrix4fv( m_ProjectionViewMatrixID, 1, GL_FALSE, glm::value_ptr(m_pvMatrix) );

						// DRAW WORLD
							for ( auto* model : m_visible_models_cache )
							{
								model->processWhenInSight( &bodypart_to_attach_cam->getTransform(), m_critter_sightrange->get_float() );
							}

						++critter_counter;
					}
					++global_critter_index;
				}
			}
			++m_tick_counter;

			// Read pixels into retina
			if ( !empty )
			{
				
				unsigned int picwidth = m_retinasperrow * (m_critter_retinasize);
				unsigned int picheight = m_critter_retinasize;
				
				unsigned int rows(0);
				for_all_children_of( m_critter_containers )
				{
					rows += (*child)->get_reference()->numChildren();
				}
				
				while ( rows > m_retinasperrow )
				{
					picheight += m_critter_retinasize;
					rows -= m_retinasperrow;
				}
				// glReadBuffer(GL_BACK);
				glPixelStorei( GL_UNPACK_ALIGNMENT, 1 );
				glReadPixels(0, 0, picwidth, picheight, GL_RGBA, GL_UNSIGNED_BYTE, retina);
				
				// FEED TO BRAIN
					critter_counter = 0;
					float value;
					float fvalue;
					for ( auto* critter : rendered_critters )
					{
						// cache brain input root and first vision input node
						if ( critter->m_brain_inputs == 0 )
						{
							critter->m_brain_inputs = critter->getChild("external_brain", 1)->get_reference()->getChild("inputs", 1);
						}
						if ( critter->m_brain_vision_input_start == 0 )
						{
							const auto& brain_inputs_children_vector = critter->m_brain_inputs->children();
							for ( unsigned int i = 0; i < brain_inputs_children_vector.size(); ++i )
							{
								auto* input = brain_inputs_children_vector[i];
								if ( input->name() == "vision_value_R" )
								{
									critter->m_brain_vision_input_start = input;
									critter->m_brain_vision_input_start_index = i;
									break;
								}
							}
						}
						if ( !critter->m_brain_vision_input_start )
						{
							++critter_counter;
							continue;
						}
						const auto& brain_inputs_children_vector = critter->m_brain_inputs->children();
						auto brain_input_index = critter->m_brain_vision_input_start_index;
						if ( brain_input_index >= brain_inputs_children_vector.size() )
						{
							critter->m_brain_vision_input_start = 0;
							++critter_counter;
							continue;
						}

						// FEED
						calcFramePos( critter_counter );
						for ( unsigned int h=retinaRowStart; h < retinaRowStart+(m_critter_retinasize*retinaRowLength); h += retinaRowLength )
						{
							for ( unsigned int w=h+retinaColumnStart; w < h+retinaColumnStart+(m_critter_retinasize*4); ++w )
							{
								value = (int)retina[w];
								if ( value > 0 )
								{
									fvalue = (float)value / 255;
									// std::cout << "setting brain_input " << (*brain_input)->name() << " to " << ((float)(int)retina[w]) / 255 << std::endl;

									// if it's the same, force update to outputs
									auto* brain_input = brain_inputs_children_vector[brain_input_index];
									if ( !brain_input->set( fvalue ) )
									{
										brain_input->onUpdate();
									}
								}
								++brain_input_index;
								if ( brain_input_index >= brain_inputs_children_vector.size() )
								{
									break;
								}
							}
						}

						++critter_counter;
					}

				// PRINT
					if ( m_print->get_bool() )
					{
						critter_counter = 0;
						for ( auto* critter : rendered_critters )
						{
							(void)critter;
							calcFramePos(critter_counter);
							std::cout << "critter " << critter_counter << ":" << " x:" << retinaColumnStart << " y:" << retinaRowStart << " retinaRowLength: " << retinaRowLength << std::endl;

							for ( unsigned int h=retinaRowStart; h < retinaRowStart+(m_critter_retinasize*retinaRowLength); h += retinaRowLength )
							{
								for ( unsigned int w=h+retinaColumnStart; w < h+retinaColumnStart+(m_critter_retinasize*4); w+=4 )
								{
									if ( (int)retina[w+0] > 80 ) std::cout << "\033[1;31mR\033[0m";
									else std::cout << ".";
									if ( (int)retina[w+1] > 80 ) std::cout << "\033[1;32mG\033[0m";
									else std::cout << ".";
									if ( (int)retina[w+2] > 80 ) std::cout << "\033[1;34mB\033[0m";
									else std::cout << ".";
									if ( (int)retina[w+3] > 80 ) std::cout << "\033[1;35mA\033[0m";
									else std::cout << ".";
								}
								std::cout << std::endl;
							}
							std::cout << std::endl;

							critter_counter++;
						}
					}
			}

			glBindFramebufferEXT(GL_FRAMEBUFFER_EXT, 0); 
			// std::cout << "------------- vision system STOP" << std::endl;
		}
	}

	bool CdVisionSystem::set( const Bstring& id, BEntity* value )
	{
		if ( id == "register_container" )
		{
			// std::cout << "CdVisionSystem::registered container " << value->name() << std::endl;
			m_critter_containers->addChild( "container", new BEntity_reference() )->set( value ) ;

			return true;
		}

		std::cout << "CdVisionSystem::warning: unknown command '" << id << "'" << std::endl;
		return false;
	}	
	
	void CdVisionSystem::calcFramePos(unsigned int pos)
	{
		// unsigned int m_retinasperrow = 50;
		visionPosition = pos;

		// Calc 2D cartesian vectors X & Y for frame positioning of retina
		framePosY = 0;
		while ( pos >= m_retinasperrow )
		{
			pos -= m_retinasperrow;
			framePosY += m_critter_retinasize;
		}
		framePosX = (pos * m_critter_retinasize);

		// Calculate where in the Great Retina this critter shold start (column & row)
		unsigned int target = visionPosition;
		retinaRowStart = 0;

		// determine on which row of the retina to start for this critter
		retinaRowLength = m_retinasperrow * m_critter_retinasize * 4;

		// determine on which column to start
		while ( target >= m_retinasperrow )
		{
			retinaRowStart += m_critter_retinasize * retinaRowLength;
			target -= m_retinasperrow;
		}
		retinaColumnStart = target * m_critter_retinasize * 4;
	}

	CdVisionSystem::~CdVisionSystem()
	{
		free(retina);
	}
	

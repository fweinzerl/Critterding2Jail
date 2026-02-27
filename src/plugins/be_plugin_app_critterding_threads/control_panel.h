#ifndef CONTROL_PANEL_H_INCLUDED
#define CONTROL_PANEL_H_INCLUDED

#include "kernel/be_entity_interface.h"

	class CdControlPanel: public BEntity
	{
		public:
			CdControlPanel() : BEntity()
			{
				setProcessing();
			};
			virtual ~CdControlPanel() {};
			const char* class_id() const { return "CdControlPanel"; }

			virtual void construct();
			virtual void process();
		private:
			BEntity* m_critter_unit_containers;
			BEntity* m_brain_unit_containers;

			BEntity* m_line_edit_sim_speed_fps;
			BEntity* m_line_edit_sim_steps_per_sec;
			BEntity* m_line_edit_total_neurons;
			BEntity* m_line_edit_total_synapses;
			BEntity* m_line_edit_avg_neurons;
			BEntity* m_line_edit_avg_synapses;

			BEntity* m_timer_frame;
			BEntity* m_timer_ms_total;
			BEntity* m_sleeper_target_fps;
			Buint m_last_frame;
			Buint m_last_ms_total;
	};

#endif

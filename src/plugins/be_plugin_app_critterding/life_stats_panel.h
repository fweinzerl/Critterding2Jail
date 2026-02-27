#ifndef LIFE_STATS_PANEL_H_INCLUDED
#define LIFE_STATS_PANEL_H_INCLUDED

#include "kernel/be_entity_interface.h"

class CdLifeStatsPanel : public BEntity
{
	public:
		CdLifeStatsPanel() : BEntity() { setProcessing(); }
		virtual ~CdLifeStatsPanel() {}
		const char* class_id() const { return "CdLifeStatsPanel"; }

		virtual void construct();
		virtual void process();

	private:
		BEntity* m_critter_unit_containers;
		BEntity* m_food_unit_containers;
		BEntity* m_critter_settings_refs;
		BEntity* m_critter_stats_refs;
		BEntity* m_food_stats_refs;
		BEntity* m_timer_frame;

		BEntity* m_label_generations;
		BEntity* m_label_critters;
		BEntity* m_label_food;
		BEntity* m_label_critter_births;
		BEntity* m_label_critter_deaths;
		BEntity* m_label_food_births;
		BEntity* m_label_food_deaths;
		BEntity* m_label_avg_adam_distance;
		BEntity* m_label_critter_energy_total;
		BEntity* m_label_food_energy_total;
		BEntity* m_label_critter_extinct;
		BEntity* m_label_food_extinct;
};

#endif

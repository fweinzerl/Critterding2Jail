#include "life_stats_panel.h"
#include "kernel/be_entity_core_types.h"
#include <iomanip>
#include <sstream>

void CdLifeStatsPanel::construct()
{
	setFps(2);

	auto critterding = topParent()->getChild("bin", 1)->getChild("Critterding", 1);
	m_critter_unit_containers = addChild("critter_unit_containers", new BEntity());
	m_food_unit_containers = addChild("food_unit_containers", new BEntity());
	m_critter_settings_refs = addChild("critter_settings_refs", new BEntity());
	m_critter_stats_refs = addChild("critter_stats_refs", new BEntity());
	m_food_stats_refs = addChild("food_stats_refs", new BEntity());

	bool found_threaded_layout = false;
	auto& critterding_children = critterding->children();
	for (auto* child : critterding_children)
	{
		if (child->class_id() == std::string("thread"))
		{
			auto local_cd = child->getChild("Critterding", 1);
			if (!local_cd)
			{
				continue;
			}
			auto critter_system = local_cd->getChild("critter_system", 1);
			auto food_system = local_cd->getChild("food_system", 1);
			if (!critter_system || !food_system)
			{
				continue;
			}
			auto critter_settings = critter_system->getChild("settings", 1);
			auto food_settings = food_system->getChild("settings", 1);
			if (!critter_settings || !food_settings)
			{
				continue;
			}

			m_critter_unit_containers->addChild("unit_container_ref", new BEntity_reference())->set(critter_system->getChild("unit_container", 1));
			m_food_unit_containers->addChild("unit_container_ref", new BEntity_reference())->set(food_system->getChild("unit_container", 1));
			m_critter_settings_refs->addChild("settings_ref", new BEntity_reference())->set(critter_settings);
			m_critter_stats_refs->addChild("stats_ref", new BEntity_reference())->set(critter_settings->getChild("stats", 1));
			m_food_stats_refs->addChild("stats_ref", new BEntity_reference())->set(food_settings->getChild("stats", 1));
			found_threaded_layout = true;
		}
	}

	// Fallback for single-scene Critterding layout.
	if (!found_threaded_layout)
	{
		auto critter_system = critterding->getChild("critter_system", 1);
		auto food_system = critterding->getChild("food_system", 1);
		if (critter_system && food_system)
		{
			auto critter_settings = critter_system->getChild("settings", 1);
			auto food_settings = food_system->getChild("settings", 1);
			m_critter_unit_containers->addChild("unit_container_ref", new BEntity_reference())->set(critter_system->getChild("unit_container", 1));
			m_food_unit_containers->addChild("unit_container_ref", new BEntity_reference())->set(food_system->getChild("unit_container", 1));
			m_critter_settings_refs->addChild("settings_ref", new BEntity_reference())->set(critter_settings);
			m_critter_stats_refs->addChild("stats_ref", new BEntity_reference())->set(critter_settings->getChild("stats", 1));
			m_food_stats_refs->addChild("stats_ref", new BEntity_reference())->set(food_settings->getChild("stats", 1));
		}
	}

	auto found_sys = topParent()->getChild("sys", 1);
	auto timer = found_sys->getChild("timer", 1);
	m_timer_frame = timer->getChild("frame", 1);

	auto qwindow = addChild("QT MainWindow", "QMainWindow");
	qwindow->addChild("title", "string_property")->set("Critterding Life Stats");
	qwindow->addChild("x", "uint_property")->set(Buint(980));
	qwindow->addChild("y", "uint_property")->set(Buint(40));
	qwindow->addChild("width", "uint_property")->set(Buint(460));
	qwindow->addChild("height", "uint_property")->set(Buint(420));
	qwindow->set("on_close_destroy_entity", this);

	auto root_layout = qwindow->addChild("root_layout", "QVBoxLayout");
	auto stats_box = root_layout->addChild("stats_box", "QGroupBox");
	stats_box->set("title", "Life Statistics");
	auto layout_V = stats_box->addChild("stats_layout", "QVBoxLayout");

	auto add_stat_row = [layout_V](const char* id, const char* title, BEntity*& out_label)
	{
		auto row = layout_V->addChild(id, "QHBoxLayout");
		auto text = row->addChild("text", "QLabel");
		text->set("text", title);
		text->set("width", Buint(220));
		out_label = row->addChild("value", "QLabel");
		out_label->set("width", Buint(180));
		out_label->set("height", Buint(28));
		out_label->set("-");
	};

	add_stat_row("row_generations", "generations", m_label_generations);
	add_stat_row("row_critters", "critters", m_label_critters);
	add_stat_row("row_food", "food", m_label_food);
	add_stat_row("row_critter_births", "critter births", m_label_critter_births);
	add_stat_row("row_critter_deaths", "critter deaths", m_label_critter_deaths);
	add_stat_row("row_food_births", "food births", m_label_food_births);
	add_stat_row("row_food_deaths", "food deaths", m_label_food_deaths);
	add_stat_row("row_avg_adam_distance", "avg adam distance", m_label_avg_adam_distance);
	add_stat_row("row_critter_energy_total", "critter energy total", m_label_critter_energy_total);
	add_stat_row("row_food_energy_total", "food energy total", m_label_food_energy_total);
	add_stat_row("row_critter_extinct", "critter extinct", m_label_critter_extinct);
	add_stat_row("row_food_extinct", "food extinct", m_label_food_extinct);
}

void CdLifeStatsPanel::process()
{
	auto format_float = [](float value)
	{
		std::ostringstream ss;
		ss << std::fixed << std::setprecision(2) << value;
		return ss.str();
	};

	Buint critters = 0;
	Buint food = 0;

	Buint total_adam_distance = 0;
	float critter_energy_total = 0.0f;
	auto& critter_unit_refs = m_critter_unit_containers->children();
	for (auto* unit_ref : critter_unit_refs)
	{
		auto container = unit_ref->get_reference();
		if (!container)
		{
			continue;
		}
		critters += container->numChildren();
		auto& critter_children = container->children();
		for (auto* critter : critter_children)
		{
			total_adam_distance += critter->getChild("adam_distance", 1)->get_uint();
			critter_energy_total += critter->getChild("energy", 1)->get_float();
		}
	}

	float food_energy_total = 0.0f;
	auto& food_unit_refs = m_food_unit_containers->children();
	for (auto* unit_ref : food_unit_refs)
	{
		auto container = unit_ref->get_reference();
		if (!container)
		{
			continue;
		}
		food += container->numChildren();
		auto& food_children = container->children();
		for (auto* food_item : food_children)
		{
			food_energy_total += food_item->getChild("energy", 1)->get_float();
		}
	}

	Buint critter_births = 0;
	Buint critter_deaths = 0;
	auto& critter_stats_refs = m_critter_stats_refs->children();
	for (auto* stats_ref : critter_stats_refs)
	{
		auto stats = stats_ref->get_reference();
		if (!stats)
		{
			continue;
		}
		critter_births += stats->getChild("births_total", 1)->get_uint();
		critter_deaths += stats->getChild("deaths_total", 1)->get_uint();
	}

	Buint food_births = 0;
	Buint food_deaths = 0;
	auto& food_stats_refs = m_food_stats_refs->children();
	for (auto* stats_ref : food_stats_refs)
	{
		auto stats = stats_ref->get_reference();
		if (!stats)
		{
			continue;
		}
		food_births += stats->getChild("births_total", 1)->get_uint();
		food_deaths += stats->getChild("deaths_total", 1)->get_uint();
	}

	float avg_maximum_age = 0.0f;
	Buint settings_count = 0;
	auto& critter_settings_refs = m_critter_settings_refs->children();
	for (auto* settings_ref : critter_settings_refs)
	{
		auto settings = settings_ref->get_reference();
		if (!settings)
		{
			continue;
		}
		avg_maximum_age += settings->getChild("maximum_age", 1)->get_uint();
		++settings_count;
	}
	if (settings_count > 0)
	{
		avg_maximum_age /= settings_count;
	}

	const auto generations = (avg_maximum_age > 0.0f)
		? float(m_timer_frame->get_uint()) / avg_maximum_age
		: 0.0f;
	const auto avg_adam_distance = (critters > 0)
		? float(total_adam_distance) / critters
		: 0.0f;

	m_label_generations->set(format_float(generations).c_str());
	m_label_critters->set(Buint(critters));
	m_label_food->set(Buint(food));
	m_label_critter_births->set(critter_births);
	m_label_critter_deaths->set(critter_deaths);
	m_label_food_births->set(food_births);
	m_label_food_deaths->set(food_deaths);
	m_label_avg_adam_distance->set(format_float(avg_adam_distance).c_str());
	m_label_critter_energy_total->set(format_float(critter_energy_total).c_str());
	m_label_food_energy_total->set(format_float(food_energy_total).c_str());
	m_label_critter_extinct->set((critters == 0) ? "yes" : "no");
	m_label_food_extinct->set((food == 0) ? "yes" : "no");
}

#include "control_panel.h"
#include "kernel/be_entity_core_types.h"
#include <iomanip>
#include <sstream>

void CdControlPanel::construct()
{
	setFps(2);

	auto critterding = topParent()->getChild("bin", 1)->getChild("Critterding", 1);

	m_critter_unit_containers = addChild("critter_unit_containers", new BEntity());
	m_brain_unit_containers = addChild("brain_unit_containers", new BEntity());
	auto critter_settings_refs = addChild("critter_settings_refs", new BEntity());
	auto food_settings_refs = addChild("food_settings_refs", new BEntity());
	auto brain_settings_refs = addChild("brain_settings_refs", new BEntity());

	for_all_children_of(critterding)
	{
		if ((*child)->class_id() == std::string("thread"))
		{
			auto local_cd = (*child)->getChild("Critterding", 1);
			auto critter_system = local_cd->getChild("critter_system", 1);
			auto food_system = local_cd->getChild("food_system", 1);
			auto brain_system = critter_system->getChild("brain_system", 1);
			auto critter_settings = critter_system->getChild("settings", 1);
			auto food_settings = food_system->getChild("settings", 1);
			auto brain_settings = brain_system->getChild("settings", 1);

			m_critter_unit_containers->addChild("unit_container_ref", new BEntity_reference())->set(critter_system->getChild("unit_container", 1));
			m_brain_unit_containers->addChild("unit_container_ref", new BEntity_reference())->set(brain_system->getChild("unit_container", 1));
			critter_settings_refs->addChild("settings_ref", new BEntity_reference())->set(critter_settings);
			food_settings_refs->addChild("settings_ref", new BEntity_reference())->set(food_settings);
			brain_settings_refs->addChild("settings_ref", new BEntity_reference())->set(brain_settings);
		}
	}

	auto found_sys = topParent()->getChild("sys", 1);
	auto timer = found_sys->getChild("timer", 1);
	m_timer_frame = timer->getChild("frame", 1);
	m_timer_ms_total = timer->getChild("ms_total", 1);
	auto sleeper = found_sys->getChild("sleeper", 1);
	m_sleeper_target_fps = sleeper->getChild("target_fps", 1);
	m_last_frame = m_timer_frame->get_uint();
	m_last_ms_total = m_timer_ms_total->get_uint();

	auto qwindow = addChild("QT MainWindow", "QMainWindow");
	qwindow->addChild("title", "string_property")->set("Critterding Control Panel");
	qwindow->addChild("x", "uint_property")->set(Buint(650));
	qwindow->addChild("y", "uint_property")->set(Buint(40));
	qwindow->addChild("width", "uint_property")->set(Buint(820));
	qwindow->addChild("height", "uint_property")->set(Buint(640));
	qwindow->set("on_close_destroy_entity", this);

	auto root_layout = qwindow->addChild("root_layout", "QVBoxLayout");

	auto settings_box = root_layout->addChild("settings_box", "QGroupBox");
	settings_box->set("title", "Settings");
	auto settings_layout = settings_box->addChild("settings_layout", "QVBoxLayout");

	auto stats_box = root_layout->addChild("stats_box", "QGroupBox");
	stats_box->set("title", "Statistics");
	auto stats_layout = stats_box->addChild("stats_layout", "QVBoxLayout");

	auto layout_H_row = settings_layout->addChild("settings_row1", "QHBoxLayout");
	auto layout_H = layout_H_row->addChild("min_critters_layout", "QHBoxLayout");
	{
		auto text_min_critters = layout_H->addChild("min_critters", "QLabel");
		text_min_critters->set("text", "minimum critters");
		text_min_critters->set("width", Buint(165));
		for_all_children_of(critter_settings_refs)
		{
			auto line_edit = layout_H->addChild("min_critters_lineedit", "QLineEdit_uint");
			line_edit->set("width", Buint(41));
			line_edit->set("height", Buint(28));
			auto value = (*child)->get_reference()->getChild("minimum_number_of_units", 1);
			line_edit->set(value->get_uint());
			value->connectServerServer(line_edit);
			line_edit->connectServerServer(value);
		}
	}

	layout_H = layout_H_row->addChild("energy_layout", "QHBoxLayout");
	{
		auto text_energy = layout_H->addChild("energy", "QLabel");
		text_energy->set("text", "energy");
		text_energy->set("width", Buint(165));
		for_all_children_of(food_settings_refs)
		{
			auto line_edit = layout_H->addChild("energy_lineedit", "QLineEdit_uint");
			line_edit->set("width", Buint(41));
			line_edit->set("height", Buint(28));
			auto value = (*child)->get_reference()->getChild("number_of_units", 1);
			line_edit->set(value->get_uint());
			value->connectServerServer(line_edit);
			line_edit->connectServerServer(value);
		}
	}

	layout_H_row = settings_layout->addChild("settings_row2", "QHBoxLayout");
	layout_H = layout_H_row->addChild("mutation_chance_layout", "QHBoxLayout");
	{
		auto text_mutation_chance = layout_H->addChild("mutation_chance", "QLabel");
		text_mutation_chance->set("text", "mutation_chance");
		text_mutation_chance->set("width", Buint(165));
		for_all_children_of(brain_settings_refs)
		{
			auto line_edit = layout_H->addChild("mutation_chance_lineedit", "QLineEdit_uint");
			line_edit->set("width", Buint(41));
			line_edit->set("height", Buint(28));
			auto value = (*child)->get_reference()->getChild("mutation_chance", 1);
			line_edit->set(value->get_uint());
			value->connectServerServer(line_edit);
			line_edit->connectServerServer(value);
		}
	}

	layout_H = layout_H_row->addChild("mutationruns_max_layout", "QHBoxLayout");
	{
		auto text_mutationruns_max = layout_H->addChild("mutationruns_max", "QLabel");
		text_mutationruns_max->set("text", "mutationruns_max");
		text_mutationruns_max->set("width", Buint(165));
		for_all_children_of(brain_settings_refs)
		{
			auto line_edit = layout_H->addChild("mutationruns_max_lineedit", "QLineEdit_uint");
			line_edit->set("width", Buint(41));
			line_edit->set("height", Buint(28));
			auto value = (*child)->get_reference()->getChild("mutationruns_max", 1);
			line_edit->set(value->get_uint());
			value->connectServerServer(line_edit);
			line_edit->connectServerServer(value);
		}
	}

	layout_H_row = settings_layout->addChild("settings_row3", "QHBoxLayout");
	layout_H = layout_H_row->addChild("sim_speed_layout", "QHBoxLayout");
	{
		auto text_sim_speed = layout_H->addChild("sim_speed", "QLabel");
		text_sim_speed->set("text", "target fps (0=unlimited)");
		text_sim_speed->set("width", Buint(165));
		m_line_edit_sim_speed_fps = layout_H->addChild("sim_speed_fps_lineedit", "QLineEdit_uint");
		m_line_edit_sim_speed_fps->set("width", Buint(84));
		m_line_edit_sim_speed_fps->set("height", Buint(28));
		m_line_edit_sim_speed_fps->set(m_sleeper_target_fps->get_uint());
		m_sleeper_target_fps->connectServerServer(m_line_edit_sim_speed_fps);
		m_line_edit_sim_speed_fps->connectServerServer(m_sleeper_target_fps);
	}

	layout_H_row = stats_layout->addChild("stats_row1", "QHBoxLayout");
	layout_H = layout_H_row->addChild("sim_steps_layout", "QHBoxLayout");
	{
		auto text_sim_steps = layout_H->addChild("sim_steps", "QLabel");
		text_sim_steps->set("text", "sim steps / sec");
		text_sim_steps->set("width", Buint(165));
		m_line_edit_sim_steps_per_sec = layout_H->addChild("sim_steps_value", "QLabel");
		m_line_edit_sim_steps_per_sec->set("width", Buint(164));
		m_line_edit_sim_steps_per_sec->set("height", Buint(28));
		m_line_edit_sim_steps_per_sec->set("0.00");
	}

	layout_H = layout_H_row->addChild("neurons_layout", "QHBoxLayout");
	{
		auto text_total_neurons = layout_H->addChild("text_total_neurons", "QLabel");
		text_total_neurons->set("text", "neurons");
		text_total_neurons->set("width", Buint(165));
		m_line_edit_total_neurons = layout_H->addChild("neurons_value", "QLabel");
		m_line_edit_total_neurons->set("width", Buint(164));
		m_line_edit_total_neurons->set("height", Buint(28));
		m_line_edit_total_neurons->set(Buint(0));
	}

	layout_H_row = stats_layout->addChild("stats_row2", "QHBoxLayout");
	layout_H = layout_H_row->addChild("synapses_layout", "QHBoxLayout");
	{
		auto text_total_synapses = layout_H->addChild("text_total_synapses", "QLabel");
		text_total_synapses->set("text", "synapses");
		text_total_synapses->set("width", Buint(165));
		m_line_edit_total_synapses = layout_H->addChild("total_synapses_value", "QLabel");
		m_line_edit_total_synapses->set("width", Buint(164));
		m_line_edit_total_synapses->set("height", Buint(28));
		m_line_edit_total_synapses->set(Buint(0));
	}

	layout_H = layout_H_row->addChild("avg_neurons_layout", "QHBoxLayout");
	{
		auto text_avg_neurons = layout_H->addChild("text_avg_neurons", "QLabel");
		text_avg_neurons->set("text", "avg neurons / critter");
		text_avg_neurons->set("width", Buint(165));
		m_line_edit_avg_neurons = layout_H->addChild("avg_neurons_value", "QLabel");
		m_line_edit_avg_neurons->set("width", Buint(164));
		m_line_edit_avg_neurons->set("height", Buint(28));
		m_line_edit_avg_neurons->set("0.00");
	}

	layout_H_row = stats_layout->addChild("stats_row3", "QHBoxLayout");
	layout_H = layout_H_row->addChild("avg_synapses_layout", "QHBoxLayout");
	{
		auto text_avg_synapses = layout_H->addChild("text_avg_synapses", "QLabel");
		text_avg_synapses->set("text", "avg synapses / neuron");
		text_avg_synapses->set("width", Buint(165));
		m_line_edit_avg_synapses = layout_H->addChild("avg_synapses_value", "QLabel");
		m_line_edit_avg_synapses->set("width", Buint(164));
		m_line_edit_avg_synapses->set("height", Buint(28));
		m_line_edit_avg_synapses->set("0.00");
	}
}

void CdControlPanel::process()
{
	auto format_float = [](float value)
	{
		std::ostringstream ss;
		ss << std::fixed << std::setprecision(2) << value;
		return ss.str();
	};

	Buint total_critters = 0;
	auto& critter_unit_container_refs = m_critter_unit_containers->children();
	for (auto* unit_ref : critter_unit_container_refs)
	{
		auto unit_container = unit_ref->get_reference();
		if (unit_container)
		{
			total_critters += unit_container->numChildren();
		}
	}

	Buint neurons = 0;
	Buint synapses = 0;
	auto& brain_unit_container_refs = m_brain_unit_containers->children();
	for (auto* unit_ref : brain_unit_container_refs)
	{
		auto brain_unit_container = unit_ref->get_reference();
		if (!brain_unit_container)
		{
			continue;
		}
		auto& brains = brain_unit_container->children();
		for (auto* brain : brains)
		{
			auto neuron_container = brain->getChild("neurons", 1);
			neurons += neuron_container->numChildren();
			auto& neurons_in_brain = neuron_container->children();
			for (auto* neuron : neurons_in_brain)
			{
				auto synapse_container = neuron->getChild("synapses", 1);
				synapses += synapse_container->numChildren();
			}
		}
	}

	m_line_edit_total_neurons->set(neurons);
	m_line_edit_total_synapses->set(synapses);
	m_line_edit_avg_neurons->set(format_float((total_critters > 0) ? float(neurons) / total_critters : 0.0f).c_str());
	m_line_edit_avg_synapses->set(format_float((neurons > 0) ? float(synapses) / neurons : 0.0f).c_str());

	const Buint current_frame = m_timer_frame->get_uint();
	const Buint current_ms_total = m_timer_ms_total->get_uint();
	if (current_ms_total > m_last_ms_total)
	{
		const auto delta_frame = current_frame - m_last_frame;
		const auto delta_ms = current_ms_total - m_last_ms_total;
		const auto sim_steps_per_sec = 1000.0f * (float)delta_frame / (float)delta_ms;
		m_line_edit_sim_steps_per_sec->set(format_float(sim_steps_per_sec).c_str());
	}
	m_last_frame = current_frame;
	m_last_ms_total = current_ms_total;
}

#include "control_panel.h"
#include <iomanip>
#include <sstream>

void CdControlPanel::construct()
{
	setFps(2);

	// Find scene systems and frequently used entities.
	auto critterding = topParent()->getChild("bin", 1)->getChild("Critterding", 1);
	auto critter_system = critterding->getChild("critter_system", 1);
	auto food_system = critterding->getChild("food_system", 1);
	auto brain_system = critter_system->getChild("brain_system", 1);
	auto critter_settings = critter_system->getChild("settings", 1);
	auto food_settings = food_system->getChild("settings", 1);
	auto brain_settings = brain_system->getChild("settings", 1);

	m_critter_unit_container = critter_system->getChild("unit_container", 1);
	m_brain_unit_container = brain_system->getChild("unit_container", 1);

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
	qwindow->addChild("x", "uint_property")->set(Buint(460));
	qwindow->addChild("y", "uint_property")->set(Buint(40));
	qwindow->addChild("width", "uint_property")->set(Buint(720));
	qwindow->addChild("height", "uint_property")->set(Buint(620));
	qwindow->set("on_close_destroy_entity", this);

	auto root_layout = qwindow->addChild("root_layout", "QVBoxLayout");

	auto settings_box = root_layout->addChild("settings_box", "QGroupBox");
	settings_box->set("title", "Settings");
	auto settings_layout = settings_box->addChild("settings_layout", "QVBoxLayout");

	auto stats_box = root_layout->addChild("stats_box", "QGroupBox");
	stats_box->set("title", "Statistics");
	auto stats_layout = stats_box->addChild("stats_layout", "QVBoxLayout");

	// SETTINGS ROW 1
	auto layout_H_row = settings_layout->addChild("settings_row1", "QHBoxLayout");
	auto layout_H = layout_H_row->addChild("min_critters_layout", "QHBoxLayout");
	{
		auto text_min_critters = layout_H->addChild("min_critters", "QLabel");
		text_min_critters->set("text", "minimum critters");
		text_min_critters->set("width", Buint(165));
		auto line_edit_min_critters = layout_H->addChild("min_critters_lineedit", "QLineEdit_uint");
		line_edit_min_critters->set("width", Buint(84));
		line_edit_min_critters->set("height", Buint(28));
		auto critter_number_of_units = critter_settings->getChild("minimum_number_of_units", 1);
		line_edit_min_critters->set(critter_number_of_units->get_uint());
		critter_number_of_units->connectServerServer(line_edit_min_critters);
		line_edit_min_critters->connectServerServer(critter_number_of_units);
	}

	layout_H = layout_H_row->addChild("energy_layout", "QHBoxLayout");
	{
		auto text_energy = layout_H->addChild("energy", "QLabel");
		text_energy->set("text", "energy");
		text_energy->set("width", Buint(165));
		auto line_edit_energy = layout_H->addChild("energy_lineedit", "QLineEdit_uint");
		line_edit_energy->set("width", Buint(84));
		line_edit_energy->set("height", Buint(28));
		auto food_number_of_units = food_settings->getChild("number_of_units", 1);
		line_edit_energy->set(food_number_of_units->get_uint());
		food_number_of_units->connectServerServer(line_edit_energy);
		line_edit_energy->connectServerServer(food_number_of_units);
	}

	// SETTINGS ROW 2
	layout_H_row = settings_layout->addChild("settings_row2", "QHBoxLayout");
	layout_H = layout_H_row->addChild("mutation_chance_layout", "QHBoxLayout");
	{
		auto text_mutation_chance = layout_H->addChild("mutation_chance", "QLabel");
		text_mutation_chance->set("text", "mutation_chance");
		text_mutation_chance->set("width", Buint(165));
		auto line_edit_mutation_chance = layout_H->addChild("mutation_chance_lineedit", "QLineEdit_uint");
		line_edit_mutation_chance->set("width", Buint(84));
		line_edit_mutation_chance->set("height", Buint(28));
		auto brain_mutation_chance = brain_settings->getChild("mutation_chance", 1);
		line_edit_mutation_chance->set(brain_mutation_chance->get_uint());
		brain_mutation_chance->connectServerServer(line_edit_mutation_chance);
		line_edit_mutation_chance->connectServerServer(brain_mutation_chance);
	}

	layout_H = layout_H_row->addChild("mutationruns_max_layout", "QHBoxLayout");
	{
		auto text_mutationruns_max = layout_H->addChild("mutationruns_max", "QLabel");
		text_mutationruns_max->set("text", "mutationruns_max");
		text_mutationruns_max->set("width", Buint(165));
		auto line_edit_mutationruns_max = layout_H->addChild("mutationruns_max_lineedit", "QLineEdit_uint");
		line_edit_mutationruns_max->set("width", Buint(84));
		line_edit_mutationruns_max->set("height", Buint(28));
		auto brain_mutationruns_max = brain_settings->getChild("mutationruns_max", 1);
		line_edit_mutationruns_max->set(brain_mutationruns_max->get_uint());
		brain_mutationruns_max->connectServerServer(line_edit_mutationruns_max);
		line_edit_mutationruns_max->connectServerServer(brain_mutationruns_max);
	}

	// SETTINGS ROW 3
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

	// STATS ROW 1
	layout_H_row = stats_layout->addChild("stats_row1", "QHBoxLayout");
	layout_H = layout_H_row->addChild("sim_steps_layout", "QHBoxLayout");
	{
		auto text_sim_steps = layout_H->addChild("sim_steps", "QLabel");
		text_sim_steps->set("text", "sim steps / sec");
		text_sim_steps->set("width", Buint(165));
		m_line_edit_sim_steps_per_sec = layout_H->addChild("sim_steps_value", "QLabel");
		m_line_edit_sim_steps_per_sec->set("width", Buint(84));
		m_line_edit_sim_steps_per_sec->set("height", Buint(28));
		m_line_edit_sim_steps_per_sec->set("0.00");
	}

	layout_H = layout_H_row->addChild("neurons_layout", "QHBoxLayout");
	{
		auto text_total_neurons = layout_H->addChild("text_total_neurons", "QLabel");
		text_total_neurons->set("text", "neurons");
		text_total_neurons->set("width", Buint(165));
		m_line_edit_total_neurons = layout_H->addChild("neurons_value", "QLabel");
		m_line_edit_total_neurons->set("width", Buint(84));
		m_line_edit_total_neurons->set("height", Buint(28));
		m_line_edit_total_neurons->set(Buint(0));
	}

	// STATS ROW 2
	layout_H_row = stats_layout->addChild("stats_row2", "QHBoxLayout");
	layout_H = layout_H_row->addChild("synapses_layout", "QHBoxLayout");
	{
		auto text_total_synapses = layout_H->addChild("text_total_synapses", "QLabel");
		text_total_synapses->set("text", "synapses");
		text_total_synapses->set("width", Buint(165));
		m_line_edit_total_synapses = layout_H->addChild("total_synapses_value", "QLabel");
		m_line_edit_total_synapses->set("width", Buint(84));
		m_line_edit_total_synapses->set("height", Buint(28));
		m_line_edit_total_synapses->set(Buint(0));
	}

	layout_H = layout_H_row->addChild("avg_neurons_layout", "QHBoxLayout");
	{
		auto text_avg_neurons = layout_H->addChild("text_avg_neurons", "QLabel");
		text_avg_neurons->set("text", "avg neurons / critter");
		text_avg_neurons->set("width", Buint(165));
		m_line_edit_avg_neurons = layout_H->addChild("avg_neurons_value", "QLabel");
		m_line_edit_avg_neurons->set("width", Buint(84));
		m_line_edit_avg_neurons->set("height", Buint(28));
		m_line_edit_avg_neurons->set("0.00");
	}

	// STATS ROW 3
	layout_H_row = stats_layout->addChild("stats_row3", "QHBoxLayout");
	layout_H = layout_H_row->addChild("avg_synapses_layout", "QHBoxLayout");
	{
		auto text_avg_synapses = layout_H->addChild("text_avg_synapses", "QLabel");
		text_avg_synapses->set("text", "avg synapses / neuron");
		text_avg_synapses->set("width", Buint(165));
		m_line_edit_avg_synapses = layout_H->addChild("avg_synapses_value", "QLabel");
		m_line_edit_avg_synapses->set("width", Buint(84));
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

	const Buint critters = m_critter_unit_container->numChildren();

	Buint neurons = 0;
	Buint synapses = 0;
	for_all_children_of(m_brain_unit_container)
	{
		auto neuron_container = (*child)->getChild("neurons", 1);
		neurons += neuron_container->numChildren();
		for_all_children_of2(neuron_container)
		{
			auto synapse_container = (*child2)->getChild("synapses", 1);
			synapses += synapse_container->numChildren();
		}
	}

	m_line_edit_total_neurons->set(neurons);
	m_line_edit_total_synapses->set(synapses);
	m_line_edit_avg_neurons->set(format_float((critters > 0) ? float(neurons) / critters : 0.0f).c_str());
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

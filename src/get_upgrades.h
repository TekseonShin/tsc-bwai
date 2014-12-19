

namespace get_upgrades {
;
a_unordered_map<upgrade_type*, double> upgrade_value_overrides;
void set_upgrade_value(upgrade_type*upg, double value) {
	upgrade_value_overrides[upg] = value;
}
bool no_auto_upgrades = false;
void set_no_auto_upgrades(bool val) {
	for (auto&b : build::build_tasks) {
		if (b.type->upgrade && !b.upgrade_done_frame) {
			b.dead = true;
		}
	}
	no_auto_upgrades = val;
}

void get_upgrades() {

	for (auto&b : build::build_tasks) {
		if (b.type->upgrade && !b.upgrade_done_frame) {
			b.dead = true;
		}
	}

	a_unordered_set<unit_type*> visited;
	for (auto&upg : upgrades::upgrade_type_container) {
		if (upg.gas_cost && current_gas_per_frame == 0) continue;
		if (upg.prev && !players::my_player->upgrades.count(upg.prev)) continue;
		if (players::my_player->upgrades.count(&upg)) continue;
		double sum = 0.0;
		for (unit_type*ut : upg.what_uses) {
			if (ut->is_worker) continue;
			double val = ut->total_minerals_cost + ut->total_gas_cost;
			sum += my_units_of_type[ut].size()*val;
		}
		double val = upg.minerals_cost + upg.gas_cost;
		visited.clear();
		std::function<void(unit_type*)> add = [&](unit_type*ut) {
			if (!visited.insert(ut).second) return;
			if (!my_units_of_type[ut].empty()) return;
			val += ut->minerals_cost + ut->gas_cost;
			for (unit_type*req : ut->required_units) add(req);
		};
		for (unit_type*req : upg.required_units) add(req);
		if (upgrade_value_overrides[&upg]) val = upgrade_value_overrides[&upg];
		if (val >= 0 && no_auto_upgrades) continue;
		if (sum >= val*1.5) {
			bool already_upgrading = false;
			for (build::build_task&b : build::build_tasks) {
				if (b.type->upgrade == &upg) {
					already_upgrading = true;
					break;
				}
			}
			if (!already_upgrading) {
				bool builder_found = false;
				for (unit*u : my_units_of_type[upg.builder_type]) {
					if (u->remaining_whatever_time) continue;
					builder_found = true;
					break;
				}
				double prio = val / 1000.0;
				if (builder_found) {
					build::add_build_sum(prio, &upg, 1);
					break;
				} else {
					if (sum >= val * 3) {
						build::add_build_sum(prio, upg.builder_type, 1);
						break;
					}
				}
			}
		}

	}

}

void get_upgrades_task() {

	while (true) {
		multitasking::sleep(60);

		get_upgrades();

	}

}

void init() {

	multitasking::spawn(get_upgrades_task, "get upgrades");

}

}


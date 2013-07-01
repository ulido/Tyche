#ifndef OUTPUT_IMPL_H_
#define OUTPUT_IMPL_H_

namespace Tyche {
template<typename T>
void OutputCompareWithFunction<T>::integrate(const double dt) {


	if (is_execute_time() && get_time() > start_time) {
		LOG(2, "Starting Operator: " << *this);
		const int n = get_species().size();
		ASSERT(n==1,"OutputConcentrations only implemented for one species");
		Species &s = *(get_species()[0]);
		std::vector<double> concentrations;
		s.get_concentration(grid,concentrations);
		ASSERT(grid.size()==concentrations.size(), "copy numbers size is not the same as grid!");

		std::vector<Vect3d> centers;
		std::vector<double> volumes;
		const int ngrid = concentrations.size();
		for (int i = 0; i < ngrid; ++i) {
			centers.push_back(grid.get_cell_centre(i));
			volumes.push_back(grid.get_cell_volume(i));
		}
		errors.push_back(calc_error(get_time(), concentrations,centers,volumes));
		if (errors.size() >= average_over) {
			data["Time"].push_back(get_time());
			data["Error"].push_back(std::accumulate(errors.begin(),errors.end(),0.0)/double(average_over));
			for (std::map<std::string,double>::const_iterator i = params.begin(); i != params.end(); i++) {
				data[i->first].push_back(i->second);
			}
			errors.clear();
			write();
		}

	}


}


template<typename T>
void OutputCompareWithFunction<T>::set_param(const std::string name, const double value) {
	std::map<std::string, double>::iterator val = params.find(name);
	if (val==params.end()) {
		ASSERT(data.begin()->second.size() == 0, "Can only add new parameters if no data exists");
		params[name] = value;
		data.insert(std::pair<std::string,std::vector<double> >(name,std::vector<double>()));
	} else {
		val->second = value;
	}
}

template<typename T>
void OutputCompareWithFunction<T>::reset_execute() {
	errors.clear();
}
}

#endif /* OUTPUT_IMPL_H_ */

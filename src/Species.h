/*
 * species.h
 *
 * Copyright 2012 Martin Robinson
 *
  * This file is part of RD_3D.
 *
 * RD_3D is free software: you can redistribute it and/or modify
 * it under the terms of the GNU Lesser General Public License as published by
 * the Free Software Foundation, either version 3 of the License, or
 * (at your option) any later version.
 *
 * RD_3D is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU Lesser General Public License for more details.
 *
 * You should have received a copy of the GNU Lesser General Public License
 * along with RD_3D.  If not, see <http://www.gnu.org/licenses/>.
 *
 *  Created on: 11 Oct 2012
 *      Author: robinsonm
 */

#ifndef SPECIES_H_
#define SPECIES_H_

#include <vector>
#include <boost/foreach.hpp>
#include "MyRandom.h"
#include "Vector.h"
#include "StructuredGrid.h"

#include <vtkUnstructuredGrid.h>
#include <vtkSmartPointer.h>


namespace Tyche {

#define DATA_typename MolData
#define DATA_names   (r)(r0)(alive)(id)(saved_index)
#define DATA_types   (Vect3d)(Vect3d)(bool)(int)(int)
#include "Data.h"


class Molecules: public MolData {
public:
	Molecules() {
		next_id = 0;
	}
	void fill_uniform(const Vect3d low, const Vect3d high, const unsigned int N);
	int delete_molecule(const unsigned int i);
	int delete_molecules();
	int add_molecule(const Vect3d& position);
	int add_molecule(const Vect3d& position, const Vect3d& old_position);

	int mark_for_deletion(const unsigned int i);

	void save_indicies();
	vtkSmartPointer<vtkUnstructuredGrid> get_vtk_grid();
private:
	int next_id;
};



const int SPECIES_SAVED_INDEX_FOR_NEW_PARTICLE = -1;


static const StructuredGrid empty_grid;

class Species {
public:
	Species(double D):D(D),grid(NULL) {
		id = species_count++;
		clear();
	}
	Species(double D, const StructuredGrid* grid):D(D),grid(grid)  {
		id = species_count++;
		clear();
	}
	static std::auto_ptr<Species> New(double D) {
		return std::auto_ptr<Species>(new Species(D));
	}
	void clear() {
		mols.clear();
		if (grid!=NULL) copy_numbers.assign(grid->size(),0);
	}
	void set_grid(const Grid* new_grid) {
		grid = new_grid;
		if (grid!=NULL) copy_numbers.assign(grid->size(),0);
	}
	void fill_uniform(const Vect3d low, const Vect3d high, const unsigned int N);
	void fill_uniform(const Vect3d low, const Vect3d high, const Box &interface, const unsigned int N);
	void get_concentrations(const StructuredGrid& calc_grid, std::vector<double>& mol_concentrations, std::vector<double>& compartment_concentrations) const;
	void get_concentration(const StructuredGrid& calc_grid, std::vector<double>& concentration) const;
	void get_concentration(const Vect3d low, const Vect3d high, const Vect3i n, std::vector<double>& concentration) const;
	vtkSmartPointer<vtkUnstructuredGrid> get_vtk();
	std::string get_status_string() const;
	friend std::ostream& operator<<( std::ostream& out, const Species& b ) {
		return out << b.get_status_string();
	}

	double D;
	Molecules mols;
	std::vector<int> copy_numbers;
	std::vector<int> mol_copy_numbers;
	const Grid* grid;
	int id;
	std::vector<double> tmpx,tmpy,tmpz;
private:
	static int species_count;
};


extern Species null_species;
}


#endif /* SPECIES_H_ */

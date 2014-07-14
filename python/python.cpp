/* 
 * python.cpp
 *
 * Copyright 2012 Martin Robinson
 *
 * This file is part of Tyche.
 *
 * Tyche is free software: you can redistribute it and/or modify
 * it under the terms of the GNU Lesser General Public License as published by
 * the Free Software Foundation, either version 3 of the License, or
 * (at your option) any later version.
 *
 * Tyche is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU Lesser General Public License for more details.
 *
 * You should have received a copy of the GNU Lesser General Public License
 * along with PDE_BD.  If not, see <http://www.gnu.org/licenses/>.
 *
 *  Created on: Feb 9, 2013
 *      Author: mrobins
 */


#include <boost/python.hpp>
#include <boost/python/suite/indexing/vector_indexing_suite.hpp>
#include <numpy/ndarrayobject.h>
#include "Tyche.h"
#include <numpy/arrayobject.h>
#include <boost/python/tuple.hpp>

#include <vtkUnstructuredGrid.h>
#include <vtkSmartPointer.h>


#include "StructuredGrid.h"
#include "OctreeGrid.h"


using namespace boost::python;

namespace Tyche {

struct ReactionEquation_from_python_list
{

	ReactionEquation_from_python_list() {
		boost::python::converter::registry::push_back(
				&convertible,
				&construct,
				boost::python::type_id<ReactionEquation>());
	}

	static void* convertible(PyObject* obj_ptr) {
		if (!PyList_Check(obj_ptr)) return 0;
		if (PyList_Size(obj_ptr) != 2) return 0;
		if (!PyList_Check(PyList_GetItem(obj_ptr,0)) || !PyList_Check(PyList_GetItem(obj_ptr,1))) return 0;
		return obj_ptr;
	}

	static void construct(
			PyObject* obj_ptr,
			boost::python::converter::rvalue_from_python_stage1_data* data) {

		// Extract the character data from the python string
		PyObject* reactants = PyList_GetItem(obj_ptr,0);
		PyObject* products = PyList_GetItem(obj_ptr,1);
		const int num_reactants = PyList_Size(reactants);
		const int num_products = PyList_Size(products);


		ReactionSide lhs;
		for (int i = 0; i < num_reactants; ++i) {
			Species* s = extract<Species*>(PyList_GetItem(reactants,i));
			lhs = lhs + *s;
		}
		ReactionSide rhs;
		for (int i = 0; i < num_products; ++i) {
			Species* s = extract<Species*>(PyList_GetItem(products,i));
			rhs = rhs + *s;
		}
		// Grab pointer to memory into which to construct the new QString
		void* storage = (
				(boost::python::converter::rvalue_from_python_storage<ReactionEquation>*)
				data)->storage.bytes;

		// in-place construct the new QString using the character data
		// extraced from the python object
		new (storage) ReactionEquation(lhs,rhs);

		// Stash the memory chunk pointer for later use by boost.python
		data->convertible = storage;
	}

};

template<typename T>
struct Vect3_from_python_list
{

	Vect3_from_python_list() {
		boost::python::converter::registry::push_back(
				&convertible,
				&construct,
				boost::python::type_id<Eigen::Matrix<T,3,1> >());
	}

	static void* convertible(PyObject* obj_ptr) {
		if (!PyList_Check(obj_ptr)) return 0;
		if (PyList_Size(obj_ptr) != 3) return 0;
		return obj_ptr;
	}

	static void construct(
			PyObject* obj_ptr,
			boost::python::converter::rvalue_from_python_stage1_data* data) {
		const int size = PyList_Size(obj_ptr);

		// Extract the character data from the python string
		const double x = extract<T>(PyList_GetItem(obj_ptr,0));
		const double y = extract<T>(PyList_GetItem(obj_ptr,1));
		const double z = extract<T>(PyList_GetItem(obj_ptr,2));


		// Grab pointer to memory into which to construct the new QString
		void* storage = (
				(boost::python::converter::rvalue_from_python_storage<Eigen::Matrix<T,3,1> >*)
				data)->storage.bytes;

		// in-place construct the new QString using the character data
		// extraced from the python object
		new (storage) Eigen::Matrix<T,3,1>(x,y,z);

		// Stash the memory chunk pointer for later use by boost.python
		data->convertible = storage;
	}

};

template<class T>
struct VecToList
{
    static PyObject* convert(const std::vector<T>& vec)
    {
        boost::python::list* l = new boost::python::list();
        for(size_t i = 0; i < vec.size(); i++)
            (*l).append(vec[i]);

        return l->ptr();
    }
};

void python_init(boost::python::list& py_argv) {
	using boost::python::len;

	int argc = len(py_argv);
	char **argv = new char*[len(py_argv)];
	for (int i = 0; i < len(py_argv); ++i) {
		std::string this_argv = boost::python::extract<std::string>(py_argv[i]);
		argv[i] = new char[this_argv.size()];
		std::strcpy(argv[i],this_argv.c_str());
	}
	init(argc,argv);
	for (int i = 0; i < len(py_argv); ++i) {
		delete argv[i];
	}
	delete argv;

	// register the Vect3-to-python converter


}


BOOST_PYTHON_FUNCTION_OVERLOADS(new_uni_reaction_overloads, UniMolecularReaction::New, 2, 3);


std::auto_ptr<Operator> new_bi_reaction(const double rate, const ReactionEquation& eq,
					const double binding,
					const double unbinding,
					const double dt,
					const Vect3d& min, const Vect3d& max, const Vect3b& periodic,
					const bool reversible=false) {

	return BiMolecularReaction<BucketSort>::New(rate,eq,binding,unbinding,dt,min,max,periodic,reversible);
}

BOOST_PYTHON_FUNCTION_OVERLOADS(new_bi_reaction_overloads, new_bi_reaction, 8, 9);

std::auto_ptr<Operator> new_bi_reaction2(const double rate, const ReactionEquation& eq,
					const double dt,
					const Vect3d& min, const Vect3d& max, const Vect3b& periodic,
					const bool reversible=false) {

	return BiMolecularReaction<BucketSort>::New(rate,eq,dt,min,max,periodic,reversible);
}

BOOST_PYTHON_FUNCTION_OVERLOADS(new_bi_reaction_overloads2, new_bi_reaction2, 6, 7);


boost::python::numeric::array Species_get_compartments(Species& self) {
  if (self.grid!=NULL) {
    Vect3i grid_size = self.grid->get_cells_along_axes();
    npy_intp size[3] = {grid_size[0],grid_size[1],grid_size[2]};
    PyObject *out = PyArray_SimpleNew(3, size, NPY_INT);
    const StructuredGrid *sgrid = dynamic_cast<const StructuredGrid*>(self.grid);
    if (sgrid!=NULL) {
      for (int i = 0; i < grid_size[0]; ++i) {
	for (int j = 0; j < grid_size[1]; ++j) {
	  for (int k = 0; k < grid_size[2]; ++k) {
	    *((int *)PyArray_GETPTR3(out, i, j, k)) = self.copy_numbers[sgrid->vect_to_index(i,j,k)];
	  }
	}
      }
    } else {
      const OctreeGrid *ogrid = dynamic_cast<const OctreeGrid*>(self.grid);
      for (int i = 0; i < grid_size[0]; ++i) {
	for (int j = 0; j < grid_size[1]; ++j) {
	  for (int k = 0; k < grid_size[2]; ++k) {
	    int num = 0;
	    for (auto ind : ogrid->get_leaf_indices(Vect3i(i,j,k)))
	      num += self.copy_numbers[ind];
	    *((int *)PyArray_GETPTR3(out, i, j, k)) = num;
	  }
	}
      }
    }

    boost::python::handle<> handle(out);
    boost::python::numeric::array arr(handle);
    return arr;
  }
  return boost::python::numeric::array(0);
}

void Species_set_compartments(Species& self,boost::python::numeric::array array) {
	PyObject *in = array.ptr();
	if (self.grid!=NULL) {
		Vect3i grid_size = self.grid->get_cells_along_axes();
		npy_intp size[3] = {grid_size[0],grid_size[1],grid_size[2]};

		CHECK(PyArray_NDIM(in)==3,"Python array dimensions does not match compartments");
		CHECK((PyArray_DIMS(in)[0]==size[0])&&(PyArray_DIMS(in)[1]==size[1])&&(PyArray_DIMS(in)[2]==size[2]),"shape of Python array dimensions does not match compartments");
		const StructuredGrid *sgrid = dynamic_cast<const StructuredGrid*>(self.grid);
		if (sgrid!=NULL) {
			for (int i = 0; i < grid_size[0]; ++i) {
				for (int j = 0; j < grid_size[1]; ++j) {
					for (int k = 0; k < grid_size[2]; ++k) {
						self.copy_numbers[sgrid->vect_to_index(i,j,k)] = *((int *)PyArray_GETPTR3(in,i,j,k));
					}
				}
			}
		} else {
			ERROR("set_compartments not implemented for oct-tree grids");
		}
	}
}

boost::python::tuple Species_get_particles(Species& self) {
	const int n = self.mols.size();

	npy_intp size = {n};
	PyObject *out_x = PyArray_SimpleNew(1, &size, NPY_DOUBLE);
	PyObject *out_y = PyArray_SimpleNew(1, &size, NPY_DOUBLE);
	PyObject *out_z = PyArray_SimpleNew(1, &size, NPY_DOUBLE);

	for (int i = 0; i < n; ++i) {
		*((double *)PyArray_GETPTR1(out_x,i)) = self.mols.r[i][0];
		*((double *)PyArray_GETPTR1(out_y,i)) = self.mols.r[i][1];
		*((double *)PyArray_GETPTR1(out_z,i)) = self.mols.r[i][2];
	}
	return boost::python::make_tuple(boost::python::handle<>(out_x),boost::python::handle<>(out_y),boost::python::handle<>(out_z));
}

boost::python::object Species_get_concentration1(Species& self, const Vect3d& min, const Vect3d& max, const Vect3i& n) {
	const Vect3d spacing = (max-min).cwiseQuotient(n.cast<double>());
	std::vector<double> concentration;
	StructuredGrid calc_grid(min,max,spacing);
	self.get_concentration(calc_grid,concentration);
	Vect3i grid_size = calc_grid.get_cells_along_axes();
	npy_intp size[3] = {grid_size[0],grid_size[1],grid_size[2]};

	PyObject *out = PyArray_SimpleNew(3, size, NPY_DOUBLE);
	for (int i = 0; i < grid_size[0]; ++i) {
		for (int j = 0; j < grid_size[1]; ++j) {
			for (int k = 0; k < grid_size[2]; ++k) {
				const int index = calc_grid.vect_to_index(i,j,k);
				*((double *)PyArray_GETPTR3(out, i, j, k)) = concentration[index];
			}
		}
	}
	boost::python::handle<> handle(out);
	boost::python::numeric::array arr(handle);
	return arr;
//	std::cout <<"c after set PyObject"<<std::endl;
//	object obj(handle<>(out));
//	return extract<numeric::array>(obj);
}

void Species_fill_uniform(Species &self, const Vect3d &low, const Vect3d &high, unsigned int N) {
  self.fill_uniform(low, high, N);
}

void Species_fill_uniform_interface(Species &self, const Vect3d &low, const Vect3d &high, const Box interface, unsigned int N) {
  self.fill_uniform(low, high, interface, N);
}

struct BR_Python_Callback {
  boost::python::object py_callable;
  boost::python::tuple args;
  void operator()(double time, std::vector<unsigned int> state) {
    py_callable(time, boost::python::list(state), args);
  }
};

void BindingReaction_set_state_changed_cb(BindingReaction& self, boost::python::object& callable, boost::python::tuple& args)
{
  BR_Python_Callback cb {callable, args};
  self.set_state_changed_cb(boost::function<void(double, std::vector<unsigned int>)>(cb));
}

std::auto_ptr<Operator> group(const boost::python::list& ops) {
	OperatorList* result = new OperatorList();
	const int n = len(ops);
	for (int i = 0; i < n; ++i) {
		Operator *s = extract<Operator*>(ops[i]);
		result->push_back(s);
	}
	return std::auto_ptr<Operator>(result);
}

boost::python::list OctreeGrid_get_visualisation_boxes(OctreeGrid& self)
{
  std::vector<Octree*> cells = self.get_cells();
  boost::python::list ret;
  for (auto c : cells) {
    const Vect3d center = c->get_center();
    auto pcenter = boost::python::make_tuple(center[0],center[1],center[2]);
    boost::python::list neighbours;
    for (int n : c->get_neighbour_indices())
      neighbours.append(n);
    ret.append(boost::python::make_tuple(c->get_cell_index(), pcenter, c->get_edge_length(), neighbours));
  }
  return ret;
}

boost::python::list OctreeGrid_get_concentration(OctreeGrid& self, Species& s)
{
  std::vector<Octree*> cells = self.get_cells();
  boost::python::list ret;
  for (int i = 0; i < cells.size(); i++)
    ret.append(s.copy_numbers[i]/self.get_cell_volume(i));
  return ret;
}

boost::python::list Grid_get_slice(Grid &self, const Geometry& geometry)
{
  std::vector<int> slice_indices;
  boost::python::list ret;
  self.get_slice(geometry, slice_indices);
  for (auto i : slice_indices)
    ret.append(i);
  return ret;
}

boost::python::list Grid_get_neighbour_indicies(Grid &self, const unsigned int i)
{
  std::vector<int> neighbours = self.get_neighbour_indicies(i);
  boost::python::list ret;
  for (auto i : neighbours)
    ret.append(i);
  return ret;
}

template<class T>
struct vtkSmartPointer_to_python {
	static PyObject *convert(const vtkSmartPointer<T> &p) {
		std::ostringstream oss;
		oss << (void*) p.GetPointer();
		std::string address_str = oss.str();

		using namespace boost::python;
		object obj = import("vtk").attr("vtkObject")(address_str);
		return incref(obj.ptr());
	}
};

//
// This python to C++ converter uses the fact that VTK Python objects have an
// attribute called __this__, which is a string containing the memory address
// of the VTK C++ object and its class name.
// E.g. for a vtkPoints object __this__ might be "_0000000105a64420_p_vtkPoints"
//
void* extract_vtk_wrapped_pointer(PyObject* obj)
{
    char thisStr[] = "__this__";
    //first we need to get the __this__ attribute from the Python Object
    if (!PyObject_HasAttrString(obj, thisStr))
        return NULL;

    PyObject* thisAttr = PyObject_GetAttrString(obj, thisStr);
    if (thisAttr == NULL)
        return NULL;

    char* str = PyString_AsString(thisAttr);
    if(str == 0 || strlen(str) < 1)
        return NULL;

    char hex_address[32], *pEnd;
    char *_p_ = strstr(str, "_p_vtk");
    if(_p_ == NULL) return NULL;
    char *class_name = strstr(_p_, "vtk");
    if(class_name == NULL) return NULL;
    strcpy(hex_address, str+1);
    hex_address[_p_-str-1] = '\0';

    long address = strtol(hex_address, &pEnd, 16);

    vtkObjectBase* vtk_object = (vtkObjectBase*)((void*)address);
    std::cout <<"check if vtk_object is a "<<class_name<<std::endl;
    if(vtk_object->IsA(class_name))
    {
        std::cout <<"yup, it is"<<std::endl;

        return vtk_object;
    }

    return NULL;
}


#define VTK_PYTHON_CONVERSION(type) \
    /* register the to-python converter */ \
    to_python_converter<vtkSmartPointer<type>,vtkSmartPointer_to_python<type> >(); \
    /* register the from-python converter */ \
    converter::registry::insert(&extract_vtk_wrapped_pointer, type_id<type>());


std::auto_ptr<NextSubvolumeMethod> (*NSM_New1)(const Vect3d&, const Vect3d&, const Vect3d&) = &NextSubvolumeMethod::New;
std::auto_ptr<NextSubvolumeMethod> (*NSM_New2)(Grid&) = &NextSubvolumeMethod::New;
void (NextSubvolumeMethod::*NSM_add_diffusion_between)(Species&, const double, Geometry&, Geometry&) = &NextSubvolumeMethod::add_diffusion_between;
void (NextSubvolumeMethod::*NSM_scale_diffusion_across)(Species&, Geometry&, const double) = &NextSubvolumeMethod::scale_diffusion_across;

bool (Grid::*Grid_is_in)(const Geometry&, const int) const = &Grid::is_in;

std::auto_ptr<Species> (*Species_New_double)(double) = Species::New;
std::auto_ptr<Species> (*Species_New_Vect3d)(Vect3d) = Species::New;

BOOST_PYTHON_MODULE(pyTyche) {
	import_array();
	numeric::array::set_module_and_type("numpy", "ndarray");
	def("init", python_init);
	def("random_seed", random_seed);


	/*
	 * vector
	 */
	class_<std::vector<double> >("Vectd")
	        .def(boost::python::vector_indexing_suite<std::vector<double> >())
	    ;

	VTK_PYTHON_CONVERSION(vtkUnstructuredGrid);
	VTK_PYTHON_CONVERSION(vtkPolyData);


	Vect3_from_python_list<double>();
	Vect3_from_python_list<int>();
	Vect3_from_python_list<bool>();
	ReactionEquation_from_python_list();

	to_python_converter<std::vector<unsigned int>, VecToList<unsigned int> >();

	/*
	 * Species
	 */

	class_<Species,typename std::auto_ptr<Species> >("Species",boost::python::init<double>())
			.def("fill_uniform",Species_fill_uniform)
			.def("fill_uniform",Species_fill_uniform_interface)
			.def("get_concentration",Species_get_concentration1)
			.def("get_vtk",&Species::get_vtk)
			.def("get_compartments",Species_get_compartments,
					"Returns numpy (3-dimensional) array with a copy of the current copy numbers in each compartment")
			.def("set_compartments",Species_set_compartments,args("input"),
					"Sets the compartment copy numbers for this species equal to the input numpy array. Note: use NextSubvolumeMethod.reset_all_propensities() to recaculate propensities and next reaction times")
			.def("get_particles",Species_get_particles)
			.def(self_ns::str(self_ns::self))
			;
	def("new_species",Species_New_double);
	def("new_species",Species_New_Vect3d);


   /*
    * Operator
    */
	class_<Operator,typename std::auto_ptr<Operator> >("Operator",boost::python::no_init)
			.def("integrate",&Operator::operator())
			.def("reset",&Operator::reset)
			.def("add_species",&Operator::add_species)
			.def("get_species_index",&Operator::get_species_index)
			.def("integrate_for_time",&Operator::integrate_for_time)
	                .def("get_active", &Operator::get_active)
	                .def("set_active", &Operator::set_active)
			.def(self_ns::str(self_ns::self))
			;

	def("group",group);



	/*
	 * Geometry
	 */
	def("new_xplane",xplane::New);
	def("new_yplane",yplane::New);
	def("new_zplane",zplane::New);
	def("new_xrect",xrect::New);
	def("new_yrect",xrect::New);
	def("new_zrect",xrect::New);
	def("new_xcylinder",xcylinder::New);
	def("new_ycylinder",ycylinder::New);
	def("new_zcylinder",zcylinder::New);
	def("new_vtkGeometry",vtkGeometry::New);

	class_<Geometry, boost::noncopyable, typename std::auto_ptr<Geometry> >("Geometry", boost::python::no_init);

	class_<xplane, bases<Geometry>,typename std::auto_ptr<xplane> >("Xplane",boost::python::no_init);
	class_<yplane, bases<Geometry>,typename std::auto_ptr<yplane> >("Yplane",boost::python::no_init);
	class_<zplane, bases<Geometry>,typename std::auto_ptr<zplane> >("Zplane",boost::python::no_init);

	class_<xrect, bases<Geometry>,typename std::auto_ptr<xrect> >("Xrect",boost::python::no_init);
	class_<yrect, bases<Geometry>,typename std::auto_ptr<yrect> >("Yrect",boost::python::no_init);
	class_<zrect, bases<Geometry>,typename std::auto_ptr<zrect> >("Zrect",boost::python::no_init);

	class_<xcylinder,typename std::auto_ptr<xcylinder> >("Xcylinder",boost::python::no_init);
	class_<ycylinder,typename std::auto_ptr<ycylinder> >("Ycylinder",boost::python::no_init);
	class_<zcylinder,typename std::auto_ptr<zcylinder> >("Zcylinder",boost::python::no_init);

	class_<vtkGeometry,typename std::auto_ptr<vtkGeometry> >("vtkGeometry",boost::python::no_init);


	def("new_box", Box::New);
	def("new_multiple_boxes", MultipleBoxes::New);

	class_<Box, bases<Geometry>, typename std::auto_ptr<Box> >("Box",boost::python::no_init)
						;
	class_<MultipleBoxes, bases<Geometry>, typename std::auto_ptr<MultipleBoxes> >("MultipleBoxes",boost::python::no_init)
				.def("add_hole",&MultipleBoxes::add_box)
				;

	/*
	 * Boundaries
	 */
	def("new_coupling_boundary",CouplingBoundary<xplane>::New);
	def("new_coupling_boundary",CouplingBoundary<yplane>::New);
	def("new_coupling_boundary",CouplingBoundary<zplane>::New);
	def("new_coupling_boundary",CouplingBoundary<xrect>::New);
	def("new_coupling_boundary",CouplingBoundary<yrect>::New);
	def("new_coupling_boundary",CouplingBoundary<Box>::New);


    def("new_reflective_boundary",ReflectiveBoundary<xplane>::New);
    def("new_reflective_boundary",ReflectiveBoundary<yplane>::New);
    def("new_reflective_boundary",ReflectiveBoundary<zplane>::New);
    def("new_reflective_boundary",ReflectiveBoundary<xrect>::New);
    def("new_reflective_boundary",ReflectiveBoundary<yrect>::New);
    def("new_reflective_boundary",ReflectiveBoundary<zrect>::New);
    def("new_reflective_boundary",ReflectiveBoundary<xcylinder>::New);
    def("new_reflective_boundary",ReflectiveBoundary<ycylinder>::New);
    def("new_reflective_boundary",ReflectiveBoundary<zcylinder>::New);
    def("new_reflective_boundary",ReflectiveBoundary<vtkGeometry>::New);


    def("new_jump_boundary",JumpBoundary<xplane>::New);
    def("new_jump_boundary",JumpBoundary<yplane>::New);
    def("new_jump_boundary",JumpBoundary<zplane>::New);
    def("new_jump_boundary",JumpBoundary<xrect>::New);
    def("new_jump_boundary",JumpBoundary<yrect>::New);
    def("new_jump_boundary",JumpBoundary<zrect>::New);


    /*
     * Diffusion
     */
    def("new_diffusion",Diffusion::New);
    def("new_diffusion_with_tracking",DiffusionWithTracking<xplane>::New);
    def("new_diffusion_with_tracking",DiffusionWithTracking<yplane>::New);
    def("new_diffusion_with_tracking",DiffusionWithTracking<zplane>::New);
    def("new_diffusion_with_tracking",DiffusionWithTracking<xrect>::New);
    def("new_diffusion_with_tracking",DiffusionWithTracking<yrect>::New);
    def("new_diffusion_with_tracking",DiffusionWithTracking<Box>::New);

    /*
     * Reactions
     */
    def("new_zero_reaction",ZeroOrderMolecularReaction::New);


    def("new_uni_reaction",UniMolecularReaction::New, new_uni_reaction_overloads());

    def("new_bi_reaction",new_bi_reaction, new_bi_reaction_overloads());
    def("new_bi_reaction",new_bi_reaction2, new_bi_reaction_overloads2());
    def("new_tri_reaction",TriMolecularReaction::New);
    def("new_binding_reaction", BindingReaction::New);

	class_<BindingReaction, bases<Operator>, std::auto_ptr<BindingReaction> >("BindingReaction", boost::python::no_init)
		.def("get_site_state", &BindingReaction::get_site_state)
	        .def("set_state_changed_cb", &BindingReaction_set_state_changed_cb);

    /*
     * Compartments
     */
	class_<Grid, boost::noncopyable, typename std::auto_ptr<Grid> >("Grid", boost::python::no_init)
	  .def("is_in", Grid_is_in)
	  .def("get_neighbour_indicies", Grid_get_neighbour_indicies)
	  .def("get_slice", Grid_get_slice);
	class_<StructuredGrid, bases<Grid>, std::auto_ptr<StructuredGrid> >("StructuredGrid", boost::python::no_init);
	def("new_structured_grid", &StructuredGrid::New);

	class_<OctreeGrid, bases<Grid>, std::auto_ptr<OctreeGrid> >("OctreeGrid", boost::python::no_init)
	  .def("refine", &OctreeGrid::refine)
	  .def("finalise", &OctreeGrid::finalise)
	  .def("get_concentration", &OctreeGrid_get_concentration)
	  .def("get_visualisation_boxes", OctreeGrid_get_visualisation_boxes);
	def("new_octree_grid", &OctreeGrid::New);
	
    def("new_compartments",NSM_New1);
    def("new_compartments",NSM_New2);

    /*
     * NextSubvolume
     */
    class_<NextSubvolumeMethod, bases<Operator>, std::auto_ptr<NextSubvolumeMethod> >("NextSubvolumeMethod",boost::python::no_init)
      .def("list_reactions",&NextSubvolumeMethod::list_reactions)
    	.def("set_interface",&NextSubvolumeMethod::set_interface)
    	.def("unset_interface",&NextSubvolumeMethod::unset_interface)
    	.def("set_ghost_cell_interface",&NextSubvolumeMethod::set_ghost_cell_interface)
    	.def("add_diffusion",&NextSubvolumeMethod::add_diffusion)
    	.def("add_diffusion_between",NSM_add_diffusion_between)
    	.def("add_reaction",&NextSubvolumeMethod::add_reaction)
    	.def("add_reaction_on",&NextSubvolumeMethod::add_reaction_on)
    	.def("scale_diffusion_across",NSM_scale_diffusion_across)
    	.def("fill_uniform",&NextSubvolumeMethod::fill_uniform)
    	.def("reset_all_propensities",&NextSubvolumeMethod::reset_all_priorities,
    			"Recalculates the propensities and next reaction times for all compartments")
    	;

}

}

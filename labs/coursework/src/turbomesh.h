#pragma once

#include "geometry.h"
#include "material.h"
#include "stdafx.h"
#include "transform.h"

namespace graphics_framework {
/*
An object combining a transform, geometry and material to create a renderable
turbomesh object
*/
class turbomesh {
private:
  // The transform object of the turbomesh
  transform _transform;
  // The geometry object of the turbomesh
  geometry _geometry;
  // The material object of the turbomesh
  material _material;
  // The minimal of the AABB defining the turbomesh
  glm::vec3 _minimal;
  // The maximal of the AABB defining the turbomesh
  glm::vec3 _maximal;





  // Parent for hierarchies
  turbomesh* _parent = nullptr;






public:
  // Creates a turbomesh object
  turbomesh() {}
  // Creates a turbomesh object with the provided geometry
  explicit turbomesh(geometry &geom)
      : _geometry(geom), _minimal(geom.get_minimal_point()), _maximal(geom.get_maximal_point()) {}
  // Creates a turbomesh object with the provided geometry and material
  turbomesh(geometry &geom, material &mat)
      : _geometry(geom), _minimal(geom.get_minimal_point()), _maximal(geom.get_maximal_point()) {}
  // Default copy constructor and assignment operator
  turbomesh(const turbomesh &other) = default;
  turbomesh &operator=(const turbomesh &rhs) = default;
  // Destroys the turbomesh object
  ~turbomesh() {}
  // Gets the transform object for the turbomesh
  transform &get_transform() { return _transform; }
  // Gets the geometry object for the turbomesh
  const geometry &get_geometry() const { return _geometry; }
  // Sets the geometry object for the turbomesh
  void set_geometry(const geometry &value) { _geometry = value; }
  // Gets the material object for the turbomesh
  material &get_material() { return _material; }
  // Sets the material object for the turbomesh
  void set_material(const material &value) { _material = value; }
  // Gets the minimal point of the AABB defining the turbomesh
  glm::vec3 get_minimal() const { return _minimal * _transform.scale; }
  // Gets the maximal point of the AABB defining the mes
  glm::vec3 get_maximal() const { return _maximal * _transform.scale; }



  // Gets the parent pointer
  turbomesh* get_parent() { return _parent; }
  // Gets the parent pointer
  void set_parent(turbomesh* parent) { _parent = parent; }

  // Gets the transform matrix for an object is a hierarchy
  glm::mat4 get_hierarchical_transform_matrix()
  {
	  glm::mat4 M = _transform.get_transform_matrix();
	  turbomesh *current = this;
	  while (current->get_parent() != nullptr)
	  {
		  current = current->get_parent();
		  M = current->get_transform().get_transform_matrix() * M;
	  }
	  return M;
  }

  // Gets the normal matrix for an object is a hierarchy
  glm::mat3 get_hierarchical_normal_matrix()
  {
	  glm::mat3 N = _transform.get_normal_matrix();
	  turbomesh *current = this;
	  while (current->get_parent() != nullptr)
	  {
		  current = current->get_parent();
		  N = current->get_transform().get_normal_matrix() * N;
	  }
	  return N;
  }


};
}
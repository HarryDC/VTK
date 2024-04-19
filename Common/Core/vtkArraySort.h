// SPDX-FileCopyrightText: Copyright (c) Ken Martin, Will Schroeder, Bill Lorensen
// SPDX-FileCopyrightText: Copyright 2008 Sandia Corporation
// SPDX-License-Identifier: LicenseRef-BSD-3-Clause-Sandia-USGov

/**
 * @class   vtkArraySort
 * @brief   Controls sorting of sparse array coordinates.
 *
 *
 * vtkArraySort stores an ordered set of dimensions along which the
 * values stored in a sparse array should be sorted.
 *
 * Convenience constructors are provided for specifying one, two, and
 * three dimensions.  To sort along more than three dimensions, use the
 * default constructor, SetDimensions(), and operator[] to assign each
 * dimension to be sorted.
 *
 * @sa
 * vtkSparseArray
 *
 * @par Thanks:
 * Developed by Timothy M. Shead (tshead@sandia.gov) at Sandia National
 * Laboratories.
 */

#ifndef vtkArraySort_h
#define vtkArraySort_h

#include "vtkArrayCoordinates.h"
#include "vtkCommonCoreModule.h" // For export macro
#include "vtkSystemIncludes.h"
#include <vector>

VTK_ABI_NAMESPACE_BEGIN
class VTKCOMMONCORE_EXPORT vtkArraySort
{
public:
  typedef vtkArrayCoordinates::DimensionT DimensionT;

  /**
   * Create an empty set of dimensions.  Use SetDimensions() and
   * operator[] to populate them.
   */
  vtkArraySort();

  /**
   * Sorts an array along one dimension.
   */
  explicit vtkArraySort(DimensionT i);

  /**
   * Sorts an array along two dimensions.
   */
  vtkArraySort(DimensionT i, DimensionT j);

  /**
   * Sorts an array along three dimensions.
   */
  vtkArraySort(DimensionT i, DimensionT j, DimensionT k);

  /**
   * Return the number of dimensions for sorting.
   */
  DimensionT GetDimensions() const;

  /**
   * Set the number of dimensions to be sorted.  Note that this method
   * resets every dimension to zero, so you must set every dimension
   * explicitly using operator[] after calling SetDimensions().
   */
  void SetDimensions(DimensionT dimensions);

  /**
   * Returns the i-th dimension to be sorted.
   */
  DimensionT& operator[](DimensionT i);

  /**
   * Returns the i-th dimension to be sorted.
   */
  const DimensionT& operator[](DimensionT i) const;

  /**
   * Equality comparison
   */
  bool operator==(const vtkArraySort& rhs) const;

  /**
   * Inequality comparison
   */
  bool operator!=(const vtkArraySort& rhs) const;

  /**
   * Serialization
   */
  VTKCOMMONCORE_EXPORT friend ostream& operator<<(ostream& stream, const vtkArraySort& rhs);

private:
  std::vector<DimensionT> Storage;
};

VTK_ABI_NAMESPACE_END
#endif

// VTK-HeaderTest-Exclude: vtkArraySort.h

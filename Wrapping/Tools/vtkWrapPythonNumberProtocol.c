// SPDX-FileCopyrightText: Copyright (c) Ken Martin, Will Schroeder, Bill Lorensen
// SPDX-License-Identifier: BSD-3-Clause
#include "vtkWrapPythonNumberProtocol.h"

#include <string.h>

/* Overrides __rshift__ operator for vtkAlgorithm and vtkDataObject */
int vtkWrapPython_GenerateNumberProtocolDefintions(FILE* fp, ClassInfo* classInfo)
{
  const int isVtkAlgorithm = strcmp("vtkAlgorithm", classInfo->Name) == 0;
  const int isVtkDataObject = strcmp("vtkDataObject", classInfo->Name) == 0;
  if (!(isVtkAlgorithm || isVtkDataObject))
  {
    return 0;
  }
  fprintf(fp,
    "static PyNumberMethods Py%s_NumberMethods[] = {\n"
    "  {\n"
    "    nullptr, // nb_add\n"
    "    nullptr, // nb_subtract\n"
    "    nullptr, // nb_multiply\n"
    "    nullptr, // nb_remainder\n"
    "    nullptr, // nb_divmod\n"
    "    nullptr, // nb_power\n"
    "    nullptr, // nb_negative\n"
    "    nullptr, // nb_positive\n"
    "    nullptr, // nb_absolute\n"
    "    nullptr, // nb_bool\n"
    "    nullptr, // nb_invert\n"
    "    nullptr, // nb_lshift\n",
    classInfo->Name);
  // lhs >> rhs is equivalent to:
  // def func(lhs: vtkDataObject, rhs: vtkAlgorithm):
  //   rhs.input_data_object = lhs
  //   return rhs
  fprintf(fp,
    "    [](PyObject* lhs, PyObject* rhs) -> PyObject*\n"
    "    {\n"
    "      // Import the module\n"
    "      PyObject *moduleName = PyUnicode_DecodeFSDefault(\"vtkmodules.util.execution_model\");\n"
    "      PyObject *internalModule = PyImport_Import(moduleName);\n"
    "      Py_DECREF(moduleName);\n"
    "      PyObject *pipeline = nullptr;\n"
    "      if (internalModule != nullptr)\n"
    "      {\n"
    "        // Get the class from the module\n"
    "        PyObject *pipelineClass = PyObject_GetAttrString(internalModule, \"Pipeline\");\n"
    "        if (pipelineClass != nullptr)\n"
    "        {\n"
    "          // Create an instance of the class\n"
    "          auto args = PyTuple_Pack(2, lhs, rhs); // Pass any arguments required by your "
    "constructor\n"
    "          pipeline = PyObject_CallObject(pipelineClass, args);\n"
    "          Py_XDECREF(args);\n"
    "          if (pipeline == nullptr)\n"
    "          {\n"
    "            return nullptr;\n"
    "          }\n"
    "          Py_DECREF(pipelineClass);\n"
    "        }\n"
    "        else\n"
    "        {\n"
    "           return nullptr;\n"
    "        }\n"
    "        Py_DECREF(internalModule);\n"
    "      }\n"
    "      else\n"
    "      {\n"
    "        return nullptr;\n"
    "      }\n"
    "      return pipeline;\n"
    "    }, // nb_rshift\n");

  fprintf(fp,
    "    nullptr, // nb_and\n"
    "    nullptr, // nb_xor\n"
    "    nullptr, // nb_or\n"
    "    nullptr, // nb_int\n"
    "    nullptr, // nb_reserved;\n"
    "    nullptr, // nb_float\n"
    "    nullptr, // nb_inplace_add\n"
    "    nullptr, // nb_inplace_subtract\n"
    "    nullptr, // nb_inplace_multiply\n"
    "    nullptr, // nb_inplace_remainder\n"
    "    nullptr, // nb_inplace_power\n"
    "    nullptr, // nb_inplace_lshift\n"
    "    nullptr, // nb_inplace_rshift\n"
    "    nullptr, // nb_inplace_and\n"
    "    nullptr, // nb_inplace_xor\n"
    "    nullptr, // nb_inplace_or\n"
    "    nullptr, // nb_floor_divide\n"
    "    nullptr, // nb_true_divide\n"
    "    nullptr, // nb_inplace_floor_divide\n"
    "    nullptr, // nb_inplace_true_divide\n"
    "    nullptr, // nb_index\n"
    "    nullptr, // nb_matrix_multiply\n"
    "    nullptr, // nb_inplace_matrix_multiply\n"
    "  },\n"
    "  { // sentinel\n"
    "    nullptr, // nb_add\n"
    "    nullptr, // nb_subtract\n"
    "    nullptr, // nb_multiply\n"
    "    nullptr, // nb_remainder\n"
    "    nullptr, // nb_divmod\n"
    "    nullptr, // nb_power\n"
    "    nullptr, // nb_negative\n"
    "    nullptr, // nb_positive\n"
    "    nullptr, // nb_absolute\n"
    "    nullptr, // nb_bool\n"
    "    nullptr, // nb_invert\n"
    "    nullptr, // nb_lshift\n"
    "    nullptr, // nb_rshift\n"
    "    nullptr, // nb_and\n"
    "    nullptr, // nb_xor\n"
    "    nullptr, // nb_or\n"
    "    nullptr, // nb_int\n"
    "    nullptr, // nb_reserved;\n"
    "    nullptr, // nb_float\n"
    "    nullptr, // nb_inplace_add\n"
    "    nullptr, // nb_inplace_subtract\n"
    "    nullptr, // nb_inplace_multiply\n"
    "    nullptr, // nb_inplace_remainder\n"
    "    nullptr, // nb_inplace_power\n"
    "    nullptr, // nb_inplace_lshift\n"
    "    nullptr, // nb_inplace_rshift\n"
    "    nullptr, // nb_inplace_and\n"
    "    nullptr, // nb_inplace_xor\n"
    "    nullptr, // nb_inplace_or\n"
    "    nullptr, // nb_floor_divide\n"
    "    nullptr, // nb_true_divide\n"
    "    nullptr, // nb_inplace_floor_divide\n"
    "    nullptr, // nb_inplace_true_divide\n"
    "    nullptr, // nb_index\n"
    "    nullptr, // nb_matrix_multiply\n"
    "    nullptr, // nb_inplace_matrix_multiply\n"
    "  }\n"
    "};\n");
  return 1;
}

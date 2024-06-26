// SPDX-FileCopyrightText: Copyright (c) Ken Martin, Will Schroeder, Bill Lorensen
// SPDX-License-Identifier: BSD-3-Clause

#include "vtkActor.h"
#include "vtkElevationFilter.h"
#include "vtkGLTFExporter.h"
#include "vtkNew.h"
#include "vtkPolyDataMapper.h"
#include "vtkRenderWindow.h"
#include "vtkRenderer.h"
#include "vtkSphereSource.h"
#include "vtkTestUtilities.h"
#include <vtksys/SystemTools.hxx>

#include <cstdlib>

namespace
{
size_t fileSize(const std::string& filename)
{
  size_t size = 0;
  FILE* f = vtksys::SystemTools::Fopen(filename, "r");
  if (f)
  {
    fseek(f, 0, SEEK_END);
    size = ftell(f);
    fclose(f);
  }
  else
  {
    std::cerr << "Error: cannot open file " << filename << std::endl;
  }

  return size;
}
}

int TestGLTFExporter(int argc, char* argv[])
{
  char* tempDir =
    vtkTestUtilities::GetArgOrEnvOrDefault("-T", argc, argv, "VTK_TEMP_DIR", "Testing/Temporary");
  if (!tempDir)
  {
    std::cout << "Could not determine temporary directory.\n";
    return EXIT_FAILURE;
  }
  std::string testDirectory = tempDir;
  delete[] tempDir;

  std::string filename = testDirectory + std::string("/") + std::string("Export");

  vtkNew<vtkSphereSource> sphere;
  vtkNew<vtkElevationFilter> elev;
  elev->SetInputConnection(sphere->GetOutputPort());
  vtkNew<vtkPolyDataMapper> mapper;
  mapper->SetInputConnection(elev->GetOutputPort());
  vtkNew<vtkActor> actor;
  actor->SetMapper(mapper);
  vtkNew<vtkRenderer> renderer;
  renderer->AddActor(actor);
  vtkNew<vtkRenderWindow> window;
  window->AddRenderer(renderer);
  window->Render();

  filename += ".gltf";

  mapper->SetInterpolateScalarsBeforeMapping(true); // To generate texture
  vtkNew<vtkGLTFExporter> exporter;
  exporter->SetRenderWindow(window);
  exporter->SetFileName(filename.c_str());
  exporter->InlineDataOn();
  // for debugging uncomment below
  // std::string estring = exporter->WriteToString();
  // cout << estring;
  exporter->Write();

  size_t correctSize = fileSize(filename);
  if (correctSize == 0)
  {
    return EXIT_FAILURE;
  }
  exporter->SetSaveNaNValues(false);
  exporter->Write();
  size_t noNaNValueSize = fileSize(filename);
  // GLTF File size and not texture file size.
  size_t correctNoNaNValueSize = correctSize - 16;
  if (noNaNValueSize != correctNoNaNValueSize)
  {
    std::cerr
      << "Error: file should not contain NaN value color in texture, when SaveNaNValues is false"
      << std::endl;
    return EXIT_FAILURE;
  }
  exporter->SetSaveNaNValues(true);

  actor->VisibilityOff();
  exporter->Write();
  size_t noDataSize = fileSize(filename);
  if (noDataSize == 0)
  {
    return EXIT_FAILURE;
  }

  if (noDataSize >= correctSize)
  {
    std::cerr << "Error: file should contain data for a visible actor"
                 "and not for a hidden one."
              << std::endl;
    return EXIT_FAILURE;
  }

  actor->VisibilityOn();
  actor->SetMapper(nullptr);
  exporter->Write();
  size_t size = fileSize(filename);
  if (size == 0)
  {
    return EXIT_FAILURE;
  }
  if (size > noDataSize)
  {
    std::cerr << "Error: file should not contain geometry"
                 " (actor has no mapper)"
              << std::endl;
    return EXIT_FAILURE;
  }

  actor->SetMapper(mapper);
  mapper->RemoveAllInputConnections(0);
  exporter->Write();
  size = fileSize(filename);
  if (size == 0)
  {
    return EXIT_FAILURE;
  }
  if (size > noDataSize)
  {
    std::cerr << "Error: file should not contain geometry"
                 " (mapper has no input)"
              << std::endl;
    return EXIT_FAILURE;
  }

  return EXIT_SUCCESS;
}

// SPDX-FileCopyrightText: Copyright (c) Ken Martin, Will Schroeder, Bill Lorensen
// SPDX-License-Identifier: BSD-3-Clause
#include "vtkPProbeFilter.h"

#include "vtkCellData.h"
#include "vtkCharArray.h"
#include "vtkCompositeDataPipeline.h"
#include "vtkIdTypeArray.h"
#include "vtkInformation.h"
#include "vtkInformationVector.h"
#include "vtkMultiProcessController.h"
#include "vtkObjectFactory.h"
#include "vtkPointData.h"
#include "vtkPolyData.h"
#include "vtkStreamingDemandDrivenPipeline.h"

VTK_ABI_NAMESPACE_BEGIN
vtkStandardNewMacro(vtkPProbeFilter);

vtkCxxSetObjectMacro(vtkPProbeFilter, Controller, vtkMultiProcessController);

//------------------------------------------------------------------------------
vtkPProbeFilter::vtkPProbeFilter()
{
  this->Controller = nullptr;
  this->SetController(vtkMultiProcessController::GetGlobalController());
}

//------------------------------------------------------------------------------
vtkPProbeFilter::~vtkPProbeFilter()
{
  this->SetController(nullptr);
}

//------------------------------------------------------------------------------
int vtkPProbeFilter::RequestData(
  vtkInformation* request, vtkInformationVector** inputVector, vtkInformationVector* outputVector)
{
  if (!this->Superclass::RequestData(request, inputVector, outputVector))
  {
    return 0;
  }

  vtkInformation* outInfo = outputVector->GetInformationObject(0);
  vtkDataSet* output = vtkDataSet::SafeDownCast(outInfo->Get(vtkDataObject::DATA_OBJECT()));

  int procid = 0;
  int numProcs = 1;
  if (this->Controller)
  {
    procid = this->Controller->GetLocalProcessId();
    numProcs = this->Controller->GetNumberOfProcesses();
  }

  vtkIdType numPoints = this->GetValidPoints()->GetNumberOfTuples();
  if (procid)
  {
    // Satellite node
    this->Controller->Send(&numPoints, 1, 0, PROBE_COMMUNICATION_TAG);
    if (numPoints > 0)
    {
      this->Controller->Send(output, 0, PROBE_COMMUNICATION_TAG);
    }
    output->ReleaseData();
  }
  else if (numProcs > 1)
  {
    vtkIdType numRemoteValidPoints = 0;
    vtkDataSet* remoteProbeOutput = output->NewInstance();
    vtkPointData* remotePointData;
    vtkPointData* pointData = output->GetPointData();
    vtkIdType i;
    vtkIdType k;
    vtkIdType pointId;
    bool validFound = false;
    if (numPoints > 0)
    {
      validFound = true;
    }
    for (i = 1; i < numProcs; i++)
    {
      this->Controller->Receive(&numRemoteValidPoints, 1, i, PROBE_COMMUNICATION_TAG);
      if (numRemoteValidPoints > 0)
      {
        this->Controller->Receive(remoteProbeOutput, i, PROBE_COMMUNICATION_TAG);

        remotePointData = remoteProbeOutput->GetPointData();

        if (!validFound)
        {
          validFound = true;
          pointData->ShallowCopy(remotePointData);
          continue;
        }
        vtkCharArray* maskArray =
          vtkArrayDownCast<vtkCharArray>(remotePointData->GetArray(this->ValidPointMaskArrayName));

        // Iterate over all point data in the output gathered from the remove
        // and copy array values from all the pointIds which have the mask array
        // bit set to 1.
        vtkIdType numRemotePoints = remoteProbeOutput->GetNumberOfPoints();
        if (output->GetNumberOfCells() != remoteProbeOutput->GetNumberOfCells() ||
          output->GetNumberOfPoints() != remoteProbeOutput->GetNumberOfPoints())
        {
          vtkErrorMacro("vtkPProbeFilter assumes the whole geometry dataset "
                        "(which determines positions to probe) is available "
                        "on all nodes, however nodes 0 is different than node "
            << i);
        }
        else if (maskArray)
        {
          for (pointId = 0; pointId < numRemotePoints; ++pointId)
          {
            if (maskArray->GetValue(pointId) == 1)
            {
              for (k = 0; k < pointData->GetNumberOfArrays(); ++k)
              {
                vtkAbstractArray* oaa = pointData->GetArray(k);
                vtkAbstractArray* raa = remotePointData->GetArray(oaa->GetName());
                if (raa != nullptr)
                {
                  oaa->SetTuple(pointId, pointId, raa);
                }
              }
            }
          }
        }
      }
    }
    remoteProbeOutput->Delete();
  }

  return 1;
}

VTK_ABI_NAMESPACE_END
#include "vtkInformationIntegerVectorKey.h"
//------------------------------------------------------------------------------
VTK_ABI_NAMESPACE_BEGIN
int vtkPProbeFilter::RequestUpdateExtent(
  vtkInformation*, vtkInformationVector** inputVector, vtkInformationVector* outputVector)
{
  vtkInformation* inInfo = inputVector[0]->GetInformationObject(0);
  vtkInformation* sourceInfo = inputVector[1]->GetInformationObject(0);
  vtkInformation* outInfo = outputVector->GetInformationObject(0);

  inInfo->Set(vtkStreamingDemandDrivenPipeline::UPDATE_PIECE_NUMBER(), 0);
  inInfo->Set(vtkStreamingDemandDrivenPipeline::UPDATE_NUMBER_OF_PIECES(), 1);
  inInfo->Set(vtkStreamingDemandDrivenPipeline::UPDATE_NUMBER_OF_GHOST_LEVELS(), 0);

  // inInfo->Set(vtkStreamingDemandDrivenPipeline::UPDATE_PIECE_NUMBER(), 0);
  // inInfo->Set(vtkStreamingDemandDrivenPipeline::UPDATE_NUMBER_OF_PIECES(), 1);
  // inInfo->Set(vtkStreamingDemandDrivenPipeline::UPDATE_NUMBER_OF_GHOST_LEVELS(),
  //            0);
  // If structured data, we want the whole extent. This is necessary because
  // the pipeline will copy the update extent from the output to all inputs.
  sourceInfo->Set(vtkStreamingDemandDrivenPipeline::UPDATE_EXTENT(),
    sourceInfo->Get(vtkStreamingDemandDrivenPipeline::WHOLE_EXTENT()), 6);
  // Then we want the same as output pieces.
  sourceInfo->Set(vtkStreamingDemandDrivenPipeline::UPDATE_PIECE_NUMBER(),
    outInfo->Get(vtkStreamingDemandDrivenPipeline::UPDATE_PIECE_NUMBER()));
  sourceInfo->Set(vtkStreamingDemandDrivenPipeline::UPDATE_NUMBER_OF_PIECES(),
    outInfo->Get(vtkStreamingDemandDrivenPipeline::UPDATE_NUMBER_OF_PIECES()));
  sourceInfo->Set(vtkStreamingDemandDrivenPipeline::UPDATE_NUMBER_OF_GHOST_LEVELS(),
    outInfo->Get(vtkStreamingDemandDrivenPipeline::UPDATE_NUMBER_OF_GHOST_LEVELS()));

  return 1;
}

//------------------------------------------------------------------------------
int vtkPProbeFilter::FillInputPortInformation(int port, vtkInformation* info)
{
  if (!this->Superclass::FillInputPortInformation(port, info))
  {
    return 0;
  }

  if (port == 1)
  {
    info->Set(vtkAlgorithm::INPUT_REQUIRED_DATA_TYPE(), "vtkDataObject");
  }
  return 1;
}

//------------------------------------------------------------------------------
void vtkPProbeFilter::PrintSelf(ostream& os, vtkIndent indent)
{
  this->Superclass::PrintSelf(os, indent);
  os << indent << "Controller " << this->Controller << endl;
}
VTK_ABI_NAMESPACE_END

// SPDX-FileCopyrightText: Copyright (c) Ken Martin, Will Schroeder, Bill Lorensen
// SPDX-License-Identifier: BSD-3-Clause
#include "vtkInformationObjectBaseVectorKey.h"
#include "vtkInformation.h" // For vtkErrorWithObjectMacro
#include "vtkSmartPointer.h"
#include <algorithm>
#include <vector>

//============================================================================
VTK_ABI_NAMESPACE_BEGIN
class vtkInformationObjectBaseVectorValue : public vtkObjectBase
{
public:
  typedef std::vector<vtkSmartPointer<vtkObjectBase>> VectorType;

  vtkBaseTypeMacro(vtkInformationObjectBaseVectorValue, vtkObjectBase);
  std::vector<vtkSmartPointer<vtkObjectBase>>& GetVector() { return this->Vector; }

private:
  std::vector<vtkSmartPointer<vtkObjectBase>> Vector;
};

//============================================================================

//------------------------------------------------------------------------------
vtkInformationObjectBaseVectorKey::vtkInformationObjectBaseVectorKey(
  const char* name, const char* location, const char* requiredClass)
  : vtkInformationKey(name, location)
  , RequiredClass(requiredClass)
{
  vtkCommonInformationKeyManager::Register(this);
}

//------------------------------------------------------------------------------
vtkInformationObjectBaseVectorKey::~vtkInformationObjectBaseVectorKey() = default;

//------------------------------------------------------------------------------
void vtkInformationObjectBaseVectorKey::PrintSelf(ostream& os, vtkIndent indent)
{
  this->Superclass::PrintSelf(os, indent);
}

//------------------------------------------------------------------------------
vtkInformationObjectBaseVectorValue* vtkInformationObjectBaseVectorKey::GetObjectBaseVector(
  vtkInformation* info)
{
  // Grab the vector associated with this key.
  vtkInformationObjectBaseVectorValue* base =
    static_cast<vtkInformationObjectBaseVectorValue*>(this->GetAsObjectBase(info));

  // If we don't already have a vector then associated,
  // we will create it here.
  if (base == nullptr)
  {
    base = new vtkInformationObjectBaseVectorValue;
    base->InitializeObjectBase();
    this->SetAsObjectBase(info, base);
    base->Delete();
  }

  return base;
}

//------------------------------------------------------------------------------
bool vtkInformationObjectBaseVectorKey::ValidateDerivedType(
  vtkInformation* info, vtkObjectBase* aValue)
{
  // verify that type of aValue is compatible with
  // this container.
  if (aValue != nullptr && this->RequiredClass != nullptr && !aValue->IsA(this->RequiredClass))
  {
    vtkErrorWithObjectMacro(info,
      "Cannot store object of type " << aValue->GetClassName() << " with key " << this->Location
                                     << "::" << this->Name << " which requires objects of type "
                                     << this->RequiredClass << ".");
    return false;
  }
  return true;
}

//------------------------------------------------------------------------------
void vtkInformationObjectBaseVectorKey::Append(vtkInformation* info, vtkObjectBase* aValue)
{
  if (!this->ValidateDerivedType(info, aValue))
  {
    return;
  }
  //
  vtkInformationObjectBaseVectorValue* base = this->GetObjectBaseVector(info);
  //
  if (aValue != nullptr)
  {
    aValue->Register(base);
  }
  //
  base->GetVector().emplace_back(aValue);
}

//------------------------------------------------------------------------------
void vtkInformationObjectBaseVectorKey::Set(vtkInformation* info, vtkObjectBase* aValue, int i)
{
  if (!this->ValidateDerivedType(info, aValue))
  {
    return;
  }
  // Get the vector associated with this key, resize if this
  // set would run off the end.
  vtkInformationObjectBaseVectorValue* base = this->GetObjectBaseVector(info);
  int n = static_cast<int>(base->GetVector().size());
  if (i >= n)
  {
    base->GetVector().resize(i + 1);
  }
  // Set.
  base->GetVector()[i] = aValue;
}

//------------------------------------------------------------------------------
void vtkInformationObjectBaseVectorKey::Remove(vtkInformation* info, vtkObjectBase* val)
{
  if (!this->ValidateDerivedType(info, val))
  {
    return;
  }
  vtkInformationObjectBaseVectorValue* base = this->GetObjectBaseVector(info);

  typedef vtkInformationObjectBaseVectorValue::VectorType Vector;
  Vector& vector = base->GetVector();
  Vector::iterator end = std::remove(vector.begin(), vector.end(), val);
  if (end != vector.end())
  {
    vector.resize(std::distance(vector.begin(), end));
    if (val)
    {
      val->UnRegister(base);
    }
  }
}

//------------------------------------------------------------------------------
void vtkInformationObjectBaseVectorKey::Remove(vtkInformation* info, int idx)
{
  typedef vtkInformationObjectBaseVectorValue::VectorType Vector;
  vtkInformationObjectBaseVectorValue* base = this->GetObjectBaseVector(info);
  Vector& vector = base->GetVector();
  if (idx >= static_cast<int>(vector.size()))
  {
    return;
  }

  vtkObjectBase* val = vector[idx];
  if (val)
  {
    val->UnRegister(base);
  }

  vector.erase(vector.begin() + idx);
}

//------------------------------------------------------------------------------
void vtkInformationObjectBaseVectorKey::SetRange(
  vtkInformation* info, vtkObjectBase** sourceVec, int from, int to, int n)
{
  // Get the vector associated with this key, resize if this
  // set would run off the end.
  vtkInformationObjectBaseVectorValue* base = this->GetObjectBaseVector(info);
  int m = static_cast<int>(base->GetVector().size());
  int reqsz = to + n;
  if (reqsz > m)
  {
    base->GetVector().resize(reqsz);
  }
  // Set.
  for (int i = 0; i < n; ++i, ++from, ++to)
  {
    base->GetVector()[to] = sourceVec[from];
  }
}

// //----------------------------------------------------------------------------
// vtkSmartPointer<vtkObjectBase> *vtkInformationObjectBaseVectorKey::Get(
//         vtkInformation* info)
// {
//   vtkInformationObjectBaseVectorValue* base =
//     static_cast<vtkInformationObjectBaseVectorValue *>(this->GetAsObjectBase(info));
//
//   return
//     (base!=nullptr && !base->GetVector().empty())?(&base->GetVector()[0]):0;
// }

//------------------------------------------------------------------------------
void vtkInformationObjectBaseVectorKey::GetRange(
  vtkInformation* info, vtkObjectBase** dest, int from, int to, int n)
{
  vtkInformationObjectBaseVectorValue* base =
    static_cast<vtkInformationObjectBaseVectorValue*>(this->GetAsObjectBase(info));

  // Source vector exists?
  if (base == nullptr)
  {
    vtkErrorWithObjectMacro(info, "Copy of empty vector has been requested.");
    return;
  }

  int m = static_cast<int>(base->GetVector().size());
  // check source start.
  if (from >= m)
  {
    vtkErrorWithObjectMacro(info, "Copy starting past the end of the vector has been requested.");
    return;
  }

  // limit copy to what's there.
  if (n > m - from + 1)
  {
    vtkErrorWithObjectMacro(info, "Copy past the end of the vector has been requested.");
    n = m - from + 1;
  }

  // copy
  for (int i = 0; i < n; ++i, ++from, ++to)
  {
    dest[to] = base->GetVector()[from];
  }
}

//------------------------------------------------------------------------------
vtkObjectBase* vtkInformationObjectBaseVectorKey::Get(vtkInformation* info, int idx)
{
  vtkInformationObjectBaseVectorValue* base =
    static_cast<vtkInformationObjectBaseVectorValue*>(this->GetAsObjectBase(info));

  if (base == nullptr || idx >= static_cast<int>(base->GetVector().size()))
  {
    vtkErrorWithObjectMacro(info,
      "Information does not contain " << idx << " elements. Cannot return information value.");
    return nullptr;
  }

  return base->GetVector()[idx];
}

//------------------------------------------------------------------------------
int vtkInformationObjectBaseVectorKey::Size(vtkInformation* info)
{
  vtkInformationObjectBaseVectorValue* base =
    static_cast<vtkInformationObjectBaseVectorValue*>(this->GetAsObjectBase(info));

  return (base == nullptr ? 0 : static_cast<int>(base->GetVector().size()));
}

//------------------------------------------------------------------------------
void vtkInformationObjectBaseVectorKey::Resize(vtkInformation* info, int size)
{
  vtkInformationObjectBaseVectorValue* base = this->GetObjectBaseVector(info);
  base->GetVector().resize(size);
}

//------------------------------------------------------------------------------
void vtkInformationObjectBaseVectorKey::Clear(vtkInformation* info)
{
  vtkInformationObjectBaseVectorValue* base = this->GetObjectBaseVector(info);
  base->GetVector().clear();
}

//------------------------------------------------------------------------------
void vtkInformationObjectBaseVectorKey::ShallowCopy(vtkInformation* source, vtkInformation* dest)
{
  vtkInformationObjectBaseVectorValue* sourceBase =
    static_cast<vtkInformationObjectBaseVectorValue*>(this->GetAsObjectBase(source));

  if (sourceBase == nullptr)
  {
    this->SetAsObjectBase(dest, nullptr);
    return;
  }

  int sourceSize = static_cast<int>(sourceBase->GetVector().size());
  vtkInformationObjectBaseVectorValue* destBase = this->GetObjectBaseVector(dest);

  destBase->GetVector().resize(sourceSize);
  destBase->GetVector() = sourceBase->GetVector();
}

//------------------------------------------------------------------------------
void vtkInformationObjectBaseVectorKey::Print(ostream& os, vtkInformation* info)
{
  vtkIndent indent;
  // Grab the vector associated with this key.
  vtkInformationObjectBaseVectorValue* base =
    static_cast<vtkInformationObjectBaseVectorValue*>(this->GetAsObjectBase(info));
  // Print each valid item.
  if (base != nullptr)
  {
    int n = static_cast<int>(base->GetVector().size());
    if (n > 0)
    {
      vtkObjectBase* itemBase = base->GetVector()[0];
      os << indent << "item " << 0 << "=";
      itemBase->PrintSelf(os, indent);
      os << endl;
    }
    for (int i = 1; i < n; ++i)
    {
      os << indent << "item " << i << "=";
      vtkObjectBase* itemBase = base->GetVector()[i];
      if (itemBase != nullptr)
      {
        itemBase->PrintSelf(os, indent);
      }
      else
      {
        os << "nullptr;";
      }
      os << endl;
    }
  }
}
VTK_ABI_NAMESPACE_END

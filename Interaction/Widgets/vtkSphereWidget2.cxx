// SPDX-FileCopyrightText: Copyright (c) Ken Martin, Will Schroeder, Bill Lorensen
// SPDX-License-Identifier: BSD-3-Clause
#include "vtkSphereWidget2.h"
#include "vtkCallbackCommand.h"
#include "vtkCommand.h"
#include "vtkEvent.h"
#include "vtkObjectFactory.h"
#include "vtkRenderWindow.h"
#include "vtkRenderWindowInteractor.h"
#include "vtkRenderer.h"
#include "vtkSphereRepresentation.h"
#include "vtkWidgetCallbackMapper.h"
#include "vtkWidgetEvent.h"
#include "vtkWidgetEventTranslator.h"

#include <algorithm>

VTK_ABI_NAMESPACE_BEGIN
vtkStandardNewMacro(vtkSphereWidget2);

//------------------------------------------------------------------------------
vtkSphereWidget2::vtkSphereWidget2()
{
  this->WidgetState = vtkSphereWidget2::Start;
  this->ManagesCursor = 1;

  this->TranslationEnabled = 1;
  this->ScalingEnabled = 1;

  // Define widget events
  this->CallbackMapper->SetCallbackMethod(
    vtkCommand::LeftButtonPressEvent, vtkWidgetEvent::Select, this, vtkSphereWidget2::SelectAction);
  this->CallbackMapper->SetCallbackMethod(vtkCommand::LeftButtonReleaseEvent,
    vtkWidgetEvent::EndSelect, this, vtkSphereWidget2::EndSelectAction);
  this->CallbackMapper->SetCallbackMethod(vtkCommand::MiddleButtonPressEvent,
    vtkWidgetEvent::Translate, this, vtkSphereWidget2::TranslateAction);
  this->CallbackMapper->SetCallbackMethod(vtkCommand::MiddleButtonReleaseEvent,
    vtkWidgetEvent::EndTranslate, this, vtkSphereWidget2::EndSelectAction);
  this->CallbackMapper->SetCallbackMethod(
    vtkCommand::RightButtonPressEvent, vtkWidgetEvent::Scale, this, vtkSphereWidget2::ScaleAction);
  this->CallbackMapper->SetCallbackMethod(vtkCommand::RightButtonReleaseEvent,
    vtkWidgetEvent::EndScale, this, vtkSphereWidget2::EndSelectAction);
  this->CallbackMapper->SetCallbackMethod(
    vtkCommand::MouseMoveEvent, vtkWidgetEvent::Move, this, vtkSphereWidget2::MoveAction);

  this->KeyEventCallbackCommand = vtkCallbackCommand::New();
  this->KeyEventCallbackCommand->SetClientData(this);
  this->KeyEventCallbackCommand->SetCallback(vtkSphereWidget2::ProcessKeyEvents);
}

//------------------------------------------------------------------------------
vtkSphereWidget2::~vtkSphereWidget2()
{
  this->KeyEventCallbackCommand->Delete();
}

//------------------------------------------------------------------------------
void vtkSphereWidget2::SetEnabled(int enabling)
{
  int enabled = this->Enabled;

  // We do this step first because it sets the CurrentRenderer
  this->Superclass::SetEnabled(enabling);

  // We defer enabling the handles until the selection process begins
  if (enabling && !enabled)
  {
    if (this->Parent)
    {
      this->Parent->AddObserver(
        vtkCommand::KeyPressEvent, this->KeyEventCallbackCommand, this->Priority);
      this->Parent->AddObserver(
        vtkCommand::KeyReleaseEvent, this->KeyEventCallbackCommand, this->Priority);
    }
    else
    {
      this->Interactor->AddObserver(
        vtkCommand::KeyPressEvent, this->KeyEventCallbackCommand, this->Priority);
      this->Interactor->AddObserver(
        vtkCommand::KeyReleaseEvent, this->KeyEventCallbackCommand, this->Priority);
    }
  }
  else if (!enabling && enabled)
  {
    if (this->Parent)
    {
      this->Parent->RemoveObserver(this->KeyEventCallbackCommand);
    }
    else
    {
      this->Interactor->RemoveObserver(this->KeyEventCallbackCommand);
    }
  }
}

//------------------------------------------------------------------------------
void vtkSphereWidget2::SelectAction(vtkAbstractWidget* w)
{
  // We are in a static method, cast to ourself
  vtkSphereWidget2* self = reinterpret_cast<vtkSphereWidget2*>(w);

  // Get the event position
  int X = self->Interactor->GetEventPosition()[0];
  int Y = self->Interactor->GetEventPosition()[1];

  // Okay, make sure that the pick is in the current renderer
  if (!self->CurrentRenderer || !self->CurrentRenderer->IsInViewport(X, Y))
  {
    self->WidgetState = vtkSphereWidget2::Start;
    return;
  }

  // Begin the widget interaction which has the side effect of setting the
  // interaction state.
  double e[2];
  e[0] = static_cast<double>(X);
  e[1] = static_cast<double>(Y);
  self->WidgetRep->StartWidgetInteraction(e);
  int interactionState = self->WidgetRep->GetInteractionState();
  if (interactionState == vtkSphereRepresentation::Outside)
  {
    return;
  }

  // We are definitely selected
  self->WidgetState = vtkSphereWidget2::Active;
  self->GrabFocus(self->EventCallbackCommand);

  // Modifier keys force us into translare mode
  // The SetInteractionState has the side effect of highlighting the widget
  if (interactionState == vtkSphereRepresentation::OnSphere || self->Interactor->GetShiftKey() ||
    self->Interactor->GetControlKey())
  {
    // If translation is disabled, do it
    if (self->TranslationEnabled)
    {
      reinterpret_cast<vtkSphereRepresentation*>(self->WidgetRep)
        ->SetInteractionState(vtkSphereRepresentation::Translating);
    }
  }
  else
  {
    reinterpret_cast<vtkSphereRepresentation*>(self->WidgetRep)
      ->SetInteractionState(interactionState);
  }

  // start the interaction
  self->EventCallbackCommand->SetAbortFlag(1);
  self->StartInteraction();
  self->InvokeEvent(vtkCommand::StartInteractionEvent, nullptr);
  self->Render();
}

//------------------------------------------------------------------------------
void vtkSphereWidget2::TranslateAction(vtkAbstractWidget* w)
{
  // We are in a static method, cast to ourself
  vtkSphereWidget2* self = reinterpret_cast<vtkSphereWidget2*>(w);

  // If  translation is disabled, get out of here
  if (!self->TranslationEnabled)
  {
    return;
  }

  // Get the event position
  int X = self->Interactor->GetEventPosition()[0];
  int Y = self->Interactor->GetEventPosition()[1];

  // Okay, make sure that the pick is in the current renderer
  if (!self->CurrentRenderer || !self->CurrentRenderer->IsInViewport(X, Y))
  {
    self->WidgetState = vtkSphereWidget2::Start;
    return;
  }

  // Begin the widget interaction which has the side effect of setting the
  // interaction state.
  double e[2];
  e[0] = static_cast<double>(X);
  e[1] = static_cast<double>(Y);
  self->WidgetRep->StartWidgetInteraction(e);
  int interactionState = self->WidgetRep->GetInteractionState();
  if (interactionState == vtkSphereRepresentation::Outside)
  {
    return;
  }

  // We are definitely selected
  self->WidgetState = vtkSphereWidget2::Active;
  self->GrabFocus(self->EventCallbackCommand);
  reinterpret_cast<vtkSphereRepresentation*>(self->WidgetRep)
    ->SetInteractionState(vtkSphereRepresentation::Translating);

  // start the interaction
  self->EventCallbackCommand->SetAbortFlag(1);
  self->StartInteraction();
  self->InvokeEvent(vtkCommand::StartInteractionEvent, nullptr);
  self->Render();
}

//------------------------------------------------------------------------------
void vtkSphereWidget2::ScaleAction(vtkAbstractWidget* w)
{
  // We are in a static method, cast to ourself
  vtkSphereWidget2* self = reinterpret_cast<vtkSphereWidget2*>(w);

  // If scaling is disabled, get out of here
  if (!self->ScalingEnabled)
  {
    return;
  }

  // Get the event position
  int X = self->Interactor->GetEventPosition()[0];
  int Y = self->Interactor->GetEventPosition()[1];

  // Okay, make sure that the pick is in the current renderer
  if (!self->CurrentRenderer || !self->CurrentRenderer->IsInViewport(X, Y))
  {
    self->WidgetState = vtkSphereWidget2::Start;
    return;
  }

  // Begin the widget interaction which has the side effect of setting the
  // interaction state.
  double e[2];
  e[0] = static_cast<double>(X);
  e[1] = static_cast<double>(Y);
  self->WidgetRep->StartWidgetInteraction(e);
  int interactionState = self->WidgetRep->GetInteractionState();
  if (interactionState == vtkSphereRepresentation::Outside)
  {
    return;
  }

  // We are definitely selected
  self->WidgetState = vtkSphereWidget2::Active;
  self->GrabFocus(self->EventCallbackCommand);
  reinterpret_cast<vtkSphereRepresentation*>(self->WidgetRep)
    ->SetInteractionState(vtkSphereRepresentation::Scaling);

  // start the interaction
  self->EventCallbackCommand->SetAbortFlag(1);
  self->StartInteraction();
  self->InvokeEvent(vtkCommand::StartInteractionEvent, nullptr);
  self->Render();
}

//------------------------------------------------------------------------------
void vtkSphereWidget2::MoveAction(vtkAbstractWidget* w)
{
  vtkSphereWidget2* self = reinterpret_cast<vtkSphereWidget2*>(w);

  // See whether we're active
  if (self->WidgetState == vtkSphereWidget2::Start)
  {
    return;
  }

  // compute some info we need for all cases
  int X = self->Interactor->GetEventPosition()[0];
  int Y = self->Interactor->GetEventPosition()[1];

  // Okay, adjust the representation
  double e[2];
  e[0] = static_cast<double>(X);
  e[1] = static_cast<double>(Y);
  self->WidgetRep->WidgetInteraction(e);

  // moving something
  self->EventCallbackCommand->SetAbortFlag(1);
  self->InvokeEvent(vtkCommand::InteractionEvent, nullptr);
  self->Render();
}

//------------------------------------------------------------------------------
void vtkSphereWidget2::EndSelectAction(vtkAbstractWidget* w)
{
  vtkSphereWidget2* self = reinterpret_cast<vtkSphereWidget2*>(w);
  if (self->WidgetState == vtkSphereWidget2::Start)
  {
    return;
  }

  // Return state to not active
  self->WidgetState = vtkSphereWidget2::Start;
  reinterpret_cast<vtkSphereRepresentation*>(self->WidgetRep)
    ->SetInteractionState(vtkSphereRepresentation::Outside);
  self->ReleaseFocus();

  self->EventCallbackCommand->SetAbortFlag(1);
  self->EndInteraction();
  self->InvokeEvent(vtkCommand::EndInteractionEvent, nullptr);
  self->Render();
}

//------------------------------------------------------------------------------
void vtkSphereWidget2::CreateDefaultRepresentation()
{
  if (!this->WidgetRep)
  {
    this->WidgetRep = vtkSphereRepresentation::New();
  }
}

//------------------------------------------------------------------------------
void vtkSphereWidget2::ProcessKeyEvents(vtkObject*, unsigned long event, void* clientdata, void*)
{
  vtkSphereWidget2* self = static_cast<vtkSphereWidget2*>(clientdata);
  vtkSphereRepresentation* rep = vtkSphereRepresentation::SafeDownCast(self->WidgetRep);
  char* cKeySym = self->Interactor->GetKeySym();
  std::string keySym = cKeySym != nullptr ? cKeySym : "";
  std::transform(keySym.begin(), keySym.end(), keySym.begin(), ::toupper);
  if (event == vtkCommand::KeyPressEvent)
  {
    if (keySym == "X")
    {
      rep->SetXTranslationAxisOn();
    }
    else if (keySym == "Y")
    {
      rep->SetYTranslationAxisOn();
    }
    else if (keySym == "Z")
    {
      rep->SetZTranslationAxisOn();
    }
  }
  else if (event == vtkCommand::KeyReleaseEvent)
  {
    if (keySym == "X" || keySym == "Y" || keySym == "Z")
    {
      rep->SetTranslationAxisOff();
    }
  }
}

//------------------------------------------------------------------------------
void vtkSphereWidget2::PrintSelf(ostream& os, vtkIndent indent)
{
  this->Superclass::PrintSelf(os, indent);

  os << indent << "Translation Enabled: " << (this->TranslationEnabled ? "On\n" : "Off\n");
  os << indent << "Scaling Enabled: " << (this->ScalingEnabled ? "On\n" : "Off\n");
}
VTK_ABI_NAMESPACE_END

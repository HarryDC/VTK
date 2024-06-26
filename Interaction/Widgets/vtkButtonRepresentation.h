// SPDX-FileCopyrightText: Copyright (c) Ken Martin, Will Schroeder, Bill Lorensen
// SPDX-License-Identifier: BSD-3-Clause
/**
 * @class   vtkButtonRepresentation
 * @brief   abstract class defines the representation for a vtkButtonWidget
 *
 * This abstract class is used to specify how the vtkButtonWidget should
 * interact with representations of the vtkButtonWidget. This class may be
 * subclassed so that alternative representations can be created. The class
 * defines an API, and a default implementation, that the vtkButtonWidget
 * interacts with to render itself in the scene.
 *
 * The vtkButtonWidget assumes an n-state button so that traversal methods
 * are available for changing, querying and manipulating state. Derived
 * classed determine the actual appearance. The state is represented by an
 * integral value 0<=state<numStates.
 *
 * To use this representation, always begin by specifying the number of states.
 * Then follow with the necessary information to represent each state (done through
 * a subclass API).
 *
 * @sa
 * vtkButtonWidget
 */

#ifndef vtkButtonRepresentation_h
#define vtkButtonRepresentation_h

#include "vtkInteractionWidgetsModule.h" // For export macro
#include "vtkWidgetRepresentation.h"
#include "vtkWrappingHints.h" // For VTK_MARSHALAUTO

VTK_ABI_NAMESPACE_BEGIN
class VTKINTERACTIONWIDGETS_EXPORT VTK_MARSHALAUTO vtkButtonRepresentation
  : public vtkWidgetRepresentation
{
public:
  ///@{
  /**
   * Standard methods for the class.
   */
  vtkTypeMacro(vtkButtonRepresentation, vtkWidgetRepresentation);
  void PrintSelf(ostream& os, vtkIndent indent) override;
  ///@}

  ///@{
  /**
   * Retrieve the current button state.
   */
  vtkSetClampMacro(NumberOfStates, int, 1, VTK_INT_MAX);
  ///@}

  ///@{
  /**
   * Retrieve the current button state.
   */
  vtkGetMacro(State, int);
  ///@}

  ///@{
  /**
   * Manipulate the state. Note that the NextState() and PreviousState() methods
   * use modulo traversal. The "state" integral value will be clamped within
   * the possible state values (0<=state<NumberOfStates). Note that subclasses
   * will override these methods in many cases.
   */
  virtual void SetState(int state);
  virtual void NextState();
  virtual void PreviousState();
  ///@}

  enum InteractionStateType
  {
    Outside = 0,
    Inside
  };

  ///@{
  /**
   * These methods control the appearance of the button as it is being
   * interacted with. Subclasses will behave differently depending on their
   * particulars.  HighlightHovering is used when the mouse pointer moves
   * over the button. HighlightSelecting is set when the button is selected.
   * Otherwise, the HighlightNormal is used. The Highlight() method will throw
   * a vtkCommand::HighlightEvent.
   */
  enum HighlightStateType
  {
    HighlightNormal,
    HighlightHovering,
    HighlightSelecting
  };

  void Highlight(int) override;
  vtkGetMacro(HighlightState, int);
  ///@}

  /**
   * Satisfy some of vtkProp's API.
   */
  void ShallowCopy(vtkProp* prop) override;

protected:
  vtkButtonRepresentation();
  ~vtkButtonRepresentation() override;

  // Values
  int NumberOfStates;
  int State;
  int HighlightState;

private:
  vtkButtonRepresentation(const vtkButtonRepresentation&) = delete;
  void operator=(const vtkButtonRepresentation&) = delete;
};

VTK_ABI_NAMESPACE_END
#endif

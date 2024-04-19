// SPDX-FileCopyrightText: Copyright (c) Ken Martin, Will Schroeder, Bill Lorensen
// SPDX-License-Identifier: BSD-3-Clause
/**
 * @class   vtkSDL2OpenGL2RenderWindow
 * @brief   OpenGL rendering window
 *
 * vtkSDL2OpenGL2RenderWindow is a concrete implementation of the abstract
 * class vtkRenderWindow. vtkSDL2OpenGL2Renderer interfaces to the standard
 * OpenGL graphics library using SDL2
 */

#ifndef vtkSDL2OpenGLRenderWindow_h
#define vtkSDL2OpenGLRenderWindow_h

#include "vtkOpenGLRenderWindow.h"
#include "vtkRenderingOpenGL2Module.h" // For export macro
#include "vtkWrappingHints.h"          // For VTK_MARSHALAUTO
// Ignore reserved-identifier warnings from
// 1. SDL2/SDL_stdinc.h: warning: identifier '_SDL_size_mul_overflow_builtin'
// 2. SDL2/SDL_stdinc.h: warning: identifier '_SDL_size_add_overflow_builtin'
// 3. SDL2/SDL_audio.h: warning: identifier '_SDL_AudioStream'
// 4. SDL2/SDL_joystick.h: warning: identifier '_SDL_Joystick'
// 5. SDL2/SDL_sensor.h: warning: identifier '_SDL_Sensor'
// 6. SDL2/SDL_gamecontroller.h: warning: identifier '_SDL_GameController'
// 7. SDL2/SDL_haptic.h: warning: identifier '_SDL_Haptic'
#ifdef __clang__
#pragma clang diagnostic push
#pragma clang diagnostic ignored "-Wreserved-identifier"
#endif
#include "SDL.h" // for ivar
#ifdef __clang__
#pragma clang diagnostic pop
#endif
#include <stack> // for ivar

VTK_ABI_NAMESPACE_BEGIN
class VTKRENDERINGOPENGL2_EXPORT VTK_MARSHALAUTO vtkSDL2OpenGLRenderWindow
  : public vtkOpenGLRenderWindow
{
public:
  static vtkSDL2OpenGLRenderWindow* New();
  vtkTypeMacro(vtkSDL2OpenGLRenderWindow, vtkOpenGLRenderWindow);
  void PrintSelf(ostream& os, vtkIndent indent) override;

  /**
   * Initialize the rendering window.  This will setup all system-specific
   * resources.  This method and Finalize() must be symmetric and it
   * should be possible to call them multiple times, even changing WindowId
   * in-between.  This is what WindowRemap does.
   */
  void Initialize() override;

  /**
   * Finalize the rendering window.  This will shutdown all system-specific
   * resources.  After having called this, it should be possible to destroy
   * a window that was used for a SetWindowId() call without any ill effects.
   */
  void Finalize() override;

  /**
   * Change the window to fill the entire screen.
   */
  void SetFullScreen(vtkTypeBool) override;

  /**
   * Show or not Show the window
   */
  void SetShowWindow(bool val) override;

  ///@{
  /**
   * Set the size of the window in pixels.
   */
  void SetSize(int, int) override;
  void SetSize(int a[2]) override { vtkOpenGLRenderWindow::SetSize(a); }
  ///@}

  /**
   * Get the current size of the window in pixels.
   */
  int* GetSize() VTK_SIZEHINT(2) override;

  ///@{
  /**
   * Set the position of the window.
   */
  void SetPosition(int, int) override;
  void SetPosition(int a[2]) override { vtkOpenGLRenderWindow::SetPosition(a); }
  ///@}

  /**
   * Get the current size of the screen in pixels.
   */
  int* GetScreenSize() VTK_SIZEHINT(2) override;

  /**
   * Get the position in screen coordinates of the window.
   */
  int* GetPosition() VTK_SIZEHINT(2) override;

  /**
   * Set the name of the window. This appears at the top of the window
   * normally.
   */
  void SetWindowName(const char*) override;

  void* GetGenericDisplayId() override { return (void*)this->ContextId; }
  void* GetGenericWindowId() override { return (void*)this->WindowId; }
  void* GetGenericDrawable() override { return (void*)this->WindowId; }

  /**
   * Make this windows OpenGL context the current context.
   */
  void MakeCurrent() override;

  /**
   * Release the current context.
   */
  void ReleaseCurrent() override;

  /**
   * Tells if this window is the current OpenGL context for the calling thread.
   */
  bool IsCurrent() override;

  /**
   * Clean up device contexts, rendering contexts, etc.
   */
  void Clean();

  /**
   * A termination method performed at the end of the rendering process
   * to do things like swapping buffers (if necessary) or similar actions.
   */
  void Frame() override;

  ///@{
  /**
   * Ability to push and pop this window's context
   * as the current context. The idea being to
   * if needed make this window's context current
   * and when done releasing resources restore
   * the prior context
   */
  void PushContext() override;
  void PopContext() override;
  ///@}

  /**
   * Set the number of vertical syncs required between frames.
   * A value of 0 means swap buffers as quickly as possible
   * regardless of the vertical refresh. A value of 1 means swap
   * buffers in sync with the vertical refresh to eliminate tearing.
   * A value of -1 means use a value of 1 unless we missed a frame
   * in which case swap immediately. Returns true if the call
   * succeeded.
   */
  bool SetSwapControl(int i) override;

  /**
   * Get the size of the color buffer.
   * Returns 0 if not able to determine otherwise sets R G B and A into buffer.
   */
  int GetColorBufferSizes(int* rgba) override;

  ///@{
  /**
   * Hide or Show the mouse cursor, it is nice to be able to hide the
   * default cursor if you want VTK to display a 3D cursor instead.
   */
  void HideCursor() override;
  void ShowCursor() override;
  ///@}

protected:
  vtkSDL2OpenGLRenderWindow();
  ~vtkSDL2OpenGLRenderWindow() override;

  SDL_Window* WindowId;
  SDL_GLContext ContextId;
  std::stack<SDL_GLContext> ContextStack;
  std::stack<SDL_Window*> WindowStack;
  static const std::string DEFAULT_BASE_WINDOW_NAME;

  void CleanUpRenderers();
  void CreateAWindow() override;
  void DestroyWindow() override;

private:
  vtkSDL2OpenGLRenderWindow(const vtkSDL2OpenGLRenderWindow&) = delete;
  void operator=(const vtkSDL2OpenGLRenderWindow&) = delete;
};

VTK_ABI_NAMESPACE_END
#endif

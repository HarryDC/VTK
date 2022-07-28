#!/usr/bin/env python
import vtk
from vtk.util.misc import vtkGetDataRoot
VTK_DATA_ROOT = vtkGetDataRoot()

# this test covers a lot of the code in vtkAbstractTransform that
# is not covered elsewhere
# create a rendering window
renWin = vtk.vtkRenderWindow()
renWin.SetMultiSamples(0)
renWin.SetSize(600,300)
# set up first set of polydata
p1 = vtk.vtkPlaneSource()
p1.SetOrigin(0.5,0.508,-0.5)
p1.SetPoint1(-0.5,0.508,-0.5)
p1.SetPoint2(0.5,0.508,0.5)
p1.SetXResolution(5)
p1.SetYResolution(5)
p1.Update()
p2 = vtk.vtkPlaneSource()
p2.SetOrigin(-0.508,0.5,-0.5)
p2.SetPoint1(-0.508,-0.5,-0.5)
p2.SetPoint2(-0.508,0.5,0.5)
p2.SetXResolution(5)
p2.SetYResolution(5)
p2.Update()
p3 = vtk.vtkPlaneSource()
p3.SetOrigin(-0.5,-0.508,-0.5)
p3.SetPoint1(0.5,-0.508,-0.5)
p3.SetPoint2(-0.5,-0.508,0.5)
p3.SetXResolution(5)
p3.SetYResolution(5)
p3.Update()
p4 = vtk.vtkPlaneSource()
p4.SetOrigin(0.508,-0.5,-0.5)
p4.SetPoint1(0.508,0.5,-0.5)
p4.SetPoint2(0.508,-0.5,0.5)
p4.SetXResolution(5)
p4.SetYResolution(5)
p4.Update()
p5 = vtk.vtkPlaneSource()
p5.SetOrigin(0.5,0.5,-0.508)
p5.SetPoint1(0.5,-0.5,-0.508)
p5.SetPoint2(-0.5,0.5,-0.508)
p5.SetXResolution(5)
p5.SetYResolution(5)
p5.Update()
p6 = vtk.vtkPlaneSource()
p6.SetOrigin(0.5,0.5,0.508)
p6.SetPoint1(-0.5,0.5,0.508)
p6.SetPoint2(0.5,-0.5,0.508)
p6.SetXResolution(5)
p6.SetYResolution(5)
p6.Update()
# append together
ap = vtk.vtkAppendPolyData()
ap.AddInputData(p1.GetOutput())
ap.AddInputData(p2.GetOutput())
ap.AddInputData(p3.GetOutput())
ap.AddInputData(p4.GetOutput())
ap.AddInputData(p5.GetOutput())
ap.AddInputData(p6.GetOutput())
#--------------------------
tLinear = vtk.vtkTransform()
tPerspective = vtk.vtkPerspectiveTransform()
tGeneral = vtk.vtkGeneralTransform()
# set up a linear transformation
tLinear.Scale(1.2,1.0,0.8)
tLinear.RotateX(30)
tLinear.RotateY(10)
tLinear.RotateZ(80)
tLinear.Translate(0.2,0.3,-0.1)
tLinear.Update()
# set up a perspective transform
tPerspective.SetInput(tLinear)
tPerspective.SetInput(tLinear.GetInverse())
tPerspective.Scale(2,2,2)
# these should cancel
tPerspective.AdjustViewport(-0.5,0.5,-0.5,0.5,-1,1,-1,1)
tPerspective.AdjustViewport(-1,1,-1,1,-0.5,0.5,-0.5,0.5)
# test shear transformation
tPerspective.Shear(0.2,0.3,0.0)
tPerspective.Update()
# the following 6 operations cancel out
tPerspective.RotateWXYZ(30,1,1,1)
tPerspective.RotateWXYZ(-30,1,1,1)
tPerspective.Scale(2,2,2)
tPerspective.Scale(0.5,0.5,0.5)
tPerspective.Translate(10,0.1,0.3)
tPerspective.Translate(-10,-0.1,-0.3)
tPerspective.Concatenate(tLinear)
# test push and pop
tPerspective.Push()
tPerspective.RotateX(30)
tPerspective.RotateY(10)
tPerspective.RotateZ(80)
tPerspective.Translate(0.1,-0.2,0.0)
# test copy of transforms
tNew = tPerspective.MakeTransform()
tNew.DeepCopy(tPerspective)
tPerspective.Pop()
# test general transform
tGeneral.SetInput(tLinear)
tGeneral.SetInput(tPerspective)
tGeneral.PostMultiply()
tGeneral.Concatenate(tNew)
tGeneral.Concatenate(tNew.GetInverse())
tGeneral.PreMultiply()
# the following 6 operations cancel out
tGeneral.RotateWXYZ(30,1,1,1)
tGeneral.RotateWXYZ(-30,1,1,1)
tGeneral.Scale(2,2,2)
tGeneral.Scale(0.5,0.5,0.5)
tGeneral.Translate(10,0.1,0.3)
tGeneral.Translate(-10,-0.1,-0.3)
#--------------------------
# identity transform
f11 = vtk.vtkTransformPolyDataFilter()
f11.SetInputConnection(ap.GetOutputPort())
f11.SetTransform(tLinear)
m11 = vtk.vtkDataSetMapper()
m11.SetInputConnection(f11.GetOutputPort())
a11 = vtk.vtkActor()
a11.SetMapper(m11)
a11.GetProperty().SetColor(1,0,0)
a11.GetProperty().SetRepresentationToWireframe()
ren11 = vtk.vtkRenderer()
ren11.SetViewport(0.0,0.5,0.25,1.0)
ren11.ResetCamera(-0.5,0.5,-0.5,0.5,-1,1)
ren11.AddActor(a11)
renWin.AddRenderer(ren11)
# inverse identity transform
f12 = vtk.vtkTransformPolyDataFilter()
f12.SetInputConnection(ap.GetOutputPort())
f12.SetTransform(tLinear.GetInverse())
m12 = vtk.vtkDataSetMapper()
m12.SetInputConnection(f12.GetOutputPort())
a12 = vtk.vtkActor()
a12.SetMapper(m12)
a12.GetProperty().SetColor(0.9,0.9,0)
a12.GetProperty().SetRepresentationToWireframe()
ren12 = vtk.vtkRenderer()
ren12.SetViewport(0.0,0.0,0.25,0.5)
ren12.ResetCamera(-0.5,0.5,-0.5,0.5,-1,1)
ren12.AddActor(a12)
renWin.AddRenderer(ren12)
#--------------------------
# linear transform
f21 = vtk.vtkTransformPolyDataFilter()
f21.SetInputConnection(ap.GetOutputPort())
f21.SetTransform(tPerspective)
m21 = vtk.vtkDataSetMapper()
m21.SetInputConnection(f21.GetOutputPort())
a21 = vtk.vtkActor()
a21.SetMapper(m21)
a21.GetProperty().SetColor(1,0,0)
a21.GetProperty().SetRepresentationToWireframe()
ren21 = vtk.vtkRenderer()
ren21.SetViewport(0.25,0.5,0.50,1.0)
ren21.ResetCamera(-0.5,0.5,-0.5,0.5,-1,1)
ren21.AddActor(a21)
renWin.AddRenderer(ren21)
# inverse linear transform
f22 = vtk.vtkTransformPolyDataFilter()
f22.SetInputConnection(ap.GetOutputPort())
f22.SetTransform(tPerspective.GetInverse())
m22 = vtk.vtkDataSetMapper()
m22.SetInputConnection(f22.GetOutputPort())
a22 = vtk.vtkActor()
a22.SetMapper(m22)
a22.GetProperty().SetColor(0.9,0.9,0)
a22.GetProperty().SetRepresentationToWireframe()
ren22 = vtk.vtkRenderer()
ren22.SetViewport(0.25,0.0,0.50,0.5)
ren22.ResetCamera(-0.5,0.5,-0.5,0.5,-1,1)
ren22.AddActor(a22)
renWin.AddRenderer(ren22)
#--------------------------
# perspective transform
matrix = vtk.vtkMatrix4x4()
matrix.SetElement(3,0,0.1)
matrix.SetElement(3,1,0.2)
matrix.SetElement(3,2,0.5)
f31 = vtk.vtkTransformPolyDataFilter()
f31.SetInputConnection(ap.GetOutputPort())
f31.SetTransform(tNew)
m31 = vtk.vtkDataSetMapper()
m31.SetInputConnection(f31.GetOutputPort())
a31 = vtk.vtkActor()
a31.SetMapper(m31)
a31.GetProperty().SetColor(1,0,0)
a31.GetProperty().SetRepresentationToWireframe()
ren31 = vtk.vtkRenderer()
ren31.SetViewport(0.50,0.5,0.75,1.0)
ren31.ResetCamera(-0.5,0.5,-0.5,0.5,-1,1)
ren31.AddActor(a31)
renWin.AddRenderer(ren31)
# inverse linear transform
f32 = vtk.vtkTransformPolyDataFilter()
f32.SetInputConnection(ap.GetOutputPort())
f32.SetTransform(tNew.GetInverse())
m32 = vtk.vtkDataSetMapper()
m32.SetInputConnection(f32.GetOutputPort())
a32 = vtk.vtkActor()
a32.SetMapper(m32)
a32.GetProperty().SetColor(0.9,0.9,0)
a32.GetProperty().SetRepresentationToWireframe()
ren32 = vtk.vtkRenderer()
ren32.SetViewport(0.5,0.0,0.75,0.5)
ren32.ResetCamera(-0.5,0.5,-0.5,0.5,-1,1)
ren32.AddActor(a32)
renWin.AddRenderer(ren32)
#--------------------------
# perspective transform concatenation
f41 = vtk.vtkTransformPolyDataFilter()
f41.SetInputConnection(ap.GetOutputPort())
f41.SetTransform(tGeneral)
m41 = vtk.vtkDataSetMapper()
m41.SetInputConnection(f41.GetOutputPort())
a41 = vtk.vtkActor()
a41.SetMapper(m41)
a41.GetProperty().SetColor(1,0,0)
a41.GetProperty().SetRepresentationToWireframe()
ren41 = vtk.vtkRenderer()
ren41.SetViewport(0.75,0.5,1.0,1.0)
ren41.ResetCamera(-0.5,0.5,-0.5,0.5,-1,1)
ren41.AddActor(a41)
renWin.AddRenderer(ren41)
# inverse linear transform
f42 = vtk.vtkTransformPolyDataFilter()
f42.SetInputConnection(ap.GetOutputPort())
f42.SetTransform(tGeneral.GetInverse())
m42 = vtk.vtkDataSetMapper()
m42.SetInputConnection(f42.GetOutputPort())
a42 = vtk.vtkActor()
a42.SetMapper(m42)
a42.GetProperty().SetColor(0.9,0.9,0)
a42.GetProperty().SetRepresentationToWireframe()
ren42 = vtk.vtkRenderer()
ren42.SetViewport(0.75,0.0,1.0,0.5)
ren42.ResetCamera(-0.5,0.5,-0.5,0.5,-1,1)
ren42.AddActor(a42)
renWin.AddRenderer(ren42)
renWin.Render()
# free what we did a MakeTransform on
tNew.UnRegister(None) # not needed in python
# --- end of script --

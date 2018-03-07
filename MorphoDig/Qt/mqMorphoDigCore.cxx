/*=========================================================================

  Program:   MorphoDig
  Module:    MorphoDigCore.cxx


=========================================================================*/
#include "mqMorphoDigCore.h"
#include "vtkMDActor.h"
#include "vtkLMActor.h"
#include "vtkOrientationHelperActor.h"
#include "vtkOrientationHelperWidget.h"
#include "vtkBezierCurveSource.h"
#include <vtkThreshold.h>
#include <vtkMaskFields.h>
#include <vtkActor.h>
#include <vtkPolyDataConnectivityFilter.h>
#include <vtkFillHolesFilter.h>
#include <vtkDensifyPolyData.h>
#include <vtkDecimatePro.h>
#include <vtkQuadricDecimation.h>
#include <vtkSmoothPolyDataFilter.h>
#include <vtkReverseSense.h>
#include <vtkUnstructuredGrid.h>
#include <vtkReflectionFilter.h>
#include <vtkTransform.h>
#include <vtkPiecewiseFunction.h>
#include <vtkCellPicker.h>
#include <vtkProperty.h>
#include <vtkPointData.h>
#include <vtkCellData.h>
#include <vtkPolyDataMapper.h>
#include <vtkSTLWriter.h>
#include <vtkPLYWriter.h>
#include <vtkPolyDataWriter.h>
#include <vtkThinPlateSplineTransform.h>
#include <vtkTransformPolyDataFilter.h>

#include <vtkSmartPointer.h>
#include <vtkDataSetMapper.h>
#include <vtkPolyDataReader.h>
#include <vtkPolyDataNormals.h>
#include <vtkPLYReader.h>
#include <vtkMath.h>
#include <vtkSTLReader.h>
#include <vtkCleanPolyData.h>
#include <vtkFloatArray.h>
#include <vtkDoubleArray.h>
#include <vtkMassProperties.h>

#include <vtkDelaunay3D.h>
#include <vtkQuadricDecimation.h>
#include <vtkCenterOfMass.h>
#include <vtkSphere.h>
#include <vtkDataSetSurfaceFilter.h>

#include <vtkGeometryFilter.h>

#include <vtkCubeAxesActor.h>
#include <vtkAppendPolyData.h>
#include <vtkPolyDataMapper.h>
#include <vtkRenderer.h>
#include <vtkRenderWindow.h>
#include <vtkLookupTable.h>
#include <vtkDiscretizableColorTransferFunction.h>
#include <QFile>
#include <QFileInfo>
#include <QMessageBox>
#include <QTextStream>
#include <QProgressDialog>

#include "mqUndoStack.h"

#define NORMAL_LMK 0
#define TARGET_LMK 1
#define NODE_LMK 2
#define HANDLE_LMK 3
#define FLAG_LMK 4

#define NORMAL_NODE 0
#define STARTING_NODE 1
#define MILESTONE_NODE 2
#define CONNECT_NODE 3

#define VTK_CREATE(type, name) \
  vtkSmartPointer<type> name = vtkSmartPointer<type>::New()
//-----------------------------------------------------------------------------
mqMorphoDigCore* mqMorphoDigCore::Instance = 0;

//-----------------------------------------------------------------------------
mqMorphoDigCore* mqMorphoDigCore::instance()
{
	return mqMorphoDigCore::Instance;
}


mqMorphoDigCore::mqMorphoDigCore()
{

	mqMorphoDigCore::Instance = this;
	this->ScalarRangeMin = 0;
	this->ScalarRangeMax = 1;
	this->mui_ClippingPlane = 0; // no x=0 clipping plane by default
	this->mui_BackfaceCulling = 0; //no backface culling
	this->TagLut= vtkSmartPointer<vtkLookupTable>::New();
	this->ScalarRedLut = vtkSmartPointer<vtkDiscretizableColorTransferFunction>::New();
	this->ScalarRainbowLut = vtkSmartPointer<vtkDiscretizableColorTransferFunction>::New();
	this->TagTableSize = 25;
	this->mui_ActiveColorMap = new ActiveColorMap;
	this->mui_ExistingColorMaps = new ExistingColorMaps;
	this->InitLuts();
	this->ActorCollection = vtkSmartPointer<vtkMDActorCollection>::New();
	cout << "try to create mui_ActiveScalars" << endl;
	this->mui_ActiveScalars = new ActiveScalars;
	this->mui_ExistingScalars = new ExistingScalars;

	this->ScalarBarActor = vtkSmartPointer<vtkScalarBarActor>::New();
	this->ScalarBarActor->SetOrientationToHorizontal();
	this->ScalarBarActor->SetHeight(0.1);


	cout << "mui_ActiveScalars creaed" << endl;
	QString none = QString("none");
	this->Setmui_ActiveScalars(none, -1, 0);
	cout << "mui_ActiveScalars instantiated" << endl;
	this->Addmui_ExistingScalars(this->mui_ActiveScalars->Name, this->mui_ActiveScalars->DataType, this->mui_ActiveScalars->NumComp);

	this->MainWindow = NULL;
	this->OrientationHelperWidget = vtkOrientationHelperWidget::New();
	this->mui_DefaultLandmarkMode = this->mui_LandmarkMode = 0;

	this->mui_LandmarkBodyType = this->mui_DefaultLandmarkBodyType = 0;
	this->mui_LandmarkRenderingSize=this->mui_DefaultLandmarkRenderingSize=1;
	this->mui_AdjustLandmarkRenderingSize= this->mui_DefaultAdjustLandmarkRenderingSize=1;
	this->mui_FlagRenderingSize= this->mui_DefaultFlagRenderingSize=5;
	this->mui_AdjustScaleFactor = this->mui_DefaultAdjustScaleFactor = 1;


	this->mui_ScalarVisibility = this->mui_DefaultScalarVisibility = 1;


	this->mui_Anaglyph = this->mui_DefaultAnaglyph = 0;
	this->mui_ShowGrid = this->mui_DefaultShowGrid = 1;
	this->mui_GridSpacing = this->mui_DefaultGridSpacing = 10;
	this->mui_SizeUnit = this->mui_DefaultSizeUnit = "mm";

	this->mui_MoveAll = this->mui_DefaultMoveAll = 1;
	this->mui_FlagColor[0] = this->mui_DefaultFlagColor[0] = 0;
	this->mui_FlagColor[1] = this->mui_DefaultFlagColor[1] = 0.7;
	this->mui_FlagColor[2] = this->mui_DefaultFlagColor[2] = 0.7;

	this->mui_MeshColor[0] = this->mui_DefaultMeshColor[0] = 0.631373;
	this->mui_MeshColor[1] = this->mui_DefaultMeshColor[1] = 0.572549;
	this->mui_MeshColor[2] = this->mui_DefaultMeshColor[2] = 0.372549;
	this->mui_MeshColor[3] = this->mui_DefaultMeshColor[3] = 1;
	
	this->mui_BackGroundColor2[0] = this->mui_DefaultBackGroundColor2[0] = 0;
	this->mui_BackGroundColor2[1] = this->mui_DefaultBackGroundColor2[1] = 0;
	this->mui_BackGroundColor2[2] = this->mui_DefaultBackGroundColor2[2] = 0;

	this->mui_BackGroundColor[0] = this->mui_DefaultBackGroundColor[0] = 0.5;
	this->mui_BackGroundColor[1] = this->mui_DefaultBackGroundColor[1] = 0.5;
	this->mui_BackGroundColor[2] = this->mui_DefaultBackGroundColor[2] = 1;

	this->mui_ShowOrientationHelper = this->mui_DefaultShowOrientationHelper = 1;

	this->mui_X1Label = this->mui_DefaultX1Label = "Anterior";
	this->mui_X2Label = this->mui_DefaultX2Label = "Posterior";

	this->mui_Y1Label = this->mui_DefaultY1Label = "Left";
	this->mui_Y2Label = this->mui_DefaultY2Label = "Right";
	this->mui_Z1Label = this->mui_DefaultZ1Label = "Dorsal";
	this->mui_Z2Label = this->mui_DefaultZ2Label = "Ventral";

	this->mui_CameraOrtho = this->mui_DefaultCameraOrtho = 1;
	this->mui_CameraCentreOfMassAtOrigin = this->mui_DefaultCameraCentreOfMassAtOrigin = 0;
	//this->UndoStack = vtkSmartPointer<vtkUndoStack>::New();
	mqUndoStack* undoStack = new mqUndoStack();
	this->setUndoStack(undoStack);
	//this->mUndoStack = undoStack;
	//MorphoDig::testint = 10;
	//MorphoDig::Instance = this;
	//this->SetUndoCount(0);
	//vtkUndoStack* undoStack = vtkUndoStack::New();
	//vtkUndoSet* undoSet = vtkUndoSet::New();
	//vtkUndoElement* undoElement = vtkUndoElement::New();
	//undoStack->Push("Test", undoSet);

	/*
	vtkSMSession* session = vtkSMSession::New();
	vtkSMSessionProxyManager* pxm = session->GetSessionProxyManager();

	vtkSMProxy* sphere = pxm->NewProxy("sources", "SphereSource");
	sphere->UpdateVTKObjects();
	QVERIFY(sphere != NULL);
	QCOMPARE(vtkSMPropertyHelper(sphere, "Radius").GetAsDouble(), 0.5);

	vtkSMUndoStack* undoStack = vtkSMUndoStack::New();
	vtkUndoSet* undoSet = vtkUndoSet::New();
	vtkSMRemoteObjectUpdateUndoElement* undoElement = vtkSMRemoteObjectUpdateUndoElement::New();
	undoElement->SetSession(session);

	vtkSMMessage before;
	before.CopyFrom(*sphere->GetFullState());
	vtkSMPropertyHelper(sphere, "Radius").Set(1.2);
	sphere->UpdateVTKObjects();
	vtkSMMessage after;
	after.CopyFrom(*sphere->GetFullState());
	undoElement->SetUndoRedoState(&before, &after);

	undoSet->AddElement(undoElement);
	undoElement->Delete();
	undoStack->Push("ChangeRadius", undoSet);
	undoSet->Delete();

	QVERIFY(static_cast<bool>(undoStack->CanUndo()) == true);
	undoStack->Undo();
	QVERIFY(static_cast<bool>(undoStack->CanUndo()) == false);
	sphere->UpdateVTKObjects();
	QCOMPARE(vtkSMPropertyHelper(sphere, "Radius").GetAsDouble(), 0.5);

	QVERIFY(static_cast<bool>(undoStack->CanRedo()) == true);
	undoStack->Redo();
	sphere->UpdateVTKObjects();
	QCOMPARE(vtkSMPropertyHelper(sphere, "Radius").GetAsDouble(), 1.2);
	QVERIFY(static_cast<bool>(undoStack->CanRedo()) == false);

	undoStack->Delete();

	sphere->Delete();
	session->Delete();
	*/
	//this->RenderWindow = vtkSmartPointer<vtkRenderWindow>::New();
	this->RenderWindow = NULL;
	this->Renderer = vtkSmartPointer<vtkRenderer>::New();
	

	this->ActorCollection->SetRenderer(this->Renderer);

	this->BezierCurveSource = vtkSmartPointer<vtkBezierCurveSource>::New();
	this->BezierMapper=vtkSmartPointer<vtkPolyDataMapper>::New();
	this->BezierSelectedMapper = vtkSmartPointer<vtkPolyDataMapper>::New();
	BezierMapper->SetInputConnection(this->BezierCurveSource->GetOutputPort(0));
	BezierSelectedMapper->SetInputConnection(this->BezierCurveSource->GetOutputPort(1));
	BezierSelectedActor = vtkSmartPointer<vtkActor>::New();
	this->BezierActor=vtkSmartPointer<vtkActor>::New();

	this->BezierActor->SetMapper(this->BezierMapper);
	this->BezierActor->GetProperty()->SetColor(0, 0.5, 0);
	this->BezierActor->GetProperty()->SetLineWidth(2.0);

	this->BezierSelectedActor->SetMapper(this->BezierSelectedMapper);
	this->BezierSelectedActor->GetProperty()->SetColor(0.8, 0.2, 0);
	this->BezierSelectedActor->GetProperty()->SetLineWidth(3.0);

	this->BezierNHMapper = vtkSmartPointer<vtkPolyDataMapper>::New();
	this->BezierNHMapper->SetInputConnection(this->BezierCurveSource->GetOutputPort(2));
	this->BezierNHActor = vtkSmartPointer<vtkActor>::New();

	this->BezierNHActor->SetMapper(this->BezierNHMapper);
	this->BezierNHActor->GetProperty()->SetColor(0, 1, 1);

	

	//vtkSmartPointer<vtkBezierCurveSource> bezierCurve =
	this->NormalLandmarkCollection = vtkSmartPointer<vtkLMActorCollection>::New();
	this->NormalLandmarkCollection->SetRenderer(this->Renderer);
	this->TargetLandmarkCollection = vtkSmartPointer<vtkLMActorCollection>::New();
	this->TargetLandmarkCollection->SetRenderer(this->Renderer);
	this->NodeLandmarkCollection = vtkSmartPointer<vtkLMActorCollection>::New();
	this->NodeLandmarkCollection->SetRenderer(this->Renderer);
	this->HandleLandmarkCollection = vtkSmartPointer<vtkLMActorCollection>::New();
	this->HandleLandmarkCollection->SetRenderer(this->Renderer);

	this->FlagLandmarkCollection = vtkSmartPointer<vtkLMActorCollection>::New();
	this->FlagLandmarkCollection->SetRenderer(this->Renderer);

	this->Renderer->SetUseDepthPeeling(1);
	this->Renderer->SetMaximumNumberOfPeels(100);
	this->Renderer->SetOcclusionRatio(0.1);
	this->Camera = this->Renderer->GetActiveCamera();
	this->GridActor = vtkSmartPointer<vtkGridActor>::New();
	this->GridActor->SetGridSpacing(this->Getmui_GridSpacing());
	this->GridActor->SetGridType(2);	

	//@@@TEST OPEN ONE SURFACE AND RENDER IT!!! NICE TRIANGLE NORMALS

	/*
	vtkSmartPointer<vtkPolyData> input1;
	auto reader1 =	vtkSmartPointer<vtkPolyDataReader>::New();
	//reader1->SetFileName("test.vtk");
	reader1->SetFileName("ear_scalar.vtk");
	//reader1->SetFileName("test_pinkRGB.vtk");
	reader1->Update();
	input1 = reader1->GetOutput();
   auto clean1 =	  vtkSmartPointer<vtkCleanPolyData>::New();
   clean1->SetInputData(input1);
   clean1->Update();

  auto mapper =   vtkSmartPointer<vtkPolyDataMapper>::New();
  mapper->SetInputData(clean1->GetOutput());
  mapper->Update();
  //mapper->SetInputConnection(clean1->GetOutputPort());
 

  auto actor =	  vtkSmartPointer<vtkActor>::New();
  actor->SetMapper(mapper);
   this->Renderer->AddActor(actor);
   */
  
	//

	cornerAnnotation= vtkSmartPointer<vtkCornerAnnotation>::New();

	this->Renderer->AddViewProp(cornerAnnotation);
	this->Renderer->AddActor(this->GridActor);
	this->Renderer->TwoSidedLightingOff();
		

}

mqMorphoDigCore::~mqMorphoDigCore()
{
	//this->ActorCollection->Delete();
	this->mui_ExistingScalars->Stack.clear();
	delete this->mui_ExistingScalars;
	delete this->mui_ActiveScalars;

	this->mui_ExistingColorMaps->Stack.clear();
	delete this->mui_ExistingColorMaps;
	delete this->mui_ActiveColorMap;

	if (mqMorphoDigCore::Instance == this)
	{
		mqMorphoDigCore::Instance = 0;
	}
}
void mqMorphoDigCore::ActivateClippingPlane()
{
	if (this->Getmui_ClippinPlane() == 1)
	{

		double cr[2];
		double cameracentre[3];
		double camerafocalpoint[3];
		
		this->getRenderer()->GetActiveCamera()->GetPosition(cameracentre);
		this->getRenderer()->GetActiveCamera()->GetFocalPoint(camerafocalpoint);
		double dist = sqrt((cameracentre[0] - camerafocalpoint[0])*(cameracentre[0] - camerafocalpoint[0])
		+ (cameracentre[1] - camerafocalpoint[1])*(cameracentre[1] - camerafocalpoint[1])
			+ (cameracentre[2] - camerafocalpoint[2])*(cameracentre[2] - camerafocalpoint[2])
		);
		this->getRenderer()->GetActiveCamera()->GetClippingRange(cr);

		this->getRenderer()->GetActiveCamera()->SetClippingRange(dist, cr[1]);
	}
}
void mqMorphoDigCore::ActivateBackfaceCulling() {

	this->ActorCollection->InitTraversal();

	for (vtkIdType i = 0; i < this->ActorCollection->GetNumberOfItems(); i++)
	{
		vtkMDActor * myActor = vtkMDActor::SafeDownCast(this->ActorCollection->GetNextActor());
		
			if (this->mui_BackfaceCulling == 0)
			{
				myActor->GetProperty()->BackfaceCullingOff();
			}
			else
			{
				myActor->GetProperty()->BackfaceCullingOn();
			}
					
	}

}
void mqMorphoDigCore::ChangeBackfaceCulling() {
	if (this->mui_BackfaceCulling == 0) { this->mui_BackfaceCulling = 1; }
	else { this->mui_BackfaceCulling = 0; }
	this->ActivateBackfaceCulling();
}

int mqMorphoDigCore::Getmui_BackfaceCulling() { return this->mui_BackfaceCulling; }
void mqMorphoDigCore::Setmui_BackfaceCulling(int on_off) {
	if (on_off == 0 || on_off == 1) { this->mui_BackfaceCulling = on_off; }
}

void mqMorphoDigCore::ChangeClippingPlane() 
{
	if (this->mui_ClippingPlane == 0) { this->mui_ClippingPlane = 1; }
	else{ this->mui_ClippingPlane = 0; }
	this->ActivateClippingPlane();

}
int mqMorphoDigCore::Getmui_ClippinPlane() 
{
	return this->mui_ClippingPlane;
}
void mqMorphoDigCore::Setmui_ClippinPlane(int on_off) 
{
	if (on_off == 0 || on_off == 1) { this->mui_ClippingPlane = on_off; }
}

vtkSmartPointer<vtkLookupTable> mqMorphoDigCore::GetTagLut() 
{
	return this->TagLut;
}
vtkSmartPointer<vtkDiscretizableColorTransferFunction> mqMorphoDigCore::GetScalarRainbowLut()
{
	return this->ScalarRainbowLut;
}
vtkSmartPointer<vtkDiscretizableColorTransferFunction> mqMorphoDigCore::GetScalarRedLut()
{
	return this->ScalarRedLut;
}

double mqMorphoDigCore::GetScalarRangeMin()
{

	return this->ScalarRangeMin;
}

double mqMorphoDigCore::GetScalarRangeMax()
{

	return this->ScalarRangeMax;
}

double mqMorphoDigCore::GetSuggestedScalarRangeMin()
{

	double my_min;
	double my_currmin;

	my_min = DBL_MAX;
	my_currmin = DBL_MAX;

	this->ActorCollection->InitTraversal();

	for (vtkIdType i = 0; i < this->ActorCollection->GetNumberOfItems(); i++)
	{
		vtkMDActor * myActor = vtkMDActor::SafeDownCast(this->ActorCollection->GetNextActor());
		vtkPolyDataMapper *mapper = vtkPolyDataMapper::SafeDownCast(myActor->GetMapper());

		if (mapper != NULL && vtkPolyData::SafeDownCast(mapper->GetInput()) != NULL)
		{
			vtkPolyData *myPD = vtkPolyData::SafeDownCast(mapper->GetInput());
			if (myPD->GetPointData()->GetScalars(this->Getmui_ActiveScalars()->Name.toStdString().c_str()) != NULL
				&& (
					this->Getmui_ActiveScalars()->DataType == VTK_FLOAT ||
					this->Getmui_ActiveScalars()->DataType == VTK_DOUBLE

					)
				&& this->Getmui_ActiveScalars()->NumComp == 1
				)
			{
				if (this->Getmui_ActiveScalars()->DataType == VTK_FLOAT)
				{
					vtkFloatArray *currentScalars = (vtkFloatArray*)myPD->GetPointData()->GetScalars(this->Getmui_ActiveScalars()->Name.toStdString().c_str());
					if (currentScalars != NULL)
					{
						my_currmin = DBL_MAX;
						my_currmin = (double)currentScalars->GetTuple(0)[0];


						std::vector<float> vals;
						for (int i = 0; i < myPD->GetNumberOfPoints(); i++)
						{
							vals.push_back(currentScalars->GetTuple(i)[0]);
						}

						std::sort(vals.begin(), vals.end());

						int iQ = (int)(0.05*myPD->GetNumberOfPoints());
						my_currmin = (double)vals.at(iQ);

						if (my_currmin < my_min) { my_min = my_currmin; }
					}
				}
				else
				{
					vtkDoubleArray *currentScalars = (vtkDoubleArray*)myPD->GetPointData()->GetScalars(this->Getmui_ActiveScalars()->Name.toStdString().c_str());
					if (currentScalars != NULL)
					{
						my_currmin = DBL_MAX;
						my_currmin = currentScalars->GetTuple(0)[0];


						std::vector<double> vals;
						for (int i = 0; i < myPD->GetNumberOfPoints(); i++)
						{
							vals.push_back(currentScalars->GetTuple(i)[0]);
						}

						std::sort(vals.begin(), vals.end());

						int iQ = (int)(0.05*myPD->GetNumberOfPoints());
						my_currmin = vals.at(iQ);

						if (my_currmin < my_min) { my_min = my_currmin; }
					}

				}

			}
		}
	}
	if (my_min == VTK_DOUBLE_MAX || my_min == VTK_FLOAT_MAX)
	{
		cout << "Strange!!!" << endl;
		return 0;
	}
	else
	{
		return my_min;
	}

}

double mqMorphoDigCore::GetSuggestedScalarRangeMax()
{
	double my_max;
	double my_currmax;

	my_max = -DBL_MAX;
	
	my_currmax = -DBL_MAX;

	this->ActorCollection->InitTraversal();

	for (vtkIdType i = 0; i < this->ActorCollection->GetNumberOfItems(); i++)
	{
		vtkMDActor * myActor = vtkMDActor::SafeDownCast(this->ActorCollection->GetNextActor());
		vtkPolyDataMapper *mapper = vtkPolyDataMapper::SafeDownCast(myActor->GetMapper());

		if (mapper != NULL && vtkPolyData::SafeDownCast(mapper->GetInput()) != NULL)
		{
			vtkPolyData *myPD =vtkPolyData::SafeDownCast(mapper->GetInput());
			if (myPD->GetPointData()->GetScalars(this->Getmui_ActiveScalars()->Name.toStdString().c_str()) != NULL
				&& (
					this->Getmui_ActiveScalars()->DataType ==VTK_FLOAT ||
					this->Getmui_ActiveScalars()->DataType == VTK_DOUBLE

					)
				&& this->Getmui_ActiveScalars()->NumComp == 1
				)
			{
				if (this->Getmui_ActiveScalars()->DataType == VTK_FLOAT)
				{
					vtkFloatArray *currentScalars = (vtkFloatArray*)myPD->GetPointData()->GetScalars(this->Getmui_ActiveScalars()->Name.toStdString().c_str());					
					if (currentScalars != NULL)
					{
						
						my_currmax = (double)currentScalars->GetTuple(0)[0];
					
						std::vector<float> vals;
						for (int i = 0; i < myPD->GetNumberOfPoints(); i++)
						{
							vals.push_back(currentScalars->GetTuple(i)[0]);
						}

						std::sort(vals.begin(), vals.end());

						int iQ = (int)(0.95*myPD->GetNumberOfPoints());
					
						my_currmax = (double)vals.at(iQ);
						if (my_currmax > my_max) { my_max = my_currmax; }
						
					}
				}
				else
				{
					vtkDoubleArray *currentScalars = (vtkDoubleArray*)myPD->GetPointData()->GetScalars(this->Getmui_ActiveScalars()->Name.toStdString().c_str());
					if (currentScalars != NULL)
					{
						
						my_currmax = currentScalars->GetTuple(0)[0];

					
						std::vector<double> vals;
						for (int i = 0; i < myPD->GetNumberOfPoints(); i++)
						{
							vals.push_back(currentScalars->GetTuple(i)[0]);
						}

						std::sort(vals.begin(), vals.end());

						int iQ = (int)(0.95*myPD->GetNumberOfPoints());
						my_currmax = vals.at(iQ);
						
						if (my_currmax > my_max) { my_max = my_currmax; }
					}

				}

			}
		}							
	}
	if (my_max == VTK_DOUBLE_MIN || my_max == VTK_FLOAT_MIN)
	{
		return 1;
	}
	else 
	{
		cout << "my_max=" << my_max << endl;
		return my_max;
	}
	
	
}

void mqMorphoDigCore::UpddateLookupTablesRanges(double min, double max)
{
	for (int i = 0; i < this->mui_ExistingColorMaps->Stack.size(); i++)
	{
		vtkSmartPointer<vtkDiscretizableColorTransferFunction> CM = this->mui_ExistingColorMaps->Stack.at(i).ColorMap;
		
		double *pts = CM->GetDataPointer();
		
		int numnodes = CM->GetSize();
		cout << this->mui_ExistingColorMaps->Stack.at(i).Name.toStdString() << ": num nodes = " << numnodes << endl;
		double old_min = DBL_MAX;
		double old_max = -DBL_MAX;
		for (int j = 0; j < numnodes; j++)
		{
			double curr = pts[4 * j];
			cout << "x" << j << "=" << curr << endl;
			//if (curr < old_min) { old_min = curr; }
			if (curr > old_max) { old_max = curr; }

		}
		if (old_max > old_min)
		{
			double old_range = old_max - old_min;
			double new_range = max - min;
			double mult = new_range / old_range;
			double c = min - old_min*mult;
			for (int k = 0; k < numnodes; k++)
			{
				pts[4 * k] = pts[4 * k] * mult + c;
				//cout << "nx" << k << "=" << pts[4 * k] << endl;
			}
			CM->FillFromDataPointer(numnodes, pts);

		}
		vtkPiecewiseFunction* OF = CM->GetScalarOpacityFunction();
		int numnodes2 = OF->GetSize();
		double *pts2 = OF->GetDataPointer();
		//cout << this->mui_ExistingColorMaps->Stack.at(i).Name.toStdString() << ": OF num nodes = " << numnodes2 << endl;
		double old_min2 = DBL_MAX;
		double old_max2 = -DBL_MAX;
		for (int j = 0; j < numnodes2; j++)
		{
			double curr = pts2[2*j];
			//cout << "x" << j << "=" << curr << endl;
			if (curr < old_min2) { old_min2 = curr; }
			if (curr > old_max2) { old_max2 = curr; }

		}
		if (old_max2 > old_min2)
		{
			double old_range = old_max2 - old_min2;
			double new_range = max - min;
			double mult = new_range / old_range;
			double c = min - old_min2*mult;
			for (int k = 0; k < numnodes2; k++)
			{
				pts2[2*k] = pts2[2*k] * mult + c;
				//cout << "nx" << k << "=" << pts2[2*k] << endl;
			}
			OF->FillFromDataPointer(numnodes2, pts2);

		}
	}
	this->Render();

}

void mqMorphoDigCore::InitLuts()
{
	cout << "Start Init LUTS!" << endl;
	this->TagLut->SetNumberOfTableValues(this->TagTableSize);
	this->TagLut->Build();

	// Fill in a few known colors, the rest will be generated if needed
	TagLut->SetTableValue(0, 0, 0, 0, 1);  //Black
	TagLut->SetTableValue(1, 0.8900, 0.8100, 0.3400, 1); // Banana
	TagLut->SetTableValue(2, 1.0000, 0.3882, 0.2784, 1); // Tomato
	TagLut->SetTableValue(3, 0.9608, 0.8706, 0.7020, 1); // Wheat
	TagLut->SetTableValue(4, 0.9020, 0.9020, 0.9804, 1); // Lavender
	TagLut->SetTableValue(5, 1.0000, 0.4900, 0.2500, 1); // Flesh
	TagLut->SetTableValue(6, 0.5300, 0.1500, 0.3400, 1); // Raspberry
	TagLut->SetTableValue(7, 0.9804, 0.5020, 0.4471, 1); // Salmon
	TagLut->SetTableValue(8, 0.7400, 0.9900, 0.7900, 1); // Mint
	TagLut->SetTableValue(9, 0.2000, 0.6300, 0.7900, 1);
	
	this->ScalarRainbowLut->DiscretizeOn();
	this->ScalarRainbowLut->SetColorSpaceToRGB();
	//this->ScalarRainbowLut->EnableOpacityMappingOn();

	
	this->ScalarRainbowLut->AddRGBPoint(0.0, 1.0, 0.0, 1.0); //# purple
	this->ScalarRainbowLut->AddRGBPoint(0.2, 0.0, 0.0, 1.0); //# blue
	this->ScalarRainbowLut->AddRGBPoint(0.4, 0.0, 1.0, 1.0); //# purple
	this->ScalarRainbowLut->AddRGBPoint(0.6, 0.0, 1.0, 0.0); //# green
	this->ScalarRainbowLut->AddRGBPoint(0.8, 1.0, 1.0, 0.0); //# yellow
	this->ScalarRainbowLut->AddRGBPoint(1.0, 1.0, 0.0, 0.0); //# red
	vtkSmartPointer<vtkPiecewiseFunction> opacityRfunction = vtkSmartPointer<vtkPiecewiseFunction>::New();

	opacityRfunction->AddPoint(0, 0.3);
	opacityRfunction->AddPoint(0.2, 0.6);
	opacityRfunction->AddPoint(0.8, 0.8);
	opacityRfunction->AddPoint(1, 1);

	this->ScalarRainbowLut->SetScalarOpacityFunction(opacityRfunction);
	this->ScalarRainbowLut->EnableOpacityMappingOn();
	this->ScalarRainbowLut->Build();
	
    this->ScalarRedLut->SetColorSpaceToRGB();
	this->ScalarRedLut->EnableOpacityMappingOn();
	this->ScalarRedLut->AddRGBPoint(0.0, 0.0, 0.0, 0.0); //# black
	this->ScalarRedLut->AddRGBPoint(0.4, 1.0, 0, 0.0); //# reddish
	this->ScalarRedLut->AddRGBPoint(0.8, 1.0, 0.4900, 0.25);// # flesh
	this->ScalarRedLut->AddRGBPoint(1.0, 1.0, 1.0, 1.0);// # white

	// 	vtkPiecewiseFunction
	//cmap->SetScalarOpacityFunction();
	
	

	vtkSmartPointer<vtkPiecewiseFunction> opacityfunction =  vtkSmartPointer<vtkPiecewiseFunction>::New();

	opacityfunction->AddPoint(0, 0.3);
	opacityfunction->AddPoint(0.4, 0.4);
	opacityfunction->AddPoint(0.8, 0.6);
	opacityfunction->AddPoint(1, 1);
	
	this->ScalarRedLut->SetScalarOpacityFunction(opacityfunction);
	this->ScalarRedLut->Build();

	/*this->mui_ActiveColorMap->ColorMap = this->ScalarRedLut;
	this->mui_ActiveColorMap->Name = QString("Black-Red-White_Alpha");*/
	cout << "Set Active color map!" << endl;
	this->mui_ActiveColorMap->ColorMap = this->ScalarRainbowLut;
	cout << "Set Active color map2!" << endl;
	QString Rainbow = QString("Rainbow");
	this->mui_ActiveColorMap->Name = Rainbow;

	cout << "Try to set existing color maps!!" << endl;
	this->mui_ExistingColorMaps->Stack.push_back(ExistingColorMaps::Element(Rainbow, this->ScalarRainbowLut));

	cout << "Try to set existing color maps 2!!" << endl;
	QString BRWA = QString("Black-Red-White_Alpha");

	this->mui_ExistingColorMaps->Stack.push_back(ExistingColorMaps::Element(BRWA,this->ScalarRedLut));
	cout << "Try to set existing color maps 3!!" << endl;


	vtkSmartPointer<vtkFloatArray> scalarValues = vtkSmartPointer<vtkFloatArray>::New();
	scalarValues->SetNumberOfComponents(1);
	scalarValues->SetNumberOfTuples(256);
	for (int i = 0; i < 256; i++)
	{

		const float val = (float)i / 255;
		//		scalarValues->SetTupleValue(i, &val);
		scalarValues->SetTypedTuple(i, &val);
	}
	
	//cmap->
	//cmap->MapScalars(scalarValues, 0, -1);
	//this->ScalarRedLut = vtkLookupTable::SafeDownCast(); 
	//this->ScalarRainbowLut = vtkLookupTable::SafeDownCast(cmap->MapScalars(scalarValues, 0, 1));
	
	/*for (int i = 0; i < 256; i++)
	{
		double* machin= scalarValues->GetTuple(i);
		//scalarValues->GetTupleGetTupleValue(i, &machin);

		//cout << "tuple i=" << machin[0] << endl;
	}*/

	/*
	
> for i in xrange(256) :
>     scalarValues.SetTupleValue(i, [i / 255.0])
>
> table = cmap.MapScalars(scalarValues, 0, -1)
>
> for i in xrange(table.GetNumberOfTuples()) :
>     print table.GetTuple(i)*/
	
	

}
void mqMorphoDigCore::ComputeSelectedNamesLists()
{
	
		g_selected_names.clear();
		g_distinct_selected_names.clear();
	
		this->ActorCollection->InitTraversal();

	for (vtkIdType i = 0; i < this->ActorCollection->GetNumberOfItems(); i++)
	{
		vtkMDActor * myActor = vtkMDActor::SafeDownCast(this->ActorCollection->GetNextActor());
		if (myActor->GetSelected() == 1)
		{
			g_selected_names.push_back(myActor->GetName());

			int already = 0;

			for (int j = 0; j<g_distinct_selected_names.size(); j++)
			{
				//std::cout<<"i"<<i<<std::endl;
				std::size_t found = g_distinct_selected_names.at(j).find(myActor->GetName());
				size_t length1 = myActor->GetName().length();
				size_t length2 = g_distinct_selected_names.at(j).length();
				if (length1 == length2 && found != std::string::npos)
				{
					already = 1;
				}
			}
			if (already == 0)
			{
				g_distinct_selected_names.push_back(myActor->GetName());
			}


		}
	}
	
}

int mqMorphoDigCore::context_file_exists(std::string path, std::string ext, std::string postfix)
{
	// used by the save NTW function : 
	//looks whether a non-mesh related file = context file (.ori, .flg, .tag, .ver, .cur, .stv) constructed with only a postfix + extension (project name) already exists 
	std::string filename;
	int exists = 0;

	filename = path.c_str();
	if (postfix.length()>0)
	{
		filename.append(postfix.c_str());
	}
	filename.append(ext.c_str());
	ifstream file(filename.c_str());
	if (file)
	{
		file.close();
		exists = 1;
	}
	return exists;
}

int mqMorphoDigCore::selected_file_exists(std::string path, std::string ext, std::string postfix)
{
	// used by the save NTW function : 
	//looks whether mesh related files (vtk, vtp, stl, ply, pos) constructed with a prefix (object name) and a postfix (project name) already exist 
	std::string filename;
	int exists = 0;
	for (int i = 0; i<g_distinct_selected_names.size(); i++)
	{
		filename = path.c_str();
		filename.append(g_distinct_selected_names.at(i).c_str());
		if (postfix.length()>0)
		{
			filename.append(postfix.c_str());
		}
		filename.append(ext.c_str());
		ifstream file(filename.c_str());
		if (file)
		{
			file.close();
			exists = 1;
			break;
		}
	}
	//cout << "file " << path << " " << ext <<  postfix<< " exists" << exists;
	return exists;

}

void mqMorphoDigCore::SaveORI(QString fileName)
{

	QString X1, X2, Y1, Y2, Z1, Z2;

	std::string ORIext = ".ori";
	std::string ORIext2 = ".ORI";
	std::size_t found = fileName.toStdString().find(ORIext);
	std::size_t found2 = fileName.toStdString().find(ORIext2);
	if (found == std::string::npos && found2 == std::string::npos)
	{
		fileName.append(".ori");
	}

	QFile file(fileName);
	if (file.open(QIODevice::WriteOnly | QIODevice::Text))
	{
		QTextStream stream(&file);

		stream << this->Getmui_Z1Label() << endl;
		stream << this->Getmui_Z2Label() << endl;
		stream << this->Getmui_Y1Label() << endl;
		stream << this->Getmui_Y2Label() << endl;
		stream << this->Getmui_X1Label() << endl;
		stream << this->Getmui_X2Label() << endl;


	}
	file.close();





}
void mqMorphoDigCore::UpdateAllSelectedFlagsColors()
{
	double r1, g1, b1;
	vtkIdType selectedflags = this->getFlagLandmarkCollection()->GetNumberOfSelectedActors();
	if (selectedflags>0)
	{

		vtkSmartPointer<vtkLMActorCollection> myColl = vtkSmartPointer<vtkLMActorCollection>::New();
		myColl = this->FlagLandmarkCollection;
		
		std::string action = "Update color of all selected flags";
		int mCount = BEGIN_UNDO_SET(action);
		
		
		myColl->InitTraversal();
		for (vtkIdType k = 0; k < myColl->GetNumberOfItems(); k++)
		{
			int ok = 0;
			vtkLMActor *myFlag = vtkLMActor::SafeDownCast(myColl->GetNextActor());
			double min_dist = DBL_MAX;
			if (myFlag->GetSelected() == 1)
			{
				double flpos[3];
				
				myFlag->GetLMOrigin(flpos);
				double closest[3] = { flpos[0] , flpos[1],flpos[2] };
				cout << "Current flag :" << flpos[0] << "," << flpos[1] << "," << flpos[2] << endl;

				this->ActorCollection->InitTraversal();
				for (vtkIdType i = 0; i < this->ActorCollection->GetNumberOfItems(); i++)
				{
					vtkMDActor *myActor = vtkMDActor::SafeDownCast(this->ActorCollection->GetNextActor());
					
					if (myActor->GetSelected() == 1) { myActor->SetSelected(0); }
					vtkPolyDataMapper *mapper = vtkPolyDataMapper::SafeDownCast(myActor->GetMapper());
					if (mapper != NULL && vtkPolyData::SafeDownCast(mapper->GetInput()) != NULL)
					{
						vtkSmartPointer<vtkPolyData> myPD = vtkSmartPointer<vtkPolyData>::New();
						myPD->DeepCopy(vtkPolyData::SafeDownCast(mapper->GetInput()));
						double ve_init_pos[3];;
						double ve_final_pos[3];
						vtkSmartPointer<vtkMatrix4x4> Mat = myActor->GetMatrix();

						vtkIdType id_min=NULL;
						for (vtkIdType j = 0; j < myPD->GetNumberOfPoints(); j++) 
						{
							// for every triangle 
							myPD->GetPoint(j, ve_init_pos);
							mqMorphoDigCore::TransformPoint(Mat, ve_init_pos, ve_final_pos);

							double curr_dist = (ve_final_pos[0] - flpos[0])*(ve_final_pos[0] - flpos[0]) +
								(ve_final_pos[1] - flpos[1])*(ve_final_pos[1] - flpos[1]) +
								(ve_final_pos[2] - flpos[2])*(ve_final_pos[2] - flpos[2]);
							if (min_dist>curr_dist)
							{
								id_min = j;
								min_dist = curr_dist;
								//now get current color!
								
							}
							
						}
						if (id_min != NULL) // means that mesh i could be the mesh that contains the closest vertex of flag k
						{
							//now get current color of point id_min of mesh i!
							//@TODO! => On va faire 1 variable globale de type G_Current_Active_Scalar => Ce premier if sera chang� par 
							// if (visibility ==0 OU GetScalar(G_Current_Active_Scalar)==NULL)
							QString none = QString("none");
							if (this->Getmui_ScalarVisibility() == 0 || this->mui_ActiveScalars->Name== none||
							myPD->GetPointData()->GetScalars(this->mui_ActiveScalars->Name.toStdString().c_str()) == NULL )
								
							{
								myActor->GetProperty()->GetColor(r1, g1, b1);
								cout << "Mesh PLAIN color " <<i<<"("<< myActor->GetName() << "): " << "r="<<r1 << ", g=" << g1<< ", b="<<b1 << endl;
								ok = 1;
							}
							else
							{
								if (
									(this->mui_ActiveScalars->DataType == VTK_INT || this->mui_ActiveScalars->DataType == VTK_UNSIGNED_INT)
									&& this->mui_ActiveScalars->NumComp==1
									)
								{ 
								// Tag-like scalar!!!!
									vtkIntArray *currentTagsLike;
									currentTagsLike = (vtkIntArray*)myPD->GetPointData()->GetScalars(this->mui_ActiveScalars->Name.toStdString().c_str());
									int mytag = currentTagsLike->GetTuple(id_min)[0];
									vtkSmartPointer<vtkLookupTable> lut = vtkSmartPointer<vtkLookupTable>::New();
									lut = vtkLookupTable::SafeDownCast(mapper->GetLookupTable());
									double rgb[3];
									lut->GetColor((double)mytag, rgb);
									r1 = rgb[0];
									g1 = rgb[1];
									b1 = rgb[2];
									cout << "Mesh TAG-like color " << i << "(" << myActor->GetName() << "): " << "r=" << r1 << ", g=" << g1 << ", b=" << b1 << endl;
									ok = 1;								
								}
								else if (
									(this->mui_ActiveScalars->DataType == VTK_UNSIGNED_CHAR)
									&& (this->mui_ActiveScalars->NumComp == 3 || this->mui_ActiveScalars->NumComp == 4)
									)
								{
									//RGB-Like Scalars
									int numcomp = this->mui_ActiveScalars->NumComp;
									vtkSmartPointer<vtkUnsignedCharArray> colors =
										vtkSmartPointer<vtkUnsignedCharArray>::New();
									colors->SetNumberOfComponents(numcomp);
									colors = (vtkUnsignedCharArray*)myPD->GetPointData()->GetScalars(this->mui_ActiveScalars->Name.toStdString().c_str());
									double cur_r = (double)colors->GetTuple(id_min)[0];
									double cur_g = (double)colors->GetTuple(id_min)[1];
									double cur_b = (double)colors->GetTuple(id_min)[2];
									r1 = cur_r / 255;
									g1 = cur_g / 255;
									b1 = cur_b / 255;
									cout << "Mesh RGB-like color " << i << "(" << myActor->GetName() << "): " << "r=" << r1 << ", g=" << g1 << ", b=" << b1 << endl;
									ok = 1;

								}
								else if (
									(this->mui_ActiveScalars->DataType == VTK_DOUBLE || this->mui_ActiveScalars->DataType == VTK_FLOAT)
									&& this->mui_ActiveScalars->NumComp == 1
									)
								{
									if (this->mui_ActiveScalars->DataType == VTK_FLOAT)
									{
										// Tag-like scalar!!!!
										vtkFloatArray *currentScalars;
										currentScalars = (vtkFloatArray*)myPD->GetPointData()->GetScalars(this->mui_ActiveScalars->Name.toStdString().c_str());
										float myscalar = currentScalars->GetTuple(id_min)[0];
										vtkSmartPointer<vtkLookupTable> lut = vtkLookupTable::SafeDownCast(mapper->GetLookupTable());
										double rgb[3];
										lut->GetColor(myscalar, rgb);
										r1 = rgb[0];
										g1 = rgb[1];
										b1 = rgb[2];
										cout << "Mesh Float-Scalar color " << i << "(" << myActor->GetName() << "): " << "r=" << r1 << ", g=" << g1 << ", b=" << b1 << endl;
										ok = 1;
									}
									else
									{
										// Tag-like scalar!!!!
										vtkDoubleArray *currentScalars;
										currentScalars = (vtkDoubleArray*)myPD->GetPointData()->GetScalars(this->mui_ActiveScalars->Name.toStdString().c_str());
										
										double myscalar = currentScalars->GetTuple(id_min)[0];
										vtkSmartPointer<vtkLookupTable> lut = vtkLookupTable::SafeDownCast(mapper->GetLookupTable());
										double rgb[3];
										lut->GetColor(myscalar, rgb);
										r1 = rgb[0];
										g1 = rgb[1];
										b1 = rgb[2];
										cout << "Mesh Double-Scalar color " << i << "(" << myActor->GetName() << "): " << "r=" << r1 << ", g=" << g1 << ", b=" << b1 << endl;
										ok = 1;

									}
								}
								
									
							}

						}
					}

				}
				
				
				myFlag->SetSelected(0);
				if (ok == 1)
				{
					myFlag->SaveState(mCount);
					myFlag->SetmColor(r1,g1,b1,0.5);

				}
				this->UpdateLandmarkSettings(myFlag);



			}

		}
		END_UNDO_SET();

		this->Render();
	}
}

void mqMorphoDigCore::UpdateAllSelectedFlagsColors(double flagcolor[4])
{
	vtkIdType selectedflags = this->getFlagLandmarkCollection()->GetNumberOfSelectedActors();
	if (selectedflags>0)
	{
		
		vtkSmartPointer<vtkLMActorCollection> myColl = vtkSmartPointer<vtkLMActorCollection>::New();
		myColl = this->FlagLandmarkCollection;


		myColl->InitTraversal();
		for (vtkIdType i = 0; i < myColl->GetNumberOfItems(); i++)
		{
			vtkLMActor *myActor = vtkLMActor::SafeDownCast(myColl->GetNextActor());
			if (myActor->GetSelected() == 1 )
			{

				myActor->SetmColor(flagcolor);
				myActor->SetSelected(0);
				this->UpdateLandmarkSettings(myActor);
				


			}

		}


	}

}
void mqMorphoDigCore::UpdateAllSelectedFlagsLengths(double flag_rendering_size)
{
	vtkIdType selectedflags = this->getFlagLandmarkCollection()->GetNumberOfSelectedActors();
	if (selectedflags>0)
	{

		vtkSmartPointer<vtkLMActorCollection> myColl = vtkSmartPointer<vtkLMActorCollection>::New();
		myColl = this->FlagLandmarkCollection;


		myColl->InitTraversal();
		for (vtkIdType i = 0; i < myColl->GetNumberOfItems(); i++)
		{
			vtkLMActor *myActor = vtkLMActor::SafeDownCast(myColl->GetNextActor());
			if (myActor->GetSelected() == 1)
			{

				myActor->SetLMSize(flag_rendering_size);
				myActor->SetSelected(0);
				this->UpdateLandmarkSettings(myActor);



			}

		}


	}

}

void mqMorphoDigCore::OpenFLG(QString fileName)
{
	//cout << "OpenFLG " << fileName.toStdString() << endl;
	double  x, y, z, nx, ny, nz, flength, r, g, b;

	QString FLGName;

	//Open a landmark file!


	size_t  length;


	length = fileName.toStdString().length();

	int done = 0;
	if (length>0)
	{
		int file_exists = 1;
		ifstream file(fileName.toStdString().c_str());
		if (file)
		{
			//std::cout<<"file:"<<filename.c_str()<<" exists."<<std::endl;
			file.close();
		}
		else
		{

			//std::cout << "file:" << fileName.toStdString().c_str() << " does not exists." << std::endl;
			file_exists = 0;
		}

		if (file_exists == 1)
		{

			std::string FLGext(".flg");
			std::string FLGext2(".FLG");


			int type = 0; // 0 = .POS Ascii File //1 = .MAT binary File or simple .MAT file

			std::size_t found = fileName.toStdString().find(FLGext);
			std::size_t found2 = fileName.toStdString().find(FLGext2);
			if (found != std::string::npos || found2 != std::string::npos)
			{
				type = 1;
				//FLG
			}



			if (type == 1)
			{


				//filein = fopen(fileName.toStdString().c_str(), "rt");
				QFile inputFile(fileName);
				int ok = 0;

				if (inputFile.open(QIODevice::ReadOnly))
				{
					QTextStream in(&inputFile);
					int cpt = 1;
					while (!in.atEnd())
					{
						QString line = in.readLine();
						QTextStream myteststream(&line);
						if (cpt % 2 == 0)
						{

							myteststream >> x >> y >> z >> nx >> ny >> nz >> flength >> r >> g >> b;
							double coord[3] = { x,y,z };
							double ncoord[3] = { nx,ny,nz };
							double ori[3];

							double length = nx*nx + ny*ny + nz*nz;
							if (length == 1)
							{
								ori[0] = ncoord[0];
								ori[1] = ncoord[1];
								ori[2] = ncoord[2];
							}
							else
							{
								vtkMath::Subtract(ncoord, coord, ori);
								vtkMath::Normalize(ori);
							}
							this->CreateLandmark(coord, ori, 4);
							vtkLMActor *myLastFlag = this->GetLastLandmark(4);
							myLastFlag->SetLMType(4);
							myLastFlag->SetmColor(r, g, b, 0.5);
							myLastFlag->SetLMText(FLGName.toStdString());
							//cout << "Set LM Size:" << flength << endl;
							myLastFlag->SetLMSize(flength);
							myLastFlag->SetChanged(1);
						}
						else
						{
							FLGName = line;
						}
						cpt++;

					}
					/**/

					inputFile.close();
					this->UpdateLandmarkSettings();

				}
			}//fin if																		

		}//file exists...
	}

}
void mqMorphoDigCore::OpenPOS(QString fileName, int mode)
{
	// mode : 0 for last inserted mesh
	// mode : 1 for all selected meshes
	// mode : 2 for all selected landmarks/flags


	//Open a position file!

	int i, j, l;
	size_t  length;


	length = fileName.toStdString().length();

	union {
		float f;
		char c[4];
	} u; // holds one float or 4 characters (bytes)



	int done = 0;
	if (length>0)
	{
		int file_exists = 1;
		ifstream file(fileName.toStdString().c_str());
		if (file)
		{
			//std::cout<<"file:"<<filename.c_str()<<" exists."<<std::endl;
			file.close();
		}
		else
		{

			//std::cout << "file:" << fileName.toStdString().c_str() << " does not exists." << std::endl;
			file_exists = 0;
		}

		if (file_exists == 1)
		{

			std::string MAText(".mat");
			std::string MAText2(".MAT");
			std::string POSext(".pos");
			std::string POSext2(".POS");

			int type = 0; // 0 = .POS Ascii File //1 = .MAT binary File or simple .MAT file

			std::size_t found = fileName.toStdString().find(MAText);
			std::size_t found2 = fileName.toStdString().find(MAText2);
			if (found != std::string::npos || found2 != std::string::npos)
			{
				type = 1;
				//MAT
			}

			found = fileName.toStdString().find(POSext);
			found2 = fileName.toStdString().find(POSext2);
			if (found != std::string::npos || found2 != std::string::npos)
			{
				type = 0; //POS
			}




			int Ok = 1;
			vtkSmartPointer<vtkMatrix4x4> Mat = vtkSmartPointer<vtkMatrix4x4>::New();



			if (type == 1)
			{
				FILE	*filein;									// Filename To Open
				filein = fopen(fileName.toStdString().c_str(), "rb");
				for (i = 0; i<4; i++)
					for (j = 0; j<4; j++)
					{

						for (l = 3; l >= 0; l--)
						{
							u.c[l] = fgetc(filein);
						}
						//Mat1[j][i] = u.f;
						//My_Obj->Mat1[i][j] = u.f;
					}


				for (i = 0; i<4; i++)
					for (j = 0; j<4; j++)
					{
						for (l = 3; l >= 0; l--)
						{
							u.c[l] = fgetc(filein);
						}
						Mat->SetElement(j, i, double(u.f));

					}

			}
			else
			{
				//filein = fopen(fileName.toStdString().c_str(), "rt");
				QFile inputFile(fileName);
				int ok = 0;

				if (inputFile.open(QIODevice::ReadOnly))
				{
					QTextStream in(&inputFile);

					// first matrix is useless (for the moment)	
					for (i = 0; i < 4; i++)
					{
						QString line = in.readLine();

					}
					//
					for (i = 0; i < 4; i++)
					{
						QString line = in.readLine();
						double n1, n2, n3, n4;
						QTextStream myteststream(&line);
						myteststream >> n1 >> n2 >> n3 >> n4;

						Mat->SetElement(0, i, n1);
						Mat->SetElement(1, i, n2);
						Mat->SetElement(2, i, n3);
						Mat->SetElement(3, i, n4);


					}
					inputFile.close();

				}
			}//fin if	



			 //cout << "call MorphoDig apply mat" << &Mat << endl;
			this->ApplyMatrix(Mat, mode);
			this->AdjustCameraAndGrid();

		}//file exists...
	}	//length

}

void mqMorphoDigCore::OpenPOSTrans(QString fileName, int mode)
{
	// mode : 0 for last inserted mesh
	// mode : 1 for all selected meshes
	// mode : 2 for all selected landmarks/flags


	//Open a position file!

	int i, j, l;
	size_t  length;


	length = fileName.toStdString().length();

	union {
		float f;
		char c[4];
	} u; // holds one float or 4 characters (bytes)



	int done = 0;
	if (length>0)
	{
		int file_exists = 1;
		ifstream file(fileName.toStdString().c_str());
		if (file)
		{
			//std::cout<<"file:"<<filename.c_str()<<" exists."<<std::endl;
			file.close();
		}
		else
		{

			//std::cout << "file:" << fileName.toStdString().c_str() << " does not exists." << std::endl;
			file_exists = 0;
		}

		if (file_exists == 1)
		{

			std::string MAText(".mat");
			std::string MAText2(".MAT");
			std::string POSext(".pos");
			std::string POSext2(".POS");

			int type = 0; // 0 = .POS Ascii File //1 = .MAT binary File or simple .MAT file

			std::size_t found = fileName.toStdString().find(MAText);
			std::size_t found2 = fileName.toStdString().find(MAText2);
			if (found != std::string::npos || found2 != std::string::npos)
			{
				type = 1;
				//MAT
			}

			found = fileName.toStdString().find(POSext);
			found2 = fileName.toStdString().find(POSext2);
			if (found != std::string::npos || found2 != std::string::npos)
			{
				type = 0; //POS
			}




			int Ok = 1;
			vtkSmartPointer<vtkMatrix4x4> Mat = vtkSmartPointer<vtkMatrix4x4>::New();



			if (type == 1)
			{
				FILE	*filein;									// Filename To Open


				filein = fopen(fileName.toStdString().c_str(), "rb");
				for (i = 0; i<4; i++)
					for (j = 0; j<4; j++)
					{

						for (l = 3; l >= 0; l--)
						{
							u.c[l] = fgetc(filein);
						}
						//My_Obj->Mat1[i][j] = u.f;
					}


				for (i = 0; i<4; i++)
					for (j = 0; j<4; j++)
					{
						for (l = 3; l >= 0; l--)
						{
							u.c[l] = fgetc(filein);
						}
						Mat->SetElement(i, j, double(u.f));

					}

			}
			else
			{
				//filein = fopen(fileName.toStdString().c_str(), "rt");
				QFile inputFile(fileName);
				int ok = 0;

				if (inputFile.open(QIODevice::ReadOnly))
				{
					QTextStream in(&inputFile);

					// first matrix is useless (for the moment)	
					for (i = 0; i < 4; i++)
					{
						QString line = in.readLine();

					}
					//
					for (i = 0; i < 4; i++)
					{
						QString line = in.readLine();
						double n1, n2, n3, n4;
						QTextStream myteststream(&line);
						myteststream >> n1 >> n2 >> n3 >> n4;

						Mat->SetElement(i, 0, n1);
						Mat->SetElement(i, 1, n2);
						Mat->SetElement(i, 2, n3);
						Mat->SetElement(i, 3, n4);


					}
					inputFile.close();

				}
			}//fin if		

			double N1, N2, N3;
			N1 = -(Mat->GetElement(3, 0) * Mat->GetElement(0, 0) +
				Mat->GetElement(3, 1) * Mat->GetElement(0, 1)
				+ Mat->GetElement(3, 2) * Mat->GetElement(0, 2));



			Mat->SetElement(0, 3, N1);



			N2 = -(Mat->GetElement(3, 0) * Mat->GetElement(1, 0) +
				Mat->GetElement(3, 1) * Mat->GetElement(1, 1)
				+ Mat->GetElement(3, 2) * Mat->GetElement(1, 2));

			Mat->SetElement(1, 3, N2);

			N3 = -(Mat->GetElement(3, 0) * Mat->GetElement(2, 0) +
				Mat->GetElement(3, 1) * Mat->GetElement(2, 1)
				+ Mat->GetElement(3, 2) * Mat->GetElement(2, 2));

			Mat->SetElement(2, 3, N3);


			Mat->SetElement(3, 0, 0);
			Mat->SetElement(3, 1, 0);
			Mat->SetElement(3, 2, 0);

			//cout << "call MorphoDig apply mat" << &Mat << endl;
			this->ApplyMatrix(Mat, mode);

			//ApplyChanges()
			this->AdjustCameraAndGrid();
		}//file exists...
	}	//length

}

void mqMorphoDigCore::OpenLMK(QString fileName, int mode)
{// mode : 0 for normal landmarks
 // mode : 1 for target landmarks
	double  x, y, z;
	QString LMKName;
	//Open a landmark file!

	cout << "Open a lmk file" << endl;
	size_t  length;


	length = fileName.toStdString().length();

	int done = 0;
	if (length>0)
	{
		int file_exists = 1;
		ifstream file(fileName.toStdString().c_str());
		if (file)
		{
			//std::cout<<"file:"<<filename.c_str()<<" exists."<<std::endl;
			file.close();
		}
		else
		{

			//std::cout << "file:" << fileName.toStdString().c_str() << " does not exists." << std::endl;
			file_exists = 0;
		}

		if (file_exists == 1)
		{

			std::string LMKext(".lmk");
			std::string LMKext2(".LMK");

			std::size_t found = fileName.toStdString().find(LMKext);
			std::size_t found2 = fileName.toStdString().find(LMKext2);
			if (found != std::string::npos || found2 != std::string::npos)
			{

				//filein = fopen(fileName.toStdString().c_str(), "rt");
				QFile inputFile(fileName);
				int ok = 0;

				if (inputFile.open(QIODevice::ReadOnly))
				{
					QTextStream in(&inputFile);

					while (!in.atEnd())
					{

						QString line = in.readLine();
						QTextStream myteststream(&line);
						myteststream >> LMKName >> x >> y >> z;
						double coord[3] = { x,y,z };
						double ori[3];


						ori[0] = 0;
						ori[1] = 0;
						ori[2] = 1;

						this->CreateLandmark(coord, ori, mode);

					}
					/**/

					inputFile.close();


				}

			}//fin if																		

		}//file exists...
	}	//length


}
void mqMorphoDigCore::OpenVER(QString fileName, int mode)
{// mode : 0 for normal landmarks
 // mode : 1 for target landmarks
 // mode : 2 for curve nodes
 // mode : 3 for curve handles
	double  x, y, z, nx, ny, nz;
	QString LMKName;
	//Open a landmark file!


	size_t  length;


	length = fileName.toStdString().length();

	int done = 0;
	if (length>0)
	{
		int file_exists = 1;
		ifstream file(fileName.toStdString().c_str());
		if (file)
		{
			//std::cout<<"file:"<<filename.c_str()<<" exists."<<std::endl;
			file.close();
		}
		else
		{

			std::cout << "file:" << fileName.toStdString().c_str() << " does not exists." << std::endl;
			file_exists = 0;
		}

		if (file_exists == 1)
		{

			std::string VERext(".ver");
			std::string VERext2(".VER");
			std::string LMKext(".LMK");
			std::string LMKext2(".LMK");

			int type = 0; // 0 = .POS Ascii File //1 = .MAT binary File or simple .MAT file

			std::size_t found = fileName.toStdString().find(LMKext);
			std::size_t found2 = fileName.toStdString().find(LMKext2);
			if (found != std::string::npos || found2 != std::string::npos)
			{
				type = 1;
				//LMK
			}

			found = fileName.toStdString().find(VERext);
			found2 = fileName.toStdString().find(VERext2);
			if (found != std::string::npos || found2 != std::string::npos)
			{
				type = 0; //VER
			}



			if (type == 1)
			{


			}
			else
			{
				//filein = fopen(fileName.toStdString().c_str(), "rt");
				QFile inputFile(fileName);
				int ok = 0;

				if (inputFile.open(QIODevice::ReadOnly))
				{
					QTextStream in(&inputFile);

					while (!in.atEnd())
					{

						QString line = in.readLine();
						QTextStream myteststream(&line);
						myteststream >> LMKName >> x >> y >> z >> nx >> ny >> nz;
						double coord[3] = { x,y,z };
						double ncoord[3] = { nx,ny,nz };
						double ori[3];

						double length = nx*nx + ny*ny + nz*nz;
						if (length == 1)
						{
							ori[0] = ncoord[0];
							ori[1] = ncoord[1];
							ori[2] = ncoord[2];
						}
						else
						{
							vtkMath::Subtract(ncoord, coord, ori);
							vtkMath::Normalize(ori);
						}
						this->CreateLandmark(coord, ori, mode);

					}
					/**/

					inputFile.close();


				}
			}//fin if																		

		}//file exists...
	}	//length


}

void mqMorphoDigCore::OpenMesh(QString fileName)
{

	int file_exists = 1;
	QFile file(fileName);
	QString name = "";
	if (file.exists()) {
		// Message
		name = file.fileName(); // Return only a file name		
		file.close();
	}
	else
	{
		file_exists = 0;


	}


	if (file_exists == 1)
	{
		std::string STLext(".stl");
		std::string STLext2(".STL");
		std::string VTKext(".vtk");
		std::string VTKext2(".VTK");
		std::string VTKext3(".vtp");
		std::string VTKext4(".VTP");
		std::string OBJext(".obj");
		std::string OBJext2(".OBJ");
		std::string PLYext(".ply");
		std::string PLYext2(".PLY");

		int type = 0; //0 = stl, 1 = vtk, 2 = obj, 3 = ply
		std::size_t found = fileName.toStdString().find(STLext);
		std::size_t found2 = fileName.toStdString().find(STLext2);
		if (found != std::string::npos || found2 != std::string::npos)
		{
			type = 0;
			//STL
		}

		//std::cout << "0Type= " <<type<< std::endl;
		found = fileName.toStdString().find(VTKext);
		found2 = fileName.toStdString().find(VTKext2);
		std::size_t found3 = fileName.toStdString().find(VTKext3);
		std::size_t found4 = fileName.toStdString().find(VTKext4);
		if (found != std::string::npos || found2 != std::string::npos || found3 != std::string::npos || found4 != std::string::npos)
		{
			type = 1; //VTK
		}

		//std::cout << "2Type= " <<type<< std::endl;
		found = fileName.toStdString().find(PLYext);
		found2 = fileName.toStdString().find(PLYext2);
		if (found != std::string::npos || found2 != std::string::npos)
		{
			type = 2; //PLY
		}

		// Read and display for verification

		vtkSmartPointer<vtkPolyData> MyPolyData = vtkSmartPointer<vtkPolyData>::New();

		if (type == 0)
		{

			vtkSmartPointer<vtkSTLReader> reader =
				vtkSmartPointer<vtkSTLReader>::New();

			reader->SetFileName(fileName.toStdString().c_str());
			reader->Update();
			MyPolyData = reader->GetOutput();
		}

		else if (type == 1)
		{

			vtkSmartPointer<vtkPolyDataReader> reader =
				vtkSmartPointer<vtkPolyDataReader>::New();
			reader->SetFileName(fileName.toStdString().c_str());
			reader->Update();
			MyPolyData = reader->GetOutput();
		}
		else
		{

			vtkSmartPointer<vtkPLYReader> reader =
				vtkSmartPointer<vtkPLYReader>::New();
			reader->SetFileName(fileName.toStdString().c_str());
			reader->Update();
			MyPolyData = reader->GetOutput();
		}
		//std::cout << "\nNumber of points 1:" << MyPolyData->GetNumberOfPoints() << std::endl;
		//std::cout << "\nNumber of cells 1:" << MyPolyData->GetNumberOfCells() << std::endl;


		vtkSmartPointer<vtkPolyDataNormals> ObjNormals = vtkSmartPointer<vtkPolyDataNormals>::New();
		ObjNormals->SetInputData(MyPolyData);
		ObjNormals->ComputePointNormalsOn();
		ObjNormals->ComputeCellNormalsOn();
		//ObjNormals->AutoOrientNormalsOff();
		ObjNormals->ConsistencyOff();

		ObjNormals->Update();

		vtkSmartPointer<vtkCleanPolyData> cleanPolyDataFilter = vtkSmartPointer<vtkCleanPolyData>::New();
		cleanPolyDataFilter->SetInputData(ObjNormals->GetOutput());
		cleanPolyDataFilter->PieceInvariantOff();
		cleanPolyDataFilter->ConvertLinesToPointsOff();
		cleanPolyDataFilter->ConvertPolysToLinesOff();
		cleanPolyDataFilter->ConvertStripsToPolysOff();
		cleanPolyDataFilter->PointMergingOn();
		cleanPolyDataFilter->Update();

		MyPolyData = cleanPolyDataFilter->GetOutput();

		cout << "\nNumber of points:" << MyPolyData->GetNumberOfPoints() << std::endl;
		cout << "\nNumber of cells:" << MyPolyData->GetNumberOfCells() << std::endl;

		MyPolyData->GetCellData();

		vtkFloatArray* norms = vtkFloatArray::SafeDownCast(MyPolyData->GetCellData()->GetNormals());
		//	cout << "Safe cell downcast done ! " << endl;
		if (norms)
		{

			cout << "There are here " << norms->GetNumberOfTuples()
				<< " Float Cell normals in norms" << endl;
		}
		else
		{
			cout << "FloatNorms CELL is null " << endl;
		}

		norms = vtkFloatArray::SafeDownCast
		(MyPolyData->GetPointData()->GetNormals());
		//cout << "Safe point downcast done ! " << endl;
		if (norms)
		{

			cout << "There are  " << norms->GetNumberOfTuples()
				<< " Float POINT normals in norms" << endl;
		}
		else
		{
			cout << "FloatNorms POINTS is null " << endl;
		}

		if (MyPolyData->GetNumberOfPoints() > 10)
		{
			cout << "More than 10 points!" << endl;
			VTK_CREATE(vtkMDActor, actor);
			if (this->mui_BackfaceCulling == 0)
			{
				actor->GetProperty()->BackfaceCullingOff();
			}
			else
			{
				actor->GetProperty()->BackfaceCullingOn();
			}
			

			QFileInfo fileInfo(fileName);
			QString onlyfilename(fileInfo.fileName());
			std::string only_filename = onlyfilename.toStdString();
			std::string newname = only_filename.c_str();
			size_t nPos = newname.find_last_of(".");
			if (nPos > 0)
			{

				newname = newname.substr(0, nPos);
			}

			//@@TODO! 
			newname = this->CheckingName(newname);
			cout << "Object Name= " << newname << endl;
			
			/*if ((vtkUnsignedCharArray*)MyPolyData->GetPointData()->GetScalars("RGB") != NULL)
			{
				QString RGB = QString("RGB");
				this->Addmui_ExistingScalars(RGB);
				//	MyPolyData->GetPointData()->SetScalars(NULL);
					cout << "found RGB colours! " << endl;
			}*/
			cout << "Current active scalar=" << this->mui_ActiveScalars->Name.toStdString().c_str() << endl;

			if (MyPolyData->GetPointData()->GetScalars(this->mui_ActiveScalars->Name.toStdString().c_str()) != NULL)
			{

				

				MyPolyData->GetPointData()->SetActiveScalars(this->mui_ActiveScalars->Name.toStdString().c_str());
			}
			//MyPolyData->GetPointData()->SetActiveScalars(NULL);
			/*
			vtkSmartPointer<vtkUnsignedCharArray> newcolors =
			vtkSmartPointer<vtkUnsignedCharArray>::New();
			newcolors->SetNumberOfComponents(4);
			newcolors->SetNumberOfTuples(numpoints);
			//ici init_RGB ou RGB_i
			if ((vtkUnsignedCharArray*)MyObj->GetPointData()->GetScalars("RGB") != NULL) {
			newcolors->DeepCopy((vtkUnsignedCharArray*)MyObj->GetPointData()->GetScalars("RGB"));

			for (int i = 0; i < numpoints; i++)
			{
			if (i < 100)
			{
			cout << newcolors->GetComponent(i, 0) << "," << newcolors->GetComponent(i, 1)
			<< "," << newcolors->GetComponent(i, 2) << std::endl;
			}
			//newcolors->SetComponent(i, 3, 255.);

			}

			cout << "found RGB colours: ";
			newcolors->SetName("Init_RGB");
			My_Obj->GetPointData()->AddArray(newcolors);
			}

			*/

			// THIS PIECE OF CODE WORKS!
			//auto mapper2 = vtkSmartPointer<vtkPolyDataMapper>::New();
			/*mapper2->SetInputData(MyPolyData);
			mapper2->Update();
			//mapper->SetInputConnection(clean1->GetOutputPort());


			auto actor2 = vtkSmartPointer<vtkActor>::New();
			actor2->SetMapper(mapper2);
			this->Renderer->AddActor(actor2);*/
			//END THIS PIECE OF CODE WORKS

			// Mapper
			VTK_CREATE(vtkPolyDataMapper, mapper);
			
			//mapper->ImmediateModeRenderingOn();
			//mapper->SetColorModeToDirectScalars();
			mapper->SetColorModeToDefault();

			
			
			// Decide which lut should be set!
			if (
				(this->mui_ActiveScalars->DataType == VTK_INT || this->mui_ActiveScalars->DataType == VTK_UNSIGNED_INT)
				&& this->mui_ActiveScalars->NumComp == 1
				)
			{
				mapper->SetScalarRange(0, this->TagTableSize - 1);
				mapper->SetLookupTable(this->GetTagLut());
			}
			else
			{

				//mapper->SetScalarRange(0,200);								
				
				mapper->SetLookupTable(this->Getmui_ActiveColorMap()->ColorMap);
				//mapper->UseLookupTableScalarRangeOff();
				//cout << "Set color map" << this->Getmui_ActiveColorMap()->Name.toStdString() << endl;
			}

			
			
			//mapper->ScalarVisibilityOn();
			//mapper->ScalarVisibilityOff();
			mapper->SetInputData(MyPolyData);
			//VTK_CREATE(vtkActor, actor);

			int num = 2;

			actor->SetmColor(this->Getmui_MeshColor());
			
			actor->SetMapper(mapper);
			actor->SetSelected(1);
			actor->SetName(newname);
			this->getActorCollection()->AddItem(actor);
			this->Initmui_ExistingScalars();
			std::string action = "Load mesh file";
			int mCount = BEGIN_UNDO_SET(action);
			this->getActorCollection()->CreateLoadUndoSet(mCount, 1);
			END_UNDO_SET();



			this->getActorCollection()->SetChanged(1);

			//double BoundingBoxLength = MyPolyData->GetLength();
			this->AdjustCameraAndGrid();
			//cout << "camera and grid adjusted" << endl;

			if (this->Getmui_AdjustLandmarkRenderingSize() == 1)
			{
				this->UpdateLandmarkSettings();
			}
			/*
			double bounds[6];
			MyPolyData->GetBounds(bounds);
			vtkSmartPointer<vtkElevationFilter> elevation =
			vtkSmartPointer<vtkElevationFilter>::New();
			elevation->SetInputData(MyPolyData);
			elevation->SetLowPoint(0, bounds[2], 0);
			elevation->SetHighPoint(0, bounds[3], 0);
			elevation->Update();
			vtkSmartPointer<vtkBandedPolyDataContourFilter> bcf =
			vtkSmartPointer<vtkBandedPolyDataContourFilter>::New();
			bcf->SetInputConnection(elevation->GetOutputPort());
			bcf->SetScalarModeToValue();
			bcf->GenerateContourEdgesOn();

			bcf->GenerateValues(10, elevation->GetScalarRange());

			bcf->Update();
			//bcf->GetNumberOfContours();
			vtkSmartPointer<vtkPolyDataMapper> contourLineMapper =
			vtkSmartPointer<vtkPolyDataMapper>::New();
			contourLineMapper->SetInputData(bcf->GetContourEdgesOutput());

			cout<<"Number of contours:"<< bcf->GetNumberOfContours();

			contourLineMapper->SetScalarRange(elevation->GetScalarRange());
			contourLineMapper->SetResolveCoincidentTopologyToPolygonOffset();
			contourLineMapper->Update();

			vtkSmartPointer<vtkMDActor> contourLineActor =
			vtkSmartPointer<vtkMDActor>::New();
			contourLineActor->SetMapper(contourLineMapper);
			contourLineActor->GetProperty()->SetColor(0.5, 0.5, 1.0);
			this->getRenderer()->AddActor(contourLineActor);

			vtkSmartPointer<vtkPolyDataMapper> mapper2 =
			vtkSmartPointer<vtkPolyDataMapper>::New();
			mapper2->SetInputConnection(bcf->GetOutputPort());
			mapper2->SetScalarModeToUseCellData();
			//mapper2->SetScalarRange(0, 1);
			vtkSmartPointer<vtkMDActor> actor2=
			vtkSmartPointer<vtkMDActor>::New();
			actor2->SetMapper(mapper2);
			this->getRenderer()->AddActor(actor2);
			*/
			/*if (this->Getmui_CameraCentreOfMassAtOrigin() == 0)
			{
			double globalcenterofmass[3];
			this->GetGlobalCenterOfMass(globalcenterofmass);
			cout << "Center of mass of all opened mesh is " << globalcenterofmass[0] << " " << globalcenterofmass[1] << " " << globalcenterofmass[2] << endl;

			double GlobalBoundingBoxLength = this->GetGlobalBoundingBoxLength();
			cout << "Global Bounding Box length is " << GlobalBoundingBoxLength << " mm" << endl;

			double campos[3];
			this->getCamera()->GetPosition(campos);
			double camfocalpoint[3];
			this->getCamera()->GetFocalPoint(camfocalpoint);
			double camscale = this->getCamera()->GetParallelScale();

			double movex, movey, movez;
			movex = (campos[0] - camfocalpoint[0])*GlobalBoundingBoxLength / camscale;
			movey = (campos[1] - camfocalpoint[1])*GlobalBoundingBoxLength / camscale;
			movez = (campos[2] - camfocalpoint[2])*GlobalBoundingBoxLength / camscale;
			this->getCamera()->SetPosition
			(globalcenterofmass[0] + movex,
			globalcenterofmass[1] + movey,
			globalcenterofmass[2] + movez);
			//this->getCamera()->SetPosition(center[0] + GlobalBoundingBoxLength, center[1], center[2]);
			this->getCamera()->SetFocalPoint(globalcenterofmass[0], globalcenterofmass[1], globalcenterofmass[2]);
			this->getCamera()->SetParallelScale(GlobalBoundingBoxLength);
			}*/
			//this->getCamera()->ParallelProjectionOn();


			//this->UpdateRenderer();

			//My_Obj = Cont_Mesh.Mesh_PDcontainerload(MyObj, (char*)newname.c_str());

			/*My_Obj->Set_Active_Scalar();
			int numpoints = My_Obj->GetNumberOfPoints();
			int numtriangles = My_Obj->GetNumberOfCells();
			std::cout << "Number of points:" << numpoints << std::endl;
			std::cout << "Number of cells:" << numtriangles << std::endl;

			//std::cout << "2 Mean x:"<<My_Obj->mean[0]<< "Mean y:"<<My_Obj->mean[1]<< "Mean z:"<<My_Obj->mean[2]<< std::endl;

			//std::cout << "3 Mean x:"<<My_Obj->mean[0]<< "Mean y:"<<My_Obj->mean[1]<< "Mean z:"<<My_Obj->mean[2]<< std::endl;

			My_Obj->selected = 1;


			cout << "color init: ";
			vtkSmartPointer<vtkUnsignedCharArray> newcolors =
			vtkSmartPointer<vtkUnsignedCharArray>::New();
			newcolors->SetNumberOfComponents(4);
			newcolors->SetNumberOfTuples(numpoints);
			//ici init_RGB ou RGB_i
			if ((vtkUnsignedCharArray*)MyObj->GetPointData()->GetScalars("RGB") != NULL) {
			newcolors->DeepCopy((vtkUnsignedCharArray*)MyObj->GetPointData()->GetScalars("RGB"));

			for (int i = 0; i < numpoints; i++)
			{
			if (i < 100)
			{
			cout << newcolors->GetComponent(i, 0) << "," << newcolors->GetComponent(i, 1)
			<< "," << newcolors->GetComponent(i, 2) << std::endl;
			}
			//newcolors->SetComponent(i, 3, 255.);

			}

			cout << "found RGB colours: ";
			newcolors->SetName("Init_RGB");
			My_Obj->GetPointData()->AddArray(newcolors);
			}
			cout << "ok." << endl;

			My_Obj->color[0] = color_obj[0];
			My_Obj->color[1] = color_obj[1];
			My_Obj->color[2] = color_obj[2];
			My_Obj->color[3] = 1;

			My_Obj->bool_init_buf = 0;
			// Only update RGB if not exists!

			vtkUnsignedCharArray* test = (vtkUnsignedCharArray*)My_Obj->GetPointData()->GetScalars("RGB");
			if (test == NULL)
			{
			My_Obj->Update_RGB();
			}


			//std::cout << "4 Mean x:"<<My_Obj->mean[0]<< "Mean y:"<<My_Obj->mean[1]<< "Mean z:"<<My_Obj->mean[2]<< std::endl;


			//Move object at center of mass only in some cases
			if (g_move_cm == 1)
			{
			My_Obj->Mat2[3][0] = -My_Obj->mean[0];
			My_Obj->Mat2[3][1] = -My_Obj->mean[1];
			My_Obj->Mat2[3][2] = -My_Obj->mean[2];
			}

			this->Compute_Global_Mean(0);
			if (g_landmark_auto_rendering_size)
			{
			this->Adjust_landmark_rendering_size();
			}
			this->Compute_Global_Scalar_List();

			}
			cout << "Reinitialize camera" << endl;
			rollinit_camera();
			cout << "G_zoom after initialization:" << g_zoom << endl;
			this->redraw();
			cout << "G_zoom after redraw:" << g_zoom << endl;*/

		}

	}


	/*	if (fileName.isEmpty()) return;

	//if (img.loadImage(fileName.toStdString().c_str()))

	fileName = QFileDialog::getOpenFileName(this,
	tr("Open File"), "/home/jana", tr("Surface Files (*.vtk *.stl *.ply)"));
	VTK_CREATE(vtkActor, actor);
	actor->GetProperty()->SetColor(0.5, 1, 0.5);
	actor->GetProperty()->SetOpacity(0.5);*/
	//this->MainWindow->vtkWidgetUpdate();
	this->Render();


}
void mqMorphoDigCore::OpenNTW(QString fileName)
{


	int file_exists = 1;
	int i = 0;
	QFile file(fileName);
	QFileInfo fileInfo(fileName);
	QString onlyfilename(fileInfo.fileName());
	if (file.exists()) {
		// Message
		file.close();
	}
	else
	{
		file_exists = 0;

	}


	if (file_exists == 1)
	{
		cout << "found file!!!!" << endl;


		std::string only_filename = onlyfilename.toStdString();;
		std::string path = fileName.toStdString().substr(0, (fileName.toStdString().length() - only_filename.length()));
		cout << "only_filename" << only_filename << endl;
		cout << "path" << path << endl;
		this->UnselectAll(-1);





		QFile inputFile(fileName);
		int ok = 0;

		if (inputFile.open(QIODevice::ReadOnly))
		{
			QTextStream in(&inputFile);
			while (!in.atEnd())
			{
				QString line = in.readLine();
				cout << "Line:" << line.toStdString() << endl;
				//sscanf(line.toStdString().c_str(), "%[^\n]s", param1);
				//cout << "param1" << param1 << endl;
				//std::string myline = param1;
				std::string myline = line.toStdString();
				cout << "My line:" << myline << endl;
				std::string FLGext(".flg");
				std::string FLGext2(".FLG");
				std::string VERext(".ver");
				std::string VERext2(".VER");
				std::string CURext(".cur");
				std::string CURext2(".CUR");
				std::string STVext(".stv");
				std::string STVext2(".STV");
				std::string TAGext(".tag");
				std::string TAGext2(".TAG");
				std::string ORIext(".ori");
				std::string ORIext2(".ORI");
				int lmk_file = 0;

				std::size_t found = myline.find(FLGext);
				std::size_t found2 = myline.find(FLGext2);
				if (found != std::string::npos || found2 != std::string::npos)
				{
					lmk_file = 1;
					// Now open flag file!
					QFileInfo flgfileInfo(line);
					QString onlyflgfilename(flgfileInfo.fileName());
					std::string flgfilename = onlyflgfilename.toStdString();
					if (myline.length() == flgfilename.length())
					{
						myline = path.c_str();
						myline.append(flgfilename.c_str());
					}
					std::cout << "Try to load flag file :<<" << myline.c_str() << std::endl;

					QString flgfile(myline.c_str());
					this->OpenFLG(flgfile);

				}

				found = myline.find(VERext);
				found2 = myline.find(VERext2);
				if (found != std::string::npos || found2 != std::string::npos)
				{
					lmk_file = 1;
					int landmark_mode = 0;
					// Now open ver file!
					QFileInfo verfileInfo(line);
					QString onlyverfilename(verfileInfo.fileName());
					std::string verfilename = onlyverfilename.toStdString();
					if (myline.length() == verfilename.length())
					{
						myline = path.c_str();
						myline.append(verfilename.c_str());
					}
					std::cout << "Try to load landmark file :<<" << myline.c_str() << std::endl;

					QString verfile(myline.c_str());
					this->OpenVER(verfile, landmark_mode);


				}

				found = myline.find(CURext);
				found2 = myline.find(CURext2);
				if (found != std::string::npos || found2 != std::string::npos)
				{
					lmk_file = 1;
					// Now open cur file!
					QFileInfo curfileInfo(line);
					QString onlycurfilename(curfileInfo.fileName());
					std::string curfilename = onlycurfilename.toStdString();
					if (myline.length() == curfilename.length())
					{
						myline = path.c_str();
						myline.append(curfilename.c_str());
					}
					std::cout << "Try to load CUR curve file :<<" << myline.c_str() << std::endl;
					QString curfile(myline.c_str());
					this->OpenCUR(curfile);

				}

				found = myline.find(STVext);
				found2 = myline.find(STVext2);
				if (found != std::string::npos || found2 != std::string::npos)
				{
					lmk_file = 1;
					// Now open STV file!
					QFileInfo stvfileInfo(line);
					QString onlystvfilename(stvfileInfo.fileName());
					std::string stvfilename = onlystvfilename.toStdString();
					if (myline.length() == stvfilename.length())
					{
						myline = path.c_str();
						myline.append(stvfilename.c_str());
					}
					std::cout << "Try to load STV curve file :<<" << myline.c_str() << std::endl;
					QString stvfile(myline.c_str());
					this->OpenSTV(stvfile);


				}
				found = myline.find(ORIext);
				found2 = myline.find(ORIext2);
				if (found != std::string::npos || found2 != std::string::npos)
				{
					lmk_file = 1;
					// Now open ORI file!
					QFileInfo orifileInfo(line);
					QString onlyorifilename(orifileInfo.fileName());
					std::string orifilename = onlyorifilename.toStdString();
					if (myline.length() == orifilename.length())
					{
						myline = path.c_str();
						myline.append(orifilename.c_str());
					}
					std::cout << "Try to load orientaiton file :<<" << myline.c_str() << std::endl;
					QString orifile(myline.c_str());
					this->OpenORI(orifile);

				}

				found = myline.find(TAGext);
				found2 = myline.find(TAGext2);
				if (found != std::string::npos || found2 != std::string::npos)
				{
					lmk_file = 1;
					// Now open TAG file!
					QFileInfo tagfileInfo(line);
					QString onlytagfilename(tagfileInfo.fileName());
					std::string tagfilename = onlytagfilename.toStdString();
					if (myline.length() == tagfilename.length())
					{
						myline = path.c_str();
						myline.append(tagfilename.c_str());
					}
					std::cout << "Try to load tag file :<<" << myline.c_str() << std::endl;
					QString tagfile(myline.c_str());
					this->OpenTAG(tagfile);


				}

				//NOW THE SURFACES!!!

				if (lmk_file == 0)
				{
					if (i == 0)
					{

						//length=(int)strlen(oneline);						
						//strncpy(param1, oneline, length-1);
						std::string meshname = line.toStdString();
						QFileInfo meshfileInfo(line);
						QString onlymeshfilename(meshfileInfo.fileName());
						std::string meshfilename = onlymeshfilename.toStdString();

						if (meshname.length() == meshfilename.length())
						{
							meshname = path.c_str();
							meshname.append(meshfilename.c_str());
						}
						QString meshfile(meshname.c_str());



						this->OpenMesh(meshfile);
						vtkMDActor* actor = this->GetLastActor();

						if (actor != NULL && actor->GetNumberOfPoints() > 10)
						{

							ok = 1;
							cout << "Object has more than 10 points <<" << endl;
						}
						else
						{
							ok = 0;
						}


					}
					if (i == 1)
					{
						if (ok)
						{

							//length= (int)strlen(oneline);						
							//strncpy(param1, oneline, length-1);
							std::string posfile = line.toStdString();
							// Now open TAG file!
							QFileInfo posfileInfo(line);
							QString onlyposfilename(posfileInfo.fileName());
							std::string posfilename = onlyposfilename.toStdString();

							if (posfile.length() == posfilename.length())
							{
								posfile = path.c_str();
								posfile.append(posfilename.c_str());
							}
							std::cout << "Try to load position :<<" << posfile.c_str() << std::endl;
							QString qposfile(posfile.c_str());
							this->OpenPOS(qposfile, 0);
							//@@TODO!
							//this->Open_POS_File(posfile, My_Obj);
							//std::cout <<"Object <<"<<My_Obj->name.c_str()<<">> position loaded"<< std::endl;
							//My_Obj->selected = 0;
						}
					}
					if (i == 2)
					{
						if (ok)
						{
							vtkMDActor *actor = this->GetLastActor();
							double r, g, b, a;
							QTextStream myteststream(&line);
							myteststream >> r >> g >> b >> a;
							if (r > 1 || g > 1 || b > 1) {
								r /= 255; g /= 255;
								b /= 255;
							}


							actor->SetmColor(r, g, b, a);
							actor->SetSelected(0);
							actor->SetChanged(1);

							this->getActorCollection()->SetChanged(1);
							/*sscanf(oneline, "%f %f %f %f\n", &color1, &color2, &color3, &color4);
							//std::cout <<"color 1"<<color1<<",color 2"<<color3<<",color 3"<<color3<<",color 4"<<color4<< std::endl;
							My_Obj->color[0] = color1; My_Obj->color[1] = color2; My_Obj->color[2] = color3; My_Obj->color[3] = color4;
							My_Obj->blend = color4;
							My_Obj->Update_RGB();*/
						}
					}
					i++;
					if (i > 2)
					{
						i = 0;
					}
				}


			}
			inputFile.close();
		}
	}
}

void mqMorphoDigCore::OpenTAG(QString fileName) {}
void mqMorphoDigCore::OpenORI(QString fileName)
{

	QString X1, X2, Y1, Y2, Z1, Z2;





	size_t  length;


	length = fileName.toStdString().length();

	int done = 0;
	if (length>0)
	{
		int file_exists = 1;
		ifstream file(fileName.toStdString().c_str());
		if (file)
		{
			//std::cout<<"file:"<<filename.c_str()<<" exists."<<std::endl;
			file.close();
		}
		else
		{

			std::cout << "file:" << fileName.toStdString().c_str() << " does not exists." << std::endl;
			file_exists = 0;
		}

		if (file_exists == 1)
		{

			std::string ORIext(".ori");
			std::string ORIext2(".ORI");


			int type = 0; // 0 = .POS Ascii File //1 = .MAT binary File or simple .MAT file

			std::size_t found = fileName.toStdString().find(ORIext);
			std::size_t found2 = fileName.toStdString().find(ORIext2);
			if (found != std::string::npos || found2 != std::string::npos)
			{
				type = 1;
				//ORI
			}

			int cpt = 0;


			if (type == 1)
			{

				//filein = fopen(fileName.toStdString().c_str(), "rt");
				QFile inputFile(fileName);
				int ok = 0;

				if (inputFile.open(QIODevice::ReadOnly))
				{
					QTextStream in(&inputFile);

					while (!in.atEnd())
					{

						QString line = in.readLine();
						if (cpt == 0) { Z1 = line; }
						if (cpt == 1) { Z2 = line; }
						if (cpt == 2) { Y1 = line; }
						if (cpt == 3) { Y2 = line; }
						if (cpt == 4) { X1 = line; }
						if (cpt == 5) { X2 = line; }
						cpt++;

					}


					inputFile.close();
					this->Setmui_X1Label(X1);
					this->Setmui_X2Label(X2);
					this->Setmui_Y1Label(Y1);
					this->Setmui_Y2Label(Y2);
					this->Setmui_Z1Label(Z1);
					this->Setmui_Z2Label(Z2);
					this->ResetOrientationHelperLabels();


				}
			}//fin if																		

		}//file exists...
	}	//length*/
}

void mqMorphoDigCore::OpenCUR(QString fileName)

{
	double  xn, yn, zn, xh, yh, zh;// coordinates of curve nodes and curve handles
	int node_type;
	QString LMKName;
	//Open a landmark file!


	size_t  length;


	length = fileName.toStdString().length();

	int done = 0;
	if (length>0)
	{
		int file_exists = 1;
		ifstream file(fileName.toStdString().c_str());
		if (file)
		{
			//std::cout<<"file:"<<filename.c_str()<<" exists."<<std::endl;
			file.close();
		}
		else
		{

			std::cout << "file:" << fileName.toStdString().c_str() << " does not exists." << std::endl;
			file_exists = 0;
		}

		if (file_exists == 1)
		{

			std::string CURext(".cur");
			std::string CURext2(".CUR");


			int type = 0;

			std::size_t found = fileName.toStdString().find(CURext);
			std::size_t found2 = fileName.toStdString().find(CURext2);
			if (found != std::string::npos || found2 != std::string::npos)
			{
				type = 1;
				//CUR
			}



			if (type == 1)
			{

				//filein = fopen(fileName.toStdString().c_str(), "rt");
				QFile inputFile(fileName);
				int ok = 0;

				if (inputFile.open(QIODevice::ReadOnly))
				{
					QTextStream in(&inputFile);

					while (!in.atEnd())
					{

						QString line = in.readLine();
						QTextStream myteststream(&line);
						myteststream >> LMKName >> xn >> yn >> zn >> xh >> yh >> zh >> node_type;
						double coordn[3] = { xn,yn,zn };
						double coordh[3] = { xh,yh,zh };
						double ori[3];


						ori[0] = 0;
						ori[1] = 0;
						ori[2] = 1;

						this->CreateLandmark(coordn, ori, 2, node_type);
						this->CreateLandmark(coordh, ori, 3);

					}
					/**/

					inputFile.close();


				}
			}//fin if																		

		}//file exists...
	}	//length

		
}
void mqMorphoDigCore::OpenSTV(QString fileName)
{
	double  x, y, z, nx, ny, nz;
	QString LMKName;
	//Open a STV file!


	size_t  length;

	int type = 1;
	length = fileName.toStdString().length();

	int done = 0;
	if (length>0)
	{
		int file_exists = 1;
		ifstream file(fileName.toStdString().c_str());
		if (file)
		{
			//std::cout<<"file:"<<filename.c_str()<<" exists."<<std::endl;
			file.close();
		}
		else
		{

			std::cout << "file:" << fileName.toStdString().c_str() << " does not exists." << std::endl;
			file_exists = 0;
		}

		if (file_exists == 1)
		{


			std::string STVext(".stv");
			std::string STVext2(".STV");


			std::size_t found = fileName.toStdString().find(STVext);
			std::size_t found2 = fileName.toStdString().find(STVext2);
			if (found != std::string::npos || found2 != std::string::npos)
			{
				type = 1;
				//STV
			}

			if (type == 1)
			{
				//filein = fopen(fileName.toStdString().c_str(), "rt");
				QFile inputFile(fileName);
				int ok = 0;
				if (inputFile.open(QIODevice::ReadOnly))
				{
					QTextStream in(&inputFile);
					int landmark_mode = 0;
					int number = 0;
					int cpt_line = 0;
					while (!in.atEnd())
					{

						QString line = in.readLine();
						QTextStream myteststream(&line);
						if (cpt_line == 0)
						{
							myteststream >> landmark_mode >> number;
						}
						else
						{
							// To do : type = 2 => information 
							int lmtype = -1;
							if (landmark_mode == 2)// curve node!
							{
								myteststream >> LMKName >> x >> y >> z >> nx >> ny >> nz >> lmtype;
								//cout << "lmtype!" << lmtype << endl;
								//lmtype: 1 curve starting point
								//lmtype: 0 normal node
								//lmtype: 2 curve milestone
								//lmtype: 3 connect to preceding starting point
							}
							else
							{
								myteststream >> LMKName >> x >> y >> z >> nx >> ny >> nz;
							}
							double coord[3] = { x,y,z };
							double ncoord[3] = { nx,ny,nz };
							double ori[3];

							double length = nx*nx + ny*ny + nz*nz;
							if (length == 1)
							{
								ori[0] = ncoord[0];
								ori[1] = ncoord[1];
								ori[2] = ncoord[2];
							}
							else
							{
								vtkMath::Subtract(ncoord, coord, ori);
								vtkMath::Normalize(ori);
							}
							/*if (lmtype != 0)
							{
							cout << "landmark_mode: " << landmark_mode << " lmtype: " << lmtype << endl;
							}*/
							this->CreateLandmark(coord, ori, landmark_mode, lmtype);
						}
						cpt_line++;
						if (cpt_line == number + 1) {
							cpt_line = 0;

						}
					}
					/**/

					inputFile.close();


				}
			}//fin if																		

		}//file exists...
	}	//length



		/*float  param2, param3, param4, param5, param6, param7;
		float m_ve[3], m_ven[3], leng;
		char param1[50];
		FILE	*filein;// Filename To Open
		char	oneline[255];
		int landmark_mode;


		int file_exists = 1;
		ifstream file(filename.c_str());

		if (file)
		{
		file.close();
		}
		else
		{
		cout << "file:" << filename.c_str() << " does not exists." << std::endl;
		file_exists = 0;
		}

		if (file_exists == 1)
		{
		std::string STVext(".stv");
		std::string STVext2(".STV");

		int type = 1; //VER

		filein = fopen(filename.c_str(), "r");
		readstr(filein, oneline);
		feof(filein);
		std::cout << "Try open landmark file " << std::endl;
		std::cout << "feof(filein)" << feof(filein) << std::endl;
		int ind = 0;
		vtkSmartPointer<vtkFloatArray> param_list = vtkSmartPointer<vtkFloatArray>::New();
		param_list->SetNumberOfComponents(1);
		int number = 0;
		int cpt_line = 0;
		while (!feof(filein))
		{
		if (cpt_line == 0) {
		sscanf(oneline, "%d %d\n", &landmark_mode, &number);
		}
		else {
		sscanf(oneline, "%s %f %f %f %f %f %f %d\n", param1, &param2, &param3, &param4, &param5, &param6, &param7, &ind);
		param_list->InsertNextTuple1(param2);
		param_list->InsertNextTuple1(param3);
		param_list->InsertNextTuple1(param4);
		param_list->InsertNextTuple1(param5);
		param_list->InsertNextTuple1(param6);
		param_list->InsertNextTuple1(param7);

		create_landmarks(landmark_mode, param_list, type);

		param_list = vtkSmartPointer<vtkFloatArray>::New();
		param_list->SetNumberOfComponents(1);

		}
		readstr(filein, oneline); //read next line
		cpt_line++;

		if (cpt_line == number + 1 && landmark_mode == 0) {
		cpt_line = 0;
		landmark_mode++;
		}
		}//While scanff...
		fclose(filein);
		}
		*/

}

int mqMorphoDigCore::SaveNTWFile(QString fileName, int save_ori, int save_tag, int save_surfaces_as_ply, int apply_position_to_surfaces)
{
	cout << "apply_position_to_surfaces=" << apply_position_to_surfaces << endl;
	std::string NTWext = ".ntw";
	std::string NTWext2 = ".NTW";
	std::size_t found = fileName.toStdString().find(NTWext);
	std::size_t found2 = fileName.toStdString().find(NTWext2);
	if (found == std::string::npos && found2 == std::string::npos)
	{
		fileName.append(".ntw");
	}

	QFileInfo fileInfo(fileName);
	QString onlyfilename(fileInfo.fileName());



	QFile file(fileName);
	QTextStream stream(&file);
	if (file.open(QIODevice::WriteOnly | QIODevice::Text))
	{
		std::string only_filename = onlyfilename.toStdString();
		std::string project_name = only_filename.c_str();
		size_t nPos = project_name.find_last_of(".");
		if (nPos > 0 && nPos <= project_name.length())
		{
			project_name = project_name.substr(0, nPos);
		}
		std::string path = fileName.toStdString().substr(0, (fileName.toStdString().length() - only_filename.length()));
		std::string posExt = ".pos";
		//std::string vtpExt = ".vtp";
		std::string vtkExt = ".vtk";
		std::string plyExt = ".ply";
		std::string tagExt = ".tag";
		std::string oriExt = ".ori";
		std::string flgExt = ".flg";
		std::string verExt = ".ver";
		std::string stvExt = ".stv";
		std::string curExt = ".cur";
		int overwrite_pos = 1;
		int overwrite_mesh = 1;
		int overwrite_ori = 1;
		int overwrite_tag = 1;
		int overwrite_flg = 1;
		int overwrite_ver = 1;
		int overwrite_cur = 1;
		int overwrite_stv = 1;

		this->ComputeSelectedNamesLists();
		std::string postfix = "_";
		postfix.append(project_name.c_str());
		std::string no_postfix = "";
		int pos_exists = 0;
		int pos_special = 0;
		if (g_distinct_selected_names.size() == 1 && project_name.compare(g_distinct_selected_names.at(0)) == 0)
		{
			pos_special = 1;
			pos_exists = this->selected_file_exists(path, posExt, no_postfix);
		}
		else
		{
			pos_exists = this->selected_file_exists(path, posExt, postfix);
		}
			

		int mesh_exists = 0;
		if (save_surfaces_as_ply == 0)
		{
			mesh_exists = this->selected_file_exists(path, vtkExt, no_postfix);
		}
		else
		{
			mesh_exists = this->selected_file_exists(path, plyExt, no_postfix);
		}

		if (save_ori == 1)
		{
			std::string _ori_fullpath;
			std::string _ori_file;
			int ori_exists = this->context_file_exists(path, oriExt, project_name);
			if (ori_exists == 1)
			{
				QMessageBox msgBox;
				msgBox.setText("Ori file already exists: overwrite existing orientation file ?");
				msgBox.setStandardButtons(QMessageBox::Yes | QMessageBox::Cancel);
				msgBox.setDefaultButton(QMessageBox::Yes);
				int ret = msgBox.exec();
				if (ret == QMessageBox::Cancel) { overwrite_ori=0; }

			}
			

			_ori_file = project_name.c_str();
			_ori_file.append(".ori");
			_ori_fullpath = path.c_str();
			_ori_fullpath.append(_ori_file.c_str());
			int write = 1;
			QString qori(_ori_fullpath.c_str());
			
			if (overwrite_ori == 1)
			{
				//write or overwrite ORI file without further question (0)
				this->SaveORI(qori);
			}
			stream << _ori_file.c_str() << endl;

		}
		if (mesh_exists == 1)
		{
			QMessageBox msgBox;
			msgBox.setText("At least one mesh file already exists: update existing mesh files?");
			msgBox.setStandardButtons(QMessageBox::Yes | QMessageBox::Cancel);
			msgBox.setDefaultButton(QMessageBox::Yes);
			int ret = msgBox.exec();
			if (ret == QMessageBox::Cancel) { overwrite_mesh = 0; }

			
		}
		if (pos_exists == 1)
		{
			QMessageBox msgBox;
			msgBox.setText("At least one position file already exists: update existing position files?");
			msgBox.setStandardButtons(QMessageBox::Yes | QMessageBox::Cancel);
			msgBox.setDefaultButton(QMessageBox::Yes);
			int ret = msgBox.exec();
			if (ret == QMessageBox::Cancel) { overwrite_pos = 0; }

			
		}

		if (save_tag == 1)
		{
			std::string _tag_fullpath;
			std::string _tag_file;
			int tag_exists = this->context_file_exists(path, tagExt, project_name);
			if (tag_exists == 1)
			{
				QMessageBox msgBox;
				msgBox.setText("Tag file already exists: overwrite existing tag colours and labels file ?");
				msgBox.setStandardButtons(QMessageBox::Yes | QMessageBox::Cancel);
				msgBox.setDefaultButton(QMessageBox::Yes);
				int ret = msgBox.exec();
				if (ret == QMessageBox::Cancel) { overwrite_tag = 0; }
				
			}

			_tag_file = project_name.c_str();
			_tag_file.append(".tag");
			_tag_fullpath = path.c_str();
			_tag_fullpath.append(_tag_file.c_str());
			int write = 1;
			
			if (overwrite_tag == 1)
			{
				//write or overwrite TAG file without further question (0)
				//@@ TODO!
				//this->SaveTAG(_tag_fullpath);
			}
			stream << _tag_file.c_str() << endl;
			

		}
		
		//flags 
		vtkIdType selectedflags = this->getFlagLandmarkCollection()->GetNumberOfSelectedActors();
		if (selectedflags>0)
		{
			std::string _flg_fullpath;
			std::string _flg_file;
			int flg_exists = this->context_file_exists(path, flgExt, project_name);
			if (flg_exists == 1)
			{
				QMessageBox msgBox;
				msgBox.setText("Flag file already exists: overwrite existing flag file ?");
				msgBox.setStandardButtons(QMessageBox::Yes | QMessageBox::Cancel);
				msgBox.setDefaultButton(QMessageBox::Yes);
				int ret = msgBox.exec();
				if (ret == QMessageBox::Cancel) { overwrite_flg = 0; }
				
			}

			_flg_file = project_name.c_str();
			_flg_file.append(".flg");
			_flg_fullpath = path.c_str();
			_flg_fullpath.append(_flg_file.c_str());
			if (overwrite_flg == 1)			
			{
				//cout << "Try to save FLG file" << endl;
				//write or overwrite FLG file without further question (0)
				QString qflg(_flg_fullpath.c_str());
				this->SaveFlagFile(qflg, 1);
			}
			stream << _flg_file.c_str() << endl;

		}

		int nselnor = this->NormalLandmarkCollection->GetNumberOfSelectedActors();
		int nseltar = this->TargetLandmarkCollection->GetNumberOfSelectedActors();
		int nselnod = this->NodeLandmarkCollection->GetNumberOfSelectedActors();
		int nselhan = this->HandleLandmarkCollection->GetNumberOfSelectedActors();
		if (nselnor > 0 || nseltar > 0|| nselnod > 0|| nselhan > 0)
		{
			std::string _stv_fullpath;
			std::string _stv_file;
			int stv_exists = this->context_file_exists(path, stvExt, project_name);
			if (stv_exists == 1)
			{
				QMessageBox msgBox;
				msgBox.setText("Landmark/Curve file already exists: overwrite existing STV file ?");
				msgBox.setStandardButtons(QMessageBox::Yes | QMessageBox::Cancel);
				msgBox.setDefaultButton(QMessageBox::Yes);
				int ret = msgBox.exec();
				if (ret == QMessageBox::Cancel) { overwrite_stv = 0; }
				
			}

			_stv_file = project_name.c_str();
			_stv_file.append(".stv");
			_stv_fullpath = path.c_str();
			_stv_fullpath.append(_stv_file.c_str());
			int write = 1;
			if (overwrite_stv == 1)
			{
				//write or overwrite stv file without further question (0)
				QString qstv(_stv_fullpath.c_str());
				this->SaveSTVFile(qstv,1);
			}
			stream << _stv_file.c_str() << endl;
			
		}

		this->ActorCollection->InitTraversal();
		for (vtkIdType i = 0; i < this->ActorCollection->GetNumberOfItems(); i++)
		{
			vtkMDActor * myActor = vtkMDActor::SafeDownCast(this->ActorCollection->GetNextActor());
			if (myActor->GetSelected() == 1)
			{
				std::string _mesh_fullpath;
				std::string _pos_fullpath;
				std::string _mesh_file;
				std::string _pos_file;												
				_mesh_file = myActor->GetName();
				_pos_file = myActor->GetName();
				if (save_surfaces_as_ply == 0)
				{
					_mesh_file.append(".vtk");
				}
				else
				{
					_mesh_file.append(".ply");
				}
				if (pos_special==0)
				{
					_pos_file.append(postfix.c_str());
				}
				_pos_file.append(".pos");

				_mesh_fullpath = path.c_str();
					_mesh_fullpath.append(_mesh_file.c_str());

				_pos_fullpath = path.c_str();
				_pos_fullpath.append(_pos_file.c_str());

				int write = 1;
				if (overwrite_mesh == 0)
				{
					// in that case, check if file exists...								
					ifstream file2(_mesh_fullpath.c_str());
					if (file2)
					{
						write = 0;
						file2.close();
					}
				}
				if (write == 1)
				{
					//int write_type = 0 : binary or binary LE;
					int File_type = 1; //: vtk-vtp

					if (save_surfaces_as_ply == 1) { File_type = 2; }//2 = PLY
	
					this->SaveSurfaceFile(QString(_mesh_fullpath.c_str()),0 , apply_position_to_surfaces, File_type, 0, myActor);
				}

				

				write = 1;
				if (overwrite_pos == 0)
				{
					// in that case, check if file exists...								
					ifstream file2(_pos_fullpath.c_str());
					if (file2)
					{
						write = 0;
						file2.close();
					}
				}

				if (write == 1)
				{
					if (apply_position_to_surfaces == 0)
					{
						vtkSmartPointer<vtkMatrix4x4> Mat = myActor->GetMatrix();
						this->SavePOS(Mat, QString(_pos_fullpath.c_str()));
					}
					else
					{
						vtkSmartPointer<vtkMatrix4x4> Mat = vtkSmartPointer<vtkMatrix4x4>::New();
						Mat->Identity();
						this->SavePOS(Mat, QString(_pos_fullpath.c_str()));

					}
				}
				stream << _mesh_file.c_str() << endl;
				stream << _pos_file.c_str() << endl;
				
				double colors[4];
				myActor->GetmColor(colors);
				stream << colors[0]<<" "<< colors[1]<<" "<< colors[2]<<" "<< colors[3] << endl;

					
				}
				

			}
				
	}
	file.close();
	return 1;
}

void mqMorphoDigCore::SavePOS(vtkSmartPointer<vtkMatrix4x4> Mat, QString fileName)
{

	std::string POSext = ".pos";
	std::string POSext2 = ".POS";
	std::size_t found = fileName.toStdString().find(POSext);
	std::size_t found2 = fileName.toStdString().find(POSext2);
	if (found == std::string::npos && found2 == std::string::npos)
	{
		fileName.append(".pos");
	}

	QFile file(fileName);
	if (file.open(QIODevice::WriteOnly | QIODevice::Text))
	{
		QTextStream stream(&file);

		stream << "1 0 0 0" << endl;
		stream << "0 1 0 0" << endl;
		stream << "0 0 1 0" << endl;
		stream << "0 0 0 1" << endl;


		stream << Mat->GetElement(0, 0) << " " << Mat->GetElement(1, 0) << " " << Mat->GetElement(2, 0) << " " << Mat->GetElement(3, 0) << endl;
		stream << Mat->GetElement(0, 1) << " " << Mat->GetElement(1, 1) << " " << Mat->GetElement(2, 1) << " " << Mat->GetElement(3, 1) << endl;
		stream << Mat->GetElement(0, 2) << " " << Mat->GetElement(1, 2) << " " << Mat->GetElement(2, 2) << " " << Mat->GetElement(3, 2) << endl;
		stream << Mat->GetElement(0, 3) << " " << Mat->GetElement(1, 3) << " " << Mat->GetElement(2, 3) << " " << Mat->GetElement(3, 3) << endl;


	}
	file.close();



}
//should only be done after main window is initialized.
int mqMorphoDigCore::SaveFlagFile(QString fileName, int save_only_selected)
{

	
		std::string FLGext = ".flg";
		std::string FLGext2 = ".FLG";
		std::size_t found = fileName.toStdString().find(FLGext);
		std::size_t found2 = fileName.toStdString().find(FLGext2);
		if (found == std::string::npos && found2 == std::string::npos)
		{
			fileName.append(".flg");
		}
	

	

	QFile file(fileName);
	if (file.open(QIODevice::WriteOnly | QIODevice::Text))
	{
		QTextStream stream(&file);

		vtkSmartPointer<vtkLMActorCollection> myColl = vtkSmartPointer<vtkLMActorCollection>::New();
		 myColl = this->FlagLandmarkCollection; 
		

		myColl->InitTraversal();
		for (vtkIdType i = 0; i < myColl->GetNumberOfItems(); i++)
		{
			vtkLMActor *myActor = vtkLMActor::SafeDownCast(myColl->GetNextActor());
			if (myActor->GetSelected() == 1 || save_only_selected == 0)
			{

				double lmpos[3];
				myActor->GetLMOrigin(lmpos);
				double ori[3];
				myActor->GetLMOrientation(ori);
				double lmori[3] = { lmpos[0] + ori[0],lmpos[1] + ori[1] ,lmpos[2] + ori[2] };
				
				stream << myActor->GetLMLabelText()<< endl;
				stream << lmpos[0] << " " << lmpos[1] << " " << lmpos[2] << " " 
					<< lmori[0] << " " << lmori[1] << " " << lmori[2] << " " << 
					myActor->GetLMSize()<<" "<<
					myActor->GetmColor()[0] << " "<<
					myActor->GetmColor()[1] << " " <<
					myActor->GetmColor()[2] << " " <<
					endl;
				
					
				

			}

		}




	}
	file.close();
	return 1;

}

int mqMorphoDigCore::SaveSTVFile(QString fileName, int save_only_selected)
{


	std::string STVext = ".stv";
	std::string STVext2 = ".STV";
	std::size_t found = fileName.toStdString().find(STVext);
	std::size_t found2 = fileName.toStdString().find(STVext2);
	if (found == std::string::npos && found2 == std::string::npos)
	{
		fileName.append(".stv");
	}




	QFile file(fileName);
	if (file.open(QIODevice::WriteOnly | QIODevice::Text))
	{
		QTextStream stream(&file);

		vtkSmartPointer<vtkLMActorCollection> myColl = vtkSmartPointer<vtkLMActorCollection>::New();
		
		for (int j = 0; j < 4; j++)
		{
			if (j == 0)
			{
				myColl = this->NormalLandmarkCollection;
			}
			else if (j == 1)
			{
				myColl = this->TargetLandmarkCollection;
			}
			else if (j == 2)
			{
				myColl = this->NodeLandmarkCollection;
			}
			else if (j == 3)
			{
				myColl = this->HandleLandmarkCollection;
			}


			

			if (myColl->GetNumberOfItems() > 0)
			{
				if (save_only_selected == 0)
				{
					stream <<j<<" "<< myColl->GetNumberOfItems() << endl;
				}
				else
				{
					stream << j << " " << myColl->GetNumberOfSelectedActors() << endl;
				}
				myColl->InitTraversal();

				for (vtkIdType i = 0; i < myColl->GetNumberOfItems(); i++)
				{
					vtkLMActor *myActor = vtkLMActor::SafeDownCast(myColl->GetNextActor());
					
					

					if (myActor->GetSelected() == 1 || save_only_selected == 0) 
					{

						double lmpos[3];
						myActor->GetLMOrigin(lmpos);
						
						
						myActor->GetLMOrigin(lmpos);
						double ori[3];
						myActor->GetLMOrientation(ori);
						double lmori[3] = { lmpos[0] + ori[0],lmpos[1] + ori[1] ,lmpos[2] + ori[2] };
						
						if (j < 2)
						{
							stream << "Landmark" << i+1 << ": " << lmpos[0] << " " << lmpos[1] << " " << lmpos[2] << " " << lmori[0] << " " << lmori[1] << " " << lmori[2] << " " << endl;
						}
						else if (j==2)
						{
							int lmnodetype = myActor->GetLMNodeType();
							stream << "CurveNode" << i+1 << ": " << lmpos[0] << " " << lmpos[1] << " " << lmpos[2] << " " << lmori[0] << " " << lmori[1] << " " << lmori[2] << " " << lmnodetype <<endl;
						}
						else
						{
							stream << "CurveHandle" << i+1 << ": " << lmpos[0] << " " << lmpos[1] << " " << lmpos[2] << " " << lmori[0] << " " << lmori[1] << " " << lmori[2] << " " << endl;
						}

						
						

					}

				}
			}
		}




	}
	file.close();
	return 1;

}


//should only be done after main window is initialized.
int mqMorphoDigCore::SaveCURasVERFile(QString fileName, int decimation, int save_format, int save_other_lmks)
{
	std::string VERext = ".ver";
	std::string VERext2 = ".VER";
	std::string LMKext = ".lmk";
	std::string LMKext2 = ".LMK";
	if (save_format == 0)
	{
		std::size_t found = fileName.toStdString().find(VERext);
		std::size_t found2 = fileName.toStdString().find(VERext2);
		if (found == std::string::npos && found2 == std::string::npos)
		{
			fileName.append(".ver");
		}
	}
	else
	{
		std::size_t found = fileName.toStdString().find(LMKext);
		std::size_t found2 = fileName.toStdString().find(LMKext2);
		if (found == std::string::npos && found2 == std::string::npos)
		{
			fileName.append(".lmk");
		}
	}


	QFile file(fileName);
	if (file.open(QIODevice::WriteOnly | QIODevice::Text))
	{
		QTextStream stream(&file);

		vtkSmartPointer<vtkLMActorCollection> Nodes = vtkSmartPointer<vtkLMActorCollection>::New();
		vtkSmartPointer<vtkLMActorCollection> Handles = vtkSmartPointer<vtkLMActorCollection>::New();
		Nodes = this->NodeLandmarkCollection;
		Handles = this->HandleLandmarkCollection;

	//START
		double length = 0;


		//cout << "Export curves!" << endl;
		// First a rigid line between node landmarks.
		if (Nodes == NULL)
		{
			return 0;
		}



		vtkLMActor  * ob_H1, *ob_H2, *ob_N1, *ob_N2, *ob_Nstart, *ob_Hstart, *ob_HA, *ob_HB, *ob_NA, *ob_NB;
		ob_H1 = NULL;
		ob_N1 = NULL;
		ob_H2 = NULL;
		ob_N2 = NULL;
		ob_Hstart = NULL;
		ob_Nstart = NULL;
		ob_NA = NULL;
		ob_NB = NULL;
		ob_HA = NULL;
		ob_HB = NULL;


		int k, m, num_seg, ind, ind2, indh, indh2;
		int nint = 1002; // will iterate nint , to draw nint+1 lines, that is nint+2 points
		double t, t2;


		int num_landmark_H = Handles->GetNumberOfItems();
		int num_landmark_N = Nodes->GetNumberOfItems();
		int nbp = num_landmark_H;
		double  nn1[3], nn2[3], hh1[3], hh2[3], intvv[3], intvv2[3], slm[3];
		
		int new_segment = 0;
		t2 = 0;
		double preceding_length = 0;
		double current_length = 0;
		int decimation_index = 0;
		double length_to_reach = 0;
		double total_length = this->getBezierCurveSource()->GetCurveSegmentLength(1);


		int print = 0;
		num_seg = 0;
		//if (num_landmark_T == num_landmark_N && num_landmark_T >0)
		if (decimation > 1 && decimation < nint)
		{

			int cpt = 0;
			//cout << "here" << endl;
			ob_H1 = Handles->GetLandmarkAfter(0);
			ob_Hstart = ob_H1;
			ob_H2 = Handles->GetLandmarkAfter(ob_H1->GetLMNumber());
			ob_N1 = Nodes->GetLandmarkAfter(0);
			ob_Nstart = ob_N1;
			ob_N2 = Nodes->GetLandmarkAfter(ob_N1->GetLMNumber());
			k = 0;

			while (ob_N1 != NULL && ob_H1 != NULL)
			{

				//stop drawing if the second point is a "start" of a new curve
				if (ob_N1->GetLMNodeType() == STARTING_NODE) {
					num_seg++;
				//	cout << "num_seg:" << num_seg << endl;
					t2 = 0;
					length_to_reach = 0;
					current_length = 0;
					preceding_length = 0;
					decimation_index = 0;
					total_length = this->getBezierCurveSource()->GetCurveSegmentLength(num_seg);
				}
				if (ob_N1->GetLMNodeType() == MILESTONE_NODE) {
					num_seg++;
				//	cout << "num_seg:" << num_seg << endl;
					t2 = 0;
					length_to_reach = 0;
					current_length = 0;
					preceding_length = 0;
					decimation_index = 0;
					total_length = this->getBezierCurveSource()->GetCurveSegmentLength(num_seg);
				}


				if (ob_N1->GetLMNodeType() == CONNECT_NODE) // in that case we should connect that node to the preceding starting point
				{
					ob_NA = ob_N1;
					ob_NB = ob_Nstart;
					ob_HA = ob_H1;
					ob_HB = ob_Hstart;

				}
				else
				{
					ob_NA = ob_N1;
					ob_HA = ob_H1;
					ob_NB = ob_N2;
					ob_HB = ob_H2;
				}

				if (ob_NB != NULL && ob_HB != NULL)
				{
					vtkMatrix4x4 *MatNA = ob_NA->GetMatrix();
					nn1[0] = MatNA->GetElement(0, 3);
					nn1[1] = MatNA->GetElement(1, 3);
					nn1[2] = MatNA->GetElement(2, 3);

					vtkMatrix4x4 *MatNB = ob_NB->GetMatrix();
					nn2[0] = MatNB->GetElement(0, 3);
					nn2[1] = MatNB->GetElement(1, 3);
					nn2[2] = MatNB->GetElement(2, 3);

					vtkMatrix4x4 *MatHA = ob_HA->GetMatrix();
					hh1[0] = MatHA->GetElement(0, 3);
					hh1[1] = MatHA->GetElement(1, 3);
					hh1[2] = MatHA->GetElement(2, 3);

					vtkMatrix4x4 *MatHB = ob_HB->GetMatrix();
					hh2[0] = MatHB->GetElement(0, 3);
					hh2[1] = MatHB->GetElement(1, 3);
					hh2[2] = MatHB->GetElement(2, 3);

					//trick : second handle is mirrored relative to the second point!
					hh2[0] = nn2[0] - (hh2[0] - nn2[0]);
					hh2[1] = nn2[1] - (hh2[1] - nn2[1]);
					hh2[2] = nn2[2] - (hh2[2] - nn2[2]);

					// At this stage : we have all the input we need!
					// Just draw the Bezier curve between nn1 and nn2

					int compute = 1;

					if (ob_NA->GetLMNodeType() == NORMAL_NODE && ob_NB->GetLMNodeType() == STARTING_NODE) { compute = 0; }
					
					if (compute == 1)
					{

						for (m = 0; m <= nint; m++)
						{
							// t is [0.. 1]
							t = (((double)m)) / (((double)nint + 1));
							// glBegin(GL_LINES);
							intvv[0] = (1 - t)*(1 - t)*(1 - t)*nn1[0] + 3 * (1 - t)*(1 - t)*t*hh1[0] + 3 * (1 - t)*t*t*hh2[0] + t*t*t*nn2[0];
							intvv[1] = (1 - t)*(1 - t)*(1 - t)*nn1[1] + 3 * (1 - t)*(1 - t)*t*hh1[1] + 3 * (1 - t)*t*t*hh2[1] + t*t*t*nn2[1];
							intvv[2] = (1 - t)*(1 - t)*(1 - t)*nn1[2] + 3 * (1 - t)*(1 - t)*t*hh1[2] + 3 * (1 - t)*t*t*hh2[2] + t*t*t*nn2[2];
							// glVertex3d(intvv[0], intvv[1], intvv[2]);


							t = (((double)m + 1)) / (((double)nint + 1));
							intvv2[0] = (1 - t)*(1 - t)*(1 - t)*nn1[0] + 3 * (1 - t)*(1 - t)*t*hh1[0] + 3 * (1 - t)*t*t*hh2[0] + t*t*t*nn2[0];
							intvv2[1] = (1 - t)*(1 - t)*(1 - t)*nn1[1] + 3 * (1 - t)*(1 - t)*t*hh1[1] + 3 * (1 - t)*t*t*hh2[1] + t*t*t*nn2[1];
							intvv2[2] = (1 - t)*(1 - t)*(1 - t)*nn1[2] + 3 * (1 - t)*(1 - t)*t*hh1[2] + 3 * (1 - t)*t*t*hh2[2] + t*t*t*nn2[2];

							double len = sqrt((intvv[0] - intvv2[0])*(intvv[0] - intvv2[0]) + (intvv[1] - intvv2[1])*(intvv[1] - intvv2[1]) + (intvv[2] - intvv2[2])*(intvv[2] - intvv2[2]));

							// cout << "connect:" << ls[0] << "," << ls[1] << endl;
							current_length += len;
							//glVertex3d(intvv2[0], intvv2[1], intvv2[2]);
							// glEnd();

							if ((current_length >= length_to_reach) && (length_to_reach >= preceding_length) && (decimation_index<decimation))
							{

								slm[0] = 0; slm[1] = 0; slm[2] = 0;
								decimation_index++;

								t = 0.5;
								if ((current_length - preceding_length)>0)
								{
									t = (length_to_reach - preceding_length) / (current_length - preceding_length);
								}
								slm[0] = (1 - t)*intvv[0] + t*intvv2[0];
								slm[1] = (1 - t)*intvv[1] + t*intvv2[1];
								slm[2] = (1 - t)*intvv[2] + t*intvv2[2];
								if (save_format ==0)
								{
									stream << "Curve_segment:" <<num_seg<<"-"<<decimation_index<<" " << slm[0] << " " << slm[1] << " " << slm[2] << " 0 0 1"<<	endl;
									
								}
								else
								{
									stream << "Curve_segment:" << num_seg << "-" << decimation_index << " " << slm[0] << " " << slm[1] << " " << slm[2]  << endl;
								}
								t2 = (double)(((double)decimation_index) / (((double)decimation - 1.0)));
								length_to_reach = total_length*t2;

							}

						}

					}

				}




				ind = ob_N1->GetLMNumber();
				indh = ob_H1->GetLMNumber();

				ob_H1 = Handles->GetLandmarkAfter(indh);
				ob_N1 = Nodes->GetLandmarkAfter(ind);
				if (ob_N1 != NULL)
				{
					ind2 = ob_N1->GetLMNumber();
					ob_N2 = Nodes->GetLandmarkAfter(ind2);
				}
				if (ob_H1 != NULL)
				{
					indh2 = ob_H1->GetLMNumber();
					ob_H2 = Handles->GetLandmarkAfter(indh2);
				}
				if (ob_N1 != NULL&& ob_N1->GetLMNodeType() == STARTING_NODE) {
					ob_Hstart = ob_H1;
					ob_Nstart = ob_N1;

				}

				k++;
			}
		}
		
//END



	}
	file.close();
	return 1;
}

int mqMorphoDigCore::SaveCURFile(QString fileName, int save_only_selected)
{


	std::string CURext = ".cur";
	std::string CURext2 = ".CUR";
	std::size_t found = fileName.toStdString().find(CURext);
	std::size_t found2 = fileName.toStdString().find(CURext2);
	if (found == std::string::npos && found2 == std::string::npos)
	{
		fileName.append(".cur");
	}




	QFile file(fileName);
	if (file.open(QIODevice::WriteOnly | QIODevice::Text))
	{
		QTextStream stream(&file);

		vtkSmartPointer<vtkLMActorCollection> myCollN = vtkSmartPointer<vtkLMActorCollection>::New();
		vtkSmartPointer<vtkLMActorCollection> myCollH = vtkSmartPointer<vtkLMActorCollection>::New();
		myCollN = this->NodeLandmarkCollection;
		myCollH = this->HandleLandmarkCollection;

		if (myCollN->GetNumberOfItems() != myCollH->GetNumberOfItems()) { return 0; }
		myCollN->InitTraversal();
		myCollH->InitTraversal();
		for (vtkIdType i = 0; i < myCollH->GetNumberOfItems(); i++)
		{
			vtkLMActor *myActorN = vtkLMActor::SafeDownCast(myCollN->GetNextActor());
			vtkLMActor *myActorH = vtkLMActor::SafeDownCast(myCollH->GetNextActor());
			if (myActorN->GetSelected() == 1 || save_only_selected == 0) // we do not care if H is selected or not. We follow "N".
			{

				double lmposN[3];
				myActorN->GetLMOrigin(lmposN);
				double lmposH[3];
				myActorH->GetLMOrigin(lmposH);
				
				int lmnodetype = myActorN->GetLMNodeType();
				stream << "CurvePart"<<i+1<<": "<< lmposN[0] << " " << lmposN[1] << " " << lmposN[2] << " "
					<< lmposH[0] << " " << lmposH[1] << " " << lmposH[2] << " " << lmnodetype<<
					endl;

			}

		}




	}
	file.close();
	return 1;

}

int mqMorphoDigCore::SaveLandmarkFile(QString fileName, int lm_type, int file_type, int save_only_selected)
{

	if (file_type == 0)
	{
		std::string VERext = ".ver";
		std::string VERext2 = ".VER";
		std::size_t found = fileName.toStdString().find(VERext);
		std::size_t found2 = fileName.toStdString().find(VERext2);
		if (found == std::string::npos && found2 == std::string::npos)
		{
			fileName.append(".ver");
		}
	}

	if (file_type == 1)
	{
		std::string LMKext = ".lmk";
		std::string LMKext2 = ".LMK";
		std::size_t found = fileName.toStdString().find(LMKext);
		std::size_t found2 = fileName.toStdString().find(LMKext2);
		if (found == std::string::npos && found2 == std::string::npos)
		{
			fileName.append(".lmk");
		}
	}

	QFile file(fileName);
	if (file.open(QIODevice::WriteOnly | QIODevice::Text))
	{
		QTextStream stream(&file);

		vtkSmartPointer<vtkLMActorCollection> myColl = vtkSmartPointer<vtkLMActorCollection>::New();
		if (lm_type == 0) { myColl = this->NormalLandmarkCollection; }
		else if (lm_type == 1) { myColl = this->TargetLandmarkCollection; }
		else if (lm_type == 2) { myColl = this->NodeLandmarkCollection; }
		else  { myColl = this->HandleLandmarkCollection; }
		
		myColl->InitTraversal();
		for (vtkIdType i = 0; i < myColl->GetNumberOfItems(); i++)
		{
			vtkLMActor *myActor = vtkLMActor::SafeDownCast(myColl->GetNextActor());
			if (myActor->GetSelected() == 1 || save_only_selected==0)
			{
				
				double lmpos[3];
				myActor->GetLMOrigin(lmpos);
				double ori[3];
				myActor->GetLMOrientation(ori);
				double lmori[3] = { lmpos[0] + ori[0],lmpos[1] + ori[1] ,lmpos[2] + ori[2] };
				if (file_type == 0)
				{
					stream << "Landmark" << i+1 << ": " << lmpos[0]<<" "<<lmpos[1]<<" "<<lmpos[2]<<" "<<lmori[0]<< " "<< lmori[1] << " " <<  lmori[2] << " " << endl;
				}
				else
				{
					stream << "Landmark" << i+1 << ": " << lmpos[0] << " " << lmpos[1] << " " << lmpos[2] << endl;
				}

			}

		}
		
		


	}
	file.close();
	return 1;

}

int mqMorphoDigCore::SaveShapeMeasures(QString fileName, int mode)
{
	//mode: 1: area and volume 
	//mode: 2: normalized shape index area and volume	
	//mode: 3: convex hull area_ratio and ch_normalized_shape_index, area, volume, ch_area, ch_volume
	
	QFile file(fileName);
	if (file.open(QIODevice::WriteOnly | QIODevice::Text))
	{
		QTextStream stream(&file);
		if (mode == 1)
		{
			stream << "Surface_name	Area Volume" << endl; 
		}
		else
		if (mode == 2)
		{
			stream << "Surface_name	Normalized_shape_index Area Volume" << endl;
		}
		else
		{
			stream << "Surface_name	Convex_Hull_Area_Ratio Convex_Hull_Shape_Index Area Volume Convex_hull_area Convex_hull_volume Normalized_shape_index" << endl;
		}
		
		//this->ComputeSelectedNamesLists();
	
		this->ActorCollection->InitTraversal();



		for (vtkIdType i = 0; i < this->ActorCollection->GetNumberOfItems(); i++)
		{
			vtkMDActor *myActor = vtkMDActor::SafeDownCast(this->ActorCollection->GetNextActor());
			if (myActor->GetSelected() == 1)
			{
			
					vtkPolyDataMapper *mapper = vtkPolyDataMapper::SafeDownCast(myActor->GetMapper());
					if (mapper != NULL && vtkPolyData::SafeDownCast(mapper->GetInput()) != NULL)
					{
					
						//vtkSmartPointer<vtkPolyData> toMeasure = vtkSmartPointer<vtkPolyData>::New();
						vtkSmartPointer<vtkMassProperties> massProp = vtkSmartPointer<vtkMassProperties>::New();
						
						massProp->SetInputData(mapper->GetInput());
						massProp->Update();
						double surface_area = massProp->GetSurfaceArea();
						double volume = massProp->GetVolume();
						if (mode == 1)
						{
							
							//stream << myActor->GetName().c_str() << "	" << massProp->GetNormalizedShapeIndex() << "	" << surface_area << "	" << volume <<  endl;
							stream << myActor->GetName().c_str() << "	"  << surface_area << "	" << volume << endl;
						}
						else if (mode == 2)
						{
							stream << myActor->GetName().c_str() << "	" << massProp->GetNormalizedShapeIndex() << "	" << surface_area << "	" << volume << endl;
						}
						else if (mode ==3)
						{

							/*vtkSmartPointer<vtkPolyData> myPD = vtkSmartPointer<vtkPolyData>::New();
							vtkSmartPointer<vtkCenterOfMass> centerOfMassFilter = vtkSmartPointer<vtkCenterOfMass>::New();
							centerOfMassFilter->SetInputData(mapper->GetInput());
							centerOfMassFilter->SetUseScalarsAsWeights(false);
							double acenter[3];
							centerOfMassFilter->Update();
							centerOfMassFilter->GetCenter(acenter);
							myPD = vtkPolyData::SafeDownCast(mapper->GetInput());
							double ve[3];

							

							vtkIdType id_min = NULL;
							for (vtkIdType j = 0; j < myPD->GetNumberOfPoints(); j++)
							{
								// for every triangle 
								myPD->GetPoint(j, ve);
								

							}*/



					
							vtkSmartPointer<vtkMassProperties> massPropConvexHull = vtkSmartPointer<vtkMassProperties>::New();
							vtkSmartPointer<vtkQuadricDecimation> decimate =
								vtkSmartPointer<vtkQuadricDecimation>::New();

							vtkSmartPointer<vtkDelaunay3D> delaunay3D =
								vtkSmartPointer<vtkDelaunay3D>::New();
							decimate->SetInputData(mapper->GetInput());
							decimate->SetVolumePreservation(1);
							double numvert = mapper->GetInput()->GetNumberOfPoints();
							double reduction_factor = 0.1;
							double target_number = 100000;
							double new_factor = target_number/ numvert;
							if (new_factor < 1)
							{
								reduction_factor = 1 - new_factor;
							}
							cout << "try to update quadric decimation by a factor of " << reduction_factor<< endl;


							
							if (new_factor < 1)
							{
								decimate->SetTargetReduction(reduction_factor);
								decimate->Update();
								delaunay3D->SetInputData(decimate->GetOutput());
							}
							else
							{
								delaunay3D->SetInputData(mapper->GetInput());
							}
							cout << "try to update Delaunay 3D" << endl;
							delaunay3D->Update();
							cout << "Delaunay 3D updated" << endl;
							vtkSmartPointer<vtkGeometryFilter> geometryFilter =
								vtkSmartPointer<vtkGeometryFilter>::New();

							geometryFilter->SetInputConnection(delaunay3D->GetOutputPort());
							cout << "Try to update geometry filter" << endl;
							geometryFilter->Update();
							cout << "Geometry filter updated" << endl;
							/*vtkSmartPointer<vtkDataSetSurfaceFilter> surfaceFilter2 =
								vtkSmartPointer<vtkDataSetSurfaceFilter>::New();
							surfaceFilter2->SetInputData(geometryFilter->GetOutput());
							surfaceFilter2->Update();*/


							//delaunay3D->SetInputConnection(mapper->GetInputConnection();

							massPropConvexHull->SetInputData(geometryFilter->GetOutput());
							//massPropConvexHull->SetInputConnection(delaunay3D->GetOutputPort());
							cout << "Try to update massPropConvexHull" << endl;
							massPropConvexHull->Update();
							cout << "massPropConvexHull updated" << endl;
							
							double sqrt_surface_area = sqrt(surface_area);

							double surface_area_convex_hull = massPropConvexHull->GetSurfaceArea();
							double sqrt_surface_area_convex_hull = sqrt(surface_area_convex_hull);


							double volume_convex_hull = massPropConvexHull->GetVolume();
							cout << myActor->GetName().c_str() << " volume=" << massProp->GetVolume() << " volume_convex_hull=" << massPropConvexHull->GetVolume() << endl;
							double cbrt_volume_convex_hull = cbrt(volume_convex_hull);
							double custom_complexity = sqrt_surface_area / (cbrt_volume_convex_hull*2.199085233);
							double surface_ratio = 1;
							if (surface_area_convex_hull > 0) { 
									surface_ratio=surface_area / surface_area_convex_hull; 
							}

							
							
							stream << myActor->GetName().c_str() << "	" << surface_ratio << "	" << custom_complexity << "	" << surface_area << "	" << volume << "	" << surface_area_convex_hull << "	" << volume_convex_hull << "	"<< massProp->GetNormalizedShapeIndex()<<  endl;
							
						}
						
					}
			
			
			}
		}
	}
	file.close();
	return 1;
}

void mqMorphoDigCore::addConvexHull()
{
	vtkSmartPointer<vtkMDActorCollection> newcoll = vtkSmartPointer<vtkMDActorCollection>::New();
	this->ActorCollection->InitTraversal();
	vtkIdType num = this->ActorCollection->GetNumberOfItems();
	int modified = 0;
	for (vtkIdType i = 0; i < num; i++)
	{
		cout << "try to get next actor:" << i << endl;
		vtkMDActor *myActor = vtkMDActor::SafeDownCast(this->ActorCollection->GetNextActor());
		if (myActor->GetSelected() == 1)
		{
			myActor->SetSelected(0);

			vtkPolyDataMapper *mymapper = vtkPolyDataMapper::SafeDownCast(myActor->GetMapper());
			if (mymapper != NULL && vtkPolyData::SafeDownCast(mymapper->GetInput()) != NULL)
			{

				vtkSmartPointer<vtkQuadricDecimation> decimate =
					vtkSmartPointer<vtkQuadricDecimation>::New();
				vtkSmartPointer<vtkDelaunay3D> delaunay3D =
					vtkSmartPointer<vtkDelaunay3D>::New();
				decimate->SetInputData(mymapper->GetInput());
				decimate->SetVolumePreservation(1);
				double numvert = mymapper->GetInput()->GetNumberOfPoints();
				double reduction_factor = 0.1;
				double target_number = 100000;
				double new_factor = target_number / numvert;
				if (new_factor < 1)
				{
					reduction_factor = 1 - new_factor;
				}
				cout << "try to update quadric decimation by a factor of " << reduction_factor << endl;
				new_factor = 0.9;

				if (new_factor < 1)
				{
					decimate->SetTargetReduction(reduction_factor);
					decimate->Update();
					delaunay3D->SetInputData(decimate->GetOutput());
				}
				else
				{
					delaunay3D->SetInputData(mymapper->GetInput());
				}
				cout << "try to update Delaunay 3D" << endl;
				delaunay3D->Update();
				cout << "Delaunay 3D updated" << endl;

				vtkSmartPointer<vtkGeometryFilter> geometryFilter =
				vtkSmartPointer<vtkGeometryFilter>::New();

				geometryFilter->SetInputConnection(delaunay3D->GetOutputPort());
				cout << "Try to update geometry filter" << endl;
				geometryFilter->Update();
				cout << "Geometry filter updated" << endl;
				


			
			

				VTK_CREATE(vtkMDActor, newactor);
				if (this->mui_BackfaceCulling == 0)
				{
					newactor->GetProperty()->BackfaceCullingOff();
				}
				else
				{
					newactor->GetProperty()->BackfaceCullingOn();
				}


				//VTK_CREATE(vtkDataSetMapper, newmapper);
				VTK_CREATE(vtkPolyDataMapper, newmapper);
				//VTK_CREATE(vtkSmartPointer<vtkDataSetMapper>
				//newmapper->ImmediateModeRenderingOn();
				newmapper->SetColorModeToDefault();

				if (
					(this->mui_ActiveScalars->DataType == VTK_INT || this->mui_ActiveScalars->DataType == VTK_UNSIGNED_INT)
					&& this->mui_ActiveScalars->NumComp == 1
					)
				{
					newmapper->SetScalarRange(0, this->TagTableSize - 1);
					newmapper->SetLookupTable(this->GetTagLut());
				}
				else
				{
					newmapper->SetLookupTable(this->Getmui_ActiveColorMap()->ColorMap);
				}


				newmapper->ScalarVisibilityOn();
				VTK_CREATE(vtkPolyData, myData);
				myData = geometryFilter->GetOutput();
				cout << "myConvexHull: nv=" << myData->GetNumberOfPoints() << endl;
				//newmapper->SetInputConnection(delaunay3D->GetOutputPort());
				newmapper->SetInputData(myData);
				
				//VTK_CREATE(vtkActor, actor);

				int num = 2;

				vtkSmartPointer<vtkMatrix4x4> Mat = myActor->GetMatrix();
				vtkTransform *newTransform = vtkTransform::New();
				newTransform->PostMultiply();
				
				newTransform->SetMatrix(Mat);
				newactor->SetPosition(newTransform->GetPosition());
				newactor->SetScale(newTransform->GetScale());
				newactor->SetOrientation(newTransform->GetOrientation());						
				newTransform->Delete();


				double color[4] = { 0.5, 0.5, 0.5, 1 };
				myActor->GetmColor(color);
				double trans = 20;
				if (trans >= 0 && trans <= 100)
				{
					color[3] = (double)((double)trans / 100);


				}
				
				newactor->SetmColor(color);

				newactor->SetMapper(newmapper);
				newactor->SetSelected(0);
				

				newactor->SetName("CH" + myActor->GetName());
				cout << "try to add new actor=" << endl;
				newcoll->AddTmpItem(newactor);
				modified = 1;
				

			}
		}
	}
	if (modified ==1)
	{
		newcoll->InitTraversal();
		vtkIdType num = newcoll->GetNumberOfItems();
		for (vtkIdType i = 0; i < num; i++)
		{
			cout << "try to get next actor from newcoll:" << i << endl;
			vtkMDActor *myActor = vtkMDActor::SafeDownCast(newcoll->GetNextActor());


			this->getActorCollection()->AddItem(myActor);
			std::string action = "Convex Hull added: " + myActor->GetName();
			int mCount = BEGIN_UNDO_SET(action);
			this->getActorCollection()->CreateLoadUndoSet(mCount, 1);
			END_UNDO_SET();

		
		}
		//cout << "camera and grid adjusted" << endl;
		cout << "new actor(s) added" << endl;
		this->Initmui_ExistingScalars();

		cout << "Set actor collection changed" << endl;
		this->getActorCollection()->SetChanged(1);
		cout << "Actor collection changed" << endl;

		this->AdjustCameraAndGrid();
		cout << "Camera and grid adjusted" << endl;

		if (this->Getmui_AdjustLandmarkRenderingSize() == 1)
		{
			this->UpdateLandmarkSettings();
		}
		this->Render();
	}
}
void mqMorphoDigCore::addMirrorXZ()
{
	vtkSmartPointer<vtkMDActorCollection> newcoll = vtkSmartPointer<vtkMDActorCollection>::New();
	this->ActorCollection->InitTraversal();
	vtkIdType num = this->ActorCollection->GetNumberOfItems();
	int modified = 0;
	for (vtkIdType i = 0; i < num; i++)
	{
		cout << "Mirror XZ try to get next actor:" << i << endl;
		vtkMDActor *myActor = vtkMDActor::SafeDownCast(this->ActorCollection->GetNextActor());
		if (myActor->GetSelected() == 1)
		{
			//myActor->SetSelected(0);

			vtkPolyDataMapper *mymapper = vtkPolyDataMapper::SafeDownCast(myActor->GetMapper());
			if (mymapper != NULL && vtkPolyData::SafeDownCast(mymapper->GetInput()) != NULL)
			{

				
				double numvert = mymapper->GetInput()->GetNumberOfPoints();

				VTK_CREATE(vtkMDActor, newactor);
				if (this->mui_BackfaceCulling == 0)
				{
					newactor->GetProperty()->BackfaceCullingOff();
				}
				else
				{
					newactor->GetProperty()->BackfaceCullingOn();
				}

				VTK_CREATE(vtkPolyDataMapper, newmapper);
				//newmapper->ImmediateModeRenderingOn();
				newmapper->SetColorModeToDefault();

				if (
					(this->mui_ActiveScalars->DataType == VTK_INT || this->mui_ActiveScalars->DataType == VTK_UNSIGNED_INT)
					&& this->mui_ActiveScalars->NumComp == 1
					)
				{
					newmapper->SetScalarRange(0, this->TagTableSize - 1);
					newmapper->SetLookupTable(this->GetTagLut());
				}
				else
				{
					newmapper->SetLookupTable(this->Getmui_ActiveColorMap()->ColorMap);
				}

				newmapper->ScalarVisibilityOn();
				VTK_CREATE(vtkPolyData, myData);
				vtkSmartPointer<vtkReflectionFilter> mfilter = vtkSmartPointer<vtkReflectionFilter>::New();
				mfilter->CopyInputOff();
				mfilter->SetInputData(vtkPolyData::SafeDownCast(mymapper->GetInput()));
				mfilter->SetPlaneToY();
				
				mfilter->Update();
				vtkSmartPointer<vtkUnstructuredGrid> grid = vtkSmartPointer<vtkUnstructuredGrid>::New();
				grid = (vtkUnstructuredGrid*)mfilter->GetOutput();
				vtkSmartPointer<vtkGeometryFilter> fgeo = vtkSmartPointer<vtkGeometryFilter>::New();

				fgeo->SetInputData(grid);
				fgeo->Update();
				//MyObj = fgeo->GetOutput();

				myData = fgeo->GetOutput();
				cout << "myMirror: nv=" << myData->GetNumberOfPoints() << endl;
				//newmapper->SetInputConnection(delaunay3D->GetOutputPort());
				

				//VTK_CREATE(vtkActor, actor);

				vtkSmartPointer<vtkPolyDataNormals> ObjNormals = vtkSmartPointer<vtkPolyDataNormals>::New();
				ObjNormals->SetInputData(myData);
				ObjNormals->ComputePointNormalsOn();
				ObjNormals->ComputeCellNormalsOn();
				ObjNormals->ConsistencyOff();
				ObjNormals->Update();

				vtkSmartPointer<vtkCleanPolyData> cleanPolyDataFilter = vtkSmartPointer<vtkCleanPolyData>::New();
				cleanPolyDataFilter->SetInputData(ObjNormals->GetOutput());
				cleanPolyDataFilter->PieceInvariantOff();
				cleanPolyDataFilter->ConvertLinesToPointsOff();
				cleanPolyDataFilter->ConvertPolysToLinesOff();
				cleanPolyDataFilter->ConvertStripsToPolysOff();
				cleanPolyDataFilter->PointMergingOn();
				cleanPolyDataFilter->Update();

				myData = cleanPolyDataFilter->GetOutput();

				std::cout << "\nVtkReflection new Number of points:" << myData->GetNumberOfPoints() << std::endl;
				std::cout << "VtkReflection new Number of cells:" << myData->GetNumberOfCells() << std::endl;

				newmapper->SetInputData(myData);

				

				
				vtkSmartPointer<vtkMatrix4x4> MatOrig = myActor->GetMatrix();
				vtkSmartPointer<vtkMatrix4x4> Mat = vtkSmartPointer<vtkMatrix4x4>::New();
				Mat->DeepCopy (myActor->GetMatrix());

				double n1, n2, n3, n4;
				n1 = -1 * MatOrig->GetElement(3, 1);
				n2 = -1 * MatOrig->GetElement(0, 1);
				n3 = -1 * MatOrig->GetElement(1, 0);
				n4 = -1 * MatOrig->GetElement(2, 1);

				Mat->SetElement(3, 1, n1);
				Mat->SetElement(0, 1, n2);
				Mat->SetElement(1, 0, n3);
				Mat->SetElement(2, 1, n4);

				

				vtkTransform *newTransform = vtkTransform::New();
				newTransform->PostMultiply();

				newTransform->SetMatrix(Mat);
				newactor->SetPosition(newTransform->GetPosition());
				newactor->SetScale(newTransform->GetScale());
				newactor->SetOrientation(newTransform->GetOrientation());
				newTransform->Delete();


				double color[4] = { 0.5, 0.5, 0.5, 1 };
				myActor->GetmColor(color);
				newactor->SetmColor(color);

				newactor->SetMapper(newmapper);
				newactor->SetSelected(0);


				newactor->SetName(myActor->GetName()+"_mir");
				cout << "try to add new actor=" << endl;
				newcoll->AddTmpItem(newactor);
				modified = 1;


			}
		}
	}
	if (modified == 1)
	{
		newcoll->InitTraversal();
		vtkIdType num = newcoll->GetNumberOfItems();
		for (vtkIdType i = 0; i < num; i++)
		{
			cout << "try to get next actor from newcoll:" << i << endl;
			vtkMDActor *myActor = vtkMDActor::SafeDownCast(newcoll->GetNextActor());


			this->getActorCollection()->AddItem(myActor);
			std::string action = "Mirror object added: " + myActor->GetName();
			int mCount = BEGIN_UNDO_SET(action);
			this->getActorCollection()->CreateLoadUndoSet(mCount, 1);
			END_UNDO_SET();

			
		}
		//cout << "camera and grid adjusted" << endl;
		cout << "new actor(s) added" << endl;
		this->Initmui_ExistingScalars();

		cout << "Set actor collection changed" << endl;
		this->getActorCollection()->SetChanged(1);
		cout << "Actor collection changed" << endl;

		this->AdjustCameraAndGrid();
		cout << "Camera and grid adjusted" << endl;

		if (this->Getmui_AdjustLandmarkRenderingSize() == 1)
		{
			this->UpdateLandmarkSettings();
		}
		this->Render();
	}
}

bool mqMorphoDigCore::RecoverLandmarks(vtkSmartPointer< vtkPoints > landmarks_list_source, vtkSmartPointer< vtkPoints > landmarks_list_target, int all) {

	//all = 0 : only selected landmarks
	//all = 1 : all landmarks
	
	if (this->NormalLandmarkCollection->GetNumberOfItems() ==0)
	{
		QMessageBox msgBox;
		msgBox.setText("ERROR:: RecoverLandmarks : no landmarks.");
		msgBox.exec();
		return false;
	}

	if (all == 0)
	{
		if (this->NormalLandmarkCollection->GetNumberOfSelectedActors() != this->TargetLandmarkCollection->GetNumberOfSelectedActors())
		{
			QMessageBox msgBox;
			msgBox.setText("ERROR:: RecoverLandmarks : not the same number of landmarks.");
			msgBox.exec();
			return false;
		}
		if (this->NormalLandmarkCollection->GetNumberOfSelectedActors() ==0)
		{
			QMessageBox msgBox;
			msgBox.setText("ERROR:: RecoverLandmarks : no selected landmarks.");
			msgBox.exec();
			return false;
			
		}
	}

	if (this->NormalLandmarkCollection->GetNumberOfItems() != this->TargetLandmarkCollection->GetNumberOfItems())
	{
		QMessageBox msgBox;
		msgBox.setText("ERROR:: RecoverLandmarks : not the same number of landmarks.");
		msgBox.exec();		
		return false;
	}
	
	// 
		
	if (all == 1)
	{
		landmarks_list_source->SetNumberOfPoints(this->NormalLandmarkCollection->GetNumberOfItems());
		landmarks_list_target->SetNumberOfPoints(this->NormalLandmarkCollection->GetNumberOfItems());
	}
	else
	{
		landmarks_list_source->SetNumberOfPoints(this->NormalLandmarkCollection->GetNumberOfSelectedActors());
		landmarks_list_target->SetNumberOfPoints(this->NormalLandmarkCollection->GetNumberOfSelectedActors());
	}

		
		vtkIdType cpt = 0;
		this->NormalLandmarkCollection->InitTraversal();
		this->TargetLandmarkCollection->InitTraversal();
		for (vtkIdType i = 0; i < NormalLandmarkCollection->GetNumberOfItems(); i++)
		{
			vtkLMActor *mySourceActor = vtkLMActor::SafeDownCast(NormalLandmarkCollection->GetNextActor());
			vtkLMActor *myTargetActor = vtkLMActor::SafeDownCast(TargetLandmarkCollection->GetNextActor());
			if (mySourceActor->GetSelected() == 1 || all == 1)
			{

				double lmsourcepos[3];
				mySourceActor->GetLMOrigin(lmsourcepos);

				double lmtargetpos[3];
				myTargetActor->GetLMOrigin(lmtargetpos);
				//cout << "source" << i <<":"<< lmsourcepos[0] << "," << lmsourcepos[1] << "," << lmsourcepos[2] << endl;
				landmarks_list_source->SetPoint(cpt, lmsourcepos);
				//cout << "target" << i << ":" << lmtargetpos[0] << ","<< lmtargetpos[1]<<","<< lmtargetpos[2]<< endl;
				landmarks_list_target->SetPoint(cpt, lmtargetpos);
				cpt++;
			}

		}
						
		return true;
	
}


void mqMorphoDigCore::addTPS(int r, double factor, int all)
{

	if (this->NormalLandmarkCollection->GetNumberOfItems() == 0)
	{
		QMessageBox msgBox;
		msgBox.setText("ERROR:: RecoverLandmarks : no landmarks.");
		msgBox.exec();
		return;
	}

	if (all == 0)
	{
		if (this->NormalLandmarkCollection->GetNumberOfSelectedActors() != this->TargetLandmarkCollection->GetNumberOfSelectedActors())
		{
			QMessageBox msgBox;
			msgBox.setText("ERROR:: RecoverLandmarks : not the same number of landmarks.");
			msgBox.exec();
			return;
		}
		if (this->NormalLandmarkCollection->GetNumberOfSelectedActors() == 0)
		{
			QMessageBox msgBox;
			msgBox.setText("ERROR:: RecoverLandmarks : no selected landmarks.");
			msgBox.exec();
			return;

		}
	}

	if (this->NormalLandmarkCollection->GetNumberOfItems() != this->TargetLandmarkCollection->GetNumberOfItems())
	{
		QMessageBox msgBox;
		msgBox.setText("ERROR:: RecoverLandmarks : not the same number of landmarks.");
		msgBox.exec();
		return;
	}

	

	vtkSmartPointer<vtkMDActorCollection> newcoll = vtkSmartPointer<vtkMDActorCollection>::New();
	this->ActorCollection->InitTraversal();
	vtkIdType num = this->ActorCollection->GetNumberOfItems();
	int modified = 0;
	for (vtkIdType i = 0; i < num; i++)
	{
		cout << "Fill holes try to get next actor:" << i << endl;
		vtkMDActor *myActor = vtkMDActor::SafeDownCast(this->ActorCollection->GetNextActor());
		if (myActor->GetSelected() == 1)
		{
			vtkPolyDataMapper *mymapper = vtkPolyDataMapper::SafeDownCast(myActor->GetMapper());
			if (mymapper != NULL && vtkPolyData::SafeDownCast(mymapper->GetInput()) != NULL)
			{
				double numvert = mymapper->GetInput()->GetNumberOfPoints();

				//		@@@

				VTK_CREATE(vtkMDActor, newactor);
				if (this->mui_BackfaceCulling == 0)
				{
					newactor->GetProperty()->BackfaceCullingOff();
				}
				else
				{
					newactor->GetProperty()->BackfaceCullingOn();
				}

				VTK_CREATE(vtkPolyDataMapper, newmapper);
				//newmapper->ImmediateModeRenderingOn();
				newmapper->SetColorModeToDefault();

				if (
					(this->mui_ActiveScalars->DataType == VTK_INT || this->mui_ActiveScalars->DataType == VTK_UNSIGNED_INT)
					&& this->mui_ActiveScalars->NumComp == 1
					)
				{
					newmapper->SetScalarRange(0, this->TagTableSize - 1);
					newmapper->SetLookupTable(this->GetTagLut());
				}
				else
				{
					newmapper->SetLookupTable(this->Getmui_ActiveColorMap()->ColorMap);
				}

				newmapper->ScalarVisibilityOn();

				cout << "TPS basis=" << r<< endl;
			
				///@@@ TPS
				vtkSmartPointer<vtkThinPlateSplineTransform> tps =
				vtkSmartPointer<vtkThinPlateSplineTransform>::New();

				if (r == 1)
				{
					tps->SetBasisToR();
				}
				else
				{
					tps->SetBasisToR2LogR();
				}
				vtkSmartPointer< vtkPoints > p1 = vtkSmartPointer< vtkPoints >::New();
				vtkSmartPointer< vtkPoints > p2 = vtkSmartPointer< vtkPoints >::New();

				// fetch source and target landmarks
				RecoverLandmarks(p1, p2, all);

				tps->SetSourceLandmarks(p1);
				tps->SetTargetLandmarks(p2);
				tps->Update();

				vtkSmartPointer<vtkTransformPolyDataFilter> transformFilter = vtkSmartPointer<vtkTransformPolyDataFilter>::New();
				vtkSmartPointer<vtkPolyData> MyTPSInput = vtkSmartPointer<vtkPolyData>::New();
				vtkSmartPointer<vtkPolyData> MyInput = vtkSmartPointer<vtkPolyData>::New();
				MyInput=mymapper->GetInput();
				MyTPSInput->DeepCopy(mymapper->GetInput());

				// recup�re la position de l'object s'il a boug�
				double ve_init_pos[3];;
				double ve_final_pos[3];
				double ve_trans_pos[3];
				vtkSmartPointer<vtkMatrix4x4> Mat = myActor->GetMatrix();


				for (vtkIdType i = 0; i < MyTPSInput->GetNumberOfPoints(); i++) {
					// for every triangle 
					MyTPSInput->GetPoint(i, ve_init_pos);
					mqMorphoDigCore::TransformPoint(Mat, ve_init_pos, ve_final_pos);

					MyTPSInput->GetPoints()->SetPoint((vtkIdType)i, ve_final_pos);
				}
				


				transformFilter->SetInputData(MyTPSInput);

				/// applique le calcul du tps � l'objet
				transformFilter->SetTransform(tps);
				transformFilter->Update();

				// mise � jour du maillage sortant pour le tps
				vtkSmartPointer<vtkPolyData> My_Output = vtkSmartPointer<vtkPolyData>::New();
				My_Output = transformFilter->GetOutput();

			

				// % deformation

				double mfactor = factor / 100;

				for (vtkIdType i = 0; i<My_Output->GetNumberOfPoints(); i++) {
					// for every triangle 
					MyTPSInput->GetPoint(i, ve_init_pos);
					My_Output->GetPoint(i, ve_final_pos);
					ve_trans_pos[0] = mfactor*(float)ve_final_pos[0] + (1 - mfactor)*ve_init_pos[0];
					ve_trans_pos[1] = mfactor*(float)ve_final_pos[1] + (1 - mfactor)*ve_init_pos[1];
					ve_trans_pos[2] = mfactor*(float)ve_final_pos[2] + (1 - mfactor)*ve_init_pos[2];

					My_Output->GetPoints()->SetPoint(i, ve_trans_pos);
				}
				


				vtkSmartPointer<vtkPolyDataNormals> ObjNormals = vtkSmartPointer<vtkPolyDataNormals>::New();
				ObjNormals->SetInputData(My_Output);
				ObjNormals->ComputePointNormalsOn();
				ObjNormals->ComputeCellNormalsOn();
				//ObjNormals->AutoOrientNormalsOff();
				ObjNormals->ConsistencyOff();

				ObjNormals->Update();

				vtkSmartPointer<vtkCleanPolyData> cleanPolyDataFilter = vtkSmartPointer<vtkCleanPolyData>::New();
				cleanPolyDataFilter->SetInputData(ObjNormals->GetOutput());
				cleanPolyDataFilter->PieceInvariantOff();
				cleanPolyDataFilter->ConvertLinesToPointsOff();
				cleanPolyDataFilter->ConvertPolysToLinesOff();
				cleanPolyDataFilter->ConvertStripsToPolysOff();
				cleanPolyDataFilter->PointMergingOn();
				cleanPolyDataFilter->Update();
				VTK_CREATE(vtkPolyData, myData);

				myData = cleanPolyDataFilter->GetOutput();

				cout << "TPS: nv=" << myData->GetNumberOfPoints() << endl;
				//newmapper->SetInputConnection(delaunay3D->GetOutputPort());



				newmapper->SetInputData(myData);




				/*vtkSmartPointer<vtkMatrix4x4> Mat = vtkSmartPointer<vtkMatrix4x4>::New();
				Mat = myActor->GetMatrix();


				vtkTransform *newTransform = vtkTransform::New();
				newTransform->PostMultiply();

				newTransform->SetMatrix(Mat);
				newactor->SetPosition(newTransform->GetPosition());
				newactor->SetScale(newTransform->GetScale());
				newactor->SetOrientation(newTransform->GetOrientation());
				newTransform->Delete();*/


				double color[4] = { 0.5, 0.5, 0.5, 1 };
				myActor->GetmColor(color);

				color[3] = 0.5;
				newactor->SetmColor(color);

				newactor->SetMapper(newmapper);
				newactor->SetSelected(0);


				newactor->SetName(myActor->GetName() + "_tps");
				cout << "try to add new actor=" << endl;
				newcoll->AddTmpItem(newactor);
				modified = 1;


			}
		}
	}
	if (modified == 1)
	{
		newcoll->InitTraversal();
		vtkIdType num = newcoll->GetNumberOfItems();
		for (vtkIdType i = 0; i < num; i++)
		{
			cout << "try to get next actor from newcoll:" << i << endl;
			vtkMDActor *myActor = vtkMDActor::SafeDownCast(newcoll->GetNextActor());


			this->getActorCollection()->AddItem(myActor);
			std::string action = "TPS object added: " + myActor->GetName();
			int mCount = BEGIN_UNDO_SET(action);
			this->getActorCollection()->CreateLoadUndoSet(mCount, 1);
			END_UNDO_SET();


		}
		//cout << "camera and grid adjusted" << endl;
		cout << "new actor(s) added" << endl;
		this->Initmui_ExistingScalars();

		cout << "Set actor collection changed" << endl;
		this->getActorCollection()->SetChanged(1);
		cout << "Actor collection changed" << endl;

		this->AdjustCameraAndGrid();
		cout << "Camera and grid adjusted" << endl;

		if (this->Getmui_AdjustLandmarkRenderingSize() == 1)
		{
			this->UpdateLandmarkSettings();
		}
		this->Render();
	}



}
void mqMorphoDigCore::addFillHoles(int maxsize) 
{
	vtkSmartPointer<vtkMDActorCollection> newcoll = vtkSmartPointer<vtkMDActorCollection>::New();
	this->ActorCollection->InitTraversal();
	vtkIdType num = this->ActorCollection->GetNumberOfItems();
	int modified = 0;
	for (vtkIdType i = 0; i < num; i++)
	{
		cout << "Fill holes try to get next actor:" << i << endl;
		vtkMDActor *myActor = vtkMDActor::SafeDownCast(this->ActorCollection->GetNextActor());
		if (myActor->GetSelected() == 1)
		{
			vtkPolyDataMapper *mymapper = vtkPolyDataMapper::SafeDownCast(myActor->GetMapper());
			if (mymapper != NULL && vtkPolyData::SafeDownCast(mymapper->GetInput()) != NULL)
			{
				double numvert = mymapper->GetInput()->GetNumberOfPoints();

				//		@@@

				VTK_CREATE(vtkMDActor, newactor);
				if (this->mui_BackfaceCulling == 0)
				{
					newactor->GetProperty()->BackfaceCullingOff();
				}
				else
				{
					newactor->GetProperty()->BackfaceCullingOn();
				}

				VTK_CREATE(vtkPolyDataMapper, newmapper);
				//newmapper->ImmediateModeRenderingOn();
				newmapper->SetColorModeToDefault();

				if (
					(this->mui_ActiveScalars->DataType == VTK_INT || this->mui_ActiveScalars->DataType == VTK_UNSIGNED_INT)
					&& this->mui_ActiveScalars->NumComp == 1
					)
				{
					newmapper->SetScalarRange(0, this->TagTableSize - 1);
					newmapper->SetLookupTable(this->GetTagLut());
				}
				else
				{
					newmapper->SetLookupTable(this->Getmui_ActiveColorMap()->ColorMap);
				}

				newmapper->ScalarVisibilityOn();
				
				cout << "holes max size =" << maxsize << endl;
				vtkSmartPointer<vtkFillHolesFilter> fillholes =
					vtkSmartPointer<vtkFillHolesFilter>::New();
				fillholes->SetInputData(vtkPolyData::SafeDownCast(mymapper->GetInput()));
				fillholes->SetHoleSize(maxsize);
				fillholes->Update();
				
				vtkSmartPointer<vtkPolyDataNormals> ObjNormals = vtkSmartPointer<vtkPolyDataNormals>::New();
				ObjNormals->SetInputData(fillholes->GetOutput());
				ObjNormals->ComputePointNormalsOn();
				ObjNormals->ComputeCellNormalsOn();
				//ObjNormals->AutoOrientNormalsOff();
				ObjNormals->ConsistencyOff();

				ObjNormals->Update();

				vtkSmartPointer<vtkCleanPolyData> cleanPolyDataFilter = vtkSmartPointer<vtkCleanPolyData>::New();
				cleanPolyDataFilter->SetInputData(ObjNormals->GetOutput());
				cleanPolyDataFilter->PieceInvariantOff();
				cleanPolyDataFilter->ConvertLinesToPointsOff();
				cleanPolyDataFilter->ConvertPolysToLinesOff();
				cleanPolyDataFilter->ConvertStripsToPolysOff();
				cleanPolyDataFilter->PointMergingOn();
				cleanPolyDataFilter->Update();
				VTK_CREATE(vtkPolyData, myData);

				myData = cleanPolyDataFilter->GetOutput();

				cout << "fill holes: nv=" << myData->GetNumberOfPoints() << endl;
				//newmapper->SetInputConnection(delaunay3D->GetOutputPort());





				newmapper->SetInputData(myData);




				vtkSmartPointer<vtkMatrix4x4> Mat = vtkSmartPointer<vtkMatrix4x4>::New();
				Mat = myActor->GetMatrix();



				vtkTransform *newTransform = vtkTransform::New();
				newTransform->PostMultiply();

				newTransform->SetMatrix(Mat);
				newactor->SetPosition(newTransform->GetPosition());
				newactor->SetScale(newTransform->GetScale());
				newactor->SetOrientation(newTransform->GetOrientation());
				newTransform->Delete();


				double color[4] = { 0.5, 0.5, 0.5, 1 };
				myActor->GetmColor(color);
				newactor->SetmColor(color);

				newactor->SetMapper(newmapper);
				newactor->SetSelected(0);


				newactor->SetName(myActor->GetName() + "_holesfilled");
				cout << "try to add new actor=" << endl;
				newcoll->AddTmpItem(newactor);
				modified = 1;


			}
		}
	}
	if (modified == 1)
	{
		newcoll->InitTraversal();
		vtkIdType num = newcoll->GetNumberOfItems();
		for (vtkIdType i = 0; i < num; i++)
		{
			cout << "try to get next actor from newcoll:" << i << endl;
			vtkMDActor *myActor = vtkMDActor::SafeDownCast(newcoll->GetNextActor());


			this->getActorCollection()->AddItem(myActor);
			std::string action = "Hole filled object added: " + myActor->GetName();
			int mCount = BEGIN_UNDO_SET(action);
			this->getActorCollection()->CreateLoadUndoSet(mCount, 1);
			END_UNDO_SET();


		}
		//cout << "camera and grid adjusted" << endl;
		cout << "new actor(s) added" << endl;
		this->Initmui_ExistingScalars();

		cout << "Set actor collection changed" << endl;
		this->getActorCollection()->SetChanged(1);
		cout << "Actor collection changed" << endl;

		this->AdjustCameraAndGrid();
		cout << "Camera and grid adjusted" << endl;

		if (this->Getmui_AdjustLandmarkRenderingSize() == 1)
		{
			this->UpdateLandmarkSettings();
		}
		this->Render();
	}

}
void mqMorphoDigCore::addDensify(int subdivisions) 
{
	vtkSmartPointer<vtkMDActorCollection> newcoll = vtkSmartPointer<vtkMDActorCollection>::New();
	this->ActorCollection->InitTraversal();
	vtkIdType num = this->ActorCollection->GetNumberOfItems();
	int modified = 0;
	for (vtkIdType i = 0; i < num; i++)
	{
		cout << "Densify try to get next actor:" << i << endl;
		vtkMDActor *myActor = vtkMDActor::SafeDownCast(this->ActorCollection->GetNextActor());
		if (myActor->GetSelected() == 1)
		{
			vtkPolyDataMapper *mymapper = vtkPolyDataMapper::SafeDownCast(myActor->GetMapper());
			if (mymapper != NULL && vtkPolyData::SafeDownCast(mymapper->GetInput()) != NULL)
			{
				double numvert = mymapper->GetInput()->GetNumberOfPoints();

				//		@@@

				VTK_CREATE(vtkMDActor, newactor);
				if (this->mui_BackfaceCulling == 0)
				{
					newactor->GetProperty()->BackfaceCullingOff();
				}
				else
				{
					newactor->GetProperty()->BackfaceCullingOn();
				}

				VTK_CREATE(vtkPolyDataMapper, newmapper);
				//newmapper->ImmediateModeRenderingOn();
				newmapper->SetColorModeToDefault();

				if (
					(this->mui_ActiveScalars->DataType == VTK_INT || this->mui_ActiveScalars->DataType == VTK_UNSIGNED_INT)
					&& this->mui_ActiveScalars->NumComp == 1
					)
				{
					newmapper->SetScalarRange(0, this->TagTableSize - 1);
					newmapper->SetLookupTable(this->GetTagLut());
				}
				else
				{
					newmapper->SetLookupTable(this->Getmui_ActiveColorMap()->ColorMap);
				}

				newmapper->ScalarVisibilityOn();
				
				cout << "densification subdivisions=" << subdivisions << endl;

				vtkSmartPointer<vtkDensifyPolyData> densify =
					vtkSmartPointer<vtkDensifyPolyData>::New();
				densify->SetInputData(vtkPolyData::SafeDownCast(mymapper->GetInput()));
				densify->SetNumberOfSubdivisions(subdivisions);

				
				
				densify->Update();
				
				vtkSmartPointer<vtkPolyDataNormals> ObjNormals = vtkSmartPointer<vtkPolyDataNormals>::New();
				//ObjNormals->SetInputData(Sfilter->GetOutput());
				
				ObjNormals->SetInputData(densify->GetOutput());
				
				ObjNormals->ComputePointNormalsOn();
				ObjNormals->ComputeCellNormalsOn();
				//ObjNormals->AutoOrientNormalsOff();
				ObjNormals->ConsistencyOff();

				ObjNormals->Update();

				vtkSmartPointer<vtkCleanPolyData> cleanPolyDataFilter = vtkSmartPointer<vtkCleanPolyData>::New();
				cleanPolyDataFilter->SetInputData(ObjNormals->GetOutput());
				cleanPolyDataFilter->PieceInvariantOff();
				cleanPolyDataFilter->ConvertLinesToPointsOff();
				cleanPolyDataFilter->ConvertPolysToLinesOff();
				cleanPolyDataFilter->ConvertStripsToPolysOff();
				cleanPolyDataFilter->PointMergingOn();
				cleanPolyDataFilter->Update();
				VTK_CREATE(vtkPolyData, myData);

				myData = cleanPolyDataFilter->GetOutput();

				cout << "densify: nv=" << myData->GetNumberOfPoints() << endl;
				//newmapper->SetInputConnection(delaunay3D->GetOutputPort());
				newmapper->SetInputData(myData);

				vtkSmartPointer<vtkMatrix4x4> Mat = vtkSmartPointer<vtkMatrix4x4>::New();
				Mat = myActor->GetMatrix();

				vtkTransform *newTransform = vtkTransform::New();
				newTransform->PostMultiply();

				newTransform->SetMatrix(Mat);
				newactor->SetPosition(newTransform->GetPosition());
				newactor->SetScale(newTransform->GetScale());
				newactor->SetOrientation(newTransform->GetOrientation());
				newTransform->Delete();


				double color[4] = { 0.5, 0.5, 0.5, 1 };
				myActor->GetmColor(color);
				newactor->SetmColor(color);

				newactor->SetMapper(newmapper);
				newactor->SetSelected(0);


				newactor->SetName(myActor->GetName() + "_densify");
				cout << "try to add new actor=" << endl;
				newcoll->AddTmpItem(newactor);
				modified = 1;


			}
		}
	}
	if (modified == 1)
	{
		newcoll->InitTraversal();
		vtkIdType num = newcoll->GetNumberOfItems();
		for (vtkIdType i = 0; i < num; i++)
		{
			cout << "try to get next actor from newcoll:" << i << endl;
			vtkMDActor *myActor = vtkMDActor::SafeDownCast(newcoll->GetNextActor());


			this->getActorCollection()->AddItem(myActor);
			std::string action = "Densified object added: " + myActor->GetName();
			int mCount = BEGIN_UNDO_SET(action);
			this->getActorCollection()->CreateLoadUndoSet(mCount, 1);
			END_UNDO_SET();


		}
		//cout << "camera and grid adjusted" << endl;
		cout << "new actor(s) added" << endl;
		this->Initmui_ExistingScalars();

		cout << "Set actor collection changed" << endl;
		this->getActorCollection()->SetChanged(1);
		cout << "Actor collection changed" << endl;

		this->AdjustCameraAndGrid();
		cout << "Camera and grid adjusted" << endl;

		if (this->Getmui_AdjustLandmarkRenderingSize() == 1)
		{
			this->UpdateLandmarkSettings();
		}
		this->Render();
	}

}
void  mqMorphoDigCore::addDecimate(int quadric, double factor)
{
	vtkSmartPointer<vtkMDActorCollection> newcoll = vtkSmartPointer<vtkMDActorCollection>::New();
	this->ActorCollection->InitTraversal();
	vtkIdType num = this->ActorCollection->GetNumberOfItems();
	int modified = 0;
	for (vtkIdType i = 0; i < num; i++)
	{
		cout << "Decimate try to get next actor:" << i << endl;
		vtkMDActor *myActor = vtkMDActor::SafeDownCast(this->ActorCollection->GetNextActor());
		if (myActor->GetSelected() == 1)
		{
			vtkPolyDataMapper *mymapper = vtkPolyDataMapper::SafeDownCast(myActor->GetMapper());
			if (mymapper != NULL && vtkPolyData::SafeDownCast(mymapper->GetInput()) != NULL)
			{
				double numvert = mymapper->GetInput()->GetNumberOfPoints();

				//		@@@

				VTK_CREATE(vtkMDActor, newactor);
				if (this->mui_BackfaceCulling == 0)
				{
					newactor->GetProperty()->BackfaceCullingOff();
				}
				else
				{
					newactor->GetProperty()->BackfaceCullingOn();
				}

				VTK_CREATE(vtkPolyDataMapper, newmapper);
				//newmapper->ImmediateModeRenderingOn();
				newmapper->SetColorModeToDefault();

				if (
					(this->mui_ActiveScalars->DataType == VTK_INT || this->mui_ActiveScalars->DataType == VTK_UNSIGNED_INT)
					&& this->mui_ActiveScalars->NumComp == 1
					)
				{
					newmapper->SetScalarRange(0, this->TagTableSize - 1);
					newmapper->SetLookupTable(this->GetTagLut());
				}
				else
				{
					newmapper->SetLookupTable(this->Getmui_ActiveColorMap()->ColorMap);
				}

				newmapper->ScalarVisibilityOn();
				double mfactor = (100 - factor)/100;
				if (mfactor == 0 || mfactor == 1) { mfactor = 0.9; }
				mfactor = 1 - mfactor;
				cout << "decimation factor=" << mfactor << endl;
				vtkSmartPointer<vtkDecimatePro> decimate =
					vtkSmartPointer<vtkDecimatePro>::New();
				decimate->SetInputData(vtkPolyData::SafeDownCast(mymapper->GetInput()));
				decimate->SetTargetReduction(mfactor);

				vtkSmartPointer<vtkQuadricDecimation> decimate2 =
					vtkSmartPointer<vtkQuadricDecimation>::New();
				decimate2->SetInputData(vtkPolyData::SafeDownCast(mymapper->GetInput()));
				decimate2->SetTargetReduction(mfactor);

				if (quadric == 0)
				{
					decimate->Update();
				}
				else
				{
					decimate2->Update();
				}
				vtkSmartPointer<vtkPolyDataNormals> ObjNormals = vtkSmartPointer<vtkPolyDataNormals>::New();
				//ObjNormals->SetInputData(Sfilter->GetOutput());
				//ObjNormals->SetComputeCellNormals(1);
				//ObjNormals->SetComputePointNormals(0);
				if (quadric == 0)
				{
					ObjNormals->SetInputData(decimate->GetOutput());
				}
				else
				{
					ObjNormals->SetInputData(decimate2->GetOutput());
				}
				ObjNormals->ComputePointNormalsOff();
				ObjNormals->ComputeCellNormalsOn();
				//ObjNormals->AutoOrientNormalsOff();
				ObjNormals->ConsistencyOff();

				ObjNormals->Update();

				vtkSmartPointer<vtkCleanPolyData> cleanPolyDataFilter = vtkSmartPointer<vtkCleanPolyData>::New();
				cleanPolyDataFilter->SetInputData(ObjNormals->GetOutput());
				cleanPolyDataFilter->PieceInvariantOff();
				cleanPolyDataFilter->ConvertLinesToPointsOff();
				cleanPolyDataFilter->ConvertPolysToLinesOff();
				cleanPolyDataFilter->ConvertStripsToPolysOff();
				cleanPolyDataFilter->PointMergingOn();
				cleanPolyDataFilter->Update();
				VTK_CREATE(vtkPolyData, myData);

				myData = cleanPolyDataFilter->GetOutput();

				cout << "decimate: nv=" << myData->GetNumberOfPoints() << endl;
				//newmapper->SetInputConnection(delaunay3D->GetOutputPort());





				newmapper->SetInputData(myData);
				



				vtkSmartPointer<vtkMatrix4x4> Mat = vtkSmartPointer<vtkMatrix4x4>::New();
				Mat = myActor->GetMatrix();



				vtkTransform *newTransform = vtkTransform::New();
				newTransform->PostMultiply();

				newTransform->SetMatrix(Mat);
				newactor->SetPosition(newTransform->GetPosition());
				newactor->SetScale(newTransform->GetScale());
				newactor->SetOrientation(newTransform->GetOrientation());
				newTransform->Delete();


				double color[4] = { 0.5, 0.5, 0.5, 1 };
				myActor->GetmColor(color);
				newactor->SetmColor(color);

				newactor->SetMapper(newmapper);
				newactor->SetSelected(0);

				
				newactor->SetName(myActor->GetName() + "_decimate");
				cout << "try to add new actor=" << endl;
				newcoll->AddTmpItem(newactor);
				modified = 1;


			}
		}
	}
	if (modified == 1)
	{
		newcoll->InitTraversal();
		vtkIdType num = newcoll->GetNumberOfItems();
		for (vtkIdType i = 0; i < num; i++)
		{
			cout << "try to get next actor from newcoll:" << i << endl;
			vtkMDActor *myActor = vtkMDActor::SafeDownCast(newcoll->GetNextActor());


			this->getActorCollection()->AddItem(myActor);
			std::string action = "Decimated object added: " + myActor->GetName();
			int mCount = BEGIN_UNDO_SET(action);
			this->getActorCollection()->CreateLoadUndoSet(mCount, 1);
			END_UNDO_SET();


		}
		//cout << "camera and grid adjusted" << endl;
		cout << "new actor(s) added" << endl;
		this->Initmui_ExistingScalars();

		cout << "Set actor collection changed" << endl;
		this->getActorCollection()->SetChanged(1);
		cout << "Actor collection changed" << endl;

		this->AdjustCameraAndGrid();
		cout << "Camera and grid adjusted" << endl;

		if (this->Getmui_AdjustLandmarkRenderingSize() == 1)
		{
			this->UpdateLandmarkSettings();
		}
		this->Render();
	}
}
void  mqMorphoDigCore::addSmooth(int iteration, double relaxation)
{

	vtkSmartPointer<vtkMDActorCollection> newcoll = vtkSmartPointer<vtkMDActorCollection>::New();
	this->ActorCollection->InitTraversal();
	vtkIdType num = this->ActorCollection->GetNumberOfItems();
	int modified = 0;
	for (vtkIdType i = 0; i < num; i++)
	{
		cout << "Smooth try to get next actor:" << i << endl;
		vtkMDActor *myActor = vtkMDActor::SafeDownCast(this->ActorCollection->GetNextActor());
		if (myActor->GetSelected() == 1)
		{
			//myActor->SetSelected(0);

			vtkPolyDataMapper *mymapper = vtkPolyDataMapper::SafeDownCast(myActor->GetMapper());
			if (mymapper != NULL && vtkPolyData::SafeDownCast(mymapper->GetInput()) != NULL)
			{


				double numvert = mymapper->GetInput()->GetNumberOfPoints();

				//		@@@

				VTK_CREATE(vtkMDActor, newactor);
				if (this->mui_BackfaceCulling == 0)
				{
					newactor->GetProperty()->BackfaceCullingOff();
				}
				else
				{
					newactor->GetProperty()->BackfaceCullingOn();
				}

				VTK_CREATE(vtkPolyDataMapper, newmapper);
				//newmapper->ImmediateModeRenderingOn();
				newmapper->SetColorModeToDefault();

				if (
					(this->mui_ActiveScalars->DataType == VTK_INT || this->mui_ActiveScalars->DataType == VTK_UNSIGNED_INT)
					&& this->mui_ActiveScalars->NumComp == 1
					)
				{
					newmapper->SetScalarRange(0, this->TagTableSize - 1);
					newmapper->SetLookupTable(this->GetTagLut());
				}
				else
				{
					newmapper->SetLookupTable(this->Getmui_ActiveColorMap()->ColorMap);
				}

				newmapper->ScalarVisibilityOn();

				vtkSmartPointer<vtkSmoothPolyDataFilter> Sfilter = vtkSmartPointer<vtkSmoothPolyDataFilter>::New();
				Sfilter->SetInputData(vtkPolyData::SafeDownCast(mymapper->GetInput()));
				Sfilter->SetNumberOfIterations(iteration);
				Sfilter->SetRelaxationFactor(relaxation);
				Sfilter->Update();
			

				vtkSmartPointer<vtkPolyDataNormals> ObjNormals = vtkSmartPointer<vtkPolyDataNormals>::New();
				ObjNormals->SetInputData(Sfilter->GetOutput());
				ObjNormals->ComputePointNormalsOn();
				ObjNormals->ComputeCellNormalsOn();
				//ObjNormals->AutoOrientNormalsOff();
				ObjNormals->ConsistencyOff();

				ObjNormals->Update();

				vtkSmartPointer<vtkCleanPolyData> cleanPolyDataFilter = vtkSmartPointer<vtkCleanPolyData>::New();
				cleanPolyDataFilter->SetInputData(ObjNormals->GetOutput());
				cleanPolyDataFilter->PieceInvariantOff();
				cleanPolyDataFilter->ConvertLinesToPointsOff();
				cleanPolyDataFilter->ConvertPolysToLinesOff();
				cleanPolyDataFilter->ConvertStripsToPolysOff();
				cleanPolyDataFilter->PointMergingOn();
				cleanPolyDataFilter->Update();
				VTK_CREATE(vtkPolyData, myData);

				myData = cleanPolyDataFilter->GetOutput();

				cout << "smooth: nv=" << myData->GetNumberOfPoints() << endl;
				//newmapper->SetInputConnection(delaunay3D->GetOutputPort());


			

			
				newmapper->SetInputData(myData);




				vtkSmartPointer<vtkMatrix4x4> Mat = vtkSmartPointer<vtkMatrix4x4>::New();
				Mat=myActor->GetMatrix();

				

				vtkTransform *newTransform = vtkTransform::New();
				newTransform->PostMultiply();

				newTransform->SetMatrix(Mat);
				newactor->SetPosition(newTransform->GetPosition());
				newactor->SetScale(newTransform->GetScale());
				newactor->SetOrientation(newTransform->GetOrientation());
				newTransform->Delete();


				double color[4] = { 0.5, 0.5, 0.5, 1 };
				myActor->GetmColor(color);
				newactor->SetmColor(color);

				newactor->SetMapper(newmapper);
				newactor->SetSelected(0);


				newactor->SetName(myActor->GetName() + "_smooth");
				cout << "try to add new actor=" << endl;
				newcoll->AddTmpItem(newactor);
				modified = 1;


			}
		}
	}
	if (modified == 1)
	{
		newcoll->InitTraversal();
		vtkIdType num = newcoll->GetNumberOfItems();
		for (vtkIdType i = 0; i < num; i++)
		{
			cout << "try to get next actor from newcoll:" << i << endl;
			vtkMDActor *myActor = vtkMDActor::SafeDownCast(newcoll->GetNextActor());


			this->getActorCollection()->AddItem(myActor);
			std::string action = "Smoothed object added: " + myActor->GetName();
			int mCount = BEGIN_UNDO_SET(action);
			this->getActorCollection()->CreateLoadUndoSet(mCount, 1);
			END_UNDO_SET();


		}
		//cout << "camera and grid adjusted" << endl;
		cout << "new actor(s) added" << endl;
		this->Initmui_ExistingScalars();

		cout << "Set actor collection changed" << endl;
		this->getActorCollection()->SetChanged(1);
		cout << "Actor collection changed" << endl;

		this->AdjustCameraAndGrid();
		cout << "Camera and grid adjusted" << endl;

		if (this->Getmui_AdjustLandmarkRenderingSize() == 1)
		{
			this->UpdateLandmarkSettings();
		}
		this->Render();
	}

}
void mqMorphoDigCore::addDecompose(int color_mode, int min_region_size)
{
	//color_mode 0 : same color as selected object
	//color_mode 1 : random color (different each time)
	//color_mode 2 : use tag lut (to be implemented!)
	vtkSmartPointer<vtkMDActorCollection> newcoll = vtkSmartPointer<vtkMDActorCollection>::New();
	this->ActorCollection->InitTraversal();
	vtkIdType num = this->ActorCollection->GetNumberOfItems();
	int modified = 0;
	int numTasks = 100000;
	//QString inprogress = QString("none");
	
	for (vtkIdType i = 0; i < num; i++)
	{
		cout << "Largest region of next actor:" << i << endl;
		vtkMDActor *myActor = vtkMDActor::SafeDownCast(this->ActorCollection->GetNextActor());
		if (myActor->GetSelected() == 1)
		{
			//myActor->SetSelected(0);

			vtkPolyDataMapper *mymapper = vtkPolyDataMapper::SafeDownCast(myActor->GetMapper());
			if (mymapper != NULL && vtkPolyData::SafeDownCast(mymapper->GetInput()) != NULL)
			{


			

				vtkSmartPointer<vtkPolyDataConnectivityFilter> cfilter = vtkSmartPointer<vtkPolyDataConnectivityFilter>::New();
				cfilter->SetInputData(mymapper->GetInput());
				cfilter->SetExtractionModeToAllRegions();
				cfilter->ColorRegionsOn();
				cfilter->Update();
				int regions = cfilter->GetNumberOfExtractedRegions();
				QProgressDialog progress("Surface decomposition into non connex subregions.", "Abort decomposition", 0, regions);
				progress.setWindowModality(Qt::WindowModal);
				std::cout << "\nVtkConnectivity number of regions:" << regions << std::endl;
				vtkSmartPointer<vtkIdTypeArray> region_sizes = vtkSmartPointer<vtkIdTypeArray>::New();
				region_sizes = cfilter->GetRegionSizes();

				vtkSmartPointer<vtkFloatArray> newScalars =
					vtkSmartPointer<vtkFloatArray>::New();

				newScalars->SetNumberOfComponents(1); //3d normals (ie x,y,z)
				newScalars->SetNumberOfTuples(mymapper->GetInput()->GetNumberOfPoints());

				vtkSmartPointer<vtkIdTypeArray> currentRegions = vtkSmartPointer<vtkIdTypeArray>::New();

				//my_data->GetNu
				currentRegions = vtkIdTypeArray::SafeDownCast(cfilter->GetOutput()->GetPointData()->GetArray("RegionId"));

				//std::cout<<"vertex"<<i<<", current region:"<<currentRegions->GetTuple(i)<<std::endl;

				for (vtkIdType i = 0; i<mymapper->GetInput()->GetNumberOfPoints(); i++)	// for each vertex 
				{

					//std::cout<<"vertex"<<i<<", current region:"<<currentRegions->GetTuple(i)[0]<<std::endl;
					newScalars->InsertTuple1(i, (double)currentRegions->GetTuple(i)[0]);


				}
				newScalars->SetName("TMP");
				vtkSmartPointer<vtkPolyData> MyObj = vtkSmartPointer<vtkPolyData>::New();
				MyObj = cfilter->GetOutput();


				MyObj->GetPointData()->RemoveArray("RegionId");
				MyObj->GetPointData()->RemoveArray("TMP");
				MyObj->GetPointData()->AddArray(newScalars);
				MyObj->GetPointData()->SetActiveScalars("TMP");


				int cpt = 0;
				for (vtkIdType j = 0; j<region_sizes->GetNumberOfTuples(); j++)
				{
										
					progress.setValue(j);
					if (progress.wasCanceled())
						break;

					
					
					if (region_sizes->GetTuple((vtkIdType)j)[0] >= (vtkIdType)min_region_size)
					{
						VTK_CREATE(vtkMDActor, newactor);
						if (this->mui_BackfaceCulling == 0)
						{
							newactor->GetProperty()->BackfaceCullingOff();
						}
						else
						{
							newactor->GetProperty()->BackfaceCullingOn();
						}

						VTK_CREATE(vtkPolyDataMapper, newmapper);
						//newmapper->ImmediateModeRenderingOn();
						newmapper->SetColorModeToDefault();

						if (
							(this->mui_ActiveScalars->DataType == VTK_INT || this->mui_ActiveScalars->DataType == VTK_UNSIGNED_INT)
							&& this->mui_ActiveScalars->NumComp == 1
							)
						{
							newmapper->SetScalarRange(0, this->TagTableSize - 1);
							newmapper->SetLookupTable(this->GetTagLut());
						}
						else
						{
							newmapper->SetLookupTable(this->Getmui_ActiveColorMap()->ColorMap);
						}

						newmapper->ScalarVisibilityOn();

						vtkSmartPointer<vtkThreshold> selector =
							vtkSmartPointer<vtkThreshold>::New();
						vtkSmartPointer<vtkMaskFields> scalarsOff =
							vtkSmartPointer<vtkMaskFields>::New();
						vtkSmartPointer<vtkGeometryFilter> geometry =
							vtkSmartPointer<vtkGeometryFilter>::New();

						selector->SetInputData(cfilter->GetOutput());

						selector->SetInputArrayToProcess(0, 0, 0,
							vtkDataObject::FIELD_ASSOCIATION_CELLS,
							vtkDataSetAttributes::SCALARS);
						selector->SetAllScalars(1);
						selector->SetInputArrayToProcess(0, 0, 0, vtkDataObject::FIELD_ASSOCIATION_POINTS, "TMP");
						selector->ThresholdBetween((double)(j), (double)(j));
						selector->Update();

						scalarsOff->SetInputData(selector->GetOutput());
						scalarsOff->CopyAttributeOff(vtkMaskFields::POINT_DATA, vtkDataSetAttributes::SCALARS);
						scalarsOff->CopyAttributeOff(vtkMaskFields::CELL_DATA, vtkDataSetAttributes::SCALARS);
						scalarsOff->Update();
						geometry->SetInputData(scalarsOff->GetOutput());
						geometry->Update();

						vtkSmartPointer<vtkPolyData> MyObj2 = vtkSmartPointer<vtkPolyData>::New();
						MyObj2 = geometry->GetOutput();

						
						cpt++;


						newmapper->SetInputData(MyObj2);

						vtkSmartPointer<vtkMatrix4x4> Mat = myActor->GetMatrix();


						vtkTransform *newTransform = vtkTransform::New();
						newTransform->PostMultiply();

						newTransform->SetMatrix(Mat);

						newactor->SetPosition(newTransform->GetPosition());
						newactor->SetScale(newTransform->GetScale());
						newactor->SetOrientation(newTransform->GetOrientation());
						newTransform->Delete();


						double color[4] = { 0.5, 0.5, 0.5, 1 };
						if (color_mode == 0)
						{
							myActor->GetmColor(color);
						}
						else
						{
							color[0] = rand() / (RAND_MAX + 1.);
							color[1] = rand() / (RAND_MAX + 1.);
							color[2] = rand() / (RAND_MAX + 1.);

						}
						newactor->SetmColor(color);

						newactor->SetMapper(newmapper);
						newactor->SetSelected(0);


						newactor->SetName(myActor->GetName() + "_" + std::to_string(cpt));
						//cout << "try to add new actor=" << endl;
						newcoll->AddTmpItem(newactor);
						modified = 1;
					

					} // if size big enough

				}// for all regions!

				

				

				



				
				progress.setValue(regions);

			}
			
		}
		
	}
	
	if (modified == 1)
	{
		newcoll->InitTraversal();
		vtkIdType num = newcoll->GetNumberOfItems();
		for (vtkIdType i = 0; i < num; i++)
		{
			cout << "try to get next actor from newcoll:" << i << endl;
			vtkMDActor *myActor = vtkMDActor::SafeDownCast(newcoll->GetNextActor());


			this->getActorCollection()->AddItem(myActor);
			std::string action = "Largest region object added: " + myActor->GetName();
			int mCount = BEGIN_UNDO_SET(action);
			this->getActorCollection()->CreateLoadUndoSet(mCount, 1);
			END_UNDO_SET();


		}
		//cout << "camera and grid adjusted" << endl;
		cout << "new actor(s) added" << endl;
		this->Initmui_ExistingScalars();

		cout << "Set actor collection changed" << endl;
		this->getActorCollection()->SetChanged(1);
		cout << "Actor collection changed" << endl;

		this->AdjustCameraAndGrid();
		cout << "Camera and grid adjusted" << endl;

		if (this->Getmui_AdjustLandmarkRenderingSize() == 1)
		{
			this->UpdateLandmarkSettings();
		}
		this->Render();
	}



}
void mqMorphoDigCore::addKeepLargest()
{
	vtkSmartPointer<vtkMDActorCollection> newcoll = vtkSmartPointer<vtkMDActorCollection>::New();
	this->ActorCollection->InitTraversal();
	vtkIdType num = this->ActorCollection->GetNumberOfItems();
	int modified = 0;
	for (vtkIdType i = 0; i < num; i++)
	{
		cout << "Largest region of next actor:" << i << endl;
		vtkMDActor *myActor = vtkMDActor::SafeDownCast(this->ActorCollection->GetNextActor());
		if (myActor->GetSelected() == 1)
		{
			//myActor->SetSelected(0);

			vtkPolyDataMapper *mymapper = vtkPolyDataMapper::SafeDownCast(myActor->GetMapper());
			if (mymapper != NULL && vtkPolyData::SafeDownCast(mymapper->GetInput()) != NULL)
			{


				double numvert = mymapper->GetInput()->GetNumberOfPoints();

				//		@@@

				VTK_CREATE(vtkMDActor, newactor);
				if (this->mui_BackfaceCulling == 0)
				{
					newactor->GetProperty()->BackfaceCullingOff();
				}
				else
				{
					newactor->GetProperty()->BackfaceCullingOn();
				}

				VTK_CREATE(vtkPolyDataMapper, newmapper);
				//newmapper->ImmediateModeRenderingOn();
				newmapper->SetColorModeToDefault();

				if (
					(this->mui_ActiveScalars->DataType == VTK_INT || this->mui_ActiveScalars->DataType == VTK_UNSIGNED_INT)
					&& this->mui_ActiveScalars->NumComp == 1
					)
				{
					newmapper->SetScalarRange(0, this->TagTableSize - 1);
					newmapper->SetLookupTable(this->GetTagLut());
				}
				else
				{
					newmapper->SetLookupTable(this->Getmui_ActiveColorMap()->ColorMap);
				}

				newmapper->ScalarVisibilityOn();
				VTK_CREATE(vtkPolyData, myData);

				vtkSmartPointer<vtkPolyDataConnectivityFilter> cfilter = vtkSmartPointer<vtkPolyDataConnectivityFilter>::New();
				cfilter->SetInputData(mymapper->GetInput());
				cfilter->SetExtractionModeToLargestRegion();
				cfilter->Update();

				

				vtkSmartPointer<vtkPolyDataNormals> ObjNormals = vtkSmartPointer<vtkPolyDataNormals>::New();
				ObjNormals->SetInputData(cfilter->GetOutput());
				ObjNormals->ComputePointNormalsOn();
				ObjNormals->ComputeCellNormalsOn();
				ObjNormals->ConsistencyOff();
				ObjNormals->Update();

				vtkSmartPointer<vtkCleanPolyData> cleanPolyDataFilter = vtkSmartPointer<vtkCleanPolyData>::New();
				cleanPolyDataFilter->SetInputData(ObjNormals->GetOutput());
				cleanPolyDataFilter->PieceInvariantOff();
				cleanPolyDataFilter->ConvertLinesToPointsOff();
				cleanPolyDataFilter->ConvertPolysToLinesOff();
				cleanPolyDataFilter->ConvertStripsToPolysOff();
				cleanPolyDataFilter->PointMergingOn();
				cleanPolyDataFilter->Update();

				myData = cleanPolyDataFilter->GetOutput();



				

				newmapper->SetInputData(myData);

				vtkSmartPointer<vtkMatrix4x4> Mat = myActor->GetMatrix();


				vtkTransform *newTransform = vtkTransform::New();
				newTransform->PostMultiply();

				newTransform->SetMatrix(Mat);
				newactor->SetPosition(newTransform->GetPosition());
				newactor->SetScale(newTransform->GetScale());
				newactor->SetOrientation(newTransform->GetOrientation());
				newTransform->Delete();


				double color[4] = { 0.5, 0.5, 0.5, 1 };
				myActor->GetmColor(color);
				newactor->SetmColor(color);

				newactor->SetMapper(newmapper);
				newactor->SetSelected(0);


				newactor->SetName(myActor->GetName() + "_largest");
				cout << "try to add new actor=" << endl;
				newcoll->AddTmpItem(newactor);
				modified = 1;


			}
		}
	}
	if (modified == 1)
	{
		newcoll->InitTraversal();
		vtkIdType num = newcoll->GetNumberOfItems();
		for (vtkIdType i = 0; i < num; i++)
		{
			cout << "try to get next actor from newcoll:" << i << endl;
			vtkMDActor *myActor = vtkMDActor::SafeDownCast(newcoll->GetNextActor());


			this->getActorCollection()->AddItem(myActor);
			std::string action = "Largest region object added: " + myActor->GetName();
			int mCount = BEGIN_UNDO_SET(action);
			this->getActorCollection()->CreateLoadUndoSet(mCount, 1);
			END_UNDO_SET();


		}
		//cout << "camera and grid adjusted" << endl;
		cout << "new actor(s) added" << endl;
		this->Initmui_ExistingScalars();

		cout << "Set actor collection changed" << endl;
		this->getActorCollection()->SetChanged(1);
		cout << "Actor collection changed" << endl;

		this->AdjustCameraAndGrid();
		cout << "Camera and grid adjusted" << endl;

		if (this->Getmui_AdjustLandmarkRenderingSize() == 1)
		{
			this->UpdateLandmarkSettings();
		}
		this->Render();
	}

}
void mqMorphoDigCore::DeleteSelectedActors()
{
	this->ActorCollection->DeleteSelectedActors();
	this->NormalLandmarkCollection->DeleteSelectedActors();
	this->TargetLandmarkCollection->DeleteSelectedActors();
	this->NodeLandmarkCollection->DeleteSelectedActors();
	this->HandleLandmarkCollection->DeleteSelectedActors();
	this->FlagLandmarkCollection->DeleteSelectedActors();
	
}
void mqMorphoDigCore::addInvert()
{
	vtkSmartPointer<vtkMDActorCollection> newcoll = vtkSmartPointer<vtkMDActorCollection>::New();
	this->ActorCollection->InitTraversal();
	vtkIdType num = this->ActorCollection->GetNumberOfItems();
	int modified = 0;
	for (vtkIdType i = 0; i < num; i++)
	{
		cout << "Invert next actor:" << i << endl;
		vtkMDActor *myActor = vtkMDActor::SafeDownCast(this->ActorCollection->GetNextActor());
		if (myActor->GetSelected() == 1)
		{
			//myActor->SetSelected(0);

			vtkPolyDataMapper *mymapper = vtkPolyDataMapper::SafeDownCast(myActor->GetMapper());
			if (mymapper != NULL && vtkPolyData::SafeDownCast(mymapper->GetInput()) != NULL)
			{


				double numvert = mymapper->GetInput()->GetNumberOfPoints();

				//		@@@

				VTK_CREATE(vtkMDActor, newactor);
				if (this->mui_BackfaceCulling == 0)
				{
					newactor->GetProperty()->BackfaceCullingOff();
				}
				else
				{
					newactor->GetProperty()->BackfaceCullingOn();
				}

				VTK_CREATE(vtkPolyDataMapper, newmapper);
				//newmapper->ImmediateModeRenderingOn();
				newmapper->SetColorModeToDefault();

				if (
					(this->mui_ActiveScalars->DataType == VTK_INT || this->mui_ActiveScalars->DataType == VTK_UNSIGNED_INT)
					&& this->mui_ActiveScalars->NumComp == 1
					)
				{
					newmapper->SetScalarRange(0, this->TagTableSize - 1);
					newmapper->SetLookupTable(this->GetTagLut());
				}
				else
				{
					newmapper->SetLookupTable(this->Getmui_ActiveColorMap()->ColorMap);
				}

				newmapper->ScalarVisibilityOn();
				VTK_CREATE(vtkPolyData, myData);
				vtkSmartPointer<vtkReverseSense> mfilter = vtkSmartPointer<vtkReverseSense>::New();
				//
				mfilter->SetInputData(vtkPolyData::SafeDownCast(mymapper->GetInput()));			
				mfilter->ReverseCellsOn();
				//mfilter->SetReverseNormals(1); => causes MorphoDig to crash... extremely odd!
				mfilter->Update();
				
				myData = mfilter->GetOutput();
				
				// dirty hack because vtkReverseSense crashes when trying to reverse normales....
				vtkSmartPointer<vtkFloatArray> normalsArray =
					vtkSmartPointer<vtkFloatArray>::New();

				normalsArray->SetNumberOfComponents(3); //3d normals (ie x,y,z)
				normalsArray->SetNumberOfTuples(myData->GetNumberOfPoints());
				double *vn;
				float vn2[3];
				vtkFloatArray* norms = vtkFloatArray::SafeDownCast
				(myData->GetPointData()->GetNormals());
				//std::cout << "\n old norms :"<<norms->GetNumberOfTuples()
				//  <<"\n Current object  number of points:"<<this->GetNumberOfPoints();

				for (int i = 0; i<myData->GetNumberOfPoints(); i++) {
					vn = norms->GetTuple((vtkIdType)i);
					vn2[0] = (float)(-vn[0]); vn2[1] = (float)(-vn[1]); vn2[2] = (float)(-vn[2]);
					normalsArray->SetTuple(i, vn2);
					//std::cout << "\n i:"<<i<<"|vn2[0]"<<vn2[0]<<"|vn2[1]"<<vn2[1]<<"|vn2[2]"<<vn2[2];
				}
				normalsArray->SetName("Normals"); // MorphoDig needs a name... 
				myData->GetPointData()->SetNormals(normalsArray);


				std::cout << "\nVtkInvert new Number of points:" << myData->GetNumberOfPoints() << std::endl;
				std::cout << "VtkInvert new Number of cells:" << myData->GetNumberOfCells() << std::endl;

				newmapper->SetInputData(myData);

				vtkSmartPointer<vtkMatrix4x4> Mat = myActor->GetMatrix();
				

				vtkTransform *newTransform = vtkTransform::New();
				newTransform->PostMultiply();

				newTransform->SetMatrix(Mat);
				newactor->SetPosition(newTransform->GetPosition());
				newactor->SetScale(newTransform->GetScale());
				newactor->SetOrientation(newTransform->GetOrientation());
				newTransform->Delete();


				double color[4] = { 0.5, 0.5, 0.5, 1 };
				myActor->GetmColor(color);
				newactor->SetmColor(color);

				newactor->SetMapper(newmapper);
				newactor->SetSelected(0);


				newactor->SetName(myActor->GetName() + "_inv");
				cout << "try to add new actor=" << endl;
				newcoll->AddTmpItem(newactor);
				modified = 1;


			}
		}
	}
	if (modified == 1)
	{
		newcoll->InitTraversal();
		vtkIdType num = newcoll->GetNumberOfItems();
		for (vtkIdType i = 0; i < num; i++)
		{
			cout << "try to get next actor from newcoll:" << i << endl;
			vtkMDActor *myActor = vtkMDActor::SafeDownCast(newcoll->GetNextActor());


			this->getActorCollection()->AddItem(myActor);
			std::string action = "Inverted object added: " + myActor->GetName();
			int mCount = BEGIN_UNDO_SET(action);
			this->getActorCollection()->CreateLoadUndoSet(mCount, 1);
			END_UNDO_SET();


		}
		//cout << "camera and grid adjusted" << endl;
		cout << "new actor(s) added" << endl;
		this->Initmui_ExistingScalars();

		cout << "Set actor collection changed" << endl;
		this->getActorCollection()->SetChanged(1);
		cout << "Actor collection changed" << endl;

		this->AdjustCameraAndGrid();
		cout << "Camera and grid adjusted" << endl;

		if (this->Getmui_AdjustLandmarkRenderingSize() == 1)
		{
			this->UpdateLandmarkSettings();
		}
		this->Render();
	}


}

/*
void mqMorphoDigCore::Invert()
{
For some reason this function works... but the rendering of the modified surface is wrong. Needs to press "W" (wireframe rendering) and then "S" (surface rendering) 
to achieve desired the correct rendering. Do not understand why yet.


	this->ActorCollection->InitTraversal();
	vtkIdType num = this->ActorCollection->GetNumberOfItems();
	int modified = 0;
	for (vtkIdType i = 0; i < num; i++)
	{
		cout << "Mirror XZ try to get next actor:" << i << endl;
		vtkMDActor *myActor = vtkMDActor::SafeDownCast(this->ActorCollection->GetNextActor());
		if (myActor->GetSelected() == 1)
		{
			myActor->SetSelected(0);
			
			vtkPolyDataMapper *mymapper = vtkPolyDataMapper::SafeDownCast(myActor->GetMapper());
			if (mymapper != NULL && vtkPolyData::SafeDownCast(mymapper->GetInput()) != NULL)
			{


				double numvert = mymapper->GetInput()->GetNumberOfPoints();
				
				VTK_CREATE(vtkPolyData, myData);
				myData = mymapper->GetInput();

				//1) Modify normals
				vtkSmartPointer<vtkFloatArray> normalsArray =
					vtkSmartPointer<vtkFloatArray>::New();

				normalsArray->SetNumberOfComponents(3); //3d normals (ie x,y,z)
				normalsArray->SetNumberOfTuples(myData->GetNumberOfPoints());
				double *vn;
				float vn2[3];
				vtkFloatArray* norms = vtkFloatArray::SafeDownCast
				(myData->GetPointData()->GetNormals());
				//std::cout << "\n old norms :"<<norms->GetNumberOfTuples()
				//  <<"\n Current object  number of points:"<<this->GetNumberOfPoints();

				for (int i = 0; i<myData->GetNumberOfPoints(); i++) {
					vn = norms->GetTuple((vtkIdType)i);
					vn2[0] = (float)(-vn[0]); vn2[1] = (float)(-vn[1]); vn2[2] = (float)(-vn[2]);
					normalsArray->SetTuple(i, vn2);
					//std::cout << "\n i:"<<i<<"|vn2[0]"<<vn2[0]<<"|vn2[1]"<<vn2[1]<<"|vn2[2]"<<vn2[2];
				}

				//add the normals to the cells in the polydata
				//std::cout << "\n Set Normals? Reference count:"<<normalsArray->GetReferenceCount();
				//normalsArray->GetReferenceCount();
				myData->GetPointData()->SetNormals(normalsArray);		

				//2) invert triangle order
				vtkIdType ve1, ve2, ve3;
				vtkSmartPointer<vtkIdList> oldpoints = vtkSmartPointer<vtkIdList>::New();
				vtkSmartPointer<vtkIdList> newpoints = vtkSmartPointer<vtkIdList>::New();

				//std::cout << "\n Mesh invert";
				//for every triangle
				for (vtkIdType i = 0; i < myData->GetNumberOfCells(); i++)
				{
					myData->GetCellPoints(i, oldpoints);
					ve1 = oldpoints->GetId(0);
					ve2 = oldpoints->GetId(1);
					ve3 = oldpoints->GetId(2);
					if (i==10)
					{  std::cout << "i:"<<i<<"BEFORE: ve1="<<ve1<<"|ve2="<<ve2<<"|ve3="<<ve3<<endl;

					}

					myData->ReverseCell(i);

					myData->GetCellPoints(i, newpoints);
					ve1 = newpoints->GetId(0);
					ve2 = newpoints->GetId(1);
					ve3 = newpoints->GetId(2);
					if (i==10)
					{
						std::cout << "i:" << i << "AFTER: ve1=" << ve1 << "|ve2=" << ve2 << "|ve3=" << ve3 << endl;

					}
				}
				myData->Modified();
				
				//myData->
				for (vtkIdType i = 0; i < myData->GetNumberOfCells(); i++)
				{
					myData->GetCellPoints(i, newpoints);
					ve1 = newpoints->GetId(0);
					ve2 = newpoints->GetId(1);
					ve3 = newpoints->GetId(2);
					if (i == 10)
					{
						std::cout << "i:" << i << "AFTER: ve1=" << ve1 << "|ve2=" << ve2 << "|ve3=" << ve3 << endl;

					}
				}

				modified = 1;


			}
		}
		
		//myActor->GetProperty()->SetRepresentationToWireframe();
		//myActor->GetProperty()->SetRepresentationToSurface();
	}
	if (modified == 1)
	{
		
		this->Render();
	}
}
*/
int mqMorphoDigCore::SaveSurfaceFile(QString fileName, int write_type, int position_mode, int file_type, int save_norms, vtkMDActor *myActor)
{
	// Write_Type 0 : Binary LE or "Default Binary"
	// Write_Type 1 : Binary BE 
	// Write_Type 2 : ASCII
	

	// Position_mode 0 : orignal position
	// Position_mode 1 : moved position

	// File_type 0 : stl
	// File_type 1 : vtk-vtp
	// File_type 2 : ply
	
	// If myActor is NULL : => what will be saved is an aggregation of all selected surface objects.
	// If myActor is not NULL: => what will be saved is the underlying surface object.


	std::string STLext(".stl");
	std::string STLext2(".STL");
	std::string VTKext(".vtk");
	std::string VTKext2(".VTK");
	std::string VTKext3(".vtp");
	std::string VTKext4(".VTP");
	std::string PLYext(".ply");
	std::string PLYext2(".PLY");


	vtkSmartPointer<vtkAppendPolyData> mergedObjects = vtkSmartPointer<vtkAppendPolyData>::New();
	int Ok = 1;

	
	if (myActor == NULL)
	{
		//cout << "myActor is null" << endl;

		this->ActorCollection->InitTraversal();
		for (vtkIdType i = 0; i < this->ActorCollection->GetNumberOfItems(); i++)
		{
			vtkMDActor *myActor2 = vtkMDActor::SafeDownCast(this->ActorCollection->GetNextActor());
			if (myActor2->GetSelected() == 1)
			{
				if (position_mode == 0)
				{
					vtkPolyDataMapper *mapper = vtkPolyDataMapper::SafeDownCast(myActor2->GetMapper());
					if (mapper != NULL && vtkPolyData::SafeDownCast(mapper->GetInput()) != NULL)
					{
						mergedObjects->AddInputData(vtkPolyData::SafeDownCast(mapper->GetInput()));
					}
				}
				else
				{
					vtkPolyDataMapper *mapper = vtkPolyDataMapper::SafeDownCast(myActor2->GetMapper());
					if (mapper != NULL && vtkPolyData::SafeDownCast(mapper->GetInput()) != NULL)
					{
						vtkSmartPointer<vtkPolyData> toSave = vtkSmartPointer<vtkPolyData>::New();
						toSave->DeepCopy(vtkPolyData::SafeDownCast(mapper->GetInput()));
						double ve_init_pos[3];;
						double ve_final_pos[3];
						vtkSmartPointer<vtkMatrix4x4> Mat = myActor2->GetMatrix();


						for (vtkIdType i = 0; i < toSave->GetNumberOfPoints(); i++) {
							// for every triangle 
							toSave->GetPoint(i, ve_init_pos);
							mqMorphoDigCore::TransformPoint(Mat, ve_init_pos, ve_final_pos);

							toSave->GetPoints()->SetPoint((vtkIdType)i, ve_final_pos);
						}
						mergedObjects->AddInputData(toSave);
					}

				}
			}
		}
	}
	else
	{
		if (position_mode == 0)
		{
			//cout << "position_mode=0..." << endl;
			vtkPolyDataMapper *mapper = vtkPolyDataMapper::SafeDownCast(myActor->GetMapper());
			if (mapper != NULL && vtkPolyData::SafeDownCast(mapper->GetInput()) != NULL)
			{
				mergedObjects->AddInputData(vtkPolyData::SafeDownCast(mapper->GetInput()));
			}
		}
		else
		{
			//cout << "I am where I should be in the save mesh function!" << endl;
			vtkPolyDataMapper *mapper = vtkPolyDataMapper::SafeDownCast(myActor->GetMapper());
			if (mapper != NULL && vtkPolyData::SafeDownCast(mapper->GetInput()) != NULL)
			{
				vtkSmartPointer<vtkPolyData> toSave = vtkSmartPointer<vtkPolyData>::New();
				toSave->DeepCopy(vtkPolyData::SafeDownCast(mapper->GetInput()));
				double ve_init_pos[3];;
				double ve_final_pos[3];
				vtkSmartPointer<vtkMatrix4x4> Mat = myActor->GetMatrix();


				for (vtkIdType i = 0; i < toSave->GetNumberOfPoints(); i++) {
					// for every triangle 
					toSave->GetPoint(i, ve_init_pos);
					mqMorphoDigCore::TransformPoint(Mat, ve_init_pos, ve_final_pos);

					toSave->GetPoints()->SetPoint((vtkIdType)i, ve_final_pos);
				}
				mergedObjects->AddInputData(toSave);
			}

		}
		
	}

	Ok = 1;
	mergedObjects->Update();
	if (save_norms == 0)
	{
		mergedObjects->GetOutput()->GetPointData()->SetNormals(NULL);
		mergedObjects->GetOutput()->GetCellData()->SetNormals(NULL);
	}
	if (file_type == 0)
	{
		vtkSmartPointer<vtkSTLWriter> Writer =
			vtkSmartPointer<vtkSTLWriter>::New();
		if (write_type == 0)
		{
			Writer->SetFileTypeToBinary();
			
		}

		else
		{
			Writer->SetFileTypeToASCII();
		}
		// test if "extension exists!"
		//
		std::size_t found = fileName.toStdString().find(STLext);
		std::size_t found2 = fileName.toStdString().find(STLext2);
		if (found == std::string::npos && found2 == std::string::npos)
		{
			fileName.append(".stl");

		}
		
			Writer->SetFileName(fileName.toStdString().c_str());
			Writer->SetInputData(mergedObjects->GetOutput());
			//  stlWrite->Update();
			Writer->Write();
		
	}

	if (file_type == 1)
	{
		vtkSmartPointer<vtkPolyDataWriter> Writer =
			vtkSmartPointer<vtkPolyDataWriter>::New();
		if (write_type == 0)
		{
			Writer->SetFileTypeToBinary();
			
		}
		else
		{
			Writer->SetFileTypeToASCII();
		}
		std::size_t found = fileName.toStdString().find(VTKext);
		std::size_t found2 = fileName.toStdString().find(VTKext2);
		std::size_t found3 = fileName.toStdString().find(VTKext3);
		std::size_t found4 = fileName.toStdString().find(VTKext4);
		if (found == std::string::npos && found2 == std::string::npos && found3 == std::string::npos && found4 == std::string::npos)
		{
			fileName.append(".vtk");
		}
		
		Writer->SetFileName(fileName.toStdString().c_str());
		Writer->SetInputData(mergedObjects->GetOutput());
		//  stlWrite->Update();
		Writer->Write();
		
	}

	if (file_type == 2)
	{
		vtkSmartPointer<vtkPLYWriter> Writer =
			vtkSmartPointer<vtkPLYWriter>::New();
		if (write_type == 0)
		{
			Writer->SetFileTypeToBinary();
			Writer->SetDataByteOrderToLittleEndian();
			//std::cout << "\nBinary Little endian";
		}
		else if (write_type == 1)
		{
			Writer->SetFileTypeToBinary();
			Writer->SetDataByteOrderToBigEndian();
			// std::cout << "\nBinary Big endian";

		}
		else
		{
			Writer->SetFileTypeToASCII();
			//std::cout << "\nASCII";
		}
		
		vtkPolyData *MyMergedObject = mergedObjects->GetOutput();

		// Test if RGB scalar exists.
		vtkUnsignedCharArray* test = (vtkUnsignedCharArray*)MyMergedObject->GetPointData()->GetScalars("RGB");
		if (test != NULL)
		{
			// std::cout<<"Colors found!"<<std::endl;

			vtkSmartPointer<vtkUnsignedCharArray> colors =
				vtkSmartPointer<vtkUnsignedCharArray>::New();
			colors->SetNumberOfComponents(4);
			colors = (vtkUnsignedCharArray*)MyMergedObject->GetPointData()->GetScalars("RGB");

			vtkSmartPointer<vtkUnsignedCharArray> colorsRGB =
				vtkSmartPointer<vtkUnsignedCharArray>::New();
			colorsRGB->SetNumberOfComponents(3);
			colorsRGB->SetNumberOfTuples(MyMergedObject->GetNumberOfPoints());
			for (int i = 0; i<MyMergedObject->GetNumberOfPoints(); i++)	// for each vertex 
			{			//@@@@@

				int nr, ng, nb;

				nr = colors->GetTuple(i)[0];
				ng = colors->GetTuple(i)[1];
				nb = colors->GetTuple(i)[2];


				colorsRGB->InsertTuple3(i, nr, ng, nb);
			}
			colorsRGB->SetName("Colors");
			MyMergedObject->GetPointData()->AddArray(colorsRGB);
			//colors->SetName("Colors");	  
			//std::cout << "Colors num of tuples :" << colors->GetNumberOfTuples() << std::endl;
			for (int i = 0; i<10; i++)
			{
				//std::cout<<"RGB stuff i:"<<colors->GetTuple(i)[0]<<","<<colors->GetTuple(i)[1]<<","<<colors->GetTuple(i)[2]<<std::endl;
				//std::cout<<"RGB "<<i<<"="<<cur_r<<","<<cur_g<<","<<cur_b<<std::endl;
			}
			Writer->SetArrayName("Colors");
		}


		std::size_t found = fileName.toStdString().find(PLYext);
		std::size_t found2 = fileName.toStdString().find(PLYext2);
		if (found == std::string::npos && found2 == std::string::npos)
		{
			fileName.append(".ply");
		}
		ifstream file(fileName.toStdString().c_str());
		
		
			Writer->SetFileName(fileName.toStdString().c_str());
			//
			Writer->SetInputData(MyMergedObject);
			//Writer->SetInputData((vtkPolyData*)My_Obj);	
			Writer->Write();
		
		//std::cout << "\nWriter should have written : "<<filename.c_str();
	}
	return 1;
}

void mqMorphoDigCore::Setmui_LastUsedDir(QString dir) { this->mui_LastUsedDir = dir; }

QString mqMorphoDigCore::Getmui_LastUsedDir() { return this->mui_LastUsedDir; }
void mqMorphoDigCore::Setmui_X1Label(QString label) { this->mui_X1Label = label;
//cout << "this->mui_X1Label " << this->mui_X1Label.toStdString() << endl;
}
QString mqMorphoDigCore::Getmui_DefaultX1Label() { return this->mui_DefaultX1Label; }
QString mqMorphoDigCore::Getmui_X1Label() { return this->mui_X1Label; }


void mqMorphoDigCore::Setmui_X2Label(QString label) { this->mui_X2Label = label; 
//cout << "this->mui_X2Label " << this->mui_X2Label.toStdString() << endl;
}
QString mqMorphoDigCore::Getmui_DefaultX2Label() { return this->mui_DefaultX2Label; }
QString mqMorphoDigCore::Getmui_X2Label() { return this->mui_X2Label; }

void mqMorphoDigCore::Setmui_Y1Label(QString label) { this->mui_Y1Label = label; 
//cout << "this->mui_Y1Label " << this->mui_Y1Label.toStdString() << endl;
}
QString mqMorphoDigCore::Getmui_DefaultY1Label() { return this->mui_DefaultY1Label; }
QString mqMorphoDigCore::Getmui_Y1Label() { return this->mui_Y1Label; }


void mqMorphoDigCore::Setmui_Y2Label(QString label) { this->mui_Y2Label = label; 

//cout << "this->mui_Y2Label " << this->mui_Y2Label.toStdString() << endl;
}
QString mqMorphoDigCore::Getmui_DefaultY2Label() { return this->mui_DefaultY2Label; }
QString mqMorphoDigCore::Getmui_Y2Label() { return this->mui_Y2Label; }

void mqMorphoDigCore::Setmui_Z1Label(QString label) { this->mui_Z1Label = label; }
QString mqMorphoDigCore::Getmui_DefaultZ1Label() { return this->mui_DefaultZ1Label; }
QString mqMorphoDigCore::Getmui_Z1Label() { return this->mui_Z1Label; }


void mqMorphoDigCore::Setmui_Z2Label(QString label) { this->mui_Z2Label = label; }
QString mqMorphoDigCore::Getmui_DefaultZ2Label() { return this->mui_DefaultZ2Label; }
QString mqMorphoDigCore::Getmui_Z2Label() { return this->mui_Z2Label; }

void mqMorphoDigCore::LandmarksMoveUp()
{
	this->NormalLandmarkCollection->LandmarksMoveUp();
	this->TargetLandmarkCollection->LandmarksMoveUp();
	this->NodeLandmarkCollection->LandmarksMoveUp();
	this->HandleLandmarkCollection->LandmarksMoveUp();
	this->Render();
}
void mqMorphoDigCore::LandmarksMoveDown()
{
	this->NormalLandmarkCollection->LandmarksMoveDown();
	this->TargetLandmarkCollection->LandmarksMoveDown();
	this->NodeLandmarkCollection->LandmarksMoveDown();
	this->HandleLandmarkCollection->LandmarksMoveDown();
	this->Render();
}

void mqMorphoDigCore::SelectLandmarkRange(int start, int end, int lm_type)
{
	//cout << "Select landmark range: " << start << ", " << end << ", " << lm_type << endl;
	vtkSmartPointer<vtkLMActorCollection> myColl = vtkSmartPointer<vtkLMActorCollection>::New();
	if (lm_type==0)
	{ 
		myColl = this->NormalLandmarkCollection;
	}
	else if (lm_type == 1)
	{
		myColl = this->TargetLandmarkCollection;
	}
	else if (lm_type == 2)
	{
		//cout << "Node landmark!" << endl;

		
		myColl = this->NodeLandmarkCollection;
		//cout << "myColl->GetNumberOfItems()=" << myColl->GetNumberOfItems() << endl;
	}
	else //if (lm_type == 3)
	{
		myColl = this->HandleLandmarkCollection;
	}
	int coll_modified = 0;

	myColl->InitTraversal();
	for (vtkIdType i = 0; i < myColl->GetNumberOfItems(); i++)
	{
		vtkLMActor *myActor = vtkLMActor::SafeDownCast(myColl->GetNextActor());
		if (myActor->GetSelected() == 1)
		{
			myActor->SetSelected(0);
			myActor->SetChanged(1);
			coll_modified = 1;
			//cout << "Unselect one landmark:" << myActor->GetLMNumber() << endl;

		}
	}
	myColl->InitTraversal();
	for (vtkIdType i = 0; i < myColl->GetNumberOfItems(); i++)
	{
		vtkLMActor *myActor = vtkLMActor::SafeDownCast(myColl->GetNextActor());
		int mynum = myActor->GetLMNumber();
		//cout << "Actor number: " << mynum << endl;
		if (myActor->GetLMNumber()<=end && myActor->GetLMNumber() >=start)
		{

			myActor->SetSelected(1);
			myActor->SetChanged(1);
			coll_modified = 1;
			//cout << "Select one landmark:" << myActor->GetLMNumber() << endl;
		}
	}
	if (coll_modified == 1) {
		myColl->Modified();
	}
	this->Render();
}


void mqMorphoDigCore::CreateLandmark(double coord[3], double ori[3], int lmk_type, int node_type)
{
	
	//cout << "CreateLandmark:" << lmk_type <<", "<< node_type << endl;
	// lmk_type : 0 for normal landmarks
	// lmk_type : 1 for target landmarks
	// lmk_type : 2 for curve nodes
	// lmk_type : 3 for curve handles
	// lmk_type : 4 for flag landmark


	//node_type: only used if mode ==2, curve node
	//node_type: 1 curve starting point 
	//node_type: 0 normal node
	//node_type: 2 curve milestone
	//node_type: 3 connect to preceding starting point

	VTK_CREATE(vtkLMActor, myLM);
	int num = 0;
	myLM->SetLMType(lmk_type);

	if (lmk_type == NORMAL_LMK)
	{
		num = this->NormalLandmarkCollection->GetNextLandmarkNumber();
	}
	else if (lmk_type == TARGET_LMK)
	{
		num = this->TargetLandmarkCollection->GetNextLandmarkNumber();
	}
	else if (lmk_type == NODE_LMK)
	{
		num = this->NodeLandmarkCollection->GetNextLandmarkNumber();
	}
	else if (lmk_type == HANDLE_LMK)
	{
		num = this->HandleLandmarkCollection->GetNextLandmarkNumber();
	}
	else if (lmk_type== FLAG_LMK)
	{
		num = this->FlagLandmarkCollection->GetNextLandmarkNumber();
	}

	myLM->SetLMOriginAndOrientation(coord, ori);
	//myLM->SetLMOrigin(pos[0], pos[1], pos[2]);
	//myLM->SetLMOrientation(norm[0], norm[1], norm[2]);
	if (lmk_type != FLAG_LMK)
	{
		if (this->Getmui_AdjustLandmarkRenderingSize() == 1)
		{
			myLM->SetLMSize(this->AdjustedLandmarkSize());
		}
		else
		{
			myLM->SetLMSize(this->Getmui_LandmarkRenderingSize());
		}
	}
	else
	{
		myLM->SetLMSize(this->Getmui_FlagRenderingSize());
	}
	/*
		double green[4] = { 0.5, 1, 0, 1 }; // LMType=0
	double yellow[4] = { 1, 1, 0,0.5 }; // LMType = 1 (target LM)
	double darkred[4] = { 0.5, 0, 0, 1 }; // LMType = 2 (curve node: dark red)
	double orange[4] = { 1, 0.5, 0, 1 }; // LMType = 3 (curve handle : orange)
	double red[4] = { 1, 0.4, 0.4, 1 }; // LMType=4 (curve starting point)
	double blue[4] = { 0, 0.5, 1, 1 }; // LMType = 5 (curve milestone)
	double cyan[4] = { 0, 1, 1, 1 }; // LMType = 6 (curve ending point)
	*/
	if (lmk_type == NORMAL_LMK)
	{
		myLM->SetLMType(NORMAL_LMK);
	}
	else if (lmk_type== TARGET_LMK)
	{
		myLM->SetLMType(TARGET_LMK);
	}
	else if (lmk_type == NODE_LMK)
	{
		// to do : 
		myLM->SetLMType(NODE_LMK);
		if (node_type>-1)
		{ 
			//lmtype: 1 curve starting point
			//lmtype: 0 normal node
			//lmtype: 2 curve milestone
			//lmtype: 3 connect to preceding starting point
			if (node_type == NORMAL_NODE) { myLM->SetLMType(NODE_LMK);  myLM->SetLMNodeType(NORMAL_NODE);	}
			if (node_type == STARTING_NODE) { myLM->SetLMType(NODE_LMK);  myLM->SetLMNodeType(STARTING_NODE); }
			if (node_type == MILESTONE_NODE) { myLM->SetLMType(NODE_LMK);  myLM->SetLMNodeType(MILESTONE_NODE); }
			if (node_type == CONNECT_NODE) { myLM->SetLMType(NODE_LMK);  myLM->SetLMNodeType(CONNECT_NODE); }
			


		}
		else
		{			
			if (num > 1)
			{
				vtkLMActor *myPrecedingLM = NULL;
				//@implement GETLMBefore(num)
				//vtkLMActor *myPrecedingLM = this->NodeLandmarkCollection->GetLMBefore(num);
				if (myPrecedingLM != NULL)
				{
					if (myPrecedingLM->GetLMNodeType() == CONNECT_NODE)// if curve ending point
					{
						myLM->SetLMNodeType(STARTING_NODE); // curve starting point
					}
					else
					{
						myLM->SetLMNodeType(NORMAL_NODE); // curve conventional node
					}
				}
				else
				{
					myLM->SetLMNodeType(NORMAL_NODE); // curve conventional node
				}
			}
			else // num ==1
			{
				myLM->SetLMNodeType(STARTING_NODE); //curve starting point
			}
		}
	}
	else if(lmk_type == HANDLE_LMK)
	{
		myLM->SetLMType(HANDLE_LMK); //curve handle
	}
	else 
	{
		//cout << "Set LM TYPE TO FLAG!" << endl;
		myLM->SetLMType(FLAG_LMK);
		std::string flag = "Flag ";
		std::string flag_num = flag + std::to_string(num);
		myLM->SetLMText(flag_num);
	}
	
	myLM->SetLMNumber(num);
	
	myLM->SetLMBodyType(this->Getmui_LandmarkBodyType());
	
	myLM->SetSelected(0);

	vtkSmartPointer<vtkPolyDataMapper> mapper =
		vtkSmartPointer<vtkPolyDataMapper>::New();
	mapper->SetInputData(myLM->getLMBody());
	mapper->Update();
	myLM->SetMapper(mapper);


	//myLM->PrintSelf(cout, vtkIndent(1));
	

	if (lmk_type == NORMAL_LMK)
	{
		
		
		this->NormalLandmarkCollection->AddItem(myLM);
		this->NodeLandmarkCollection->ReorderLandmarks();
		this->NormalLandmarkCollection->SetChanged(1);
		std::string action = "Create Normal landmark";
		int mCount = BEGIN_UNDO_SET(action);
		this->getNormalLandmarkCollection()->CreateLoadUndoSet(mCount, 1);
		END_UNDO_SET();
		
	}
	else if (lmk_type == TARGET_LMK)
	{

		this->TargetLandmarkCollection->AddItem(myLM);
		this->NodeLandmarkCollection->ReorderLandmarks();
		this->TargetLandmarkCollection->SetChanged(1);
		std::string action = "Create Target landmark";
		int mCount = BEGIN_UNDO_SET(action);
		this->getTargetLandmarkCollection()->CreateLoadUndoSet(mCount, 1);
		END_UNDO_SET();
		
	}
	else if (lmk_type == NODE_LMK)
	{
		this->NodeLandmarkCollection->AddItem(myLM);
		this->NodeLandmarkCollection->ReorderLandmarks();
		this->NodeLandmarkCollection->SetChanged(1);
		std::string action = "Create Curve Node";
		int mCount = BEGIN_UNDO_SET(action);
		this->getNodeLandmarkCollection()->CreateLoadUndoSet(mCount, 1);
		END_UNDO_SET();
		
	}
	else if (lmk_type == HANDLE_LMK)
	{
		this->HandleLandmarkCollection->AddItem(myLM);
		this->HandleLandmarkCollection->ReorderLandmarks();
		this->HandleLandmarkCollection->SetChanged(1);
		std::string action = "Create Curve Handle";
		int mCount = BEGIN_UNDO_SET(action);
		this->getHandleLandmarkCollection()->CreateLoadUndoSet(mCount, 1);
		END_UNDO_SET();
	}
	else if (lmk_type == FLAG_LMK)
	{
		this->FlagLandmarkCollection->AddItem(myLM);		
		this->FlagLandmarkCollection->SetChanged(1);
		std::string action = "Create Flag Landmark";
		int mCount = BEGIN_UNDO_SET(action);
		this->getFlagLandmarkCollection()->CreateLoadUndoSet(mCount, 1);
		END_UNDO_SET();
	}
	
	
}
void mqMorphoDigCore::UpdateFirstSelectedLandmark(double coord[3], double ori[3])
{

	int lmk_type = this->Getmui_LandmarkMode();
	vtkSmartPointer<vtkLMActor> MyFirst = NULL;
	vtkSmartPointer<vtkLMActorCollection> myColl = NULL;
	if (lmk_type == NORMAL_LMK)
	{
		myColl = this->NormalLandmarkCollection;				

	}
	else if (lmk_type == TARGET_LMK)
	{
		myColl = this->TargetLandmarkCollection;		
	}
	else if (lmk_type == NODE_LMK)
	{
		myColl = this->NodeLandmarkCollection;
		
	}
	else if (lmk_type == HANDLE_LMK)
	{
		myColl = this->HandleLandmarkCollection;
	}
	else //if (lmk_type == FLAG_LMK)
	{
		myColl = this->FlagLandmarkCollection;
	}

	MyFirst = myColl->GetFirstSelectedActor();

	if (MyFirst == NULL)
	{
		myColl = this->NormalLandmarkCollection;
		MyFirst = myColl->GetFirstSelectedActor();
		if (MyFirst == NULL)
		{
			myColl = this->TargetLandmarkCollection;
			MyFirst = myColl->GetFirstSelectedActor();
			if (MyFirst == NULL)
			{
				myColl = this->NodeLandmarkCollection;
				MyFirst = myColl->GetFirstSelectedActor();
				if (MyFirst == NULL)
				{
					myColl = this->HandleLandmarkCollection;
					MyFirst = myColl->GetFirstSelectedActor();
					if (MyFirst == NULL)
					{
						myColl = this->FlagLandmarkCollection;
						MyFirst = myColl->GetFirstSelectedActor();
						

					}
				}
			}
		}
	}
		
	if (MyFirst != NULL)
	{
		std::string action = "Update xyz and nx ny nz coordinates ";
		int mCount = BEGIN_UNDO_SET(action);
			MyFirst->SaveState(mCount);
			MyFirst->SetLMOrigin(coord);
			MyFirst->SetLMOrientation(ori);
			MyFirst->Modified();
			myColl->Modified();
		END_UNDO_SET();
	}


}
void mqMorphoDigCore::ResetOrientationHelperLabels()
{
	this->SetOrientationHelperLabels(this->mui_X1Label.toStdString(), this->mui_X2Label.toStdString(), this->mui_Y1Label.toStdString(), this->mui_Y2Label.toStdString(), this->mui_Z1Label.toStdString(), this->mui_Z2Label.toStdString());

}
void mqMorphoDigCore::SetOrientationHelperLabels(std::string X1, std::string X2, std::string Y1, std::string Y2, std::string Z1, std::string Z2 )
{
	vtkSmartPointer<vtkOrientationHelperActor> axes = vtkOrientationHelperActor::SafeDownCast(this->OrientationHelperWidget->GetOrientationMarker());
	axes->SetXAxisLabelText(X1.c_str());
	axes->SetX2AxisLabelText(X2.c_str());
	axes->SetYAxisLabelText(Y1.c_str());
	axes->SetY2AxisLabelText(Y2.c_str());
	axes->SetZAxisLabelText(Z1.c_str());
	axes->SetZ2AxisLabelText(Z2.c_str());
}
void mqMorphoDigCore::InitializeOrientationHelper()
{
	vtkSmartPointer<vtkOrientationHelperActor> axes =
		vtkSmartPointer<vtkOrientationHelperActor>::New();

	//

	//vtkOrientationMarkerWidget* widget = vtkOrientationMarkerWidget::New();
	// Does not work with a smart pointer, can't figure out why


	this->OrientationHelperWidget->SetOutlineColor(0.9300, 0.5700, 0.1300);
	this->OrientationHelperWidget->SetOrientationMarker(axes);
	this->OrientationHelperWidget->SetDefaultRenderer(this->getRenderer());
	this->OrientationHelperWidget->SetInteractor(this->RenderWindow->GetInteractor());
	this->OrientationHelperWidget->SetViewport(0.0, 0.0, 0.2, 0.2);
	this->OrientationHelperWidget->SetEnabled(1);
	this->OrientationHelperWidget->InteractiveOff();
	this->OrientationHelperWidget->PickingManagedOn();
	this->ResetOrientationHelperLabels();


}
void mqMorphoDigCore::SetMainWindow(QMainWindow *_mainWindow)
{
	this->MainWindow = _mainWindow;
}
QMainWindow* mqMorphoDigCore::GetMainWindow() {
	return this->MainWindow	;
}

//Called to repplace camera and grid positions when switching from "orange grid mode" to "blue grid mode"
//= when camera focalpoint and grid center are changed between 0,0,0 and COM of all opened meshes.
void mqMorphoDigCore::ReplaceCameraAndGrid()
{
	double newcamerafocalpoint[3] = { 0,0,0 };
	if (this->Getmui_CameraCentreOfMassAtOrigin() == 0)
	{
		this->getActorCollection()->GetCenterOfMass(newcamerafocalpoint);
	}

	double oldcampos[3];
	double newcampos[3];
	this->getCamera()->GetPosition(oldcampos);
	double oldcamerafocalpoint[3];
	this->getCamera()->GetFocalPoint(oldcamerafocalpoint);

	double dispvector[3];
	vtkMath::Subtract(newcamerafocalpoint, oldcamerafocalpoint, dispvector);
	vtkMath::Add(oldcampos, dispvector, newcampos);
	this->getCamera()->SetPosition(newcampos);
	this->getCamera()->SetFocalPoint(newcamerafocalpoint);

	this->getGridActor()->SetGridOrigin(newcamerafocalpoint);
	this->getGridActor()->SetOutlineMode(this->Getmui_CameraCentreOfMassAtOrigin());
	//this->getGridActor()->SetGridType(gridtype);	
	this->Render();


}
double mqMorphoDigCore::GetBoundingBoxLengthOfSelectedActors()
{
	double boundsa[6];
	this->ActorCollection->GetBoundingBoxSelected(boundsa);
	double Normalboundslm[6];
	this->NormalLandmarkCollection->GetBoundingBoxSelected(Normalboundslm);
	double Targetboundslm[6];
	this->TargetLandmarkCollection->GetBoundingBoxSelected(Targetboundslm);
	double Nodeboundslm[6];
	this->NodeLandmarkCollection->GetBoundingBoxSelected(Nodeboundslm);
	double Handleboundslm[6];
	this->HandleLandmarkCollection->GetBoundingBoxSelected(Handleboundslm);
	double Flagboundslm[6];
	this->FlagLandmarkCollection->GetBoundingBoxSelected(Flagboundslm);



	double largestboundsselected[6];
	largestboundsselected[0] = DBL_MAX;
	largestboundsselected[1] = -DBL_MAX;
	largestboundsselected[2] = DBL_MAX;
	largestboundsselected[3] = -DBL_MAX;
	largestboundsselected[4] = DBL_MAX;
	largestboundsselected[5] = -DBL_MAX;

	if (boundsa[0] < largestboundsselected[0]) { largestboundsselected[0] = boundsa[0]; }
	if (boundsa[1] > largestboundsselected[1]) { largestboundsselected[1] = boundsa[1]; }
	if (boundsa[2] < largestboundsselected[2]) { largestboundsselected[2] = boundsa[2]; }
	if (boundsa[3] > largestboundsselected[3]) { largestboundsselected[3] = boundsa[3]; }
	if (boundsa[4] < largestboundsselected[4]) { largestboundsselected[4] = boundsa[4]; }
	if (boundsa[5] > largestboundsselected[5]) { largestboundsselected[5] = boundsa[5]; }

	if (Normalboundslm[0] < largestboundsselected[0]) { largestboundsselected[0] = Normalboundslm[0]; }
	if (Normalboundslm[1] > largestboundsselected[1]) { largestboundsselected[1] = Normalboundslm[1]; }
	if (Normalboundslm[2] < largestboundsselected[2]) { largestboundsselected[2] = Normalboundslm[2]; }
	if (Normalboundslm[3] > largestboundsselected[3]) { largestboundsselected[3] = Normalboundslm[3]; }
	if (Normalboundslm[4] < largestboundsselected[4]) { largestboundsselected[4] = Normalboundslm[4]; }
	if (Normalboundslm[5] > largestboundsselected[5]) { largestboundsselected[5] = Normalboundslm[5]; }

	if (Targetboundslm[0] < largestboundsselected[0]) { largestboundsselected[0] = Targetboundslm[0]; }
	if (Targetboundslm[1] > largestboundsselected[1]) { largestboundsselected[1] = Targetboundslm[1]; }
	if (Targetboundslm[2] < largestboundsselected[2]) { largestboundsselected[2] = Targetboundslm[2]; }
	if (Targetboundslm[3] > largestboundsselected[3]) { largestboundsselected[3] = Targetboundslm[3]; }
	if (Targetboundslm[4] < largestboundsselected[4]) { largestboundsselected[4] = Targetboundslm[4]; }
	if (Targetboundslm[5] > largestboundsselected[5]) { largestboundsselected[5] = Targetboundslm[5]; }

	if (Nodeboundslm[0] < largestboundsselected[0]) { largestboundsselected[0] = Nodeboundslm[0]; }
	if (Nodeboundslm[1] > largestboundsselected[1]) { largestboundsselected[1] = Nodeboundslm[1]; }
	if (Nodeboundslm[2] < largestboundsselected[2]) { largestboundsselected[2] = Nodeboundslm[2]; }
	if (Nodeboundslm[3] > largestboundsselected[3]) { largestboundsselected[3] = Nodeboundslm[3]; }
	if (Nodeboundslm[4] < largestboundsselected[4]) { largestboundsselected[4] = Nodeboundslm[4]; }
	if (Nodeboundslm[5] > largestboundsselected[5]) { largestboundsselected[5] = Nodeboundslm[5]; }

	if (Handleboundslm[0] < largestboundsselected[0]) { largestboundsselected[0] = Handleboundslm[0]; }
	if (Handleboundslm[1] > largestboundsselected[1]) { largestboundsselected[1] = Handleboundslm[1]; }
	if (Handleboundslm[2] < largestboundsselected[2]) { largestboundsselected[2] = Handleboundslm[2]; }
	if (Handleboundslm[3] > largestboundsselected[3]) { largestboundsselected[3] = Handleboundslm[3]; }
	if (Handleboundslm[4] < largestboundsselected[4]) { largestboundsselected[4] = Handleboundslm[4]; }
	if (Handleboundslm[5] > largestboundsselected[5]) { largestboundsselected[5] = Handleboundslm[5]; }

	if (Flagboundslm[0] < largestboundsselected[0]) { largestboundsselected[0] = Flagboundslm[0]; }
	if (Flagboundslm[1] > largestboundsselected[1]) { largestboundsselected[1] = Flagboundslm[1]; }
	if (Flagboundslm[2] < largestboundsselected[2]) { largestboundsselected[2] = Flagboundslm[2]; }
	if (Flagboundslm[3] > largestboundsselected[3]) { largestboundsselected[3] = Flagboundslm[3]; }
	if (Flagboundslm[4] < largestboundsselected[4]) { largestboundsselected[4] = Flagboundslm[4]; }
	if (Flagboundslm[5] > largestboundsselected[5]) { largestboundsselected[5] = Flagboundslm[5]; }

	double A[3];
	double B[3];
	double diag[3];
	A[0] = largestboundsselected[0];
	A[1] = largestboundsselected[2];
	A[2] = largestboundsselected[4];
	B[0] = largestboundsselected[1];
	B[1] = largestboundsselected[3];
	B[2] = largestboundsselected[5];
	diag[0] = B[0] - A[0];
	diag[1] = B[1] - A[1];
	diag[2] = B[2] - A[2];
	double lengthxyz = sqrt((diag[0])*(diag[0]) + (diag[1])*(diag[1]) + (diag[2])*(diag[2]));
	return lengthxyz;

}
void mqMorphoDigCore::GetCenterOfMassOfSelectedActors(double com[3])
{
	com[0] = 0;
	com[1] = 0;
	com[2] = 0;
	//cout << "start a com:" << endl;
	//double com[3] = { 0, 0,0 };	
	//Conception weakness : call COM before VN otherwise computation may not have been triggered!!



	int nv = 0;
	double *coma = this->ActorCollection->GetCenterOfMassOfSelectedActors();
	int nva = this->ActorCollection->GetGlobalSelectedVN();
	if (nva>0) {
		com[0] += coma[0] * nva;
		com[1] += coma[1] * nva;
		com[2] += coma[2] * nva;
	}
	//cout << " com:" << com[0] << "," << com[1] << "," << com[2] << endl;
	//cout << "selected nva:" << nva << endl;
	//cout << "start  lm com:" << endl;
	//Conception weakness : call COM before VN otherwise computation may not have been triggered!!
	nv = nva;
	double *Normalcomlm = this->NormalLandmarkCollection->GetCenterOfMassOfSelectedActors();
	int Normalnvlm = this->NormalLandmarkCollection->GetGlobalSelectedVN();
	if (Normalnvlm > 0) {
		//cout << "nvlm>0" << endl;

		com[0] += Normalcomlm[0] * Normalnvlm;
		com[1] += Normalcomlm[1] * Normalnvlm;
		com[2] += Normalcomlm[2] * Normalnvlm;
	}
	nv += Normalnvlm;

	double *Targetcomlm = this->TargetLandmarkCollection->GetCenterOfMassOfSelectedActors();
	int Targetnvlm = this->TargetLandmarkCollection->GetGlobalSelectedVN();
	if (Targetnvlm > 0) {
		//cout << "nvlm>0" << endl;

		com[0] += Targetcomlm[0] * Targetnvlm;
		com[1] += Targetcomlm[1] * Targetnvlm;
		com[2] += Targetcomlm[2] * Targetnvlm;
	}
	nv += Targetnvlm;

	double *Nodecomlm = this->NodeLandmarkCollection->GetCenterOfMassOfSelectedActors();
	int Nodenvlm = this->NodeLandmarkCollection->GetGlobalSelectedVN();
	if (Nodenvlm > 0) {
		//cout << "nvlm>0" << endl;

		com[0] += Nodecomlm[0] * Nodenvlm;
		com[1] += Nodecomlm[1] * Nodenvlm;
		com[2] += Nodecomlm[2] * Nodenvlm;
	}
	nv += Nodenvlm;

	double *Handlecomlm = this->HandleLandmarkCollection->GetCenterOfMassOfSelectedActors();
	int Handlenvlm = this->HandleLandmarkCollection->GetGlobalSelectedVN();
	if (Handlenvlm > 0) {
		//cout << "nvlm>0" << endl;

		com[0] += Handlecomlm[0] * Handlenvlm;
		com[1] += Handlecomlm[1] * Handlenvlm;
		com[2] += Handlecomlm[2] * Handlenvlm;
	}
	nv += Handlenvlm;

	double *Flagcomlm = this->FlagLandmarkCollection->GetCenterOfMassOfSelectedActors();
	int Flagnvlm = this->FlagLandmarkCollection->GetGlobalSelectedVN();
	if (Flagnvlm > 0) {
		//cout << "nvlm>0" << endl;

		com[0] += Flagcomlm[0] * Flagnvlm;
		com[1] += Flagcomlm[1] * Flagnvlm;
		com[2] += Flagcomlm[2] * Flagnvlm;
	}
	nv += Flagnvlm;

	if (nv > 0) {
		com[0] /= nv; com[1] /= nv; com[2] /= nv;
	}
	//cout << "global selected com:" << com[0] << ","<<com[1] << "," << com[2] << endl;
	//cout << "global selected nv:" << nv << endl;
	//return com;
}
void mqMorphoDigCore::AdjustCameraAndGrid()
{
	double newcamerafocalpoint[3] = { 0,0,0 };
	if (this->Getmui_CameraCentreOfMassAtOrigin() == 0)
	{
		this->getActorCollection()->GetCenterOfMass(newcamerafocalpoint);
		this->getGridActor()->SetGridOrigin(newcamerafocalpoint);


	}

	double multfactor = 1 / tan(this->getCamera()->GetViewAngle() *  vtkMath::Pi() / 360.0);
	double GlobalBoundingBoxLength = this->getActorCollection()->GetBoundingBoxLength();
	if (GlobalBoundingBoxLength == std::numeric_limits<double>::infinity() || GlobalBoundingBoxLength == 0)
	{
		GlobalBoundingBoxLength = 120;
	}

	double oldcampos[3];
	double newcampos[3];
	this->getCamera()->GetPosition(oldcampos);
	double oldcamerafocalpoint[3];
	this->getCamera()->GetFocalPoint(oldcamerafocalpoint);

	double dispvector[3];

	vtkMath::Subtract(oldcampos, oldcamerafocalpoint, dispvector);
	vtkMath::Normalize(dispvector);
	double newdist = multfactor*GlobalBoundingBoxLength;
	vtkMath::MultiplyScalar(dispvector, newdist);

	vtkMath::Add(newcamerafocalpoint, dispvector, newcampos);

	this->getCamera()->SetPosition(newcampos);
	this->getCamera()->SetFocalPoint(newcamerafocalpoint);

	// now adjust if necessary..
	if (this->Getmui_CameraOrtho() == 1)
	{
		this->getCamera()->SetParallelScale(GlobalBoundingBoxLength);
		
	}
	this->getRenderer()->ResetCameraClippingRange();
	this->ActivateClippingPlane();
	//this->ui->qvtkWidget->update();
	this->Render();



}

/*void mqMorphoDigCore::RedrawBezierCurves()
{
	
	//this->BezierCurveSource->Update();
	cout << "Render" << endl;
	this->Render();
}*/
void mqMorphoDigCore::ResetCameraOrthoPerspective()
{
	if (this->Getmui_CameraOrtho() == 1)
	{
		this->getCamera()->SetParallelProjection(true);
		this->DollyCameraForParallelScale();
	}
	else
	{

		this->getCamera()->SetParallelProjection(false);
		this->DollyCameraForPerspectiveMode();


	}
	//cout << "Parallel scale"<<this->MorphoDigCore->getCamera()->GetParallelScale()<<endl;
	double dist = 0;


	double campos[3] = { 0,0,0 };
	double foc[3] = { 0,0,0 };
	this->getCamera()->GetPosition(campos);
	//cout << "Camera Position:" << campos[0] <<","<<campos[1]<<","<<campos[2]<< endl;
	this->getCamera()->GetFocalPoint(foc);
	//cout << "Camera Position:" << foc[0] << "," << foc[1] << "," << foc[2] << endl;
	dist = sqrt(pow((campos[0] - foc[0]), 2) + pow((campos[1] - foc[1]), 2) + pow((campos[2] - foc[2]), 2));
	//cout << "Distance between camera and focal point:" << dist << endl;

	//cout << "Camera viewing angle:" << this->MorphoDigCore->getCamera()->GetViewAngle() << endl;

	this->Render(); // update main window!
}
/*
In perspective mode, "zoom" (dolly) in/out changes the position of the camera
("dolly" functions of vtkInteractorStyleTrackballCamera.cxx and of vtkInteractorStyleJoystickCamera )
Beware : no real "Zoom" function is applied in these styles!!!!
=> before I create  MorphoDig' own interactor styles, camera's parallel scale (=ortho "zoom") should
be updated when switching from "perspective" to "ortho" to keep track of that change...
=> Once these styles are created, this function should be removed!

*/
void mqMorphoDigCore::DollyCameraForParallelScale()
{
	double campos[3] = { 0,0,0 };
	double foc[3] = { 0,0,0 };

	this->getCamera()->GetPosition(campos);
	this->getCamera()->GetFocalPoint(foc);
	double dist = sqrt(vtkMath::Distance2BetweenPoints(campos, foc));
	//double dist = sqrt(pow((campos[0] - foc[0]), 2) + pow((campos[1] - foc[1]), 2) + pow((campos[2] - foc[2]), 2));
	double multfactor = 1 / tan(this->getCamera()->GetViewAngle() *  vtkMath::Pi() / 360.0);

	double newparallelscale = dist / multfactor;
	this->getCamera()->SetParallelScale(newparallelscale);

}

/*
In parallel mode, "zoom" (dolly) in/out does not change the position of the camera
("dolly" functions of vtkInteractorStyleTrackballCamera.cxx and of vtkInteractorStyleJoystickCamera )
Beware : no real "Zoom" function is applied in these styles!!!!
=> before I create  MorphoDig' own interactor styles, camera's position in perspective mode should
be updated when switching from "ortho" to "perspective" to keep track of that change...
=> Once these styles are created, this function should be removed!

*/
void mqMorphoDigCore::DollyCameraForPerspectiveMode()
{
	double campos[3] = { 0,0,0 };
	double foc[3] = { 0,0,0 };
	double dispvector[3];
	this->getCamera()->GetPosition(campos);
	this->getCamera()->GetFocalPoint(foc);
	double multfactor = 3.73; // at 30� vtk : angle = 2*atan((h/2)/d). 
							  // then 2*d  =12/tan(viewangle/2) 
	multfactor = 1 / tan(this->getCamera()->GetViewAngle() *  vtkMath::Pi() / 360.0);
	//cout << "DollyCameraForPerspectiveMode" << endl;
	//cout << "multfactor" << multfactor << endl;
	//cout << "Old posisition:" << campos[0] << "," << campos[1] << "," << campos[2] << endl;

	vtkMath::Subtract(campos, foc, dispvector);
	//cout<<"Disp Vector:" << dispvector[0] << ","<<dispvector[1] << "," << dispvector[2] << endl;
	vtkMath::Normalize(dispvector);
	//cout << "Normalized Disp Vector:" << dispvector[0] << "," << dispvector[1] << "," << dispvector[2] << endl;

	double newdist = multfactor*this->getCamera()->GetParallelScale();
	//cout << "New dist:" << newdist << endl;
	vtkMath::MultiplyScalar(dispvector, newdist);
	//cout << "Multiplied Disp Vector:" << dispvector[0] << "," << dispvector[1] << "," << dispvector[2] << endl;
	double newpos[3] = { 0,0,0 };
	vtkMath::Add(foc, dispvector, newpos);
	//cout << "New pos:" << newpos[0] << "," << newpos[1] << "," << newpos[2] << endl;

	this->getCamera()->SetPosition(newpos);



}

//On ajoute un indice au nom si le nom existe d�j�.
//fonction recurente pour savoir quel indice lui donner.
std::string  mqMorphoDigCore::CheckingName(std::string name_obj) {
	//cout << "check: " << name_obj << endl;
	int cpt_name = 1;
	std::string s_cpt_name = std::to_string(cpt_name);
	std::string name_base = name_obj;
	std::string postfix = "";
	size_t nPos = name_obj.find_first_of("(");
	if (nPos >0 &&nPos<= name_obj.length())
	{
		//cout << "nPos=" << nPos << endl;		
		name_base = name_base.substr(0, nPos);
	}
	//cout << "name_base: " << name_base << endl;
	
	this->ActorCollection->InitTraversal();
	
	for (vtkIdType i = 0; i < this->ActorCollection->GetNumberOfItems(); i++)
	{
		vtkMDActor * myActor = vtkMDActor::SafeDownCast(this->ActorCollection->GetNextActor());
		std::string a_name_base = myActor->GetName();
		//cout << "actor name: " << a_name_base << endl;
		std::string apostfix = "";
		size_t anPos = a_name_base.find_first_of("(");
		//cout << "AnPos:" << anPos << endl;
		if (anPos > 0 && anPos <= a_name_base.length())
		{
			a_name_base = a_name_base.substr(0, anPos);
			
		}
		//cout << "a_name_base: " << a_name_base<<endl;
		if (a_name_base.compare(name_base) ==0) {// si il existe d�j�, on augmente l'indice
			
			cpt_name++;			
			s_cpt_name = std::to_string(cpt_name);
			name_obj = name_base + "(" + s_cpt_name+")";
		}

	}

	
	
	return name_obj;
}

void mqMorphoDigCore::SetGridVisibility()
{
	vtkPropCollection* props = this->getRenderer()->GetViewProps(); //iterate through and set each visibility to 0
	props->InitTraversal();
	std::string str1("vtkGridActor");
	for (int i = 0; i < props->GetNumberOfItems(); i++)
	{
		vtkProp *myprop = props->GetNextProp();
		if (str1.compare(myprop->GetClassName()) == 0)
		{
			if (this->Getmui_ShowGrid() == 1)
			{
				myprop->VisibilityOn();
			}
			else
			{
				myprop->VisibilityOff();
			}
		}

	}
	this->Render();
}

void mqMorphoDigCore::SetGridInfos()
{
	QString myAnnotation;
	//QString myBeginning("Size unit: ");
	//myAnnotation = myBeginning + this->Getmui_SizeUnit();
	//QString follows("\nGrid: 1 square=");
	QString follows("Grid: 1 square=");
	double pan_center[3] = { 0,0,0 };
	this->GetCenterOfMassOfSelectedActors(pan_center);
	double dPanCenter[3] = { 0,0,0 };
	double origin[4] = { 0, 0, 1,1 };
	double away[4] = { 0, 0, 2,1 };
	QString valueAsString = QString::number(this->Getmui_GridSpacing());
	
	//100 px in mm (not yet ready)
	
	//this->GetWorldToDisplay(pan_center[0], pan_center[1], pan_center[2], dPanCenter);
	//this->GetDisplayToWorld(dPanCenter[0], dPanCenter[1], dPanCenter[2], origin);
	//this->GetDisplayToWorld(dPanCenter[0], dPanCenter[1] + 100, dPanCenter[2], away);
	//double p1[3] = { away[0], away[1], away[2] };
	//double p2[3] = { origin[0], origin[1], origin[2] };

	//double dist = sqrt(vtkMath::Distance2BetweenPoints(p1, p2));
	//cout << "away:" << away[0] << "," << away[1] << ","<<away[2] << endl;
	//cout << "origin:" << origin[0] << "," << origin[1] << "," << origin[2] << endl;
	//QString hundredpx = QString::number(dist,'f',2);


	//myAnnotation = myAnnotation + follows;
	myAnnotation = follows + valueAsString;
	//myAnnotation = myAnnotation + this->Getmui_SizeUnit() + ", 100px="+hundredpx+ this->Getmui_SizeUnit();
	myAnnotation = myAnnotation + this->Getmui_SizeUnit();
	cornerAnnotation->SetText(vtkCornerAnnotation::LowerRight, myAnnotation.toStdString().c_str());
	//QString myTest("Loulou fait du ski\nEt voila\nToutou");
	if (this->Getmui_ShowGrid() == 1)
	{
		cornerAnnotation->VisibilityOn();
	}
	else
	{
		cornerAnnotation->VisibilityOff();
	}
	cornerAnnotation->SetLinearFontScaleFactor(2);  
	cornerAnnotation->SetNonlinearFontScaleFactor(1);  
	cornerAnnotation->SetMaximumFontSize(12);
	//this->Render();

	//cornerAnnotation->SetText(vtkCornerAnnotation::RightEdge, valueAsString.toStdString().c_str());
	//cornerAnnotation->SetText(vtkCornerAnnotation::RightEdge, myTest.toStdString().c_str());

	//this->LandmarkCollection->SetChanged(1);
	
	/*vtkPropCollection* props = this->getRenderer()->GetViewProps(); //iterate through and set each visibility to 0
	props->InitTraversal();
	std::string str1("vtkGridActor");
	for (int i = 0; i < props->GetNumberOfItems(); i++)
	{
		vtkProp *myprop = props->GetNextProp();
		if (str1.compare(myprop->GetClassName()) == 0)
		{
			if (this->Getmui_ShowGrid() == 1)
			{
				myprop->VisibilityOn();
			}
			else
			{
				myprop->VisibilityOff();
			}
		}

	}
	this->Render();*/

}

void mqMorphoDigCore::SetOrientationHelperVisibility()
{

	//std::string str1("vtkOrientationHelperActor");
	if (this->Getmui_ShowOrientationHelper() == 1)
	{
		this->OrientationHelperWidget->GetOrientationMarker()->VisibilityOn();
	}
	else
	{
		this->OrientationHelperWidget->GetOrientationMarker()->VisibilityOff();
	}
	this->Render();
}

vtkMDActor * mqMorphoDigCore::GetFirstSelectedActor()
{
	this->ActorCollection->InitTraversal();
	for (vtkIdType i = 0; i < this->ActorCollection->GetNumberOfItems(); i++)
	{
		vtkMDActor *myActor = vtkMDActor::SafeDownCast(this->ActorCollection->GetNextActor());
		if (myActor->GetSelected() == 1)
		{
			return myActor;
		}
	}

	return NULL;
}

vtkMDActor* mqMorphoDigCore::GetLastActor()
{
	return vtkMDActor::SafeDownCast(this->getActorCollection()->GetLastActor());
}

vtkLMActor* mqMorphoDigCore::GetLastLandmark(int mode)
{
	if (mode == 0) {return vtkLMActor::SafeDownCast(this->getNormalLandmarkCollection()->GetLastActor());
	}
	else if (mode == 1) {
		return vtkLMActor::SafeDownCast(this->getTargetLandmarkCollection()->GetLastActor());
	}
	else if (mode == 2) {
		return vtkLMActor::SafeDownCast(this->getNodeLandmarkCollection()->GetLastActor());
	}
	else if (mode == 3) {
		return vtkLMActor::SafeDownCast(this->getHandleLandmarkCollection()->GetLastActor());
	}
	//else if (mode == 4) {
	else  {
		return vtkLMActor::SafeDownCast(this->getFlagLandmarkCollection()->GetLastActor());
	}
	
}

void mqMorphoDigCore::ApplyMatrix(vtkSmartPointer<vtkMatrix4x4> Mat, int mode)
{
	// mode : 0 for last inserted mesh
	// mode : 1 for all selected meshes
	// mode : 2 for all selected landmarks/flags
	
	if (mode == 0)
	{
		vtkMDActor *actor = this->GetLastActor();
		actor->ApplyMatrix(Mat);
	}
	else
	{
		if (mode == 1 )
		{
			this->ActorCollection->InitTraversal();
			for (vtkIdType i = 0; i < this->ActorCollection->GetNumberOfItems(); i++)
			{
				vtkMDActor *myActor = vtkMDActor::SafeDownCast(this->ActorCollection->GetNextActor());
				if (myActor->GetSelected() == 1)
				{
					myActor->ApplyMatrix(Mat);
					myActor->SetSelected(0);
				}
			}

		}
		else // mode ==2
		{

			this->NormalLandmarkCollection->InitTraversal();
			for (vtkIdType i = 0; i < this->NormalLandmarkCollection->GetNumberOfItems(); i++)
			{
				vtkLMActor *myActor = vtkLMActor::SafeDownCast(this->NormalLandmarkCollection->GetNextActor());
				if (myActor->GetSelected() == 1 )
				{
					myActor->ApplyMatrix(Mat);;
					myActor->SetSelected(0);
				}
			}
			this->TargetLandmarkCollection->InitTraversal();
			for (vtkIdType i = 0; i < this->TargetLandmarkCollection->GetNumberOfItems(); i++)
			{
				vtkLMActor *myActor = vtkLMActor::SafeDownCast(this->TargetLandmarkCollection->GetNextActor());
				if (myActor->GetSelected() == 1)
				{
					myActor->ApplyMatrix(Mat);;
					myActor->SetSelected(0);
				}
			}
			this->NodeLandmarkCollection->InitTraversal();
			for (vtkIdType i = 0; i < this->NodeLandmarkCollection->GetNumberOfItems(); i++)
			{
				vtkLMActor *myActor = vtkLMActor::SafeDownCast(this->NodeLandmarkCollection->GetNextActor());
				if (myActor->GetSelected() == 1)
				{
					myActor->ApplyMatrix(Mat);;
					myActor->SetSelected(0);
				}
			}
			this->HandleLandmarkCollection->InitTraversal();
			for (vtkIdType i = 0; i < this->HandleLandmarkCollection->GetNumberOfItems(); i++)
			{
				vtkLMActor *myActor = vtkLMActor::SafeDownCast(this->HandleLandmarkCollection->GetNextActor());
				if (myActor->GetSelected() == 1)
				{
					myActor->ApplyMatrix(Mat);;
					myActor->SetSelected(0);
				}
			}
			this->FlagLandmarkCollection->InitTraversal();
			for (vtkIdType i = 0; i < this->FlagLandmarkCollection->GetNumberOfItems(); i++)
			{
				vtkLMActor *myActor = vtkLMActor::SafeDownCast(this->FlagLandmarkCollection->GetNextActor());
				if (myActor->GetSelected() == 1)
				{
					myActor->ApplyMatrix(Mat);;
					myActor->SetSelected(0);
				}
			}
		}
	}
	cout <<" Actor collection changed" << endl;
	this->ActorCollection->SetChanged(1);
}

void mqMorphoDigCore::SelectAll(int Count)
{}
void mqMorphoDigCore::UnselectAll(int Count)
{
	int node_handle_collections_changed = 0;
	
	this->ActorCollection->InitTraversal();
	for (vtkIdType i = 0; i < this->ActorCollection->GetNumberOfItems(); i++)
	{
		vtkMDActor *myActor = vtkMDActor::SafeDownCast(this->ActorCollection->GetNextActor());
		if (myActor->GetSelected() == 1 &&Count>0)
		{
			myActor->SaveState(Count);
		}
	}
	this->NormalLandmarkCollection->InitTraversal();
	for (vtkIdType i = 0; i < this->NormalLandmarkCollection->GetNumberOfItems(); i++)
	{
		vtkLMActor *myActor = vtkLMActor::SafeDownCast(this->NormalLandmarkCollection->GetNextActor());
		if (myActor->GetSelected() == 1&&Count>0)
		{
			myActor->SaveState(Count);
		}
	}
	this->TargetLandmarkCollection->InitTraversal();
	for (vtkIdType i = 0; i < this->TargetLandmarkCollection->GetNumberOfItems(); i++)
	{
		vtkLMActor *myActor = vtkLMActor::SafeDownCast(this->TargetLandmarkCollection->GetNextActor());
		if (myActor->GetSelected() == 1 && Count>0)
		{
			myActor->SaveState(Count);

		}
	}
	this->NodeLandmarkCollection->InitTraversal();
	for (vtkIdType i = 0; i < this->NodeLandmarkCollection->GetNumberOfItems(); i++)
	{
		vtkLMActor *myActor = vtkLMActor::SafeDownCast(this->NodeLandmarkCollection->GetNextActor());
		if (myActor->GetSelected() == 1 && Count>0)
		{
			myActor->SaveState(Count);
			node_handle_collections_changed = 1;
		}
	}
	this->HandleLandmarkCollection->InitTraversal();
	for (vtkIdType i = 0; i < this->HandleLandmarkCollection->GetNumberOfItems(); i++)
	{
		vtkLMActor *myActor = vtkLMActor::SafeDownCast(this->HandleLandmarkCollection->GetNextActor());
		if (myActor->GetSelected() == 1 && Count>0)
		{
			myActor->SaveState(Count);
			node_handle_collections_changed = 1;
		}
	}
	this->FlagLandmarkCollection->InitTraversal();
	for (vtkIdType i = 0; i < this->FlagLandmarkCollection->GetNumberOfItems(); i++)
	{
		vtkLMActor *myActor = vtkLMActor::SafeDownCast(this->FlagLandmarkCollection->GetNextActor());
		if (myActor->GetSelected() == 1 && Count>0)
		{
			myActor->SaveState(Count);
		}
	}

	this->ActorCollection->InitTraversal();
	for (vtkIdType i = 0; i < this->ActorCollection->GetNumberOfItems(); i++)
	{
		vtkMDActor *myActor = vtkMDActor::SafeDownCast(this->ActorCollection->GetNextActor());
		if (myActor->GetSelected() == 1)
		{
			myActor->SetSelected(0);
			myActor->SetChanged(1);

		}


	}
	this->NormalLandmarkCollection->InitTraversal();
	for (vtkIdType i = 0; i < this->NormalLandmarkCollection->GetNumberOfItems(); i++)
	{
		vtkLMActor *myActor = vtkLMActor::SafeDownCast(this->NormalLandmarkCollection->GetNextActor());
		if (myActor->GetSelected() == 1)
		{
			myActor->SetSelected(0);
			myActor->SetChanged(1);

		}


	}
	this->TargetLandmarkCollection->InitTraversal();
	for (vtkIdType i = 0; i < this->TargetLandmarkCollection->GetNumberOfItems(); i++)
	{
		vtkLMActor *myActor = vtkLMActor::SafeDownCast(this->TargetLandmarkCollection->GetNextActor());
		if (myActor->GetSelected() == 1)
		{
			myActor->SetSelected(0);
			myActor->SetChanged(1);

		}


	}
	this->NodeLandmarkCollection->InitTraversal();
	for (vtkIdType i = 0; i < this->NodeLandmarkCollection->GetNumberOfItems(); i++)
	{
		vtkLMActor *myActor = vtkLMActor::SafeDownCast(this->NodeLandmarkCollection->GetNextActor());
		if (myActor->GetSelected() == 1)
		{
			myActor->SetSelected(0);
			myActor->SetChanged(1);

		}


	}
	this->HandleLandmarkCollection->InitTraversal();
	for (vtkIdType i = 0; i < this->HandleLandmarkCollection->GetNumberOfItems(); i++)
	{
		vtkLMActor *myActor = vtkLMActor::SafeDownCast(this->HandleLandmarkCollection->GetNextActor());
		if (myActor->GetSelected() == 1)
		{
			myActor->SetSelected(0);
			myActor->SetChanged(1);

		}


	}
	this->FlagLandmarkCollection->InitTraversal();
	for (vtkIdType i = 0; i < this->FlagLandmarkCollection->GetNumberOfItems(); i++)
	{
		vtkLMActor *myActor = vtkLMActor::SafeDownCast(this->FlagLandmarkCollection->GetNextActor());
		if (myActor->GetSelected() == 1)
		{
			myActor->SetSelected(0);
			myActor->SetChanged(1);

		}


	}
	if (node_handle_collections_changed == 1)
	{
		this->NodeLandmarkCollection->Modified();
		this->HandleLandmarkCollection->Modified();
	}
}

void mqMorphoDigCore::Render()
{
	
	this->RenderWindow->Render();
}

void mqMorphoDigCore::Setmui_ShowOrientationHelper(int orientationHelper) { this->mui_ShowOrientationHelper = orientationHelper; }
int mqMorphoDigCore::Getmui_DefaultShowOrientationHelper() { return this->mui_DefaultShowOrientationHelper; }
int mqMorphoDigCore::Getmui_ShowOrientationHelper() { return this->mui_ShowOrientationHelper; }

void mqMorphoDigCore::Setmui_CameraCentreOfMassAtOrigin(int comao) { this->mui_CameraCentreOfMassAtOrigin = comao; }
int mqMorphoDigCore::Getmui_DefaultCameraCentreOfMassAtOrigin() { return this->mui_DefaultCameraCentreOfMassAtOrigin; }
int mqMorphoDigCore::Getmui_CameraCentreOfMassAtOrigin() { return this->mui_CameraCentreOfMassAtOrigin; }


void mqMorphoDigCore::Setmui_CameraOrtho(int ortho) { this->mui_CameraOrtho = ortho; }
int mqMorphoDigCore::Getmui_DefaultCameraOrtho() { return this->mui_DefaultCameraOrtho; }
int mqMorphoDigCore::Getmui_CameraOrtho() { return this->mui_CameraOrtho; }

void mqMorphoDigCore::Setmui_ShowGrid(int showgrid) { this->mui_ShowGrid = showgrid; }
int mqMorphoDigCore::Getmui_ShowGrid() { return this->mui_ShowGrid; }
int mqMorphoDigCore::Getmui_DefaultShowGrid() { return this->mui_DefaultShowGrid; };

void mqMorphoDigCore::Setmui_GridSpacing(double gridspacing) { this->mui_GridSpacing = gridspacing; this->GridActor->SetGridSpacing(gridspacing);

}
double mqMorphoDigCore::Getmui_GridSpacing() { return this->mui_GridSpacing; }
double mqMorphoDigCore::Getmui_DefaultGridSpacing() { return this->mui_DefaultGridSpacing; }

void mqMorphoDigCore::Setmui_SizeUnit(QString unit) { this->mui_SizeUnit = unit; }
QString mqMorphoDigCore::Getmui_SizeUnit() { return this->mui_SizeUnit; }
QString mqMorphoDigCore::Getmui_DefaultSizeUnit() { return this->mui_DefaultSizeUnit; }

void mqMorphoDigCore::Setmui_MoveAll(int moveall) { this->mui_MoveAll= moveall;
if (moveall == 0) { this->UnselectAll(-1);  }
}
int mqMorphoDigCore::Getmui_MoveAll() { return this->mui_MoveAll; }
int mqMorphoDigCore::Getmui_DefaultMoveAll() { return this->mui_DefaultMoveAll; };

void mqMorphoDigCore::Setmui_ScalarVisibility(int scalarvisibility)
{
	if (this->mui_ScalarVisibility != scalarvisibility)
	{
		//cout << "Scalar visibility has changed" << endl;
		//1 refresh scalar bar actor !
		int sba_refresh_needed = 0;
		if ((this->mui_ActiveScalars->DataType==VTK_FLOAT|| this->mui_ActiveScalars->DataType == VTK_DOUBLE)&& this->mui_ActiveScalars->NumComp == 1)
		{
			//cout << "SBA refresh needed" << endl;
			sba_refresh_needed = 1;
		}
		if (scalarvisibility == 1)
		{
			// turn on if needed
			if (sba_refresh_needed == 1)
			{
				this->ScalarBarActor->SetLookupTable(this->Getmui_ActiveColorMap()->ColorMap);
				this->ScalarBarActor->SetTitle(this->Getmui_ActiveScalars()->Name.toStdString().c_str());
				this->Renderer->AddActor(ScalarBarActor);
				//cout << "Add SBA" << endl;
			}

		}
		else
		{
			//turn off if needed
			if (sba_refresh_needed == 1)
			{
			//	cout << "Remove SBA" << endl;
				this->Renderer->RemoveActor(ScalarBarActor);
			}
		}
	

		// 2 refresh all actors!
		this->mui_ScalarVisibility = scalarvisibility;
		this->ActorCollection->InitTraversal();

		for (vtkIdType i = 0; i < this->ActorCollection->GetNumberOfItems(); i++)
		{
			vtkMDActor * myActor = vtkMDActor::SafeDownCast(this->ActorCollection->GetNextActor());
		
			if (myActor->GetMapper() != NULL)
			{
				if (scalarvisibility == 1 && myActor->GetSelected()==0)
				{
				
					QString none = QString("none");
					if (this->mui_ActiveScalars->Name != none)
					{
						vtkPolyData *myPD = vtkPolyData::SafeDownCast(myActor->GetMapper()->GetInput());
						if (myPD->GetPointData()->GetScalars(this->mui_ActiveScalars->Name.toStdString().c_str()) != NULL)
						{

							vtkPolyDataMapper::SafeDownCast(myActor->GetMapper())->ScalarVisibilityOn();
						}
						
					}
					
				}
				else
				{
					vtkPolyDataMapper::SafeDownCast(myActor->GetMapper())->ScalarVisibilityOff();
				}
			}
		}
		

	}
}
int mqMorphoDigCore::Getmui_DefaultScalarVisibility() { return this->mui_DefaultScalarVisibility; }

int mqMorphoDigCore::Getmui_ScalarVisibility() { return this->mui_ScalarVisibility; }

ExistingScalars * mqMorphoDigCore::Getmui_ExistingScalars()
{
	return this->mui_ExistingScalars;
}

ExistingColorMaps * mqMorphoDigCore::Getmui_ExistingColorMaps()
{
	return this->mui_ExistingColorMaps;

}

void mqMorphoDigCore::Addmui_ExistingScalars(QString Scalar, int dataType, int numComp)
{
	int exists = 0;
	QString none = QString("none");
	/*if (this->mui_ExistingScalars.size() == 1 && this->mui_ExistingScalars.at(0) == none)
	{
		cout << "1 scalar, and this is none! Confirmation:" << this->mui_ExistingScalars.at(0).toStdString() << endl;
		this->mui_ExistingScalars.clear();
		this->mui_ExistingScalars.push_back(Scalar);
		
	}
	else
	{*/
		
		//check first if Scalar already exists!
		for (int i = 0; i < this->mui_ExistingScalars->Stack.size(); i++)
		{
			QString myScalar = this->mui_ExistingScalars->Stack.at(i).Name;
			if (myScalar == Scalar)
			{
				exists = 1;
				cout << Scalar.toStdString() << " already exists" << endl;
			}

		}
		if (exists == 0)
		{
			this->mui_ExistingScalars->Stack.push_back(ExistingScalars::Element(Scalar, dataType, numComp));
			
		}
	/*}*/
}
void mqMorphoDigCore::Initmui_ExistingScalars()
{
	// browse through all actors and check for existing scalar
	// add
	cout << "Init mui existing scalars" << endl;
	QStringList existing;
	this->ActorCollection->InitTraversal();
	this->mui_ExistingScalars->Stack.clear();
	QString none = QString("none");
	this->mui_ExistingScalars->Stack.push_back(ExistingScalars::Element(none,-1,0));
	for (vtkIdType i = 0; i < this->ActorCollection->GetNumberOfItems(); i++)
	{
		vtkMDActor * myActor = vtkMDActor::SafeDownCast(this->ActorCollection->GetNextActor());
		vtkPolyDataMapper *mapper = vtkPolyDataMapper::SafeDownCast(myActor->GetMapper());
		
		if (mapper != NULL && vtkPolyData::SafeDownCast(mapper->GetInput()) != NULL)
		{
			vtkPolyData *myPD = vtkPolyData::SafeDownCast(mapper->GetInput());
			int nbarrays = myPD->GetPointData()->GetNumberOfArrays();
			for (int i = 0; i < nbarrays; i++)
			{
				//cout << "Array " << i << "=" << myPD->GetPointData()->GetArrayName(i) << endl;
				int num_comp = myPD->GetPointData()->GetArray(i)->GetNumberOfComponents();
				//cout << "Array" << i << " has "<<myPD->GetPointData()->GetArray(i)->GetNumberOfComponents()<< " components"<<endl;
				//std::cout << VTK_UNSIGNED_CHAR << " unsigned char" << std::endl;
				//std::cout << VTK_UNSIGNED_INT << " unsigned int" << std::endl;
				//std::cout << VTK_FLOAT << " float" << std::endl;
				//std::cout << VTK_DOUBLE << " double" << std::endl;
				int dataType = myPD->GetPointData()->GetArray(i)->GetDataType();
				if (dataType == VTK_UNSIGNED_CHAR) { 
					//cout << "Array" << i << " contains UNSIGNED CHARs" << endl; 
				}
				if (dataType == VTK_UNSIGNED_INT) { 
					//cout << "Array" << i << " contains UNSIGNED INTs" << endl; 
				}
				if (dataType == VTK_INT) { 
					//cout << "Array" << i << " contains INTs" << endl; 
				}
				if (dataType == VTK_FLOAT) { 
					//cout << "Array" << i << " contains FLOATs" << endl; 
				}
				if (dataType == VTK_DOUBLE) { 
				//	cout << "Array" << i << " contains DOUBLEs" << endl; 
				}

				if (dataType == VTK_UNSIGNED_CHAR && (num_comp == 3 || num_comp == 4))
				{
					// ok to add RGB like scalars
					QString RGBlike = QString(myPD->GetPointData()->GetArrayName(i));
					this->Addmui_ExistingScalars(RGBlike, dataType, num_comp);
				}
				if ((dataType == VTK_UNSIGNED_INT || dataType == VTK_INT)  && (num_comp == 1))
				{
					// ok to add TAG like scalars
					QString Taglike = QString(myPD->GetPointData()->GetArrayName(i));
					this->Addmui_ExistingScalars(Taglike, dataType, num_comp);
				}
				if ((dataType == VTK_FLOAT || (dataType == VTK_DOUBLE)) && (num_comp == 1))
				{
					// ok to add conventional scalars (like curvature, thickness, height etc... )
					QString ConvScalar = QString(myPD->GetPointData()->GetArrayName(i));
					this->Addmui_ExistingScalars(ConvScalar, dataType, num_comp);
				}
				
			}
			/*if ((vtkUnsignedCharArray*)myPD->GetPointData()->GetScalars("RGB") != NULL)
			{
				QString RGB = QString("RGB");
				this->Addmui_ExistingScalars(RGB);
			}
			if ((vtkUnsignedCharArray*)myPD->GetPointData()->GetScalars("Init_RGB") != NULL)
			{
				QString RGB = QString("Init_RGB");
				this->Addmui_ExistingScalars(RGB);
			}
			if ((vtkIntArray*)myPD->GetPointData()->GetScalars("Tags") != NULL)
			{
				
				
				QString Tags = QString("Tags");
				this->Addmui_ExistingScalars(Tags);
			}*/

		}

		
	}
	int exists = 0;
	

	for (int i = 0; i < this->mui_ExistingScalars->Stack.size(); i++)
	{
		QString myScalar = this->mui_ExistingScalars->Stack.at(i).Name;
		if (myScalar == this->mui_ActiveScalars->Name)
		{
			exists = 1;
			
		}

	}

	// if RGB exists, and this->mui_ActiveScalars ==none, set active scalar to RGB
	QString RGB = QString("RGB");
	for (int i = 0; i < this->mui_ExistingScalars->Stack.size(); i++)
	{
		QString myScalar = this->mui_ExistingScalars->Stack.at(i).Name;
		if (this->mui_ActiveScalars->Name == none && myScalar ==RGB)
		{
			exists = 1;
			this->Setmui_ActiveScalarsAndRender(this->mui_ExistingScalars->Stack.at(i).Name, this->mui_ExistingScalars->Stack.at(i).DataType, this->mui_ExistingScalars->Stack.at(i).NumComp);

		}

	}
	
	// last case : no RGB and none is not set as the active scalar before.
	if (exists == 0)
	{
		this->Setmui_ActiveScalarsAndRender(none, -1, 0);
	}

	/*
	
	(((vtkUnsignedCharArray*)myPD->GetPointData()->GetScalars("RGB") == NULL) &&
							  ((vtkIntArray*)myPD->GetPointData()->GetScalars("Tags") == NULL))
								)
	*/
	this->signal_existingScalarsChanged();
	
}

ActiveColorMap * mqMorphoDigCore::Getmui_ActiveColorMap()
{
	return this->mui_ActiveColorMap;
}

ActiveScalars* mqMorphoDigCore::Getmui_ActiveScalars()
{
	return this->mui_ActiveScalars;
}

void mqMorphoDigCore::RefreshColorMapsAndScalarVisibility()
{

	//1 refresh scalar bar actor if needed.
	int sba_refresh_needed = 0;
	if ((this->mui_ActiveScalars->DataType == VTK_FLOAT || this->mui_ActiveScalars->DataType == VTK_DOUBLE) && this->mui_ActiveScalars->NumComp == 1)
	{
		sba_refresh_needed = 1;
	}
	
	if (sba_refresh_needed == 1)
	{
			this->ScalarBarActor->SetLookupTable(this->Getmui_ActiveColorMap()->ColorMap);
			this->ScalarBarActor->SetTitle(this->Getmui_ActiveScalars()->Name.toStdString().c_str());
			
			
	}

	

	//2 refresh all actors
	this->ActorCollection->InitTraversal();
	for (vtkIdType i = 0; i < this->ActorCollection->GetNumberOfItems(); i++)
	{
		cout << "Something here or..." << endl;
		vtkMDActor * myActor = vtkMDActor::SafeDownCast(this->ActorCollection->GetNextActor());
		vtkPolyDataMapper *mapper = vtkPolyDataMapper::SafeDownCast(myActor->GetMapper());

		if (mapper != NULL && vtkPolyData::SafeDownCast(mapper->GetInput()) != NULL)
		{
			vtkPolyData *myPD = vtkPolyData::SafeDownCast(mapper->GetInput());
			//vtkPolyDataMapper::SafeDownCast(myActor->GetMapper())->ScalarVisibilityOff();
			QString none = QString("none");

			if (
				(this->mui_ActiveScalars->DataType == VTK_INT || this->mui_ActiveScalars->DataType == VTK_UNSIGNED_INT)
				&& this->mui_ActiveScalars->NumComp == 1
				)
			{
				cout << "Set Tag Lut!!!" << endl;
				mapper->SetScalarRange(0, this->TagTableSize - 1);
				mapper->SetLookupTable(this->GetTagLut());

			}
			else
			{
				//mapper->SetScalarRange(0, 200);
				mapper->SetLookupTable(this->Getmui_ActiveColorMap()->ColorMap);
			
				
				//mapper->SetLookupTable(this->GetScalarRainbowLut());
			}

			if (this->mui_ActiveScalars->Name == none)
			{
				vtkPolyDataMapper::SafeDownCast(myActor->GetMapper())->ScalarVisibilityOff();
				cout << "Scalar visibility off from Set mui active scalars" << endl;
			}
			else
			{


				if (myPD->GetPointData()->GetScalars(this->mui_ActiveScalars->Name.toStdString().c_str()) != NULL)
				{
					if (this->mui_ScalarVisibility == 1 && myActor->GetSelected() == 0)
					{

						vtkPolyDataMapper::SafeDownCast(myActor->GetMapper())->ScalarVisibilityOn();
					}
					myPD->GetPointData()->SetActiveScalars(this->mui_ActiveScalars->Name.toStdString().c_str());
				}
				else
				{
					vtkPolyDataMapper::SafeDownCast(myActor->GetMapper())->ScalarVisibilityOff();
				}
			}


		}
		

	}
}

void mqMorphoDigCore::Setmui_ActiveColorMap(QString name, vtkSmartPointer<vtkDiscretizableColorTransferFunction> colorMap)
{
	this->mui_ActiveColorMap->Name = name;
	this->mui_ActiveColorMap->ColorMap = colorMap;

	this->RefreshColorMapsAndScalarVisibility();
	cout << "Now active color map is " << name.toStdString() << endl;
}
void mqMorphoDigCore::Setmui_ActiveColorMapAndRender(QString name, vtkSmartPointer<vtkDiscretizableColorTransferFunction> colorMap)
{
	this->Setmui_ActiveColorMap( name, colorMap);
	this->Render();
}

void mqMorphoDigCore::Setmui_ActiveScalarsAndRender(QString Scalar, int dataType, int numComp)
{
	this->Setmui_ActiveScalars(Scalar, dataType, numComp);
	emit this->activeScalarChanged();
	this->Render();
}
void mqMorphoDigCore::Setmui_ActiveScalars(QString Scalar, int dataType, int numComp)
{
	int sba_refresh_needed = 0;
	int old_sba_on = 0;
	int new_sba_on = 0;
	if ((dataType == VTK_FLOAT || dataType == VTK_DOUBLE) && numComp == 1)
	{
		new_sba_on = 1;
		
	}
	if ((this->mui_ActiveScalars->DataType == VTK_FLOAT || this->mui_ActiveScalars->DataType == VTK_DOUBLE) && this->mui_ActiveScalars->NumComp == 1)
	{
		old_sba_on = 1;
	}
	if (old_sba_on != new_sba_on)
	{
		sba_refresh_needed = 1;
		if (new_sba_on == 1)
		{
			this->ScalarBarActor->SetLookupTable(this->Getmui_ActiveColorMap()->ColorMap);
			this->ScalarBarActor->SetTitle(this->Getmui_ActiveScalars()->Name.toStdString().c_str());
			
			this->Renderer->AddActor(ScalarBarActor);
		}
		else
		{
			this->Renderer->RemoveActor(ScalarBarActor);
		}
	}

	this->mui_ActiveScalars->Name = Scalar;
	this->mui_ActiveScalars->DataType = dataType;
	this->mui_ActiveScalars->NumComp = numComp;
	cout << "Now active scalar is " << Scalar.toStdString() << endl;
	
		


	this->RefreshColorMapsAndScalarVisibility();
	
	

	cout << "Hello!!!!" << endl;

	// now brows through all actors and set active scalar 
	/*
			}*/

}


int mqMorphoDigCore::Getmui_DefaultAnaglyph() { return this->mui_DefaultAnaglyph; }
int mqMorphoDigCore::Getmui_Anaglyph() { return this->mui_Anaglyph; }
void mqMorphoDigCore::Setmui_Anaglyph(int anaglyph)
{
	this->mui_Anaglyph = anaglyph;
	if (this->RenderWindow != NULL)
	{
		if (anaglyph == 1)
		{
			this->RenderWindow->StereoRenderOn();
			this->RenderWindow->StereoUpdate();
			this->RenderWindow->Render();
		}
		else
		{
			this->RenderWindow->StereoRenderOff();
			this->RenderWindow->Render();
		}
	}
}


void mqMorphoDigCore::Setmui_LandmarkBodyType(int type) {
	this->mui_LandmarkBodyType = type; 
	this->UpdateLandmarkSettings();
}
int mqMorphoDigCore::Getmui_DefaultLandmarkBodyType() { return this->mui_DefaultLandmarkBodyType; }
int mqMorphoDigCore::Getmui_LandmarkBodyType() { return this->mui_LandmarkBodyType; }


void mqMorphoDigCore::Setmui_LandmarkMode(int mode) {
	this->mui_LandmarkMode = mode;
	//this->UpdateLandmarkSettings();
}
int mqMorphoDigCore::Getmui_DefaultLandmarkMode() { return this->mui_DefaultLandmarkMode; }
int mqMorphoDigCore::Getmui_LandmarkMode() { return this->mui_LandmarkMode; }


void mqMorphoDigCore::Setmui_LandmarkRenderingSize(double size)
{ this->mui_LandmarkRenderingSize = size;
this->UpdateLandmarkSettings();
}
double mqMorphoDigCore::Getmui_DefaultLandmarkRenderingSize() { return this->mui_DefaultLandmarkRenderingSize; }
double mqMorphoDigCore::Getmui_LandmarkRenderingSize() { return this->mui_LandmarkRenderingSize; }

void mqMorphoDigCore::Setmui_AdjustLandmarkRenderingSize(int adjust)
{
	this->mui_AdjustLandmarkRenderingSize = adjust;
	this->UpdateLandmarkSettings();
}
int mqMorphoDigCore::Getmui_DefaultAdjustLandmarkRenderingSize() { return this->mui_DefaultAdjustLandmarkRenderingSize; }
int mqMorphoDigCore::Getmui_AdjustLandmarkRenderingSize() { return this->mui_AdjustLandmarkRenderingSize; }

void mqMorphoDigCore::Setmui_AdjustScaleFactor(double factor) {
	this->mui_AdjustScaleFactor = factor; 
	this->UpdateLandmarkSettings();
}
double mqMorphoDigCore::Getmui_DefaultAdjustScaleFactor() { return this->mui_DefaultAdjustScaleFactor; }
double mqMorphoDigCore::Getmui_AdjustScaleFactor() { return this->mui_AdjustScaleFactor; }


void mqMorphoDigCore::Setmui_FlagRenderingSize(double size) { 
	this->mui_FlagRenderingSize = size; 
	cout << mui_FlagRenderingSize << endl;
}
double mqMorphoDigCore::Getmui_DefaultFlagRenderingSize() { return this->mui_DefaultFlagRenderingSize; }

double mqMorphoDigCore::Getmui_FlagRenderingSize() { return this->mui_FlagRenderingSize; 
//cout << "Default f r z" << this->mui_FlagRenderingSize<<endl;
}



double* mqMorphoDigCore::Getmui_MeshColor() { return this->mui_MeshColor; }
void mqMorphoDigCore::Getmui_MeshColor(double c[4])
{
	double *co = this->Getmui_MeshColor();

	c[0] = co[0];
	c[1] = co[1];
	c[2] = co[2];
	c[3] = co[3];
}
double* mqMorphoDigCore::Getmui_DefaultMeshColor() { return this->mui_DefaultMeshColor; }
void mqMorphoDigCore::Getmui_DefaultMeshColor(double c[4])
{
	double *co = this->Getmui_DefaultMeshColor();

	c[0] = co[0];
	c[1] = co[1];
	c[2] = co[2];
	c[3] = co[3];
}
void mqMorphoDigCore::Setmui_MeshColor(double c1, double c2, double c3, double c4)
{
	double c[4];
	c[0] = c1;
	c[1] = c2;
	c[2] = c3;
	c[3] = c4;
	

	this->Setmui_MeshColor(c);
}
void mqMorphoDigCore::Setmui_MeshColor(double c[4])
{
	this->mui_MeshColor[0] = c[0];
	this->mui_MeshColor[1] = c[1];
	this->mui_MeshColor[2] = c[2];
	this->mui_MeshColor[3] = c[3];
	//cout << "Core: this->mui_MeshColor[3]="<<this->mui_MeshColor[3] << endl;
	this->ActorCollection->InitTraversal();
	for (vtkIdType i = 0; i < this->ActorCollection->GetNumberOfItems(); i++)
	{
		vtkMDActor *myActor = vtkMDActor::SafeDownCast(this->ActorCollection->GetNextActor());
		if (myActor->GetSelected()==1)
		{
			
			myActor->SetmColor(this->mui_MeshColor);
			myActor->SetSelected(0);
		}
	}
}

double* mqMorphoDigCore::Getmui_FlagColor() { return this->mui_FlagColor; }
void mqMorphoDigCore::Getmui_FlagColor(double c[3])
{
	double *co = this->Getmui_FlagColor();

	c[0] = co[0];
	c[1] = co[1];
	c[2] = co[2];	
}

double* mqMorphoDigCore::Getmui_DefaultFlagColor() { return this->mui_DefaultFlagColor; }

void mqMorphoDigCore::Getmui_DefaultFlagColor(double c[3])
{
	double *co = this->Getmui_DefaultFlagColor();

	c[0] = co[0];
	c[1] = co[1];
	c[2] = co[2];
	
}
void mqMorphoDigCore::Setmui_FlagColor(double c1, double c2, double c3)
{
	double c[3];
	c[0] = c1;
	c[1] = c2;
	c[2] = c3;
	


	this->Setmui_FlagColor(c);
}
void mqMorphoDigCore::Setmui_FlagColor(double c[3])
{
	this->mui_FlagColor[0] = c[0];
	this->mui_FlagColor[1] = c[1];
	this->mui_FlagColor[2] = c[2];
	
	//cout << "Core: this->mui_MeshColor[3]="<<this->mui_MeshColor[3] << endl;
	/*this->FlagCollection->InitTraversal();
	for (vtkIdType i = 0; i < this->FlagCollection->GetNumberOfItems(); i++)
	{
		vtkLMActor *myActor = vtkLMActor::SafeDownCast(this->ActorCollection->GetNextActor());
		if (myActor->GetSelected() == 1)
		{

			//myActor->SetmColor(this->mui_FlagColor);
			//myActor->SetSelected(0);
		}
	}*/
}



double* mqMorphoDigCore::Getmui_BackGroundColor() { return this->mui_BackGroundColor; }
void mqMorphoDigCore::Getmui_BackGroundColor(double bg[3])
{
	double *bgr = this->Getmui_BackGroundColor();

	bg[0] = bgr[0];
	bg[1] = bgr[1];
	bg[2] = bgr[2];
	
}


double* mqMorphoDigCore::Getmui_DefaultBackGroundColor() { return this->mui_DefaultBackGroundColor; }
void mqMorphoDigCore::Getmui_DefaultBackGroundColor(double bg[3])
{
	double *bgr = this->Getmui_DefaultBackGroundColor();

	bg[0] = bgr[0];
	bg[1] = bgr[1];
	bg[2] = bgr[2];

}


double* Getmui_DefaultBackGroundColor();
void Getmui_DefaultBackGroundColor(double bg[3]);
void mqMorphoDigCore::Setmui_BackGroundColor(double bg1, double bg2, double bg3)
{
	double background[3];
	background[0] = bg1;
	background[1] = bg2;
	background[2] = bg3;

	this->Setmui_BackGroundColor(background);
}
void mqMorphoDigCore::Setmui_BackGroundColor(double background[3])
{
	this->mui_BackGroundColor[0] = background[0];
	this->mui_BackGroundColor[1] = background[1];
	this->mui_BackGroundColor[2] = background[2];
	this->Renderer->SetBackground(background);
	//this->RenderWindow->Render();
}

double* mqMorphoDigCore::Getmui_BackGroundColor2() { return this->mui_BackGroundColor2; }
void mqMorphoDigCore::Getmui_BackGroundColor2(double bg[3])
{
	double *bgr = this->Getmui_BackGroundColor2();

	bg[0] = bgr[0];
	bg[1] = bgr[1];
	bg[2] = bgr[2];

}


double* mqMorphoDigCore::Getmui_DefaultBackGroundColor2() { return this->mui_DefaultBackGroundColor2; }
void mqMorphoDigCore::Getmui_DefaultBackGroundColor2(double bg[3])
{
	double *bgr = this->Getmui_DefaultBackGroundColor2();

	bg[0] = bgr[0];
	bg[1] = bgr[1];
	bg[2] = bgr[2];

}

void mqMorphoDigCore::Setmui_BackGroundColor2(double bg1, double bg2, double bg3)
{
	double background[3];
	background[0] = bg1;
	background[1] = bg2;
	background[2] = bg3;

	this->Setmui_BackGroundColor2(background);
}
void mqMorphoDigCore::Setmui_BackGroundColor2(double background[3])
{
	this->mui_BackGroundColor2[0] = background[0];
	this->mui_BackGroundColor2[1] = background[1];
	this->mui_BackGroundColor2[2] = background[2];
	this->Renderer->SetBackground2(background);
	//this->RenderWindow->Render();
}
double mqMorphoDigCore::AdjustedLandmarkSize()
{

	double bbl = this->ActorCollection->GetBoundingBoxLength();
	
	double adjusted_size = this->Getmui_AdjustScaleFactor()*bbl / 50;
	if (adjusted_size > 0 && bbl < DBL_MAX)
	{
		return adjusted_size;
	}
	else
	{
		if (this->Getmui_LandmarkRenderingSize() > 0)
		{
			return this->Getmui_LandmarkRenderingSize();

		}
		else
		{
			return this->Getmui_DefaultLandmarkRenderingSize();
		}
	}

}
void mqMorphoDigCore::UpdateLandmarkSettings(vtkLMActor *myActor)
{
	myActor->SetLMBodyType(this->Getmui_LandmarkBodyType());
	if (myActor->GetLMType() != FLAG_LMK)
	{

		if (this->Getmui_AdjustLandmarkRenderingSize() == 1)
		{
			//myActor->SetLMSize(this->Getmui_LandmarkRenderingSize());
			myActor->SetLMSize(this->AdjustedLandmarkSize());
		}
		else
		{
			myActor->SetLMSize(this->Getmui_LandmarkRenderingSize());
			//Change landmark size for all landmarks but flags.

		}
	}
	else
	{
		//Do not change size!!!
		myActor->SetLMSize(myActor->GetLMSize());
	}


	vtkSmartPointer<vtkPolyDataMapper> mapper =
		vtkSmartPointer<vtkPolyDataMapper>::New();
	mapper->SetInputData(myActor->getLMBody());
	mapper->Update();
	myActor->SetMapper(mapper);

}
void mqMorphoDigCore::UpdateLandmarkSettings()
{
	this->NormalLandmarkCollection->InitTraversal();
	for (vtkIdType i = 0; i < this->NormalLandmarkCollection->GetNumberOfItems(); i++)
	{
		vtkLMActor *myActor = vtkLMActor::SafeDownCast(this->NormalLandmarkCollection->GetNextActor());
		UpdateLandmarkSettings(myActor);
		
	}
	this->TargetLandmarkCollection->InitTraversal();
	for (vtkIdType i = 0; i < this->TargetLandmarkCollection->GetNumberOfItems(); i++)
	{
		vtkLMActor *myActor = vtkLMActor::SafeDownCast(this->TargetLandmarkCollection->GetNextActor());
		UpdateLandmarkSettings(myActor);

	}
	this->NodeLandmarkCollection->InitTraversal();
	for (vtkIdType i = 0; i < this->NodeLandmarkCollection->GetNumberOfItems(); i++)
	{
		vtkLMActor *myActor = vtkLMActor::SafeDownCast(this->NodeLandmarkCollection->GetNextActor());
		UpdateLandmarkSettings(myActor);

	}
	this->HandleLandmarkCollection->InitTraversal();
	for (vtkIdType i = 0; i < this->HandleLandmarkCollection->GetNumberOfItems(); i++)
	{
		vtkLMActor *myActor = vtkLMActor::SafeDownCast(this->HandleLandmarkCollection->GetNextActor());
		UpdateLandmarkSettings(myActor);

	}
	this->FlagLandmarkCollection->InitTraversal();
	for (vtkIdType i = 0; i < this->FlagLandmarkCollection->GetNumberOfItems(); i++)
	{
		vtkLMActor *myActor = vtkLMActor::SafeDownCast(this->FlagLandmarkCollection->GetNextActor());
		UpdateLandmarkSettings(myActor);

	}
}

void mqMorphoDigCore::Undo()
{
	// a Set is only a label (action) and an id
	//vtkUndoSet *MyUndoSet = this->UndoStack->GetNextUndoSet();
	//this->ActorCollection->Undo(MySet);
	//cout << "Root Undo!" << endl;
	this->UndoStack->undo(); // removes the next undo set.. 

}
void mqMorphoDigCore::Undo(int Count)
{
	//cout << "Undo(" <<Count<<")"<< endl;
	//Calls for the Undo method in vtkActorCollection for this particular Count etc.. 
	this->ActorCollection->InitTraversal();
	for (vtkIdType i = 0; i < this->ActorCollection->GetNumberOfItems(); i++)
	{
		vtkMDActor *myActor = vtkMDActor::SafeDownCast(this->ActorCollection->GetNextActor());
	//	cout << "MyActor undo!" << endl;
		myActor->Undo(Count);		
	}
	this->ActorCollection->Undo(Count);

	
	this->NormalLandmarkCollection->InitTraversal();
	for (vtkIdType i = 0; i < this->NormalLandmarkCollection->GetNumberOfItems(); i++)
	{
		vtkLMActor *myActor = vtkLMActor::SafeDownCast(this->NormalLandmarkCollection->GetNextActor());
		//cout << "Call MyLMActor undo from core!" << endl;
		myActor->Undo(Count);
	}
	// To update to take into account reorder!
	this->NormalLandmarkCollection->Undo(Count);

	this->TargetLandmarkCollection->InitTraversal();
	for (vtkIdType i = 0; i < this->TargetLandmarkCollection->GetNumberOfItems(); i++)
	{
		vtkLMActor *myActor = vtkLMActor::SafeDownCast(this->TargetLandmarkCollection->GetNextActor());
		//cout << "Call MyLMActor undo from core!" << endl;
		myActor->Undo(Count);
	}
	// To update to take into account reorder!
	this->TargetLandmarkCollection->Undo(Count);

	this->NodeLandmarkCollection->InitTraversal();
	for (vtkIdType i = 0; i < this->NodeLandmarkCollection->GetNumberOfItems(); i++)
	{
		vtkLMActor *myActor = vtkLMActor::SafeDownCast(this->NodeLandmarkCollection->GetNextActor());
		//cout << "Call MyLMActor undo from core!" << endl;
		myActor->Undo(Count);
	}
	//@@ dirty! Recompute bezier curve whatever we do!
	// To update to take into account reorder!
	this->NodeLandmarkCollection->Undo(Count);
	this->NodeLandmarkCollection->ReorderLandmarks();
	this->NodeLandmarkCollection->Modified();

	this->HandleLandmarkCollection->InitTraversal();
	for (vtkIdType i = 0; i < this->HandleLandmarkCollection->GetNumberOfItems(); i++)
	{
		vtkLMActor *myActor = vtkLMActor::SafeDownCast(this->HandleLandmarkCollection->GetNextActor());
		//cout << "Call MyLMActor undo from core!" << endl;
		myActor->Undo(Count);
	}
	
	// To update to take into account reorder!
	this->HandleLandmarkCollection->Undo(Count);

	//@@ dirty! Recompute bezier curve whatever we do!
	this->HandleLandmarkCollection->ReorderLandmarks();
	this->HandleLandmarkCollection->Modified();


	this->FlagLandmarkCollection->InitTraversal();
	for (vtkIdType i = 0; i < this->FlagLandmarkCollection->GetNumberOfItems(); i++)
	{
		vtkLMActor *myActor = vtkLMActor::SafeDownCast(this->FlagLandmarkCollection->GetNextActor());
		//cout << "Call MyLMActor undo from core!" << endl;
		myActor->Undo(Count);
	}
	// To update to take into account reorder!
	this->FlagLandmarkCollection->Undo(Count);
}
void mqMorphoDigCore::Redo()
{
	//cout << "Root Redo!" << endl;
	this->UndoStack->redo(); // removes the next undo set.. 
}

void mqMorphoDigCore::Redo(int Count)
{
	//cout << "Redo(" << Count << ")" << endl;
	//Calls for the Undo method in vtkActorCollection for this particular Count etc.. 
	this->ActorCollection->InitTraversal();
	for (vtkIdType i = 0; i < this->ActorCollection->GetNumberOfItems(); i++)
	{
		vtkMDActor *myActor = vtkMDActor::SafeDownCast(this->ActorCollection->GetNextActor());
		//cout << "MyActor Redo!" << endl;
		myActor->Redo(Count);
	}
	this->ActorCollection->Redo(Count);

	
	this->NormalLandmarkCollection->InitTraversal();
	for (vtkIdType i = 0; i < this->NormalLandmarkCollection->GetNumberOfItems(); i++)
	{
		vtkLMActor *myActor = vtkLMActor::SafeDownCast(this->NormalLandmarkCollection->GetNextActor());
		//cout << "Call MyLMActor redo from core!" << endl;
		myActor->Redo(Count);
	}
	// To update to take into account reorder!
	this->NormalLandmarkCollection->Redo(Count);

	this->TargetLandmarkCollection->InitTraversal();
	for (vtkIdType i = 0; i < this->TargetLandmarkCollection->GetNumberOfItems(); i++)
	{
		vtkLMActor *myActor = vtkLMActor::SafeDownCast(this->TargetLandmarkCollection->GetNextActor());
		//cout << "Call MyLMActor undo from core!" << endl;
		myActor->Redo(Count);
	}
	// To update to take into account reorder!
	this->TargetLandmarkCollection->Redo(Count);

	this->NodeLandmarkCollection->InitTraversal();
	for (vtkIdType i = 0; i < this->NodeLandmarkCollection->GetNumberOfItems(); i++)
	{
		vtkLMActor *myActor = vtkLMActor::SafeDownCast(this->NodeLandmarkCollection->GetNextActor());
		//cout << "Call MyLMActor undo from core!" << endl;
		myActor->Redo(Count);
	}
	// To update to take into account reorder!
	this->NodeLandmarkCollection->Redo(Count);
	//@@ dirty! Recompute bezier curve whatever we do!
	this->NodeLandmarkCollection->ReorderLandmarks();
	this->NodeLandmarkCollection->Modified();

	this->HandleLandmarkCollection->InitTraversal();
	for (vtkIdType i = 0; i < this->HandleLandmarkCollection->GetNumberOfItems(); i++)
	{
		vtkLMActor *myActor = vtkLMActor::SafeDownCast(this->HandleLandmarkCollection->GetNextActor());
		//cout << "Call MyLMActor undo from core!" << endl;
		myActor->Redo(Count);
	}

	// To update to take into account reorder!
	this->HandleLandmarkCollection->Redo(Count);
	//@@ dirty! Recompute bezier curve whatever we do!
	this->HandleLandmarkCollection->ReorderLandmarks();
	this->HandleLandmarkCollection->Modified();

	this->FlagLandmarkCollection->InitTraversal();
	for (vtkIdType i = 0; i < this->FlagLandmarkCollection->GetNumberOfItems(); i++)
	{
		vtkLMActor *myActor = vtkLMActor::SafeDownCast(this->FlagLandmarkCollection->GetNextActor());
		//cout << "Call MyLMActor undo from core!" << endl;
		myActor->Redo(Count);
	}
	// To update to take into account reorder!
	this->FlagLandmarkCollection->Redo(Count);

}

void mqMorphoDigCore::Erase(int Count)
{
	//cout << "Erase(" << Count << ")" << endl;
	//Calls for the Undo method in vtkActorCollection for this particular Count etc.. 
	this->ActorCollection->InitTraversal();
	for (vtkIdType i = 0; i < this->ActorCollection->GetNumberOfItems(); i++)
	{
		vtkMDActor *myActor = vtkMDActor::SafeDownCast(this->ActorCollection->GetNextActor());
		//cout << "MyActor Erase!" << endl;
		myActor->Erase(Count);
	}
	this->ActorCollection->Erase(Count);

	this->NormalLandmarkCollection->InitTraversal();
	for (vtkIdType i = 0; i < this->NormalLandmarkCollection->GetNumberOfItems(); i++)
	{
		vtkLMActor *myActor = vtkLMActor::SafeDownCast(this->NormalLandmarkCollection->GetNextActor());
		//cout << "Call MyLMActor redo from core!" << endl;
		myActor->Erase(Count);
	}
	// To update to take into account reorder!
	this->NormalLandmarkCollection->Erase(Count);

	this->TargetLandmarkCollection->InitTraversal();
	for (vtkIdType i = 0; i < this->TargetLandmarkCollection->GetNumberOfItems(); i++)
	{
		vtkLMActor *myActor = vtkLMActor::SafeDownCast(this->TargetLandmarkCollection->GetNextActor());
		//cout << "Call MyLMActor undo from core!" << endl;
		myActor->Erase(Count);
	}
	// To update to take into account reorder!
	this->TargetLandmarkCollection->Erase(Count);

	this->NodeLandmarkCollection->InitTraversal();
	for (vtkIdType i = 0; i < this->NodeLandmarkCollection->GetNumberOfItems(); i++)
	{
		vtkLMActor *myActor = vtkLMActor::SafeDownCast(this->NodeLandmarkCollection->GetNextActor());
		//cout << "Call MyLMActor undo from core!" << endl;
		myActor->Erase(Count);
	}
	// To update to take into account reorder!
	this->NodeLandmarkCollection->Erase(Count);

	this->HandleLandmarkCollection->InitTraversal();
	for (vtkIdType i = 0; i < this->HandleLandmarkCollection->GetNumberOfItems(); i++)
	{
		vtkLMActor *myActor = vtkLMActor::SafeDownCast(this->HandleLandmarkCollection->GetNextActor());
		//cout << "Call MyLMActor undo from core!" << endl;
		myActor->Erase(Count);
	}
	// To update to take into account reorder!
	this->HandleLandmarkCollection->Erase(Count);

	this->FlagLandmarkCollection->InitTraversal();
	for (vtkIdType i = 0; i < this->FlagLandmarkCollection->GetNumberOfItems(); i++)
	{
		vtkLMActor *myActor = vtkLMActor::SafeDownCast(this->FlagLandmarkCollection->GetNextActor());
		//cout << "Call MyLMActor undo from core!" << endl;
		myActor->Erase(Count);
	}
	// To update to take into account reorder!
	this->FlagLandmarkCollection->Erase(Count);


}
void mqMorphoDigCore::setUndoStack(mqUndoStack* stack)
{
	if (stack != this->UndoStack)
	{
		this->UndoStack = stack;
		/*if (stack)
		{
			stack->setParent(this);
		}*/
		//emit this->undoStackChanged(stack);
	}
}

//-----------------------------------------------------------------------------

/*vtkSmartPointer<vtkUndoStack> mqMorphoDigCore::getUndoStack()
{
	return this->UndoStack;
}*/

void mqMorphoDigCore::signal_existingScalarsChanged()
{
	cout << "Emit existing scalars changed" << endl;
	emit this->existingScalarsChanged();
}

void mqMorphoDigCore::signal_actorSelectionChanged()
{
	cout << "Emit actor Selection changed" << endl;
	emit this->actorSelectionChanged();
}
void mqMorphoDigCore::signal_lmSelectionChanged()
{
	emit this->lmSelectionChanged();
}

mqUndoStack* mqMorphoDigCore::getUndoStack()
{
return this->UndoStack;
}

vtkSmartPointer<vtkActor> mqMorphoDigCore::getBezierActor()
{
	return this->BezierActor;
}
vtkSmartPointer<vtkActor> mqMorphoDigCore::getBezierSelectedActor()
{
	return this->BezierSelectedActor;
}
vtkSmartPointer<vtkActor> mqMorphoDigCore::getBezierNHActor()
{
	return this->BezierNHActor;
}
vtkSmartPointer<vtkBezierCurveSource> mqMorphoDigCore::getBezierCurveSource()
{
	return this->BezierCurveSource;
}
vtkSmartPointer<vtkMDActorCollection> mqMorphoDigCore::getActorCollection()
{
	return this->ActorCollection;
}
vtkSmartPointer<vtkLMActorCollection> mqMorphoDigCore::getNormalLandmarkCollection()
{
	return this->NormalLandmarkCollection;
}
vtkSmartPointer<vtkLMActorCollection> mqMorphoDigCore::getTargetLandmarkCollection()
{
	return this->TargetLandmarkCollection;
}
vtkSmartPointer<vtkLMActorCollection> mqMorphoDigCore::getNodeLandmarkCollection()
{
	return this->NodeLandmarkCollection;
}
vtkSmartPointer<vtkLMActorCollection> mqMorphoDigCore::getHandleLandmarkCollection()
{
	return this->HandleLandmarkCollection;
}
vtkSmartPointer<vtkLMActorCollection> mqMorphoDigCore::getFlagLandmarkCollection()
{
	return this->FlagLandmarkCollection;
}
/*
vtkMDActorCollection* mqMorphoDigCore::getActorCollection()
{
	return this->ActorCollection;
}*/

vtkSmartPointer<vtkRenderer> mqMorphoDigCore::getRenderer()
{
	return this->Renderer;
}
vtkSmartPointer<vtkCamera> mqMorphoDigCore::getCamera()
{
	return this->Camera;
}
vtkSmartPointer<vtkGridActor> mqMorphoDigCore::getGridActor()
{
	return this->GridActor;
}

void mqMorphoDigCore::TransformPoint(vtkMatrix4x4* matrix, double pointin[3], double pointout[3]) {
	double pointPred[4]; double pointNew[4] = { 0, 0, 0, 0 };
	pointPred[0] = pointin[0];
	pointPred[1] = pointin[1];
	pointPred[2] = pointin[2];
	pointPred[3] = 1;

	matrix->MultiplyPoint(pointPred, pointNew);
	pointout[0] = pointNew[0];
	pointout[1] = pointNew[1];
	pointout[2] = pointNew[2];
}

void mqMorphoDigCore::SetSelectedActorsColor(int r, int g, int b) 
{
	int num_selected = this->ActorCollection->GetNumberOfSelectedActors();
	if (num_selected > 0)
	{
		this->ActorCollection->InitTraversal();

		std::string action = "Modify color of selected actors";
		int mCount = BEGIN_UNDO_SET(action);
		
	
		for (vtkIdType i = 0; i < this->ActorCollection->GetNumberOfItems(); i++)
		{
			vtkMDActor * myActor = vtkMDActor::SafeDownCast(this->ActorCollection->GetNextActor());
			if (myActor->GetSelected() == 1)
			{
				myActor->SaveState(mCount);
				double color[4] = { 0.5, 0.5, 0.5, 1 };
				myActor->GetmColor(color);
				if (r >= 0 && r <= 255 && g >= 0 && g <= 255 && b >= 0 && b <= 255)
				{
					color[0] = (double)((double)r / 255);
					color[1] = (double)((double)g / 255);
					color[2] = (double)((double)b / 255);

				}
				myActor->SetmColor(color);
				myActor->SetSelected(0);
				myActor->Modified();
			}
		}
		END_UNDO_SET();
		this->Render();
	}
}
void mqMorphoDigCore::GetWorldToDisplay(double x, double y, double z, double displayPt[3])
{
	this->getRenderer()->SetWorldPoint(x, y, z, 1.0);
	this->getRenderer()->WorldToDisplay();
	this->getRenderer()->GetDisplayPoint(displayPt);
}

void mqMorphoDigCore::GetDisplayToWorld(double x, double y, double z, double worldPt[4])
{

	this->getRenderer()->SetDisplayPoint(x, y, z);
	this->getRenderer()->DisplayToWorld();
	this->getRenderer()->GetWorldPoint(worldPt);
	if (worldPt[3])
	{
		worldPt[0] /= worldPt[3];
		worldPt[1] /= worldPt[3];
		worldPt[2] /= worldPt[3];
		worldPt[3] = 1.0;
	}
}
void mqMorphoDigCore::SetSelectedActorsTransparency(int trans) {

	int num_selected = this->ActorCollection->GetNumberOfSelectedActors();
	if (num_selected > 0)
	{
		this->ActorCollection->InitTraversal();

		std::string action = "Modify color of selected actors";
		int mCount = BEGIN_UNDO_SET(action);


		for (vtkIdType i = 0; i < this->ActorCollection->GetNumberOfItems(); i++)
		{
			vtkMDActor * myActor = vtkMDActor::SafeDownCast(this->ActorCollection->GetNextActor());
			if (myActor->GetSelected() == 1)
			{
				myActor->SaveState(mCount);
				double color[4] = { 0.5, 0.5, 0.5, 1 };
				myActor->GetmColor(color);
				if (trans >= 0 && trans <= 100 )
				{
					color[3] = (double)((double)trans / 100);
					

				}
				myActor->SetmColor(color);
				myActor->SetSelected(0);
				myActor->Modified();
			}
		}
		END_UNDO_SET();
		this->Render();
	}
}

void mqMorphoDigCore::slotLandmarkMoveUp()
{

	this->LandmarksMoveUp();
}
void mqMorphoDigCore::slotUpdateAllSelectedFlagsColors()
{
	this->UpdateAllSelectedFlagsColors();
}
void mqMorphoDigCore::slotLandmarkMoveDown()
{

	this->LandmarksMoveDown();
}

void mqMorphoDigCore::slotConvexHULL() { this->addConvexHull(); }
void mqMorphoDigCore::slotMirror() { this->addMirrorXZ(); }
void mqMorphoDigCore::slotInvert() { 
		this->addInvert(); 

}

void mqMorphoDigCore::slotKeepLargest() {
	this->addKeepLargest();
}


void mqMorphoDigCore::slotGrey() { this->SetSelectedActorsColor(150, 150, 150); }
void mqMorphoDigCore::slotYellow(){ this->SetSelectedActorsColor(165, 142, 22); }
void mqMorphoDigCore::slotRed(){ this->SetSelectedActorsColor(186, 37, 37); }
void mqMorphoDigCore::slotPink(){ this->SetSelectedActorsColor(173, 120, 95); }
void mqMorphoDigCore::slotBlue(){ this->SetSelectedActorsColor(64, 123, 126); }
void mqMorphoDigCore::slotViolet(){ this->SetSelectedActorsColor(120, 51, 145); }
void mqMorphoDigCore::slotBone(){ this->SetSelectedActorsColor(161, 146, 95); }
void mqMorphoDigCore::slotGreen(){ this->SetSelectedActorsColor(39, 136, 42); }
void mqMorphoDigCore::slotDarkred(){ this->SetSelectedActorsColor(115, 8, 15); }
void mqMorphoDigCore::slotDarkblue(){ this->SetSelectedActorsColor(52, 52, 160); }
void mqMorphoDigCore::slotDarkgreen(){ this->SetSelectedActorsColor(42, 110, 47); }
void mqMorphoDigCore::slotOrange(){ this->SetSelectedActorsColor(195, 91, 0); }
void mqMorphoDigCore::slotBrown(){ this->SetSelectedActorsColor(130, 78, 47); }

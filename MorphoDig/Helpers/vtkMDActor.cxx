/*=========================================================================

Program:   MorphoDig
Module:    vtkMDActor.cxx
=========================================================================*/
#include "mqMorphoDigCore.h"
#include "vtkMDActor.h"

#include <vtkObjectFactory.h>
#include <vtkMath.h>
#include <vtkDataArray.h>
#include <vtkSmartPointer.h>
#include <vtkRenderer.h>
#include <vtkProperty.h>

#include <vtkTable.h>
#include <vtkPCAStatistics.h>
#include <vtkLine.h>
#include <vtkTexture.h>
#include <vtkPolyData.h>
#include <vtkPoints.h>
#include <vtkPointData.h>
#include <vtkPolyDataMapper.h>
#include <vtkCenterOfMass.h>
#include <vtkTransform.h>
#include <vtkPolyData.h>
#include <vtkDoubleArray.h>
#include <vtkUnsignedIntArray.h>
#include <vtkFloatArray.h>
#include <vtkIntArray.h>
#include <vtkUnsignedCharArray.h>
#include <vtkPlane.h>
#include <QString>
vtkStandardNewMacro(vtkMDActor);



//----------------------------------------------------------------------------
vtkMDActor::vtkMDActor()
{
	vtkSmartPointer<vtkProperty> backFaces =
		vtkSmartPointer<vtkProperty>::New();
	//backFaces->SetDiffuseColor(.8, .8, .8);
	backFaces->SetColor(.7, .7, .7);
	backFaces->SetOpacity(0.5);

	this->SetBackfaceProperty(backFaces);
	this->UndoRedo = new vtkMDActorUndoRedo;
	this->Selected = 1;
	this->Changed = 0;
	this->Name = "New Mesh";
}

//----------------------------------------------------------------------------
vtkMDActor::~vtkMDActor()
{
	this->UndoRedo->RedoStack.clear();
	this->UndoRedo->UndoStack.clear();
	delete this->UndoRedo;
	

}
int vtkMDActor::IsInsideFrustum(vtkSmartPointer<vtkPlanes> myPlanes)
{
	int is_inside = 0;
	int is_insideALL[6] = { 0,0,0,0,0,0 };
	vtkSmartPointer<vtkMatrix4x4> Mat = vtkSmartPointer<vtkMatrix4x4>::New();
	this->GetMatrix(Mat);
	vtkPolyDataMapper *mapper = vtkPolyDataMapper::SafeDownCast(this->GetMapper());
	if (mapper != NULL && vtkPolyData::SafeDownCast(mapper->GetInput()) != NULL)
	{
		vtkPolyData *myPD = vtkPolyData::SafeDownCast(mapper->GetInput());

		//we have six planes
		for (vtkIdType j = 0; j < myPD->GetNumberOfPoints(); j+=20)
		{
			is_insideALL[0] = 0;
			is_insideALL[1] = 0;
			is_insideALL[2] = 0;
			is_insideALL[3] = 0;
			is_insideALL[4] = 0;
			is_insideALL[5] = 0;
			double pt[3];
			double ptwc[3];
			double dist;
			for (vtkIdType i = 0; i < myPlanes->GetNumberOfPlanes(); i++)
			{
				
				vtkPlane *plane = myPlanes->GetPlane(i);
			
					myPD->GetPoint(j, pt);
					mqMorphoDigCore::TransformPoint(Mat, pt, ptwc);
					 dist = plane->EvaluateFunction(ptwc);
					 if (dist < 0) {						
						 is_insideALL[i] = 1;}
			}
			if (is_insideALL[0] == 1 && is_insideALL[1] == 1 && is_insideALL[2] == 1 && is_insideALL[3] == 1 && is_insideALL[4] == 1 && is_insideALL[5] == 1)
			{
				is_inside = 1; 
				cout << "found one inside point: " << j << ", x=" << ptwc[0] << ", y=" << ptwc[1] << ", z=" << ptwc[2] << endl;
				break;
				
			}
			
		}
	}
	
		return is_inside;
	
}

vtkIdType vtkMDActor::GetNumberOfPoints()
{
	vtkIdType nv = 0;
	vtkPolyDataMapper *mapper = vtkPolyDataMapper::SafeDownCast(this->GetMapper());
	if (mapper != NULL && vtkPolyData::SafeDownCast(mapper->GetInput()) != NULL)
	{
		return vtkPolyData::SafeDownCast(mapper->GetInput())->GetNumberOfPoints();
	}
	
	return nv;
}
double vtkMDActor::GetAvgCentroidDistance()
{
	double avgCD = 0;
	vtkPolyDataMapper *mapper = vtkPolyDataMapper::SafeDownCast(this->GetMapper());
	if (mapper != NULL && vtkPolyData::SafeDownCast(mapper->GetInput()) != NULL)
	{
		vtkPolyData *mPD =  vtkPolyData::SafeDownCast(mapper->GetInput());
		vtkSmartPointer<vtkCenterOfMass> centerOfMassFilter =
			vtkSmartPointer<vtkCenterOfMass>::New();
	
		centerOfMassFilter->SetInputData(mPD);
		centerOfMassFilter->SetUseScalarsAsWeights(false);
		centerOfMassFilter->Update();
		double center[3];
		double pt[3];
		centerOfMassFilter->GetCenter(center);
		
		for (vtkIdType i = 0; i < mPD->GetNumberOfPoints(); i++)
		{
			mPD->GetPoint(i, pt);
			avgCD += sqrt((center[0] - pt[0])*(center[0] - pt[0]) + (center[1] - pt[1])*(center[1] - pt[1]) + (center[2] - pt[2])*(center[2] - pt[2]));
		}
		if (mPD->GetNumberOfPoints() > 0) { avgCD /= mPD->GetNumberOfPoints(); }
		//if (cs > 0) { cs = sqrt(cs); }

	}
	
		return avgCD;
}
double vtkMDActor::GetXYZAvgPCLength()
{
	double pc1 = this->GetXYZPCLength(0);
	double pc2 = this->GetXYZPCLength(1);
	double pc3 = this->GetXYZPCLength(2);
	double meanpc = (pc1 + pc2 + pc3) / 3;
	return meanpc;


}
double vtkMDActor::GetXYZPCLength(int pc)
{
	// gives a measure "size" of the mesh. Length of the vtkPolyData projected on PC1, PC2 or PC3 of a PCA of x,y,z coordinates of the VTK PolyData
	//as we perform a PCA on x,y,z coordinates, we can only compute 3 PCs : only PC1 (pc =0), PC2 (pc=1) or PC3 (pc=2)
	if (pc < 0 || pc>2) { pc = 0; }
	double pcL = 0;

	double maxCD = -DBL_MAX;
	vtkPolyDataMapper *mapper = vtkPolyDataMapper::SafeDownCast(this->GetMapper());
	if (mapper != NULL && vtkPolyData::SafeDownCast(mapper->GetInput()) != NULL)
	{
		vtkPolyData *mPD = vtkPolyData::SafeDownCast(mapper->GetInput());
				
		double pt[3];		

		// These are all the "x" values.
		vtkSmartPointer<vtkDoubleArray> xArray =
			vtkSmartPointer<vtkDoubleArray>::New();
		xArray->SetNumberOfComponents(1);
		xArray->SetName("x");

		// These are all the "y" values.
		vtkSmartPointer<vtkDoubleArray> yArray =
			vtkSmartPointer<vtkDoubleArray>::New();
		yArray->SetNumberOfComponents(1);
		yArray->SetName("y");

		// These are all the "z" values.
		vtkSmartPointer<vtkDoubleArray> zArray =
			vtkSmartPointer<vtkDoubleArray>::New();
		zArray->SetNumberOfComponents(1);
		zArray->SetName("z");

		for (vtkIdType i = 0; i < mPD->GetNumberOfPoints(); i++)
		{
			mPD->GetPoint(i, pt);					
			xArray->InsertNextValue(pt[0]);
			yArray->InsertNextValue(pt[1]);
			zArray->InsertNextValue(pt[2]);
		}

		vtkSmartPointer<vtkTable> datasetTable =
			vtkSmartPointer<vtkTable>::New();
		datasetTable->AddColumn(xArray);
		datasetTable->AddColumn(yArray);
		datasetTable->AddColumn(zArray);

		vtkSmartPointer<vtkPCAStatistics> pcaStatistics = vtkSmartPointer<vtkPCAStatistics>::New();

		pcaStatistics->SetInputData(vtkStatisticsAlgorithm::INPUT_DATA, datasetTable);

		pcaStatistics->SetColumnStatus("x", 1);
		pcaStatistics->SetColumnStatus("y", 1);
		pcaStatistics->SetColumnStatus("z", 1);

		pcaStatistics->RequestSelectedColumns();
		pcaStatistics->SetDeriveOption(true);
		pcaStatistics->Update();

		///////// Eigenvalues ////////////
		vtkSmartPointer<vtkDoubleArray> eigenvalues =
			vtkSmartPointer<vtkDoubleArray>::New();
		pcaStatistics->GetEigenvalues(eigenvalues);
		for (vtkIdType i = 0; i < eigenvalues->GetNumberOfTuples(); i++)
		{
			std::cout << "Eigenvalue " << i << " = " << eigenvalues->GetValue(i) << std::endl;
		}

		///////// Eigenvectors ////////////
		vtkSmartPointer<vtkDoubleArray> eigenvectors =
			vtkSmartPointer<vtkDoubleArray>::New();

		pcaStatistics->GetEigenvectors(eigenvectors);

		vtkSmartPointer<vtkDoubleArray> evec =
			vtkSmartPointer<vtkDoubleArray>::New();
		pcaStatistics->GetEigenvector(pc, evec);
			

		// Project all of the points onto the desired eigenvector

		double v[3];
		v[0] = evec->GetValue(0);
		v[1] =  evec->GetValue(1);
		v[2] =  evec->GetValue(2);

		vtkMath::Normalize(v); // just in case this would not be normalized!
		cout << "PC" << pc << " eigvect:" << v[0] << "," << v[1] << "," << v[2] << endl;

		

		
		double tmin = DBL_MAX;
		double tmax = -DBL_MAX;
		for (vtkIdType i = 0; i < mPD->GetNumberOfPoints(); i++)
		{
			mPD->GetPoint(i, pt);			
			double t = v[0] * pt[0] + v[1] * pt[1] + v[2] * pt[2];
			if (t < tmin) { tmin = t; }
			if (t > tmax) { tmax = t; }
		}
		cout << "tmin=" << tmin << ", tmax=" << tmax << endl;
		pcL = tmax - tmin;



	}
	return pcL;

}
double vtkMDActor::GetMaxCentroidDistance()
{
	double maxCD = -DBL_MAX;
	vtkPolyDataMapper *mapper = vtkPolyDataMapper::SafeDownCast(this->GetMapper());
	if (mapper != NULL && vtkPolyData::SafeDownCast(mapper->GetInput()) != NULL)
	{
		vtkPolyData *mPD = vtkPolyData::SafeDownCast(mapper->GetInput());
		vtkSmartPointer<vtkCenterOfMass> centerOfMassFilter =
			vtkSmartPointer<vtkCenterOfMass>::New();

		centerOfMassFilter->SetInputData(mPD);
		centerOfMassFilter->SetUseScalarsAsWeights(false);
		centerOfMassFilter->Update();
		double center[3];
		double pt[3];
		centerOfMassFilter->GetCenter(center);

		for (vtkIdType i = 0; i < mPD->GetNumberOfPoints(); i++)
		{
			mPD->GetPoint(i, pt);
			double currCD= sqrt((center[0] - pt[0])*(center[0] - pt[0]) + (center[1] - pt[1])*(center[1] - pt[1]) + (center[2] - pt[2])*(center[2] - pt[2]));
			if (currCD > maxCD) { maxCD = currCD; }
		}
		
		

	}

	return maxCD;
}

double vtkMDActor::GetBoundingBoxLength()
{

	
		double bounds[6];

		this->GetBounds(bounds);
		double A[3];//min
		double B[3];//max

	A[0] = bounds[0];
	A[1] = bounds[2];
	A[2] = bounds[4];
	B[0] = bounds[1];
	B[1] = bounds[3];
	B[2] = bounds[5];
	
	double diag[3];
	diag[0] = B[0] - A[0];
	diag[1] = B[1] - A[1];
	diag[2] = B[2] - A[2];
	double lengthxyz = sqrt((diag[0])*(diag[0]) + (diag[1])*(diag[1]) + (diag[2])*(diag[2]));

	return lengthxyz;
}
void vtkMDActor::SetSelected(int selected)
{
	this->Selected = selected;
	if (selected == 1)
	{
		if (this->GetMapper() != NULL)
		{
			vtkPolyDataMapper::SafeDownCast(this->GetMapper())->ScalarVisibilityOff();
		}
		//vtkPolyDataMapper::SafeDownCast(this->GetMapper())->ScalarVisibilityOff();
		/*double opac = 0.75;
		if (this->mColor[3] > 0.75)
		{
			opac = this->mColor[3];
		}
		this->SetmColor(0.5, 0.5, 0.5, opac);*/
		double opac = 0.75;
		if (this->mColor[3] > 0.75)
		{
			opac = this->mColor[3];
		}
		this->GetProperty()->SetColor(0.5, 0.5, 0.5);
		this->GetProperty()->SetOpacity(opac);
		
		if (this->GetBackfaceProperty() != NULL)
		{
			this->GetBackfaceProperty()->SetColor(.25, .25, .25);
			this->GetBackfaceProperty()->SetOpacity(opac);
		}
		//this->SetBackfaceProperty(NULL);
	}
	else
	{
		if (this->GetMapper() != NULL && mqMorphoDigCore::instance()->Getmui_ScalarVisibility() == 1)
		{
			QString none = QString("none");
			if (mqMorphoDigCore::instance()->Getmui_ActiveScalars()->Name != none)
			{
				vtkPolyDataMapper::SafeDownCast(this->GetMapper())->ScalarVisibilityOn();
			}
		}
		//vtkPolyDataMapper::SafeDownCast(this->GetMapper())->ScalarVisibilityOn();
		
		//this->SetmColor(this->mColor[0], this->mColor[1], this->mColor[2], this->mColor[3]);

		this->GetProperty()->SetColor(this->mColor[0], this->mColor[1], this->mColor[2]);
		//cout << "mColor[3](alpha) =" << this->mColor[3] << endl;
		this->GetProperty()->SetOpacity(this->mColor[3]);
		//this->SetBackfaceProperty(NULL);
		if (this->GetBackfaceProperty() != NULL)
		{
			this->GetBackfaceProperty()->SetColor(0.5*this->mColor[0], 0.5*this->mColor[1], 0.5*this->mColor[2]);
			this->GetBackfaceProperty()->SetOpacity(this->mColor[3]);
		}
		//this->SetBackfaceProperty
		//this->GetBackfaceProperty()->SetColor(0.0, 0.0, 1.0);
		//this->GetBackfaceProperty()->SetOpacity(1);
	}
}
void vtkMDActor::SetmColor(double c[4])
{
	this->SetmColor(c[0], c[1], c[2], c[3]);
}
void vtkMDActor::SetmColor(double r, double g, double b, double a)
{
	this->mColor[0] = r; 
	this->mColor[1] = g;
	this->mColor[2] = b;
	this->mColor[3] = a;
	if (this->Selected == 0)
	{
		this->GetProperty()->SetColor(this->mColor[0], this->mColor[1], this->mColor[2]);
		this->GetProperty()->SetOpacity(this->mColor[3]);
		//this->GetBackfaceProperty()->SetColor(0.0, 0.0, 1.0);
		//this->GetBackfaceProperty()->SetOpacity(1);
	}
}

void vtkMDActor::Undo(int mCount)
{

	cout << "Inside MT actor "<< this->GetName()<<" Undo " << mCount<<  endl;
	if (this->UndoRedo->UndoStack.empty())
	{
		return;
	}
	if (mCount == this->UndoRedo->UndoStack.back().UndoCount)
	{
		//cout << "Undo actor event " << this->UndoRedo->UndoStack.back().UndoCount << endl;
		// ici : faire l'appel global � undo de ce count l�!!  
		cout << "Call PopUndoStack from  MT actor" << endl;
		this->PopUndoStack();
	}
	
	
	

}
void vtkMDActor::Redo(int mCount)
{
	//cout << "Inside actor Undo, try to undo " <<mCount<< endl;
	if (this->UndoRedo->RedoStack.empty())
	{
		cout << "Redo Stack empty!" << endl;
		return;
	}
	//cout << "Youngest redo count= " << this->UndoRedo->RedoStack.back().UndoCount<< endl;

	if (mCount == this->UndoRedo->RedoStack.back().UndoCount)
	{
		//cout << "Redo actor event " << this->UndoRedo->RedoStack.back().UndoCount << endl;
		// ici : faire l'appel global � undo de ce count l�!!  
		this->PopRedoStack();
	}
}
void vtkMDActor::Erase(int mCount)
{
	if (this->UndoRedo->UndoStack.empty())
	{
		return;
	}
	int oldestCount = this->UndoRedo->UndoStack.front().UndoCount;
	if (oldestCount <= mCount)
	{
		//cout << "ERASE actor event " << oldestCount << endl;
		this->UndoRedo->UndoStack.erase(this->UndoRedo->UndoStack.begin());
	}
}

void vtkMDActor::ApplyMatrix(vtkSmartPointer<vtkMatrix4x4> Mat)

{
	vtkProp3D *prop3D = vtkProp3D::SafeDownCast(this);
	vtkTransform *newTransform = vtkTransform::New();
	newTransform->PostMultiply();
	newTransform->SetMatrix(Mat);
	prop3D->SetPosition(newTransform->GetPosition());
	prop3D->SetScale(newTransform->GetScale());
	prop3D->SetOrientation(newTransform->GetOrientation());
	newTransform->Delete();

}

void vtkMDActor::PopUndoStack()
{
	cout << "Inside MT actor "<< this->GetName()<<" PopUndoStack" << endl;
	if (this->UndoRedo->UndoStack.empty())
	{
		return;
	}
		
	vtkSmartPointer<vtkMatrix4x4> Mat = vtkSmartPointer<vtkMatrix4x4>::New();
	this->GetMatrix(Mat);	
	vtkSmartPointer<vtkMatrix4x4> SavedMat = vtkSmartPointer<vtkMatrix4x4>::New();
	SavedMat->DeepCopy(Mat);
	// Now put undo Matrix inside object : 
	vtkSmartPointer<vtkDataArray> toRestoreArray = nullptr;
	//auto toRestoreArray = vtkSmartPointer<vtkDataArray>::New();	
	vtkDataArray *toSaveArray;
	toSaveArray = NULL;
	vtkSmartPointer<vtkDataArray> savedArray = nullptr;
	//savedArray = NULL;

	if (this->UndoRedo->UndoStack.back().arrayName.isEmpty() == false)
	{ // potentially, we have to replace current scalar.
		toSaveArray = this->GetMapper()->GetInput()->GetPointData()->GetScalars(this->UndoRedo->UndoStack.back().arrayName.toStdString().c_str());;
		
		if (toSaveArray != NULL)
		{
			cout << "Here something we have scalars to save! deep copy array " << this->UndoRedo->UndoStack.back().arrayName.toStdString() << endl;
			
			int dataType = this->GetMapper()->GetInput()->GetPointData()->GetScalars(this->UndoRedo->UndoStack.back().arrayName.toStdString().c_str())->GetDataType();

			if (dataType == VTK_UNSIGNED_CHAR) {
				savedArray = vtkSmartPointer<vtkUnsignedCharArray>::New();
				savedArray->DeepCopy(toSaveArray);
				//cout << "Array" << i << " contains UNSIGNED CHARs" << endl; 
			}
			else if (dataType == VTK_UNSIGNED_INT) {
				savedArray = vtkSmartPointer<vtkUnsignedIntArray>::New();
				savedArray->DeepCopy(toSaveArray);
				//cout << "Array" << i << " contains UNSIGNED INTs" << endl; 
			}
			else if (dataType == VTK_INT) {
				savedArray = vtkSmartPointer<vtkIntArray>::New();
				savedArray->DeepCopy(toSaveArray);
				//cout << "Array" << i << " contains INTs" << endl; 
			}
			else if (dataType == VTK_FLOAT) {
				savedArray = vtkSmartPointer<vtkFloatArray>::New();
				savedArray->DeepCopy(toSaveArray);

				//cout << "Array" << i << " contains FLOATs" << endl; 
			}
			else if (dataType == VTK_DOUBLE) {
				savedArray = vtkSmartPointer<vtkDoubleArray>::New();
				savedArray->DeepCopy(toSaveArray);
				//	cout << "Array" << i << " contains DOUBLEs" << endl; 
			}
			else
			{
				//savedArray = vtkSmartPointer<vtkDoubleArray>::New();
				//savedArray->DeepCopy(toSaveArray);
			}

			

			/*savedArray= vtkSmartPointer<vtkDoubleArray>::New();
			savedArray->DeepCopy(toSaveArray);*/
		}
		
		toRestoreArray = this->UndoRedo->UndoStack.back().sauvArray;
		cout << "try to remove array" << this->UndoRedo->UndoStack.back().arrayName.toStdString() << endl;
		this->GetMapper()->GetInput()->GetPointData()->RemoveArray(this->UndoRedo->UndoStack.back().arrayName.toStdString().c_str());
		if (toRestoreArray !=nullptr)
		{
			cout << "try to restore array found in undo stack" << endl;
			this->GetMapper()->GetInput()->GetPointData()->AddArray(toRestoreArray);
			this->GetMapper()->GetInput()->GetPointData()->SetActiveScalars(mqMorphoDigCore::instance()->Getmui_ActiveScalars()->Name.toStdString().c_str());
		}

		//mqMorphoDigCore::instance()->Initmui_ExistingScalars();
		
		
		
	}




	Mat->DeepCopy(this->UndoRedo->UndoStack.back().Matrix);
	
	//std::cout << "Old Matrix: " << endl << *SavedMat << std::endl;
	//std::cout << "New Matrix: " << endl << *Mat << std::endl;

	/*vtkProp3D *prop3D = vtkProp3D::SafeDownCast(this);
	vtkTransform *newTransform = vtkTransform::New();
	newTransform->PostMultiply();
	newTransform->SetMatrix(Mat);
	prop3D->SetPosition(newTransform->GetPosition());
	prop3D->SetScale(newTransform->GetScale());
	prop3D->SetOrientation(newTransform->GetOrientation());
	newTransform->Delete();*/
	this->ApplyMatrix(Mat);


	this->GetMatrix(Mat);
	//std::cout << "Real Matrix: " << endl << *Mat << std::endl;

	
	
	double myCurrentColor[4];
	int mCurrentSelected = this->Selected;
	myCurrentColor[0] = this->mColor[0];
	myCurrentColor[1] = this->mColor[1];
	myCurrentColor[2] = this->mColor[2];
	myCurrentColor[3] = this->mColor[3];
	this->mColor[0] = this->UndoRedo->UndoStack.back().Color[0];
	this->mColor[1] = this->UndoRedo->UndoStack.back().Color[1];
	this->mColor[2] = this->UndoRedo->UndoStack.back().Color[2];
	this->mColor[3] = this->UndoRedo->UndoStack.back().Color[3];
	this->SetSelected(this->UndoRedo->UndoStack.back().Selected);
	this->Name = this->UndoRedo->UndoStack.back().Name;
	//cout << "Undo name: " << this->UndoRedo->UndoStack.back().Name;
	cout << "PopUndoStack Set Selected: " << mCurrentSelected << endl;
	this->UndoRedo->RedoStack.push_back(vtkMDActorUndoRedo::Element(SavedMat, myCurrentColor, mCurrentSelected, this->UndoRedo->UndoStack.back().UndoCount, this->UndoRedo->UndoStack.back().Name, this->UndoRedo->UndoStack.back().arrayName, savedArray));
	this->UndoRedo->UndoStack.pop_back();
	this->Modified();
}
void vtkMDActor::PopRedoStack()
{
	if (this->UndoRedo->RedoStack.empty())
	{
		return;
	}
	vtkSmartPointer<vtkMatrix4x4> Mat = vtkSmartPointer<vtkMatrix4x4>::New();
	this->GetMatrix(Mat);
	vtkSmartPointer<vtkMatrix4x4> SavedMat = vtkSmartPointer<vtkMatrix4x4>::New();
	SavedMat->DeepCopy(Mat);

	vtkSmartPointer<vtkDataArray> toRestoreArray = nullptr;
	//auto toRestoreArray = vtkSmartPointer<vtkDataArray>::New();	
	vtkDataArray *toSaveArray;
	toSaveArray = NULL;
	vtkSmartPointer<vtkDataArray> savedArray = nullptr;
	

	if (this->UndoRedo->RedoStack.back().arrayName.isEmpty() == false)
	{ // potentially, we have to replace current scalar.
		toSaveArray = this->GetMapper()->GetInput()->GetPointData()->GetScalars(this->UndoRedo->RedoStack.back().arrayName.toStdString().c_str());;

		if (toSaveArray != NULL)
		{
			savedArray = vtkSmartPointer<vtkDoubleArray>::New();
			savedArray->DeepCopy(toSaveArray);
			cout << "POPREDO: deep copy array" << this->UndoRedo->RedoStack.back().arrayName.toStdString() << endl;
		}

		toRestoreArray = this->UndoRedo->RedoStack.back().sauvArray;
		cout << "POPREDO: try to remove array" << this->UndoRedo->RedoStack.back().arrayName.toStdString() << endl;
		this->GetMapper()->GetInput()->GetPointData()->RemoveArray(this->UndoRedo->RedoStack.back().arrayName.toStdString().c_str());
		if (toRestoreArray != nullptr)
		{
			cout << "POPREDO: try to restor array found in redo stack"  << endl;
			this->GetMapper()->GetInput()->GetPointData()->AddArray(toRestoreArray);
			this->GetMapper()->GetInput()->GetPointData()->SetActiveScalars(mqMorphoDigCore::instance()->Getmui_ActiveScalars()->Name.toStdString().c_str());
		}
		//mqMorphoDigCore::instance()->Initmui_ExistingScalars();

	}

	

	// Now put redp Matrix inside object : 
	Mat->DeepCopy(this->UndoRedo->RedoStack.back().Matrix);
	/*vtkProp3D *prop3D = vtkProp3D::SafeDownCast(this);
	vtkTransform *newTransform = vtkTransform::New();
	newTransform->PostMultiply();
	newTransform->SetMatrix(Mat);
	prop3D->SetPosition(newTransform->GetPosition());
	prop3D->SetScale(newTransform->GetScale());
	prop3D->SetOrientation(newTransform->GetOrientation());
	newTransform->Delete();*/
	this->ApplyMatrix(Mat);
	
	double myCurrentColor[4];
	int mCurrentSelected = this->Selected;
	myCurrentColor[0] = this->mColor[0];
	myCurrentColor[1] = this->mColor[1];
	myCurrentColor[2] = this->mColor[2];
	myCurrentColor[3] = this->mColor[3];
	this->mColor[0] = this->UndoRedo->RedoStack.back().Color[0];
	this->mColor[1] = this->UndoRedo->RedoStack.back().Color[1];
	this->mColor[2] = this->UndoRedo->RedoStack.back().Color[2];
	this->mColor[3] = this->UndoRedo->RedoStack.back().Color[3];
	this->SetSelected(this->UndoRedo->RedoStack.back().Selected);
	cout << "PopRedoStack Set Selected: " << mCurrentSelected << endl;
	this->Name = this->UndoRedo->RedoStack.back().Name;


	this->UndoRedo->UndoStack.push_back(vtkMDActorUndoRedo::Element(SavedMat, myCurrentColor, mCurrentSelected, this->UndoRedo->RedoStack.back().UndoCount, this->UndoRedo->RedoStack.back().Name, this->UndoRedo->RedoStack.back().arrayName, savedArray));
	this->UndoRedo->RedoStack.pop_back();
	this->Modified();
}

void vtkMDActor::SaveState(int mCount, QString arrayToSave)
{
	//cout << "myActor Save Position: redostack clear." << endl;
	this->UndoRedo->RedoStack.clear();

	//int Count = 2;//MorphoDig::instance()->GetUndoCount()+1;
	int Count = mCount;
	
	// d�truit ce qui est trop vieux dans le vector!
	//to do!
	vtkSmartPointer<vtkMatrix4x4> Mat = vtkSmartPointer<vtkMatrix4x4>::New();
	this->GetMatrix(Mat);
	double myColor[4];
	int mSelected = this->Selected;
	myColor[0] = this->mColor[0];
	myColor[1] = this->mColor[1];
	myColor[2] = this->mColor[2];
	myColor[3] = this->mColor[3];
	//std::cout << "Saved Matrix: " << endl << *Mat << std::endl;
	std::string name = this->Name;
	//cout << "Save name=" << name << endl;
	vtkSmartPointer<vtkMatrix4x4> SavedMat = vtkSmartPointer<vtkMatrix4x4>::New();
	SavedMat->DeepCopy(Mat);

	vtkDataArray *myArray;
	myArray = NULL;
	vtkSmartPointer<vtkDataArray> savedArray = nullptr;
	
	if (arrayToSave.isEmpty() == false)
	{ // potentially, we have to replace current scalar.
		
		myArray = this->GetMapper()->GetInput()->GetPointData()->GetScalars(arrayToSave.toStdString().c_str());;
		cout << "Try to get " << arrayToSave.toStdString() << "array" << endl;
		if (myArray != NULL)
		{
			savedArray = vtkSmartPointer<vtkDoubleArray>::New();
			savedArray->DeepCopy(myArray);
			cout << "Have deep copied " << arrayToSave.toStdString() << "array" << endl;
		
		}
		

		//toRestoreArray = this->GetMapper()->GetInput()->GetPointData()->
		//vtkDataArray *mScalars = actor->GetMapper()->GetInput()->GetPointData()->GetScalars(oldScalarName.toStdString().c_str());
	}


	//std::cout << "Saved Matrix Copy: " << endl << *SavedMat << std::endl;
	this->UndoRedo->UndoStack.push_back(vtkMDActorUndoRedo::Element(SavedMat, myColor, mSelected, mCount, name, arrayToSave, savedArray));

}
//----------------------------------------------------------------------------
// Shallow copy of an actor.
void vtkMDActor::ShallowCopy(vtkProp *prop)
{
	vtkMDActor *f = vtkMDActor::SafeDownCast(prop);
	

	// Now do superclass
	this->vtkOpenGLActor::ShallowCopy(prop);
}


//----------------------------------------------------------------------------
void vtkMDActor::PrintSelf(ostream& os, vtkIndent indent)
{
	this->Superclass::PrintSelf(os, indent);

	os << indent << "Selected: "<< this->Selected;
	

}
// Actual actor render method.
void vtkMDActor::Render(vtkRenderer *ren, vtkMapper *mapper)
{
	this->Superclass::Render(ren, mapper);
}
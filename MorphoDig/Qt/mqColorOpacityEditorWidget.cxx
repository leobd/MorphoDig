/*=========================================================================

   Program: ParaView
   Module:    $RCSfile$

   Copyright (c) 2005,2006 Sandia Corporation, Kitware Inc.
   All rights reserved.

   ParaView is a free software; you can redistribute it and/or modify it
   under the terms of the ParaView license version 1.2.

   See License_v1.2.txt for the full ParaView license.
   A copy of this license can be obtained by contacting
   Kitware Inc.
   28 Corporate Drive
   Clifton Park, NY 12065
   USA

THIS SOFTWARE IS PROVIDED BY THE COPYRIGHT HOLDERS AND CONTRIBUTORS
``AS IS'' AND ANY EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT NOT
LIMITED TO, THE IMPLIED WARRANTIES OF MERCHANTABILITY AND FITNESS FOR
A PARTICULAR PURPOSE ARE DISCLAIMED. IN NO EVENT SHALL THE AUTHORS OR
CONTRIBUTORS BE LIABLE FOR ANY DIRECT, INDIRECT, INCIDENTAL, SPECIAL,
EXEMPLARY, OR CONSEQUENTIAL DAMAGES (INCLUDING, BUT NOT LIMITED TO,
PROCUREMENT OF SUBSTITUTE GOODS OR SERVICES; LOSS OF USE, DATA, OR
PROFITS; OR BUSINESS INTERRUPTION) HOWEVER CAUSED AND ON ANY THEORY OF
LIABILITY, WHETHER IN CONTRACT, STRICT LIABILITY, OR TORT (INCLUDING
NEGLIGENCE OR OTHERWISE) ARISING IN ANY WAY OUT OF THE USE OF THIS
SOFTWARE, EVEN IF ADVISED OF THE POSSIBILITY OF SUCH DAMAGE.

========================================================================*/
#include "mqColorOpacityEditorWidget.h"
#include "ui_mqColorOpacityEditorWidget.h"
//#include "ui_pqSavePresetOptions.h"

//#include "pqActiveObjects.h"
//#include "pqChooseColorPresetReaction.h"

//#include "pqDataRepresentation.h"

#include "mqMorphoDigCore.h"
//#include "pqPipelineRepresentation.h"
//#include "pqPropertiesPanel.h"
//#include "pqPropertyWidgetDecorator.h"
//#include "pqResetScalarRangeReaction.h"
//#include "pqSettings.h"
#include "mqTransferFunctionWidget.h"
//#include "pqUndoStack.h"
#include <vtkCommand.h>
#include <vtkDiscretizableColorTransferFunction.h>
#include <vtkEventQtSlotConnect.h>
#include <vtkNew.h>
//#include "vtkPVXMLElement.h"
#include "vtkPiecewiseFunction.h"
//#include "vtkSMCoreUtilities.h"
//#include "vtkSMPVRepresentationProxy.h"
//#include "vtkSMProperty.h"
//#include "vtkSMPropertyGroup.h"
//#include "vtkSMPropertyHelper.h"
//#include "vtkSMRenderViewProxy.h"
//#include "vtkSMSessionProxyManager.h"
//#include "vtkSMTransferFunctionPresets.h"
//#include "vtkSMTransferFunctionProxy.h"
#include <vtkVector.h>
#include <vtkWeakPointer.h>
//#include "vtk_jsoncpp.h"

#include <QDoubleValidator>
#include <QMessageBox>
#include <QPointer>
#include <QInputDialog>
#include <QTimer>
#include <QVBoxLayout>
#include <QtDebug>
#include <QHeaderView>
#include <cmath>


//-----------------------------------------------------------------------------
class mqColorOpacityEditorWidget::mqInternals
{
public:
  Ui::ColorOpacityEditorWidget Ui;
    
  //QPointer<pqColorOpacityEditorWidgetDecorator> Decorator;
  //vtkWeakPointer<vtkSMPropertyGroup> PropertyGroup;
  //vtkWeakPointer<vtkSMProxy> ScalarOpacityFunctionProxy;

  // We use this pqPropertyLinks instance to simply monitor smproperty changes.
  //pqPropertyLinks LinksForMonitoringChanges;
  vtkNew<vtkEventQtSlotConnect> IndexedLookupConnector;
  vtkNew<vtkEventQtSlotConnect> RangeConnector;

  mqInternals(mqColorOpacityEditorWidget* self) 
  {
	  cout << "mqInternals instantiation" << endl;
    this->Ui.setupUi(self);
	cout << "mqInternals instantiation : set validator" << endl;
    this->Ui.CurrentDataValue->setValidator(new QDoubleValidator(self));
    //this->Ui.mainLayout->setMargin(pqPropertiesPanel::suggestedMargin());
    // this->Ui.mainLayout->setSpacing(
    //  pqPropertiesPanel::suggestedVerticalSpacing());

   // this->Decorator = new pqColorOpacityEditorWidgetDecorator(NULL, self);
	
	this->Ui.EnableOpacityMapping->setChecked(true);
	this->Ui.Discretize->setChecked(false);
	this->Ui.currentDiscretizeValue->setButtonSymbols(QAbstractSpinBox::NoButtons);
	this->Ui.currentDiscretizeValue->setMinimum(1);
	this->Ui.currentDiscretizeValue->setMaximum(1024);
	this->Ui.currentDiscretizeValue->setValue(256);
	this->Ui.currentDiscretizeValue->setEnabled(false);
	this->Ui.discretizeSlider->setMinimum(1);
	this->Ui.discretizeSlider->setMaximum(1024);
	this->Ui.discretizeSlider->setValue(256);
	this->Ui.discretizeSlider->setEnabled(false);
	cout << "mqInternals instantiation : done" << endl;
  }

  void render()
  {
	  cout << "mqColorOpacityEditorWidget render" << endl;
	  //@@ do not think this is needed!
    /*pqDataRepresentation* repr = pqActiveObjects::instance().activeRepresentation();
    if (repr)
    {
      repr->renderViewEventually();
      return;
    }
    pqView* activeView = pqActiveObjects::instance().activeView();
    if (activeView)
    {
      activeView->render();
      return;
    }*/
    mqMorphoDigCore::instance()->Render();
  }
};

void mqColorOpacityEditorWidget::reInitialize(vtkDiscretizableColorTransferFunction *stc)
{
	this->STC = stc;
	if (stc != NULL)
	{
		if (stc->GetEnableOpacityMapping()) { this->Internals->Ui.EnableOpacityMapping->setChecked(true); }		
		else { this->Internals->Ui.EnableOpacityMapping->setChecked(false); }
		this->Internals->Ui.discretizeSlider->setValue(this->STC->GetNumberOfValues());
		this->Internals->Ui.currentDiscretizeValue->setValue(this->STC->GetNumberOfValues());
		if (stc->GetDiscretize()) { 
			this->Internals->Ui.Discretize->setChecked(true); 
			this->Internals->Ui.discretizeSlider->setDisabled(false);
			this->Internals->Ui.currentDiscretizeValue->setDisabled(false);
		}
		else{ 
			this->Internals->Ui.Discretize->setChecked(false); 
			this->Internals->Ui.discretizeSlider->setDisabled(true);
			this->Internals->Ui.currentDiscretizeValue->setDisabled(true);
		}

		this->Internals->Ui.ColorEditor->initialize(stc, true, NULL, false);
		this->initializeOpacityEditor(stc->GetScalarOpacityFunction());
		cout << "reinitialize: updateCurrentData... " << endl;
		this->updateCurrentData();
		cout << "reinitialize: updatePanel... " << endl;
		this->updatePanel();
	}
}
//-----------------------------------------------------------------------------
mqColorOpacityEditorWidget::mqColorOpacityEditorWidget(
	vtkDiscretizableColorTransferFunction *stc, QWidget* parentObject)
  : Superclass(parentObject)
  , Internals(new mqInternals(this))
{
  Ui::ColorOpacityEditorWidget& ui = this->Internals->Ui;
  this->STC = stc;
  if (stc!=NULL)
  {
	  cout << "Initialize ColorEditor widget. " << endl;
    ui.ColorEditor->initialize(stc, true, NULL, false);
   
	cout << "Initialize OpacityEditor widget " << endl;
	this->initializeOpacityEditor(stc->GetScalarOpacityFunction());
  }
  else
  {
	  cout << "STC is not null, that is something... " << endl;

  }
  //Not sure this is needed!
 /* QObject::connect(&pqActiveObjects::instance(), SIGNAL(representationChanged(pqRepresentation*)),
    this, SLOT(representationOrViewChanged()));
  QObject::connect(&pqActiveObjects::instance(), SIGNAL(viewChanged(pqView*)), this,
    SLOT(representationOrViewChanged()));*/

  QObject::connect(ui.OpacityEditor, SIGNAL(currentPointChanged(vtkIdType)), this,
    SLOT(opacityCurrentChanged(vtkIdType)));
  QObject::connect(ui.ColorEditor, SIGNAL(currentPointChanged(vtkIdType)), this,
    SLOT(colorCurrentChanged(vtkIdType)));

  QObject::connect(
    ui.ColorEditor, SIGNAL(controlPointsModified()), this, SIGNAL(xrgbPointsChanged()));
  QObject::connect(
    ui.OpacityEditor, SIGNAL(controlPointsModified()), this, SIGNAL(xvmsPointsChanged()));

  QObject::connect(
    ui.ColorEditor, SIGNAL(controlPointsModified()), this, SLOT(updateCurrentData()));
  QObject::connect(
    ui.OpacityEditor, SIGNAL(controlPointsModified()), this, SLOT(updateCurrentData()));

  QObject::connect(ui.ResetRangeToData, SIGNAL(clicked()), this, SLOT(resetRangeToData()));
  QObject::connect(ui.InvertRGB, SIGNAL(clicked()), this, SLOT(invertRGB()));
  QObject::connect(ui.InvertOpacity, SIGNAL(clicked()), this, SLOT(invertOpacity()));

  QObject::connect(ui.EnableOpacityMapping, SIGNAL(clicked()), this, SLOT(changedEnableOpacity()));
  QObject::connect(ui.Discretize, SIGNAL(clicked()), this, SLOT(changeDiscretize()));
//  QObject::connect(ui.Discretize, SIGNAL(clicked()), this, SLOT(changedDiscretize()));
  
  QObject::connect(ui.discretizeSlider, SIGNAL(valueChanged(int)), this, SLOT(changedDiscretizeValue(int)));
  QObject::connect(ui.discretizeSlider, SIGNAL(valueChanged(int)), ui.currentDiscretizeValue, SLOT(setValue(int)));
  QObject::connect(ui.currentDiscretizeValue, SIGNAL(valueChanged(int)), this, SLOT(changedDiscretizeValue(int)));
  QObject::connect(ui.currentDiscretizeValue, SIGNAL(valueChanged(int)), ui.discretizeSlider, SLOT(setValue(int)));

  //QObject::connect(ui.Discretize, SIGNAL(clicked()), this, SLOT(changedDiscretize()));

 // connect(slider, SIGNAL(valueChanged(int)), spinbox, SLOT(setValue(int)));
 // connect(slider, SIGNAL(valueChanged(int)), this, SIGNAL(valueChanged(int)));



 // QObject::connect(ui.ResetRangeToCustom, SIGNAL(clicked()), this, SLOT(resetRangeToCustom()));

 /* QObject::connect(
    ui.ResetRangeToDataOverTime, SIGNAL(clicked()), this, SLOT(resetRangeToDataOverTime()));*/

 /* QObject::connect(
    ui.ResetRangeToVisibleData, SIGNAL(clicked()), this, SLOT(resetRangeToVisibleData()));*/

/*  QObject::connect(
    ui.InvertTransferFunctions, SIGNAL(clicked()), this, SLOT(invertTransferFunctions()));*/

 // QObject::connect(ui.ChoosePreset, SIGNAL(clicked()), this, SLOT(choosePreset()));
  //QObject::connect(ui.SaveAsPreset, SIGNAL(clicked()), this, SLOT(saveAsPreset()));
  QObject::connect(ui.SaveAsCustom, SIGNAL(clicked()), this, SLOT(saveAsCustom()));
  
  

 // this->connect(
  //  ui.UseLogScaleOpacity, SIGNAL(clicked(bool)), SLOT(useLogScaleOpacityClicked(bool)));
  // if the user edits the "DataValue", we need to update the transfer function.
  QObject::connect(
    ui.CurrentDataValue, SIGNAL(textChangedAndEditingFinished()), this, SLOT(currentDataEdited()));


  /*vtkSMProperty* smproperty = smgroup->GetProperty("XRGBPoints");
  if (smproperty)
  {
    this->addPropertyLink(this, "xrgbPoints", SIGNAL(xrgbPointsChanged()), smproperty);
  }
  else
  {
    qCritical("Missing 'XRGBPoints' property. Widget may not function correctly.");
  }*/

  //ui.OpacityEditor->hide();

  /*smproperty = smgroup->GetProperty("ScalarOpacityFunction");
  if (smproperty)
  {
    this->addPropertyLink(
      this, "scalarOpacityFunctionProxy", SIGNAL(scalarOpacityFunctionProxyChanged()), smproperty);
  }*/

  /*smproperty = smgroup->GetProperty("EnableOpacityMapping");
  if (smproperty)
  {
    this->addPropertyLink(ui.EnableOpacityMapping, "checked", SIGNAL(toggled(bool)), smproperty);
  }
  else
  {
    ui.EnableOpacityMapping->hide();
    ui.UseLogScaleOpacity->hide();
  }*/

  /*smproperty = smgroup->GetProperty("UseLogScale");
  if (smproperty)
  {
    this->addPropertyLink(this, "useLogScale", SIGNAL(useLogScaleChanged()), smproperty);
    QObject::connect(ui.UseLogScale, SIGNAL(clicked(bool)), this, SLOT(useLogScaleClicked(bool)));
    // QObject::connect(ui.UseLogScale, SIGNAL(toggled(bool)),
    //  this, SIGNAL(useLogScaleChanged()));
  }
  else
  {
    ui.UseLogScale->hide();
  }*/


  // if proxy has a property named IndexedLookup, we hide this entire widget
  // when IndexedLookup is ON.

  /*if (smproxy->GetProperty("IndexedLookup"))
  {
    // we are not controlling the IndexedLookup property, we are merely
    // observing it to ensure the UI is updated correctly. Hence we don't fire
    // any signal to update the smproperty.
    this->Internals->IndexedLookupConnector->Connect(smproxy->GetProperty("IndexedLookup"),
      vtkCommand::ModifiedEvent, this, SLOT(updateIndexedLookupState()));
    this->updateIndexedLookupState();

    // Add decorator so the widget can be hidden when IndexedLookup is ON.
    this->addDecorator(this->Internals->Decorator);
  }
  */

  /*pqSettings* settings = pqApplicationCore::instance()->settings();
  if (settings)
  {
    this->Internals->Ui.AdvancedButton->setChecked(
      settings->value("showAdvancedPropertiesColorOpacityEditorWidget", false).toBool());
  }*/
  cout << "Will call updateCurrentData... " << endl;
  this->updateCurrentData();
  cout << "Will call updatePanel... " << endl;
  this->updatePanel();
}

vtkDiscretizableColorTransferFunction* mqColorOpacityEditorWidget::getSTC()
{
	return this->STC;
}
int mqColorOpacityEditorWidget::hasSTC()
{
	if (this->STC != NULL) { return 1; }
	else { return 0; }
}
//-----------------------------------------------------------------------------
mqColorOpacityEditorWidget::~mqColorOpacityEditorWidget()
{
  //pqSettings* settings = pqApplicationCore::instance()->settings();
  /*if (settings)
  {
    // save the state of the advanced button in the widget
    settings->setValue("showAdvancedPropertiesColorOpacityEditorWidget",
      this->Internals->Ui.AdvancedButton->isChecked());
  }*/

  delete this->Internals;
  this->Internals = NULL;
}

//-----------------------------------------------------------------------------
/*void pqColorOpacityEditorWidget::setScalarOpacityFunctionProxy(pqSMProxy sofProxy)
{
  pqInternals& internals = (*this->Internals);
  Ui::ColorOpacityEditorWidget& ui = internals.Ui;

  vtkSMProxy* newSofProxy = NULL;
  vtkPiecewiseFunction* pwf =
    sofProxy ? vtkPiecewiseFunction::SafeDownCast(sofProxy->GetClientSideObject()) : NULL;
  if (sofProxy && sofProxy->GetProperty("Points") && pwf)
  {
    newSofProxy = sofProxy.GetPointer();
  }
  if (internals.ScalarOpacityFunctionProxy == newSofProxy)
  {
    return;
  }
  if (internals.ScalarOpacityFunctionProxy)
  {
    // cleanup old property links.
    this->links().removePropertyLink(this, "xvmsPoints", SIGNAL(xvmsPointsChanged()),
      internals.ScalarOpacityFunctionProxy,
      internals.ScalarOpacityFunctionProxy->GetProperty("Points"));
    this->links().removePropertyLink(this, "useLogScaleOpacity",
      SIGNAL(useLogScaleOpacityChanged()), internals.ScalarOpacityFunctionProxy,
      internals.ScalarOpacityFunctionProxy->GetProperty("UseLogScale"));
  }
  internals.ScalarOpacityFunctionProxy = newSofProxy;
  if (internals.ScalarOpacityFunctionProxy)
  {
    pqDataRepresentation* repr = pqActiveObjects::instance().activeRepresentation();
    vtkSMPVRepresentationProxy* proxy = static_cast<vtkSMPVRepresentationProxy*>(repr->getProxy());

    // When representation changes, we have to initialize the opacity widget when
    // "MultiComponentsMapping" is modified
    this->Internals->RangeConnector->Disconnect();
    vtkSMProperty* msProp = proxy->GetProperty("MapScalars");
    vtkSMProperty* mcmProp = proxy->GetProperty("MultiComponentsMapping");
    if (msProp && mcmProp)
    {
      this->Internals->RangeConnector->Connect(msProp, vtkCommand::ModifiedEvent, this,
        SLOT(multiComponentsMappingChanged(vtkObject*, unsigned long, void*, void*)), pwf);

      this->Internals->RangeConnector->Connect(mcmProp, vtkCommand::ModifiedEvent, this,
        SLOT(multiComponentsMappingChanged(vtkObject*, unsigned long, void*, void*)), pwf);

      // FIXME: need to verify that repeated initializations are okay.
      this->initializeOpacityEditor(pwf);
    }

    // add new property links.
    this->links().addPropertyLink(this, "xvmsPoints", SIGNAL(xvmsPointsChanged()),
      internals.ScalarOpacityFunctionProxy,
      internals.ScalarOpacityFunctionProxy->GetProperty("Points"));
    this->links().addPropertyLink(this, "useLogScaleOpacity", SIGNAL(useLogScaleOpacityChanged()),
      internals.ScalarOpacityFunctionProxy,
      internals.ScalarOpacityFunctionProxy->GetProperty("UseLogScale"));
  }
  ui.OpacityEditor->setVisible(newSofProxy != NULL);
}*/
/*

//-----------------------------------------------------------------------------
pqSMProxy pqColorOpacityEditorWidget::scalarOpacityFunctionProxy() const
{
  return this->Internals->ScalarOpacityFunctionProxy.GetPointer();
}
*/

//-----------------------------------------------------------------------------
/*
void pqColorOpacityEditorWidget::updateIndexedLookupState()
{
  if (this->proxy()->GetProperty("IndexedLookup"))
  {
    bool val = vtkSMPropertyHelper(this->proxy(), "IndexedLookup").GetAsInt() != 0;
    this->Internals->Decorator->setHidden(val);
  }
}
*/
/*

//-----------------------------------------------------------------------------
void pqColorOpacityEditorWidget::multiComponentsMappingChanged(vtkObject* vtkNotUsed(sender),
  unsigned long vtkNotUsed(event), void* clientData, void* vtkNotUsed(callData))
{
  pqDataRepresentation* repr = pqActiveObjects::instance().activeRepresentation();
  vtkSMPVRepresentationProxy* proxy = static_cast<vtkSMPVRepresentationProxy*>(repr->getProxy());

  if (proxy->GetVolumeIndependentRanges())
  {
    // force separate color map
    vtkSMProperty* separateProperty = proxy->GetProperty("UseSeparateColorMap");
    bool sepEnabled = vtkSMPropertyHelper(separateProperty).GetAsInt() != 0;
    if (!sepEnabled)
    {
      vtkSMPropertyHelper(separateProperty).Set(1);
      vtkSMPropertyHelper helper(proxy->GetProperty("ColorArrayName"));
      proxy->SetScalarColoring(helper.GetAsString(4), vtkDataObject::POINT);
      proxy->RescaleTransferFunctionToDataRange();
      return;
    }
  }

  this->initializeOpacityEditor(static_cast<vtkPiecewiseFunction*>(clientData));
  proxy->RescaleTransferFunctionToDataRange();
}

*/
//-----------------------------------------------------------------------------
void mqColorOpacityEditorWidget::initializeOpacityEditor(vtkPiecewiseFunction* pwf)
{
  Ui::ColorOpacityEditorWidget& ui = this->Internals->Ui;
  //pqDataRepresentation* repr = pqActiveObjects::instance().activeRepresentation();
  //vtkSMPVRepresentationProxy* proxy = static_cast<vtkSMPVRepresentationProxy*>(repr->getProxy());
  /*vtkScalarsToColors* stc = nullptr;
  vtkSMProperty* separateProperty = proxy->GetProperty("UseSeparateColorMap");
  bool sepEnabled = vtkSMPropertyHelper(separateProperty).GetAsInt() != 0;
  if (!proxy->GetVolumeIndependentRanges() || !sepEnabled)
  {
    stc = vtkScalarsToColors::SafeDownCast(this->proxy()->GetClientSideObject());
  }*/
  cout << "mqColorOpacityEditorWidget initializeOpacityEditor" << endl;
  ui.OpacityEditor->initialize(this->STC, false, pwf, true);
 // ui.OpacityEditor->initialize(NULL, false, pwf, true);
}

//-----------------------------------------------------------------------------
void mqColorOpacityEditorWidget::opacityCurrentChanged(vtkIdType index)
{
	cout << "mqColorOpacityEditorWidget opacityCurrentChanged" << endl;
  if (index != -1)
  {
    Ui::ColorOpacityEditorWidget& ui = this->Internals->Ui;
    ui.ColorEditor->setCurrentPoint(-1);
  }
  this->updateCurrentData();
}

//-----------------------------------------------------------------------------
void mqColorOpacityEditorWidget::colorCurrentChanged(vtkIdType index)
{
	cout << "mqColorOpacityEditorWidget colorCurrentChanged" << endl;
  if (index != -1)
  {
    Ui::ColorOpacityEditorWidget& ui = this->Internals->Ui;
    ui.OpacityEditor->setCurrentPoint(-1);
  }
  this->updateCurrentData();
}

//-----------------------------------------------------------------------------
void mqColorOpacityEditorWidget::updatePanel()
{
	cout << "mqColorOpacityEditorWidget updatePanel" << endl;
  if (this->Internals)
  {
    /*bool advancedVisible = this->Internals->Ui.AdvancedButton->isChecked();
    this->Internals->Ui.ColorLabel->setVisible(advancedVisible);
    this->Internals->Ui.ColorTable->setVisible(advancedVisible);
    this->Internals->Ui.OpacityLabel->setVisible(advancedVisible);
    this->Internals->Ui.OpacityTable->setVisible(advancedVisible);*/
  }
}

//-----------------------------------------------------------------------------
void mqColorOpacityEditorWidget::updateCurrentData()
{
	cout << "mqColorOpacityEditorWidget updateCurrentData" << endl;
  vtkDiscretizableColorTransferFunction* stc =
    vtkDiscretizableColorTransferFunction::SafeDownCast(this->STC);
  
  //vtkSMProxy* pwfProxy = this->scalarOpacityFunctionProxy();
  //vtkPiecewiseFunction* pwf = NULL;
  //�if (stc !=NULL)
  //vtkPiecewiseFunction* pwf = vtkPiecewiseFunction::SafeDownCast(this->STC); 
  vtkPiecewiseFunction* pwf = this->STC->GetScalarOpacityFunction(); 
    //pwfProxy ? vtkPiecewiseFunction::SafeDownCast(pwfProxy->GetClientSideObject()) : NULL;
  
  Ui::ColorOpacityEditorWidget& ui = this->Internals->Ui;

  if (ui.ColorEditor->currentPoint() >= 0 && stc)
  {
	  cout << "Case 1" << endl;
    double xrgbms[6];
    stc->GetNodeValue(ui.ColorEditor->currentPoint(), xrgbms);
    ui.CurrentDataValue->setText(QString::number(xrgbms[0]));

    // Don't enable widget for first/last control point. For those, users must
    // rescale the transfer function manually
    ui.CurrentDataValue->setEnabled(ui.ColorEditor->currentPoint() != 0 &&
      ui.ColorEditor->currentPoint() != (ui.ColorEditor->numberOfControlPoints() - 1));




  }
  else if (ui.OpacityEditor->currentPoint() >= 0 && pwf)
  {
	  cout << "Case 2" << endl;
    double xvms[4];
    pwf->GetNodeValue(ui.OpacityEditor->currentPoint(), xvms);
    ui.CurrentDataValue->setText(QString::number(xvms[0]));

    // Don't enable widget for first/last control point. For those, users must
    // rescale the transfer function manually
    ui.CurrentDataValue->setEnabled(ui.OpacityEditor->currentPoint() != 0 &&
      ui.OpacityEditor->currentPoint() != (ui.OpacityEditor->numberOfControlPoints() - 1));
  }
  else
  {
	  cout << "Case 3" << endl;
    ui.CurrentDataValue->setEnabled(false);
  }
  
}

//-----------------------------------------------------------------------------
QList<QVariant> mqColorOpacityEditorWidget::xrgbPoints() const
{
	cout << "mqColorOpacityEditorWidget xrgbPoints" << endl;
	
  vtkDiscretizableColorTransferFunction* stc =
    vtkDiscretizableColorTransferFunction::SafeDownCast(this->STC);
  QList<QVariant> values;
  for (int cc = 0; stc != NULL && cc < stc->GetSize(); cc++)
  {
    double xrgbms[6];
    stc->GetNodeValue(cc, xrgbms);
    vtkVector<double, 4> value;
    values.push_back(xrgbms[0]);
    values.push_back(xrgbms[1]);
    values.push_back(xrgbms[2]);
    values.push_back(xrgbms[3]);
  }

  return values;
}

//-----------------------------------------------------------------------------
QList<QVariant> mqColorOpacityEditorWidget::xvmsPoints() const
{
	cout << "mqColorOpacityEditorWidget xvmsPoints" << endl;
  //vtkSMProxy* pwfProxy = this->scalarOpacityFunctionProxy();
	vtkPiecewiseFunction* pwf = this->STC->GetScalarOpacityFunction();
    //pwfProxy ? vtkPiecewiseFunction::SafeDownCast(pwfProxy->GetClientSideObject()) : NULL;

  QList<QVariant> values;
  for (int cc = 0; pwf != NULL && cc < pwf->GetSize(); cc++)
  {
    double xvms[4];
    pwf->GetNodeValue(cc, xvms);
    values.push_back(xvms[0]);
    values.push_back(xvms[1]);
    values.push_back(xvms[2]);
    values.push_back(xvms[3]);
  }
  return values;
}

/*
//-----------------------------------------------------------------------------
bool pqColorOpacityEditorWidget::useLogScale() const
{
  return this->Internals->Ui.UseLogScale->isChecked();
}
*/

/*

//-----------------------------------------------------------------------------
bool pqColorOpacityEditorWidget::useLogScaleOpacity() const
{
  return this->Internals->Ui.UseLogScaleOpacity->isChecked();
}

*/

/*
//-----------------------------------------------------------------------------
void pqColorOpacityEditorWidget::setUseLogScale(bool val)
{
  Ui::ColorOpacityEditorWidget& ui = this->Internals->Ui;
  ui.UseLogScale->setChecked(val);
}
*/

/*
//-----------------------------------------------------------------------------
void pqColorOpacityEditorWidget::setUseLogScaleOpacity(bool val)
{
  Ui::ColorOpacityEditorWidget& ui = this->Internals->Ui;
  ui.UseLogScaleOpacity->setChecked(val);
}

*/

/*
//-----------------------------------------------------------------------------
void pqColorOpacityEditorWidget::useLogScaleClicked(bool log_space)
{
  if (log_space)
  {
    // Make sure both color and opacity are remapped if needed:
    this->prepareRangeForLogScaling();
    vtkSMTransferFunctionProxy::MapControlPointsToLogSpace(this->proxy());
  }
  else
  {
    vtkSMTransferFunctionProxy::MapControlPointsToLinearSpace(this->proxy());
  }

  this->Internals->Ui.ColorEditor->SetLogScaleXAxis(log_space);

  emit this->useLogScaleChanged();
}
*/

/*
//-----------------------------------------------------------------------------
void pqColorOpacityEditorWidget::useLogScaleOpacityClicked(bool log_space)
{
  vtkSMProxy* opacityProxy = this->Internals->ScalarOpacityFunctionProxy;
  if (log_space)
  {
    // Make sure both color and opacity are remapped if needed:
    this->prepareRangeForLogScaling();
    vtkSMTransferFunctionProxy::MapControlPointsToLogSpace(opacityProxy);
  }
  else
  {
    vtkSMTransferFunctionProxy::MapControlPointsToLinearSpace(opacityProxy);
  }

  this->Internals->Ui.OpacityEditor->SetLogScaleXAxis(log_space);

  emit this->useLogScaleOpacityChanged();
}
*/

//-----------------------------------------------------------------------------
void mqColorOpacityEditorWidget::setXvmsPoints(const QList<QVariant>& values)
{
  Q_UNUSED(values);
  // Since the vtkPiecewiseFunction connected to the widget is directly obtained
  // from the proxy, we don't need to do anything here. The widget will be
  // updated when the proxy updates.
}

//-----------------------------------------------------------------------------
void mqColorOpacityEditorWidget::setXrgbPoints(const QList<QVariant>& values)
{
  Q_UNUSED(values);
  // Since the vtkColorTransferFunction connected to the widget is directly obtained
  // from the proxy, we don't need to do anything here. The widget will be
  // updated when the proxy updates.
}

//-----------------------------------------------------------------------------

void mqColorOpacityEditorWidget::currentDataEdited()
{
	cout << "mqColorOpacityEditorWidget currentDataEdited" << endl;
  vtkDiscretizableColorTransferFunction* stc =
    vtkDiscretizableColorTransferFunction::SafeDownCast(this->STC);
  //vtkSMProxy* pwfProxy = this->scalarOpacityFunctionProxy();
  vtkPiecewiseFunction* pwf = this->STC->GetScalarOpacityFunction();
    //pwfProxy ? vtkPiecewiseFunction::SafeDownCast(pwfProxy->GetClientSideObject()) : NULL;

  Ui::ColorOpacityEditorWidget& ui = this->Internals->Ui;
  if (ui.ColorEditor->currentPoint() >= 0 && stc)
  {
    ui.ColorEditor->setCurrentPointPosition(ui.CurrentDataValue->text().toDouble());
  }
  else if (ui.OpacityEditor->currentPoint() >= 0 && pwf)
  {
    ui.OpacityEditor->setCurrentPointPosition(ui.CurrentDataValue->text().toDouble());
  }

  this->updateCurrentData();
}

//-----------------------------------------------------------------------------
/*void pqColorOpacityEditorWidget::representationOrViewChanged()
{
  pqDataRepresentation* repr = pqActiveObjects::instance().activeRepresentation();
  bool hasRepresentation = repr != NULL;
  pqView* activeView = pqActiveObjects::instance().activeView();
  bool hasView = activeView != NULL;

  Ui::ColorOpacityEditorWidget& ui = this->Internals->Ui;
  ui.ResetRangeToData->setEnabled(hasRepresentation);
  ui.ResetRangeToDataOverTime->setEnabled(hasRepresentation);
  ui.ResetRangeToVisibleData->setEnabled(hasRepresentation && hasView);

  vtkSMProxy* pwfProxy = this->scalarOpacityFunctionProxy();
  vtkPiecewiseFunction* pwf =
    pwfProxy ? vtkPiecewiseFunction::SafeDownCast(pwfProxy->GetClientSideObject()) : nullptr;

  // When representation changes, we have to initialize the opacity widget when
  // "MultiComponentsMapping" is modified
  this->Internals->RangeConnector->Disconnect();
  vtkSMProperty* msProp = repr->getProxy()->GetProperty("MapScalars");
  vtkSMProperty* mcmProp = repr->getProxy()->GetProperty("MultiComponentsMapping");
  if (msProp && mcmProp)
  {
    this->Internals->RangeConnector->Connect(msProp, vtkCommand::ModifiedEvent, this,
      SLOT(multiComponentsMappingChanged(vtkObject*, unsigned long, void*, void*)), pwf);

    this->Internals->RangeConnector->Connect(mcmProp, vtkCommand::ModifiedEvent, this,
      SLOT(multiComponentsMappingChanged(vtkObject*, unsigned long, void*, void*)), pwf);

    this->initializeOpacityEditor(pwf);
  }
}*/

//-----------------------------------------------------------------------------


void mqColorOpacityEditorWidget::invertRGB()
{
	if (this->STC != NULL)
	{
		mqMorphoDigCore::instance()->invertRGB(this->STC);
		this->reInitialize(STC);
	}
}
void mqColorOpacityEditorWidget::invertOpacity()
{
	if (this->STC != NULL)
	{
		mqMorphoDigCore::instance()->invertOpacity(this->STC);
		this->reInitialize(STC);
	}
}

//-----------------------------------------------------------------------------
void mqColorOpacityEditorWidget::resetRangeToData()
{
	//this->Ui->suggestedMax->setValue(mqMorphoDigCore::instance()->GetSuggestedScalarRangeMax());
	//this->Ui->suggestedMin->setValue(mqMorphoDigCore::instance()->GetSuggestedScalarRangeMin());
  // passing in NULL ensure pqResetScalarRangeReaction simply uses active representation.
  //if (pqResetScalarRangeReaction::resetScalarRangeToData(NULL))
  //{
   // this->Internals->render();
	mqMorphoDigCore::instance()->UpdateLookupTablesToData();
    emit this->changeFinished();
  //}
}

void mqColorOpacityEditorWidget::changeDiscretize() 
{
	if (this->STC != NULL)
	{
		if (this->Internals->Ui.Discretize->isChecked()) {
			this->Internals->Ui.Discretize->setChecked(true);
			this->STC->DiscretizeOn();
			cout << "Discretize is on and STC has " << STC->GetNumberOfValues() << endl;
			this->Internals->Ui.discretizeSlider->setDisabled(false);
			this->Internals->Ui.currentDiscretizeValue->setDisabled(false);
		}
		else {
			this->STC->DiscretizeOff();
			
			this->Internals->Ui.discretizeSlider->setDisabled(true);
			this->Internals->Ui.currentDiscretizeValue->setDisabled(true);
		}
	}
}
void  mqColorOpacityEditorWidget::changedDiscretizeValue(int value)
{
	//this->Internals->Ui.currentDiscretizeValue->setValue(value);
	if (this->STC != NULL)
	{
		this->STC->SetNumberOfValues(value);
	}
}
void mqColorOpacityEditorWidget::changedEnableOpacity()
{
	cout << "change EnableOpacity" << endl;
	if (this->STC != NULL)
	{
		if (this->Internals->Ui.EnableOpacityMapping->isChecked())
		{
			this->STC->EnableOpacityMappingOn();
		}
		else
		{
			this->STC->EnableOpacityMappingOff();
		}
	}
	


}

//-----------------------------------------------------------------------------

/*
void pqColorOpacityEditorWidget::resetRangeToDataOverTime()
{
  // passing in NULL ensure pqResetScalarRangeReaction simply uses active representation.
  if (pqResetScalarRangeReaction::resetScalarRangeToDataOverTime(NULL))
  {
    this->Internals->render();
    emit this->changeFinished();
  }
}*/

/*
//-----------------------------------------------------------------------------
void pqColorOpacityEditorWidget::resetRangeToVisibleData()
{
  pqPipelineRepresentation* repr =
    qobject_cast<pqPipelineRepresentation*>(pqActiveObjects::instance().activeRepresentation());
  if (!repr)
  {
    qCritical() << "No active representation.";
    return;
  }

  vtkSMPVRepresentationProxy* repProxy = vtkSMPVRepresentationProxy::SafeDownCast(repr->getProxy());
  if (!repProxy)
  {
    return;
  }

  pqView* activeView = pqActiveObjects::instance().activeView();
  if (!activeView)
  {
    qCritical() << "No active view.";
    return;
  }

  vtkSMRenderViewProxy* rvproxy = vtkSMRenderViewProxy::SafeDownCast(activeView->getViewProxy());
  if (!rvproxy)
  {
    return;
  }

  BEGIN_UNDO_SET("Reset transfer function ranges using visible data");
  vtkSMPVRepresentationProxy::RescaleTransferFunctionToVisibleRange(repProxy, rvproxy);
  this->Internals->render();
  END_UNDO_SET();
}

*/
/*
//-----------------------------------------------------------------------------
void pqColorOpacityEditorWidget::resetRangeToCustom()
{
  if (pqResetScalarRangeReaction::resetScalarRangeToCustom(this->proxy()))
  {
    this->Internals->render();
    emit this->changeFinished();
  }
}
*/
//-------
/*
----------------------------------------------------------------------
void pqColorOpacityEditorWidget::invertTransferFunctions()
{
  BEGIN_UNDO_SET("Invert transfer function");
  vtkSMTransferFunctionProxy::InvertTransferFunction(this->proxy());

  emit this->changeFinished();
  // We don't invert the opacity function, for now.
  END_UNDO_SET();
}*/

/*
//-----------------------------------------------------------------------------
void pqColorOpacityEditorWidget::choosePreset(const char* presetName)
{
  QAction* tmp = new QAction(NULL);
  pqChooseColorPresetReaction* ccpr = new pqChooseColorPresetReaction(tmp, false);
  ccpr->setTransferFunction(this->proxy());
  this->connect(ccpr, SIGNAL(presetApplied()), SLOT(presetApplied()));
  ccpr->choosePreset(presetName);
  delete ccpr;
  delete tmp;
}
*/
/*
//-----------------------------------------------------------------------------
void pqColorOpacityEditorWidget::presetApplied()
{
  emit this->changeFinished();

  // Assume the color map and opacity have changed and refresh
  this->Internals->ColorTableModel.refresh();
  this->Internals->OpacityTableModel.refresh();
}
*/
void mqColorOpacityEditorWidget::saveAsCustom()
{
	QInputDialog *giveNameDialog = new QInputDialog();
	bool dialogResult;
	QString newColormapName = giveNameDialog->getText(0, "Color map name", "Name:", QLineEdit::Normal,
		"Custom_color_map", &dialogResult);
	if (dialogResult)
	{
		
		cout << "color map given:" << newColormapName.toStdString() << endl;
		if (mqMorphoDigCore::instance()->colorMapNameAlreadyExists(newColormapName) == 1)
		{
			QMessageBox msgBox;
			msgBox.setText("Can't save custom map : name already exists.");
			msgBox.exec();
			return;
		}
		if (newColormapName.length()==0)
		{
			QMessageBox msgBox;
			msgBox.setText("Can't save custom map: name length =0.");
			msgBox.exec();
			return;
		}
		mqMorphoDigCore::instance()->createCustomColorMap(newColormapName, this->STC); 
		emit this->changeFinished();
		//this->UpdateUI();
	}
	else
	{
		cout << "cancel " << endl;
	}

}

/*

//-----------------------------------------------------------------------------
void pqColorOpacityEditorWidget::saveAsPreset()
{
  QDialog dialog(this);
  Ui::SavePresetOptions ui;
  ui.setupUi(&dialog);
  ui.saveOpacities->setEnabled(this->scalarOpacityFunctionProxy() != NULL);
  ui.saveOpacities->setChecked(ui.saveOpacities->isEnabled());
  ui.saveAnnotations->setVisible(false);

  // For now, let's not provide an option to not save colors. We'll need to fix
  // the pqPresetToPixmap to support rendering only opacities.
  ui.saveColors->setChecked(true);
  ui.saveColors->setEnabled(false);
  ui.saveColors->hide();

  if (dialog.exec() != QDialog::Accepted)
  {
    return;
  }

  Q_ASSERT(ui.saveColors->isChecked());
  Json::Value preset = vtkSMTransferFunctionProxy::GetStateAsPreset(this->proxy());

  if (ui.saveOpacities->isChecked())
  {
    Json::Value opacities =
      vtkSMTransferFunctionProxy::GetStateAsPreset(this->scalarOpacityFunctionProxy());
    if (opacities.isMember("Points"))
    {
      preset["Points"] = opacities["Points"];
    }
  }

  vtkStdString presetName;
  {
    // This scoping is necessary to ensure that the vtkSMTransferFunctionPresets
    // saves the new preset to the "settings" before the choosePreset dialog is
    // shown.
    vtkNew<vtkSMTransferFunctionPresets> presets;
    presetName = presets->AddUniquePreset(preset);
  }
  this->choosePreset(presetName);
}
*/
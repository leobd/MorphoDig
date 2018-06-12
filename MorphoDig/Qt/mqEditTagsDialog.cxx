/*=========================================================================

   Program: MorphoDig
   Module:    $RCS $

 
=========================================================================*/

#include "mqEditTagsDialog.h"
#include "ui_mqEditTagsDialog.h"
#include "mqSetName.h"
#include "MorphoDigVersion.h"
#include "mqMorphoDigCore.h"
#include "mqSaveMAPDialogReaction.h"
#include "mqUndoStack.h"
#include "QDoubleSlider.h"
#include "mqColorChooserButton.h"
#include "mqColorOpacityEditorWidget.h"
#include "mqTransferFunctionWidget.h"
#include "mqMinimalWidget.h"
#include "vtkLMActor.h"
#include "vtkLMActorCollection.h"
#include <vtkLookupTable.h>
// we actually do not need glew...
//#include <GL/glew.h>
#include <QApplication>
#include <QMessageBox>
#include <QInputDialog>
#include <QFile>
#include <QTableWidgetItem>
#include <QRadioButton>
#include <QFileDialog>
#include <QCheckBox>
#include <QComboBox>
#include <QHeaderView>


#include <sstream>

#define NORMAL_NODE 0
#define STARTING_NODE 1
#define MILESTONE_NODE 2
#define CONNECT_NODE 3


// Check windows
#if _WIN32 || _WIN64
#if _WIN64
#define ENVIRONMENT "64"
#else
#define ENVIRONMENT "32"
#endif
#endif

// Check GCC
#if __GNUC__
#if __x86_64__ || __ppc64__
#define ENVIRONMENT "64"
#else
#define ENVIRONMENT "32"
#endif
#endif

//-----------------------------------------------------------------------------
mqEditTagsDialog::mqEditTagsDialog(QWidget* Parent)
  : QDialog(Parent)
  , Ui(new Ui::mqEditTagsDialog())
{

	
	//1 populate active scalar combo box, and check which tags is the active one!
	this->Ui->setupUi(this);
	this->setObjectName("mqEditTagsDialog");
	
	cout << "Try this0" << endl;
	QHeaderView *header = this->Ui->tableWidget->horizontalHeader();
	cout << "Try this1" << endl;
	header->setSectionResizeMode(QHeaderView::Stretch);
	cout << "Try this2" << endl;

	/*header->setSectionResizeMode(1, QHeaderView::Interactive);
	cout << "Try this3" << endl;
		header->setSectionResizeMode(2, QHeaderView::Interactive);
		cout << "Try this4" << endl;
	header->setSectionResizeMode(3, QHeaderView::Interactive);
	cout << "Try this5" << endl;*/
	// Can't select lines
	//this->Ui->tableWidget->setSelectionMode(QAbstractItemView::NoSelection);

	// Test: fill the list


	this->Ui->tableWidget->setColumnCount(5);
	this->Ui->tableWidget->setRowCount(255);
	cout << "Try this6" << endl;
	
	
	this->Ui->comboActiveTags->setSizeAdjustPolicy(QComboBox::AdjustToContents);
	this->Ui->comboTagMaps->setSizeAdjustPolicy(QComboBox::AdjustToContents);

	connect(mqMorphoDigCore::instance(), SIGNAL(tagMapsChanged()), this, SLOT(slotRefreshTagMaps())); // when loading a new .tag file or when deleting tag maps
	connect(mqMorphoDigCore::instance(), SIGNAL(existingScalarsChanged()), this, SLOT(slotRefreshComboTags()));
	connect(mqMorphoDigCore::instance(), SIGNAL(activeScalarChanged()), this, SLOT(slotRefreshComboTags()));
	connect(this->Ui->comboActiveTags, SIGNAL(activated(int)), this, SLOT(slotActiveTagsChanged(int)));
	connect(this->Ui->comboTagMaps, SIGNAL(activated(int)), this, SLOT(slotActiveTagMapChanged(int)));
	connect(this->Ui->pushRemoveTags, SIGNAL(pressed()), this, SLOT(slotRemoveTags()));

	this->Ui->editTagMap->setDisabled(true);
	this->Ui->deleteTagMap->setDisabled(true);
	QAction* exportAction = new QAction(tr("&Export"), this);
	exportAction->setToolTip(tr("Toggles recording."));
	this->Ui->exportTagMap->addAction(exportAction);
	this->Ui->exportTagMap->setDefaultAction(exportAction);
	QIcon icon;
	icon.addFile(QStringLiteral(":/Icons/ExportMap22.png"), QSize(), QIcon::Normal, QIcon::Off);
	exportAction->setIcon(icon);
	//TOTO : create an export tag map...
	//new mqSaveTagMAPDialogReaction(exportAction);
	connect(this->Ui->editTagMap, SIGNAL(pressed()), this, SLOT(slotEditTagMapName()));
	connect(this->Ui->deleteTagMap, SIGNAL(pressed()), this, SLOT(slotDeleteTagMap()));


	/*	
			
	
	
	*/

}




//-----------------------------------------------------------------------------
mqEditTagsDialog::~mqEditTagsDialog()
{

 //depending on what is 
	
  delete this->Ui;
}



void mqEditTagsDialog::RefreshTagMapTable()
{
	this->Ui->tableWidget->clear();

	// f(Active Tag Map)
	QString currentTagMapName = this->Ui->comboTagMaps->currentText(); 
	int numTags = 0;
	std::vector<std::string> tagNames;
	vtkSmartPointer<vtkLookupTable> TagMap;
	for (int i = 0; i < mqMorphoDigCore::instance()->Getmui_ExistingTagMaps()->Stack.size(); i++)
	{
		QString myExisingTagMapName = mqMorphoDigCore::instance()->Getmui_ExistingTagMaps()->Stack.at(i).Name;
		if (currentTagMapName == myExisingTagMapName)
		{

			
			 numTags = mqMorphoDigCore::instance()->Getmui_ExistingTagMaps()->Stack.at(i).numTags;
			 tagNames = mqMorphoDigCore::instance()->Getmui_ExistingTagMaps()->Stack.at(i).tagNames;

			 TagMap = mqMorphoDigCore::instance()->Getmui_ExistingTagMaps()->Stack.at(i).TagMap;


		}
	}
	QLabel *nom;
	QRadioButton *radio;
	QToolButton *clear;
	mqColorChooserButton *colorbutton;
	QToolButton *fill;
	if (numTags > 0)
	{
		this->Ui->tableWidget->setRowCount(numTags);
	}
	for (int i = 0; i < numTags; i++) {
		nom = new QLabel();
		nom->setText(tagNames.at(i).c_str());
		

		radio = new QRadioButton();
		clear = new QToolButton();
		fill = new QToolButton();
		colorbutton = new mqColorChooserButton(this->Ui->tableWidget);
		QColor myColor;
		myColor.setRedF(TagMap->GetTableValue(i)[0]);
		myColor.setGreenF(TagMap->GetTableValue(i)[1]);
		myColor.setBlueF(TagMap->GetTableValue(i)[2]);
		/* this->setObjectName("mqColorDialog");

  QColor myBGColor;
  QColor myBGColor2;
  double meshcolor[4];
  double backgroundcolor[3];
  double backgroundcolor2[3];
  this->Ui->meshColorButton->setShowAlphaChannel(true);
  mqMorphoDigCore::instance()->Getmui_MeshColor(meshcolor);
  mqMorphoDigCore::instance()->Getmui_BackGroundColor(backgroundcolor);
  mqMorphoDigCore::instance()->Getmui_BackGroundColor2(backgroundcolor2);
  myColor.setRedF(meshcolor[0]);
  myColor.setGreenF(meshcolor[1]);
  myColor.setBlueF(meshcolor[2]);
  myColor.setAlphaF(meshcolor[3]);
  myBGColor.setRedF(backgroundcolor[0]);
  myBGColor.setGreenF(backgroundcolor[1]);
  myBGColor.setBlueF(backgroundcolor[2]);
  
  myBGColor2.setRedF(backgroundcolor2[0]);
  myBGColor2.setGreenF(backgroundcolor2[1]);
  myBGColor2.setBlueF(backgroundcolor2[2]);



  this->Ui->meshColorButton->setChosenColor(myColor);*/
		colorbutton->setChosenColor(myColor);
		this->Ui->tableWidget->setCellWidget(i, 0, colorbutton);
		QTableWidgetItem *item = new QTableWidgetItem;
		item->setFlags(item->flags() | Qt::ItemIsEditable);
		item->setText(tagNames.at(i).c_str());
		this->Ui->tableWidget->setItem(i, 1, item);
		//this->Ui->tableWidget->setCellWidget(i, 1, nom);

		this->Ui->tableWidget->setCellWidget(i, 2, radio);
		this->Ui->tableWidget->setCellWidget(i, 3, clear);
		this->Ui->tableWidget->setCellWidget(i, 4, fill);
		
	}

}
void mqEditTagsDialog::UpdateUI()
{
	/*int color_scale_id = MT->GetColorScaleId();
	
	currentMax->value(MT->Get_sc_max());
	currentMin->value(MT->Get_sc_min());
*/

	//1 populate comboActiveTags

	this->RefreshComboActiveTags();

	// 2 populate comboTagMaps
	this->RefreshComboTagMaps();
	// 3 populate Tag Map Table
	this->RefreshTagMapTable();
	
	
}
void mqEditTagsDialog::slotRefreshTagMaps()
{
	cout << "slotRefreshTagMaps" << endl;
	this->RefreshComboTagMaps();
}
void mqEditTagsDialog::slotEditTagMapName()
{
	QString ActiveTagMap = this->Ui->comboTagMaps->currentText();
	for (int i = 0; i < mqMorphoDigCore::instance()->Getmui_ExistingTagMaps()->Stack.size(); i++)
	{
		int iscustom = mqMorphoDigCore::instance()->Getmui_ExistingTagMaps()->Stack.at(i).isCustom;
		QString myExisingTagMapName = mqMorphoDigCore::instance()->Getmui_ExistingTagMaps()->Stack.at(i).Name;
		if (ActiveTagMap == myExisingTagMapName && iscustom)
		{
			QInputDialog *giveNameDialog = new QInputDialog();
			bool dialogResult;
			QString newTagMapName = giveNameDialog->getText(0, "Change color map name", "Name:", QLineEdit::Normal,
				myExisingTagMapName, &dialogResult);
			if (dialogResult)
			{

				cout << "new color map given:" << newTagMapName.toStdString() << endl;
				if (mqMorphoDigCore::instance()->tagMapNameAlreadyExists(newTagMapName) == 1)
				{
					QMessageBox msgBox;
					msgBox.setText("Can't change custom map name : name already exists.");
					msgBox.exec();
					return;
				}
				if (newTagMapName.length() == 0)
				{
					QMessageBox msgBox;
					msgBox.setText("Can't save custom map: name length =0.");
					msgBox.exec();
					return;
				}
				mqMorphoDigCore::instance()->Getmui_ExistingTagMaps()->Stack.at(i).Name = newTagMapName;
				mqMorphoDigCore::instance()->Setmui_ActiveTagMap(newTagMapName, mqMorphoDigCore::instance()->Getmui_ExistingTagMaps()->Stack.at(i).numTags, mqMorphoDigCore::instance()->Getmui_ExistingTagMaps()->Stack.at(i).tagNames, mqMorphoDigCore::instance()->Getmui_ExistingTagMaps()->Stack.at(i).TagMap);
				this->RefreshComboTagMaps();
				//mqMorphoDigCore::instance()->createCustomTagMap(newTagMapName, this->STC);				
				//this->UpdateUI();
			}
			else
			{
				cout << "cancel " << endl;
			}
			
			//this->mTagMap->reInitialize(mqMorphoDigCore::instance()->Getmui_ExistingTagMaps()->Stack.at(i).TagMap);



		}
	}

}

void mqEditTagsDialog::slotDeleteTagMap()
{

	/*QString ActiveTagMap = this->Ui->comboTagMaps->currentText();
	for (int i = 0; i < mqMorphoDigCore::instance()->Getmui_ExistingTagMaps()->Stack.size(); i++)
	{
		int iscustom = mqMorphoDigCore::instance()->Getmui_ExistingTagMaps()->Stack.at(i).isCustom;
		QString myExisingTagMapName = mqMorphoDigCore::instance()->Getmui_ExistingTagMaps()->Stack.at(i).Name;
		if (ActiveTagMap == myExisingTagMapName && iscustom)
		{
		
			mqMorphoDigCore::instance()->deleteTagMap(i);
			//mqMorphoDigCore::instance()->Getmui_ExistingTagMaps()->Stack.erase(mqMorphoDigCore::instance()->Getmui_ExistingTagMaps()->Stack.begin() + i);
			//mqMorphoDigCore::instance()->Setmui_ActiveTagMap(newTagMapName, mqMorphoDigCore::instance()->Getmui_ExistingTagMaps()->Stack.at(i).TagMap);

			this->RefreshComboTagMaps();
			this->mTagMap->reInitialize(mqMorphoDigCore::instance()->Getmui_ActiveTagMap()->TagMap);
			mqMorphoDigCore::instance()->Render();
				//mqMorphoDigCore::instance()->createCustomTagMap(newTagMapName, this->STC);				
				//this->UpdateUI();
			

			//this->mTagMap->reInitialize(mqMorphoDigCore::instance()->Getmui_ExistingTagMaps()->Stack.at(i).TagMap);



		}
	}*/
}

void mqEditTagsDialog::RefreshComboActiveTags()
{
	
	this->Ui->comboActiveTags->clear();
	ExistingScalars *MyList = mqMorphoDigCore::instance()->Getmui_ExistingScalars();
	for (int i = 0; i < MyList->Stack.size(); i++)
	{
		if ((MyList->Stack.at(i).DataType == VTK_INT || MyList->Stack.at(i).DataType == VTK_UNSIGNED_INT) && MyList->Stack.at(i).NumComp == 1)
		{
			this->Ui->comboActiveTags->addItem(MyList->Stack.at(i).Name);
		}

	}
	QString myActiveScalars = mqMorphoDigCore::instance()->Getmui_ActiveScalars()->Name;
	cout << "DIAL myActiveTags " << myActiveScalars.toStdString() << endl;
	int exists = -1;
	for (int i = 0; i < this->Ui->comboActiveTags->count(); i++)
	{
		QString myScalar = this->Ui->comboActiveTags->itemText(i);
		if (myScalar == myActiveScalars)
		{
			cout << "DIAL found in list!!!!! " << myScalar.toStdString() << endl;
			exists = i;

		}

	}
	if (exists > -1) {
		cout << "DIAL Now current index of combo box is " << exists << endl;
		this->Ui->comboActiveTags->setCurrentIndex(exists);


	}
	
}


void mqEditTagsDialog::RefreshComboTagMaps() 
{
	
	cout << "RefreshComboTagMaps" << endl;
	this->Ui->comboTagMaps->clear();
	ExistingTagMaps *MyTM = mqMorphoDigCore::instance()->Getmui_ExistingTagMaps();
	cout << "Found" << MyTM->Stack.size() << "tag maps" << endl;
	for (int i = 0; i < MyTM->Stack.size(); i++)
	{

		this->Ui->comboTagMaps->addItem(MyTM->Stack.at(i).Name);


	}
	QString myActiveCM = mqMorphoDigCore::instance()->Getmui_ActiveTagMap()->Name;
	cout << "DIAL myActiveTagMap " << myActiveCM.toStdString() << endl;
	int exists = -1;
	for (int i = 0; i < this->Ui->comboTagMaps->count(); i++)
	{
		QString myCM = this->Ui->comboTagMaps->itemText(i);
		if (myCM == myActiveCM)
		{
			cout << "DIAL found CM in list!!!!! " << myCM.toStdString() << endl;
			exists = i;

		}

	}
	if (exists > -1) {
		cout << "DIAL Now current index of combo box is " << exists << endl;
		this->Ui->comboTagMaps->setCurrentIndex(exists);
				

	}
}

void mqEditTagsDialog::slotRemoveTags()
{
	
	mqMorphoDigCore::instance()->RemoveScalar(this->Ui->comboActiveTags->currentText(), this->Ui->selectedObjects->isChecked());
	this->UpdateUI();
	
	
}


void mqEditTagsDialog::slotRefreshComboTags()
{
	this->RefreshComboActiveTags();


}


void mqEditTagsDialog::RefreshDialog()
{
	this->UpdateUI();
	mqMorphoDigCore::instance()->Render();
}


void mqEditTagsDialog::slotActiveTagsChanged(int idx)
{
	
	cout << "looks like active tags have changed!:: " << idx << endl;
	QString NewActiveTagName = this->Ui->comboActiveTags->currentText();
	for (int i = 0; i < mqMorphoDigCore::instance()->Getmui_ExistingScalars()->Stack.size(); i++)
	{
		QString myExisingScalarName = mqMorphoDigCore::instance()->Getmui_ExistingScalars()->Stack.at(i).Name;
		if (NewActiveTagName == myExisingScalarName)
		{

			mqMorphoDigCore::instance()->Setmui_ActiveScalarsAndRender(NewActiveTagName,
				mqMorphoDigCore::instance()->Getmui_ExistingScalars()->Stack.at(i).DataType,
				mqMorphoDigCore::instance()->Getmui_ExistingScalars()->Stack.at(i).NumComp
			);
			
			

		}
	}
	

}


void mqEditTagsDialog::slotActiveTagMapChanged(int idx)
{
	
	cout << "looks like active tag map has changed!:: " << idx << endl;
	QString NewActiveTagMap = this->Ui->comboTagMaps->currentText();
	for (int i = 0; i < mqMorphoDigCore::instance()->Getmui_ExistingTagMaps()->Stack.size(); i++)
	{
		QString myExisingTagMapName = mqMorphoDigCore::instance()->Getmui_ExistingTagMaps()->Stack.at(i).Name;
		if (NewActiveTagMap == myExisingTagMapName)
		{
			
			if (mqMorphoDigCore::instance()->Getmui_ExistingTagMaps()->Stack.at(i).isCustom==1)
			{
				this->Ui->deleteTagMap->setDisabled(false);
				this->Ui->editTagMap->setDisabled(false);
			}
			else
			{
				this->Ui->deleteTagMap->setDisabled(true);
				this->Ui->editTagMap->setDisabled(true);

			}
			mqMorphoDigCore::instance()->Setmui_ActiveTagMapAndRender(NewActiveTagMap, mqMorphoDigCore::instance()->Getmui_ExistingTagMaps()->Stack.at(i).numTags,
				mqMorphoDigCore::instance()->Getmui_ExistingTagMaps()->Stack.at(i).tagNames,
				mqMorphoDigCore::instance()->Getmui_ExistingTagMaps()->Stack.at(i).TagMap);

			
			this->RefreshTagMapTable();
			
		


		}
	}
	

}




void mqEditTagsDialog::slotRefreshDialog()
{
	cout << "Let's refresh dialog!"		<< endl;

	//Dirty hack here! Replace "mqMorphoDigCore::instance()->GetScalarRangeMax()" by something which finds the bounds
	// of the currently used lookup table

	this->RefreshComboActiveTags();
	//this->RefreshDialog();
}


//-----------------------------------------------------------------------------


//-----------------------------------------------------------------------------


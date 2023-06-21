/*
 * Copyright 2015 bvtest <email>
 *
 * Licensed under the Apache License, Version 2.0 (the "License");
 * you may not use this file except in compliance with the License.
 * You may obtain a copy of the License at
 *
 *     http://www.apache.org/licenses/LICENSE-2.0
 *
 * Unless required by applicable law or agreed to in writing, software
 * distributed under the License is distributed on an "AS IS" BASIS,
 * WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
 * See the License for the specific language governing permissions and
 * limitations under the License.
 *
 */

#include "cpropertiesdialog.h"
#include <QTabWidget>
#include <jsoncpp/json/json.h>
#include <fstream>
#include <QGroupBox>
#include <QPushButton>


CPropertiesDialog::CPropertiesDialog(QWidget *parent) : QDialog(parent)
{
	_propvisibility = TCAM_PROPERTY_VISIBILITY_BEGINNER;

	_QTabs = new QTabWidget(this);
	QPushButton *Update = new QPushButton("Update");
	QPushButton *Close = new QPushButton("Close");
	QPushButton *btnSave = new QPushButton("Save");
	QPushButton *btnLoad = new QPushButton("Load");

	QHBoxLayout *Visibilitylayout = new QHBoxLayout;

	QLabel *lblVisibility = new QLabel("Visibility:");
	_cboVisibility = new QComboBox();
	_cboVisibility->addItem("Beginner");
	_cboVisibility->addItem("Expert");
	_cboVisibility->addItem("Guru");
	_cboVisibility->addItem("Invisible");
	_cboVisibility->setCurrentIndex(0);
	QObject::connect(_cboVisibility, SIGNAL(currentIndexChanged(int)), this, SLOT(OnVisibilityIndexChanged(int)));
	
	QCheckBox *chkshowToolTips = new QCheckBox("Property Descriptions");
	QObject::connect(chkshowToolTips, SIGNAL(stateChanged(int)), this, SLOT(OnClickedShowToolTips(int)));

	Visibilitylayout->addWidget(lblVisibility);	
	Visibilitylayout->addWidget(_cboVisibility);	
	Visibilitylayout->addWidget(chkshowToolTips);	


	QVBoxLayout *layout = new QVBoxLayout;
	layout->addLayout(Visibilitylayout);

	layout->addWidget(_QTabs);
	QHBoxLayout *Buttonslayout = new QHBoxLayout;

	Buttonslayout->addWidget(Update);
	Buttonslayout->addWidget(btnSave);
	Buttonslayout->addWidget(btnLoad);
	Buttonslayout->addWidget(Close);
   	layout->addLayout(Buttonslayout);

	setLayout(layout);

	connect(Update, SIGNAL(released()), this, SLOT(updateproperties()));
   	connect(Close, SIGNAL(released()), this, SLOT(onClose()) );
	connect(btnSave, SIGNAL(released()), this, SLOT(OnSaveProperties()));
	connect(btnLoad, SIGNAL(released()), this, SLOT(OnLoadProperties()));

  	//this->setAttribute(Qt::WA_AlwaysShowToolTips,false);
  	//CreatePageCategories();
}


CPropertiesDialog::~CPropertiesDialog()
{
  g_print("Dialog Destructor called\n");
  deleteControls();
}

void CPropertiesDialog::CreatePageCategories()
{
    std::ifstream ifs("properties.json");
    Json::Reader reader;
    Json::Value categories;
    reader.parse(ifs, categories); 
    _Pages.clear();
    for( auto p : categories["categories"])
	{
        std::string category =  p["name"].asString();
        CPage cat(category);
        for( auto g : p["groups"]){
            cat._groups.push_back( CPage(g["name"].asString()) );
        }

        _Pages.push_back(cat);
	}	
    _Pages.push_back(CPage("Unspecified"));

	return;

}

QGridLayout * CPropertiesDialog::findLayoutToAdd(const std::string Category, const std::string Groupname)
{
	QGridLayout *FoundLayOut = NULL;

	// Add a new page, if it does not already exist
    auto page = std::find_if(_Pages.begin(),_Pages.end(), [Category](CPage &page){ return page._Title == Category; }); 
	if(page == _Pages.end())
	{
		_Pages.push_back(CPage(Category));
	}

    page = std::find_if(_Pages.begin(),_Pages.end(), [Category](CPage &page){ return page._Title == Category; }); 
    if( page != _Pages.end())
    {
        FoundLayOut = getTabWidgetLayout(*page);
        if( Groupname != "")
        {
            auto group = std::find_if(page->_groups.begin(),page->_groups.end(), [Groupname](CPage &g){ return g._Title == Groupname; }); 
            if(group->_QWidget  == NULL) 
            {
                group->_QWidget = new QGroupBox(Groupname.c_str() );
	    	    group->_QWidget->setLayout(group->_GridLayout );
                FoundLayOut->addWidget(group->_QWidget);
            }
           return group->_GridLayout;
        }
    }
	return FoundLayOut;
}


QGridLayout * CPropertiesDialog::getTabWidgetLayout(CPage &page )
{
	if(page._QWidget == NULL )
	{
		page._QWidget = new QWidget();
		page._GridLayout = new QGridLayout();
		page._QWidget->setLayout( page._GridLayout );
        
		_QTabs->addTab(page._QWidget, page._Title.c_str());	
	}
	return page._GridLayout;
}

void CPropertiesDialog::deleteControls()
{
	// Clean up controls
	while(  _QTabs->count() > 0)
	{
		_QTabs->removeTab(0);
		QWidget * tab = _QTabs->widget(0);
		delete tab;
	}

	for( std::vector<CPropertyControl*>::iterator it = _PropertySliders.begin(); it != _PropertySliders.end(); it++ )
	{
		delete *it;
	}
	_PropertySliders.clear();

	_Pages.clear();

}

//////////////////////////////////////////////////////////////////////
// Creates the property controls.
//
void CPropertiesDialog::SetCamera(CGrabber *pGrabber )
{
	_pGrabber = pGrabber;

	setWindowTitle( _pGrabber->getDeviceName().c_str());

	buildPropertiesUI();
}

void CPropertiesDialog::buildPropertiesUI()
{
	TcamProp *ptcambin = _pGrabber->getTcamBin();
	if( ptcambin == nullptr)
		return;

	deleteControls();

	GError* err = NULL;
    GSList* names =  tcam_property_provider_get_tcam_property_names(TCAM_PROPERTY_PROVIDER(ptcambin), &err);

	for (unsigned int i = 0; i < g_slist_length(names); ++i)
    {
        char* name = (char*)g_slist_nth(names, i)->data;
		TcamPropertyBase* base_property = tcam_property_provider_get_tcam_property(TCAM_PROPERTY_PROVIDER(ptcambin), name, &err);
		if( tcam_property_base_get_visibility(base_property) <= _propvisibility )
		{
			addPropertyControl( base_property);
		}
	}
}

void CPropertiesDialog::addPropertyControl(TcamPropertyBase* base_property)
{
	QGridLayout *LayoutToAdd = findLayoutToAdd(tcam_property_base_get_category(base_property), "");

	CPropertyControl *pSld = NULL;
	TcamPropertyType type = tcam_property_base_get_property_type(base_property);
	switch( type )
	{
		case TCAM_PROPERTY_TYPE_INTEGER:
		{
			pSld = new CPropertyCtrl_int(base_property, LayoutToAdd,this );
			break;
		}
		case TCAM_PROPERTY_TYPE_FLOAT:
		{
			pSld = new CPropertyCtrl_float(base_property, LayoutToAdd,this );
			break;
		}
		case TCAM_PROPERTY_TYPE_BOOLEAN:
		{
			pSld = new CPropertyCtrl_boolean( base_property, LayoutToAdd,this );
			break;
		}
		case TCAM_PROPERTY_TYPE_COMMAND:
		{
			pSld = new CPropertyCtrl_command( base_property, LayoutToAdd ,this);
			break;
		}
		case TCAM_PROPERTY_TYPE_ENUMERATION:
		{
			pSld = new CPropertyCtrl_enum( base_property, LayoutToAdd,this );
			break;
		}
	}

	if( pSld != NULL )
	{
		_PropertySliders.push_back( pSld );
		pSld->updateControlsLocked();
	}
}

void CPropertiesDialog::updateproperties()
{
	for( int  i = 0; i < _PropertySliders.size(); i++ )
	{
		_PropertySliders[i]->UpdateValue();
	}
}

void CPropertiesDialog::onClose()
{
   this->done(Accepted);
}

void CPropertiesDialog::UpdateControls()
{
	for( int  i = 0; i < _PropertySliders.size(); i++ )
	{
		_PropertySliders[i]->updateControlsLocked();
	}

	_pGrabber->updateOverlay();
}

void CPropertiesDialog::OnVisibilityIndexChanged(int Index)
{
	_propvisibility = (TcamPropertyVisibility)Index;
	buildPropertiesUI();
}

void CPropertiesDialog::OnClickedShowToolTips(int val)
{
	for( int  i = 0; i < _PropertySliders.size(); i++ )
	{
		_PropertySliders[i]->enableToolTips( val==2);
	}
}

///////////////////////////////////////////////////////////////////////////
//
void CPropertiesDialog::OnSaveProperties()
{
	auto QfileName = QFileDialog::getSaveFileName(this, tr("Save Properties"), "~/", tr("Property Files (*.prop)"));
	std::string fileName = QfileName.toStdString();
	if( fileName == "")
		return;

	auto properties = _pGrabber->getJson();

    Json::StyledStreamWriter jsonstream;
    std::ofstream fout2(fileName);
    jsonstream.write(fout2, properties);
    fout2.close();
}

///////////////////////////////////////////////////////////////////////////
//
void CPropertiesDialog::OnLoadProperties()
{
	auto QfileName = QFileDialog::getOpenFileName(this, tr("Load Properties"), ".", tr("Property Files (*.prop)"));
	std::string fileName = QfileName.toStdString();
	if( fileName == "")
		return;

	std::ifstream ifs(fileName);
	Json::Reader reader;
	Json::Value properties;
	reader.parse(ifs, properties); 
    GValue state = G_VALUE_INIT;
	g_value_init(&state, G_TYPE_STRING);
	
	g_value_set_string(&state, properties["state"].asString().c_str());
	GstElement *ptcambin =  _pGrabber->getTcamBin();
	g_object_set_property(G_OBJECT(ptcambin), "tcam-properties-json", &state);

	updateproperties();		
}


//#include "cpropertiesdialog.moc"

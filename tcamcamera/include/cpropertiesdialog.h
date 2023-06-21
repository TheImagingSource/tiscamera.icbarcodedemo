#ifndef CPROPERTYDLG_H
#define CPROPERTYDLG_H


#include "propertyctrl.h"
#include "cgrabber.h"

class CPage
{
	public:
		CPage(std::string Title)
		{
			_Title = Title;
			_PageCreated = false;
			_QWidget = NULL;
			_GridLayout = nullptr;
		}
		
		std::string _Title;
		std::vector<std::string> _Categories;
		bool _PageCreated;
		QWidget *_QWidget;
		QGridLayout *_GridLayout;
		std::list<CPage> _groups;
		void addCategory(std::string Category)
		{
			_Categories.push_back(Category);
		}
		
};
  
class CPropertiesDialog : public QDialog
{
  Q_OBJECT
  
  public:
      CPropertiesDialog( QWidget *parent);
      ~CPropertiesDialog();
      void SetCamera(CGrabber *pGrabber);
	  void UpdateControls();

  private slots:
	void updateproperties();
	void onClose();
	void OnVisibilityIndexChanged(int);
	void OnClickedShowToolTips(int);
	void OnSaveProperties();
	void OnLoadProperties();
      
  private:
    QTabWidget *_QTabs;
	std::vector <CPage> _Pages;
    QComboBox *_cboVisibility;
	TcamPropertyVisibility _propvisibility;
    CGrabber *_pGrabber;
    std::vector<CPropertyControl*> _PropertySliders;
    void deleteControls();
	void CreatePageCategories();
    QGridLayout * findLayoutToAdd(const std::string Category, const std::string Groupname);
    QGridLayout * getTabWidgetLayout(CPage &page  );
	void buildPropertiesUI();
	void addPropertyControl(TcamPropertyBase* base_property);
};

#endif // CPROPERTIESDIALOG_H

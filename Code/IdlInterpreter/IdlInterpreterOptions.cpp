/*
 * The information in this file is
 * Copyright(c) 2007 Ball Aerospace & Technologies Corporation
 * and is subject to the terms and conditions of the
 * GNU Lesser General Public License Version 2.1
 * The license text is available from   
 * http://www.gnu.org/licenses/lgpl.html
 */

#include "AppConfig.h"
#include "AppVerify.h"
#include "ConfigurationSettings.h"
#include "FileBrowser.h"
#include "Filename.h"
#include "IdlInterpreterOptions.h"
#include "InterpreterManager.h"
#include "LabeledSection.h"
#include "ObjectResource.h"
#include "OptionQWidgetWrapper.h"
#include "PlugInManagerServices.h"
#include "PlugInRegistration.h"

#include <QtGui/QComboBox>
#include <QtGui/QGridLayout>
#include <QtGui/QLabel>
#include <QtGui/QMessageBox>
#include <QtGui/QWidget>

REGISTER_PLUGIN(Idl, IdlInterpreterOptions, OptionQWidgetWrapper<IdlInterpreterOptions>());

IdlInterpreterOptions::IdlInterpreterOptions()
{
   QWidget* pIdlConfigWidget = new QWidget(this);
   QLabel* pDllLabel = new QLabel("IDL Installation Location:", pIdlConfigWidget);
   mpDll = new FileBrowser(pIdlConfigWidget);
   mpDll->setBrowseCaption("Locate the IDL installation");
   mpDll->setBrowseExistingFile(true);
#if defined(WIN_API)
   mpDll->setBrowseFileFilters("IDL (idl*.dll)");
#else
   mpDll->setBrowseFileFilters("IDL (libidl*.so)");
#endif
   
   QLabel* pVersionLabel = new QLabel("IDL Version:", pIdlConfigWidget);
   mpVersion = new QComboBox(pIdlConfigWidget);
#pragma message(__FILE__ "(" STRING(__LINE__) ") : warning : This should scan for available "\
                  "IdlStart dll's and add valid items appropriately (tclarke)")
   mpVersion->addItems(QStringList() << "6.1" << "6.3" << "6.4" << "7.0" << "7.1");
   mpVersion->setEditable(true);
   mpVersion->setDuplicatesEnabled(false);
   mpVersion->setInsertPolicy(QComboBox::InsertAlphabetically);

   QGridLayout* pIdlConfigLayout = new QGridLayout(pIdlConfigWidget);
   pIdlConfigLayout->setMargin(0);
   pIdlConfigLayout->setSpacing(5);
   pIdlConfigLayout->addWidget(pDllLabel, 0, 0);
   pIdlConfigLayout->addWidget(mpDll, 0, 1);
   pIdlConfigLayout->addWidget(pVersionLabel, 1, 0);
   pIdlConfigLayout->addWidget(mpVersion, 1, 1, Qt::AlignLeft);
   pIdlConfigLayout->setColumnStretch(1, 10);
   pIdlConfigLayout->setRowStretch(2, 10);

   LabeledSection* pIdlConfigSection = new LabeledSection(pIdlConfigWidget, "IDL Configuration", this);
   const Filename* pTmpFile = IdlInterpreterOptions::getSettingDLL();
   setDll(pTmpFile);
   setVersion(QString::fromStdString(IdlInterpreterOptions::getSettingVersion()));

   // Initialization
   addSection(pIdlConfigSection, 100);
   addStretch(1);
}

IdlInterpreterOptions::~IdlInterpreterOptions()
{}

void IdlInterpreterOptions::setDll(const Filename* pDll)
{
   if (pDll != NULL)
   {
      mpDll->setFilename(*pDll);
   }
}

void IdlInterpreterOptions::setVersion(const QString& version)
{
   int loc = mpVersion->findText(version, Qt::MatchFixedString);
   if (loc == -1)
   {
      mpVersion->addItem(version);
      loc = mpVersion->findText(version, Qt::MatchFixedString);
   }
   mpVersion->setCurrentIndex(loc);
}

void IdlInterpreterOptions::applyChanges()
{
   FactoryResource<Filename> pTmpDll;
   pTmpDll->setFullPathAndName(mpDll->getFilename().toStdString());
   IdlInterpreterOptions::setSettingDLL(pTmpDll.get());
   IdlInterpreterOptions::setSettingVersion(mpVersion->currentText().toStdString());

   std::vector<PlugIn*> plugIns = Service<PlugInManagerServices>()->getPlugInInstances("IDL");
   if (plugIns.empty() == false)
   {
      VERIFYNR(plugIns.size() == 1);

      InterpreterManager* pInterpreter = dynamic_cast<InterpreterManager*>(plugIns.front());
      if ((pInterpreter != NULL) && (pInterpreter->isStarted() == true))
      {
         QString appName = QString::fromStdString(Service<ConfigurationSettings>()->getProduct());
         QMessageBox::warning(this, appName, "The IDL interpreter is already running so changes to the IDL "
            "installation location or version will not take effect until the application is restarted.");
      }
   }
}

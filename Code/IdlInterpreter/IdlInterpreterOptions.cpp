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
#include "FileBrowser.h"
#include "Filename.h"
#include "IdlInterpreterOptions.h"
#include "LabeledSection.h"
#include "OptionQWidgetWrapper.h"
#include "PlugInRegistration.h"
#include <QtGui/QComboBox>
#include <QtGui/QGridLayout>
#include <QtGui/QLabel>
#include <QtGui/QWidget>

REGISTER_PLUGIN(Idl, IdlInterpreterOptions, OptionQWidgetWrapper<IdlInterpreterOptions>());

IdlInterpreterOptions::IdlInterpreterOptions()
{
   QWidget* pIdlConfigWidget = new QWidget(this);
   QLabel* pDllLabel = new QLabel("IDL Installation Location", pIdlConfigWidget);
   mpDll = new FileBrowser(pIdlConfigWidget);
   mpDll->setBrowseCaption("Locate the IDL installation");
   mpDll->setBrowseExistingFile(true);
#if defined(WIN_API)
   mpDll->setBrowseFileFilters("IDL (idl*.dll)");
#else
   mpDll->setBrowseFileFilters("IDL (libidl*.so)");
#endif
   
   QLabel* pVersionLabel = new QLabel("IDL Version", pIdlConfigWidget);
   mpVersion = new QComboBox(pIdlConfigWidget);
#pragma message(__FILE__ "(" STRING(__LINE__) ") : warning : This should scan for available IdlStart dll's and add valid items appropriately (tclarke)")
   mpVersion->addItems(QStringList() << "6.1" << "6.3" << "6.4");
   mpVersion->setEditable(true);
   mpVersion->setDuplicatesEnabled(false);
   mpVersion->setInsertPolicy(QComboBox::InsertAlphabetically);

   QGridLayout* pIdlConfigLayout = new QGridLayout(pIdlConfigWidget);
   pIdlConfigLayout->addWidget(pDllLabel, 0, 0);
   pIdlConfigLayout->addWidget(mpDll, 0, 1);
   pIdlConfigLayout->addWidget(pVersionLabel, 1, 0);
   pIdlConfigLayout->addWidget(mpVersion, 1, 1);
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
{
}

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
}

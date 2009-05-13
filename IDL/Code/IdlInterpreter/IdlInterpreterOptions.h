/*
 * The information in this file is
 * Copyright(c) 2007 Ball Aerospace & Technologies Corporation
 * and is subject to the terms and conditions of the
 * GNU Lesser General Public License Version 2.1
 * The license text is available from   
 * http://www.gnu.org/licenses/lgpl.html
 */

#ifndef IDLINTERPRETEROPTIONS_H
#define IDLINTERPRETEROPTIONS_H

#include "ConfigurationSettings.h"
#include "Filename.h"
#include "IdlVersion.h"
#include "LabeledSectionGroup.h"
#include <string>
#include <vector>

class FileBrowser;
class QComboBox;

class IdlInterpreterOptions : public LabeledSectionGroup
{
   Q_OBJECT

public:
   SETTING_PTR(DLL, IdlInterpreter, Filename);
   SETTING(Version, IdlInterpreter, std::string, std::string());
   SETTING(Modules, IdlInterpreter, std::vector<Filename*>, std::vector<Filename*>());
   SETTING(InteractiveAvailable, IdlInterpreter, bool, true);

   IdlInterpreterOptions();
   virtual ~IdlInterpreterOptions();

   void setDll(const Filename* pDll);
   void setVersion(const QString& version);
   void applyChanges();

   static const std::string& getName()
   {
      static std::string var = "IDL Interpreter Options";
      return var;
   }

   static const std::string& getOptionName()
   {
      static std::string var = "Scripting/IDL Interpreter";
      return var;
   }

   static const std::string& getDescription()
   {
      static std::string var = "Configuration options for the IDL interpreter";
      return var;
   }

   static const std::string& getShortDescription()
   {
      static std::string var = "Configuration options for the IDL interpreter";
      return var;
   }

   static const std::string& getCreator()
   {
      static std::string var = "Ball Aerospace & Technologies Corp.";
      return var;
   }

   static const std::string& getCopyright()
   {
      static std::string var = IDL_COPYRIGHT;
      return var;
   }

   static const std::string& getVersion()
   {
      static std::string var = IDL_VERSION_NUMBER;
      return var;
   }

   static bool isProduction()
   {
      return IDL_IS_PRODUCTION_RELEASE;
   }

   static const std::string& getDescriptorId()
   {
      static std::string var = "{3dec05d4-deb2-4ce1-8e80-9acbf7c5dd26}";
      return var;
   }

private:
   FileBrowser* mpDll;
   QComboBox* mpVersion;
};

#endif
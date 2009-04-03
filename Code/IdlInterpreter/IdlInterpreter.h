/*
 * The information in this file is
 * subject to the terms and conditions of the
 * GNU Lesser General Public License Version 2.1
 * The license text is available from   
 * http://www.gnu.org/licenses/lgpl.html
 */

#ifndef IDLINTERPRETER_H__
#define IDLINTERPRETER_H__

#include "ConfigurationSettings.h"
#include "Filename.h"
#include "InterpreterShell.h"

#include <string>
#include <vector>

class PlugInManagerServices;

class IdlInterpreter : public InterpreterShell
{
public:
   SETTING_PTR(DLL, IdlInterpreter, Filename);
   SETTING(Version, IdlInterpreter, std::string, std::string());
   SETTING(Modules, IdlInterpreter, std::vector<std::string>, std::vector<std::string>());

   IdlInterpreter();
   virtual ~IdlInterpreter();
   virtual bool getInputSpecification(PlugInArgList*& pArgList);
   virtual bool getOutputSpecification(PlugInArgList*& pArgList);
   virtual bool execute(PlugInArgList* pInArgList, PlugInArgList* pOutArgList);
   virtual void getKeywordList(std::vector<std::string>& list) const;
   virtual bool getKeywordDescription(const std::string& keyword, std::string& description) const;
   virtual void getUserDefinedTypes(std::vector<std::string>& list) const ;
   virtual bool getTypeDescription(const std::string& type, std::string& description) const ;

private:
   bool startIdl();

   std::string mCommand;
   bool mIdlRunning;
   std::vector<DynamicModule*> mModules;
};

#endif
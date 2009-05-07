/*
 * The information in this file is
 * Copyright(c) 2007 Ball Aerospace & Technologies Corporation
 * and is subject to the terms and conditions of the
 * GNU Lesser General Public License Version 2.1
 * The license text is available from   
 * http://www.gnu.org/licenses/lgpl.html
 */

#ifndef IDLINTERPRETER_H__
#define IDLINTERPRETER_H__

#include "ExecutableShell.h"
#include "InterpreterShell.h"
#include "WizardShell.h"

#include <string>
#include <vector>

class PlugInManagerServices;

class IdlProxy : public ExecutableShell
{
public:
   IdlProxy();
   virtual ~IdlProxy();
   virtual bool getInputSpecification(PlugInArgList*& pArgList);
   virtual bool getOutputSpecification(PlugInArgList*& pArgList);
   virtual bool execute(PlugInArgList* pInArgList, PlugInArgList* pOutArgList);
   bool processCommand(const std::string& command, std::string& returnText, std::string& errorText, Progress* pProgress);

private:
   bool startIdl(const char** pOutput = NULL, const char** pErrorOutput = NULL);

   bool mIdlRunning;
   std::vector<DynamicModule*> mModules;
};

class IdlInterpreter : public InterpreterShell
{
public:
   IdlInterpreter();
   virtual ~IdlInterpreter();
   virtual bool getInputSpecification(PlugInArgList*& pArgList);
   virtual bool getOutputSpecification(PlugInArgList*& pArgList);
   virtual bool execute(PlugInArgList* pInArgList, PlugInArgList* pOutArgList);
   virtual void getKeywordList(std::vector<std::string>& list) const;
   virtual bool getKeywordDescription(const std::string& keyword, std::string& description) const;
   virtual void getUserDefinedTypes(std::vector<std::string>& list) const ;
   virtual bool getTypeDescription(const std::string& type, std::string& description) const ;
};

class IdlInterpreterWizardItem : public WizardShell
{
public:
   IdlInterpreterWizardItem();
   virtual ~IdlInterpreterWizardItem();
   virtual bool getInputSpecification(PlugInArgList*& pArgList);
   virtual bool getOutputSpecification(PlugInArgList*& pArgList);
   virtual bool execute(PlugInArgList* pInArgList, PlugInArgList* pOutArgList);
};

#endif
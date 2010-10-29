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

#include "ApplicationServices.h"
#include "AttachmentPtr.h"
#include "Interpreter.h"
#include "InterpreterManagerShell.h"
#include "SubjectImp.h"
#include "WizardShell.h"

#include <string>
#include <vector>

class PlugInManagerServices;

class IdlProxy : public Interpreter, public SubjectImp
{
public:
   IdlProxy();
   virtual ~IdlProxy();

   bool isIdlRunning() const;
   bool startIdl();
   std::string getStartupMessage() const;

   virtual std::string getPrompt() const;
   virtual bool executeCommand(const std::string& command);
   virtual bool executeScopedCommand(const std::string& command, const Slot& output,
      const Slot& error, Progress* pProgress);
   virtual bool isGlobalOutputShown() const;
   virtual void showGlobalOutput(bool newValue);

   virtual const std::string& getObjectType() const;
   virtual bool isKindOf(const std::string& className) const;

   void sendOutput(const std::string& text);
   void sendError(const std::string& text);

   SUBJECTADAPTER_METHODS(SubjectImp)
private:
   bool executeCommandInternal(const std::string& command, Progress* pProgress);
   SIGNAL_METHOD(ScriptorExecutor, ScopedOutputText);
   SIGNAL_METHOD(ScriptorExecutor, ScopedErrorText);

   void applicationClosed(Subject& subject, const std::string& signal, const boost::any& data);

   AttachmentPtr<ApplicationServices> mpAppServices;
   bool mIdlRunning;
   bool mRunningScopedCommand;
   std::vector<DynamicModule*> mModules;
   bool mGlobalOutputShown;
   std::string mStartupMessage;
};

class IdlInterpreterManager : public InterpreterManagerShell, public SubjectImp
{
public:
   IdlInterpreterManager();
   virtual ~IdlInterpreterManager();

   virtual bool execute(PlugInArgList* pInArgList, PlugInArgList* pOutArgList);

   virtual bool isStarted() const;
   virtual bool start();
   virtual std::string getStartupMessage() const;
   virtual Interpreter* getInterpreter() const;

   virtual const std::string& getObjectType() const;
   virtual bool isKindOf(const std::string& className) const;

   SUBJECTADAPTER_METHODS(SubjectImp)
private:
   std::auto_ptr<IdlProxy> mpInterpreter;
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
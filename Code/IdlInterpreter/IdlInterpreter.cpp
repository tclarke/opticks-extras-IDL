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
#include "DynamicModule.h"
#include "External.h"
#include "IdlVersion.h"
#include "IdlInterpreter.h"
#include "IdlInterpreterOptions.h"
#include "InterpreterUtilities.h"
#include "PlugInArgList.h"
#include "PlugInManagerServices.h"
#include "PlugInRegistration.h"
#include "ProgressTracker.h"

#include <boost/tokenizer.hpp>
#include <QtCore/QString>

REGISTER_PLUGIN_BASIC(Idl, IdlInterpreterManager);
REGISTER_PLUGIN_BASIC(Idl, IdlInterpreterWizardItem);

typedef void (*output_callback_t)(char* pBuf, int num_chars, int error, int addNewline);

namespace
{
   IdlProxy* spProxyHandle = NULL;
   void handleOutput(char* pBuf, int num_chars, int error, int addNewline)
   {
      if (spProxyHandle == NULL)
      {
         return;
      }
      std::string output = std::string(pBuf, num_chars);
      if (addNewline)
      {
         output += "\n";
      }
      if (error)
      {
         spProxyHandle->sendError(output);
      }
      else
      {
         spProxyHandle->sendOutput(output);
      }
   }
};

IdlProxy::IdlProxy() :
   mIdlRunning(false),
   mpAppServices(Service<ApplicationServices>().get(), SIGNAL_NAME(ApplicationServices, ApplicationClosed),
      Slot(this, &IdlProxy::applicationClosed)),
   mGlobalOutputShown(false),
   mRunningScopedCommand(false)
{
   spProxyHandle = this;
}

IdlProxy::~IdlProxy()
{
   spProxyHandle = NULL;
}

bool IdlProxy::isIdlRunning() const
{
   return mIdlRunning;
}

std::string IdlProxy::getStartupMessage() const
{
   return mStartupMessage;
}

bool IdlProxy::startIdl()
{
   if (mIdlRunning)
   {
      return true;
   }
   mStartupMessage.clear();

   // figure out which version of IDL to use
   std::string idlDll;

   const Filename* pDll = IdlInterpreterOptions::getSettingDLL();
   if (pDll != NULL)
   {
#if defined(WIN_API)
      QString tmp = QString::fromStdString(pDll->getFullPathAndName());
      tmp.replace("/", "\\");
      idlDll = tmp.toStdString();
#else
      idlDll = pDll->getFullPathAndName();
#endif
   }

   std::string idlVersion = IdlInterpreterOptions::getSettingVersion();

   // Create the postfix for the start dll name. If a version is specified
   // such as 6.4, remove the .
   if (idlVersion.size() == 3 && idlVersion[1] == '.')
   {
      idlVersion.erase(1, 1);
   }
#if defined(WIN_API)
   std::string idlPostfix = idlVersion + ".dll";
#else
   std::string idlPostfix = idlVersion + ".so";
#endif
   std::vector<Filename*> idlModules = IdlInterpreterOptions::getSettingModules();

   const char* pOutput = NULL;
   for (std::vector<Filename*>::size_type i = 0; i < idlModules.size(); ++i)
   {
      std::string startDll = idlModules[i]->getFullPathAndName();
      startDll += idlPostfix;

      DynamicModule* pModule = Service<PlugInManagerServices>()->getDynamicModule(startDll);
      if (pModule != NULL)
      {
         External* pExternal = ModuleManager::instance()->getService();
         bool(*start_idl)(const char*, External*, const char**, output_callback_t) =
            reinterpret_cast<bool(*)(const char*, External*, const char**, output_callback_t)>(
            pModule->getProcedureAddress("start_idl"));
         if (start_idl != NULL && start_idl(idlDll.c_str(), pExternal, &pOutput, &handleOutput) != 0)
         {
            mModules.push_back(pModule);
            mIdlRunning = true;
         }
         else
         {
            Service<PlugInManagerServices>()->destroyDynamicModule(pModule);
         }
         if (pOutput != NULL)
         {
            mStartupMessage += pOutput;
            mStartupMessage += "\n";
         }
      }
   }

   if (!mIdlRunning)
   {
      mStartupMessage = 
         "Unable to start the IDL interpreter. Make sure you have located "
         "your IDL installation in the options.\n" + mStartupMessage;
      return false;
   }

   if (!IdlInterpreterOptions::getSettingInteractiveAvailable())
   {
      mStartupMessage = "The ability to type IDL commands into the Scripting Window "
         "has been disabled by another extension.\n" + mStartupMessage;
   }

   return mIdlRunning;
}

std::string IdlProxy::getPrompt() const
{
   return "> ";
}

bool IdlProxy::executeCommand(const std::string& command)
{
   mRunningScopedCommand = false;
   bool retVal = executeCommandInternal(command, NULL);
   mRunningScopedCommand = false;
   return retVal;
}

bool IdlProxy::executeScopedCommand(const std::string& command, const Slot& output, const Slot& error, Progress* pProgress)
{
   mRunningScopedCommand = true;
   attach(SIGNAL_NAME(IdlProxy, ScopedOutputText), output);
   attach(SIGNAL_NAME(IdlProxy, ScopedErrorText), error);
   bool retValue = executeCommandInternal(command, pProgress);
   detach(SIGNAL_NAME(IdlProxy, ScopedErrorText), error);
   detach(SIGNAL_NAME(IdlProxy, ScopedOutputText), output);
   mRunningScopedCommand = false;
   return retValue;
}

bool IdlProxy::executeCommandInternal(const std::string& command, Progress* pProgress)
{
   if (!mIdlRunning || mModules.empty())
   {
      return false;
   }

   /***
    * We only need to run one of the dlls.  All the dlls 'execute_idl'
    * command call the same IDL dll which has one list of commands
    * registered by all the .dlls.  The last dll is used because it was
    * the last one to push an output function into IDL's output function
    * stack. IDL will only run the last output function so we need to
    * run the dll with that output function.
    *
    * P.S. This is a hack and we need to find a better way so there is
    * only one 'execute_idl' and one output function for all dlls.
    */
   DynamicModule* pMod = mModules.back();
   VERIFY(pMod != NULL);

   //get the location of the function that will call the IDL
   int (*execute_idl)(int, char**, int, Progress*) =
      reinterpret_cast<int (*)(int, char**, int, Progress*)>(
            *pMod->getProcedureAddress("execute_idl"));
   VERIFY(execute_idl);

   std::vector<std::string> lines;
   typedef boost::tokenizer<boost::char_separator<char> > tokenizer;
   boost::char_separator<char> newlineSep("\n", "", boost::keep_empty_tokens);
   tokenizer tokens(command, newlineSep);
   std::copy(tokens.begin(), tokens.end(), std::back_inserter(lines));

   if (lines.empty())
   {
      return true;
   }
   char** pCommands = new (std::nothrow) char*[lines.size()];
   if (pCommands == NULL)
   {
      sendError("Could not allocate memory to send commands to IDL");
      return false;
   }
   for (std::vector<std::string>::size_type count = 0; count < lines.size(); ++count)
   {
      pCommands[count] = const_cast<char*>(lines[count].c_str());
   }

   //execute the function and capture the output
   int retVal = execute_idl(lines.size(), pCommands, mRunningScopedCommand, pProgress);
   delete [] pCommands;
   pCommands = NULL;

   return (retVal == 0);
}

bool IdlProxy::isGlobalOutputShown() const
{
   return mGlobalOutputShown;
}

void IdlProxy::showGlobalOutput(bool newValue)
{
   mGlobalOutputShown = newValue;
}

const std::string& IdlProxy::getObjectType() const
{
   static std::string sType("IdlProxy");
   return sType;
}

bool IdlProxy::isKindOf(const std::string& className) const
{
   if (className == getObjectType())
   {
      return true;
   }

   return SubjectImp::isKindOf(className);
}

void IdlProxy::sendOutput(const std::string& text)
{
   if (text.empty())
   {
      return;
   }
   if (mRunningScopedCommand)
   {
      notify(SIGNAL_NAME(IdlProxy, ScopedOutputText), text);
   }
   if (!mRunningScopedCommand || mGlobalOutputShown)
   {
      notify(SIGNAL_NAME(Interpreter, OutputText), text);
   } 
}

void IdlProxy::sendError(const std::string& text)
{
   if (text.empty())
   {
      return;
   }
   if (mRunningScopedCommand)
   {
      notify(SIGNAL_NAME(IdlProxy, ScopedErrorText), text);
   }
   if (!mRunningScopedCommand || mGlobalOutputShown)
   {
      notify(SIGNAL_NAME(Interpreter, ErrorText), text);
   } 
}

void IdlProxy::applicationClosed(Subject& subject, const std::string& signal, const boost::any& data)
{
   if (mIdlRunning && !mModules.empty()) 
   {
      for (std::vector<DynamicModule*>::const_iterator module = mModules.begin(); module != mModules.end(); ++module)
      {
         bool(*close_idl)() =
            reinterpret_cast<bool(*)()>((*module)->getProcedureAddress("close_idl"));
         VERIFYNRV(close_idl);
         close_idl();
         (*module)->unload();
         Service<PlugInManagerServices>()->destroyDynamicModule(*module);
      }
   }
}

IdlInterpreterManager::IdlInterpreterManager()
   : mpInterpreter(new IdlProxy())
{
   setName("IDL");
   setDescription("Provides command line utilities to execute IDL commands.");
   setDescriptorId("{09BBB1FD-D12A-43B8-AB09-95AA8028BFFE}");
   setCopyright(IDL_COPYRIGHT);
   setVersion(IDL_VERSION_NUMBER);
   setProductionStatus(IDL_IS_PRODUCTION_RELEASE);
   allowMultipleInstances(false);
   setWizardSupported(false);
   setFileExtensions("IDL Scripts (*.pro)");
   setInteractiveEnabled(IdlInterpreterOptions::getSettingInteractiveAvailable());
}

IdlInterpreterManager::~IdlInterpreterManager()
{
}

bool IdlInterpreterManager::execute(PlugInArgList* pInArgList, PlugInArgList* pOutArgList)
{
   mpInterpreter->startIdl();
   return true;
}

bool IdlInterpreterManager::isStarted() const
{
   return mpInterpreter->isIdlRunning();
}

bool IdlInterpreterManager::start()
{
   bool alreadyStarted = mpInterpreter->isIdlRunning();
   bool started = mpInterpreter->startIdl();
   if (!alreadyStarted && started)
   {
      notify(SIGNAL_NAME(InterpreterManager, InterpreterStarted));
   }
   return started;
}

std::string IdlInterpreterManager::getStartupMessage() const
{
   return mpInterpreter->getStartupMessage();
}

Interpreter* IdlInterpreterManager::getInterpreter() const
{
   if (mpInterpreter->isIdlRunning())
   {
      return mpInterpreter.get();
   }
   return NULL;
}
const std::string& IdlInterpreterManager::getObjectType() const
{
   static std::string sType("IdlInterpreterManager");
   return sType;
}

bool IdlInterpreterManager::isKindOf(const std::string& className) const
{
   if (className == getObjectType())
   {
      return true;
   }

   return SubjectImp::isKindOf(className);
}

IdlInterpreterWizardItem::IdlInterpreterWizardItem()
{
   setName("IDL Interpreter");
   setDescription("Allow execution of IDL code from within a wizard. "
      "This is DEPRECATED, please use the RunInterpreterCommands wizard item.");
   setDescriptorId("{006e0f34-b7b1-4b50-aa6f-24abbc205b91}");
   setCopyright(IDL_COPYRIGHT);
   setVersion(IDL_VERSION_NUMBER);
   setProductionStatus(IDL_IS_PRODUCTION_RELEASE);
}

IdlInterpreterWizardItem::~IdlInterpreterWizardItem()
{
}

bool IdlInterpreterWizardItem::getInputSpecification(PlugInArgList*& pArgList)
{
   VERIFY((pArgList = Service<PlugInManagerServices>()->getPlugInArgList()) != NULL);
   VERIFY(pArgList->addArg<Progress>(ProgressArg(), NULL));
   VERIFY(pArgList->addArg<std::string>("Command", std::string()));

   return true;
}

bool IdlInterpreterWizardItem::getOutputSpecification(PlugInArgList*& pArgList)
{
   VERIFY((pArgList = Service<PlugInManagerServices>()->getPlugInArgList()) != NULL);
   VERIFY(pArgList->addArg<std::string>("Output Text"));
   VERIFY(pArgList->addArg<std::string>("Return Type"));

   return true;
}

bool IdlInterpreterWizardItem::execute(PlugInArgList* pInArgList, PlugInArgList* pOutArgList)
{
   VERIFY(pInArgList != NULL && pOutArgList != NULL);

   ProgressTracker progress(pInArgList->getPlugInArgValue<Progress>(ProgressArg()),
      "Execute IDL command.", "extras", "{592287e7-abbf-4581-9389-93b3bd5d8070}");

   progress.report("This wizard item is DEPRECATED, please use the RunInterpreterCommands wizard item.",
      0, WARNING, true);

   std::string command;
   if (!pInArgList->getPlugInArgValue("Command", command))
   {
      return false;
   }

   std::vector<PlugIn*> plugins = Service<PlugInManagerServices>()->getPlugInInstances("IDL");
   if (plugins.size() != 1)
   {
      progress.report("Unable to locate an IDL interpreter.", 0, ERRORS, true);
      return false;
   }
   IdlInterpreterManager* pMgr = dynamic_cast<IdlInterpreterManager*>(plugins.front());
   VERIFY(pMgr != NULL);
   pMgr->start();
   Interpreter* pInterpreter = pMgr->getInterpreter();
   if (pInterpreter == NULL)
   {
      progress.report("Unable to start IDL interpreter. " + pMgr->getStartupMessage(), 0, ERRORS, true);
      return false;
   }

   progress.report("Executing IDL command.", 1, NORMAL);
   std::string outputAndErrorText;
   bool hasErrorText = false;
   bool retVal = InterpreterUtilities::executeScopedCommand("IDL", command, outputAndErrorText, hasErrorText,
      progress.getCurrentProgress());
   if (!retVal)
   {
      progress.report("Error executing IDL command.", 0, ERRORS, true);
   }
   std::string returnType = (hasErrorText ? "Error" : "Output");
   VERIFY(pOutArgList->setPlugInArgValue("Return Type", &returnType));
   VERIFY(pOutArgList->setPlugInArgValue("Output Text", &outputAndErrorText));
   progress.report("Executing IDL command.", 100, NORMAL);
   progress.upALevel();
   return retVal;
}

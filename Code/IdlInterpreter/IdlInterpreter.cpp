/*
 * The information in this file is
 * subject to the terms and conditions of the
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
#include "PlugInArgList.h"
#include "PlugInManagerServices.h"
#include "PlugInRegistration.h"
#include "ProgressTracker.h"

REGISTER_PLUGIN_BASIC(Idl, IdlProxy);
REGISTER_PLUGIN_BASIC(Idl, IdlInterpreter);
REGISTER_PLUGIN_BASIC(Idl, IdlInterpreterWizardItem);

IdlProxy::IdlProxy() : mIdlRunning(false)
{
   setName("IdlProxy");
   setDescription("Proxies commands from Opticks to IDL.");
   setDescriptorId("{60fd1d3c-ebed-4268-aaa2-6adeb528b9e6}");
   setCopyright(IDL_COPYRIGHT);
   setVersion(IDL_VERSION_NUMBER);
   setProductionStatus(IDL_IS_PRODUCTION_RELEASE);
   setType("Manager");
   executeOnStartup(true);
   destroyAfterExecute(false);
   allowMultipleInstances(false);
   setWizardSupported(false);
}

IdlProxy::~IdlProxy()
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
      }
   }
}

bool IdlProxy::getInputSpecification(PlugInArgList*& pArgList)
{
   pArgList = NULL;
   return true;
}

bool IdlProxy::getOutputSpecification(PlugInArgList*& pArgList)
{
   pArgList = NULL;
   return true;
}

bool IdlProxy::execute(PlugInArgList* pInArgList, PlugInArgList* pOutArgList)
{
   return true;
}

bool IdlProxy::processCommand(const std::string& command, std::string& returnText, std::string& errorText, Progress* pProgress)
{
   const char* pOutput = NULL;
   const char* pErrorOutput = NULL;
   if (!mIdlRunning && !startIdl(&pOutput, &pErrorOutput))
   {
      returnText = "Unable to start the IDL interpreter. Make sure you have located your IDL installation in the options.";
      if (pErrorOutput != NULL)
      {
         returnText += "\n";
         returnText += pErrorOutput;
      }
      return false;
   }
   if (mModules.empty())
   {
      returnText = "Unable to start the IDL interpreter. Make sure you have located your IDL installation in the options.";
      if (pErrorOutput != NULL)
      {
         returnText += "\n";
         returnText += pErrorOutput;
      }
      return false;
   }
   if (pOutput != NULL)
   {
      returnText += pOutput;
      returnText += "\n";
      pOutput = NULL;
   }
   if (pErrorOutput != NULL)
   {
      errorText += pErrorOutput;
      errorText += "\n";
      pErrorOutput = NULL;
   }

   /***
    * We only need to run one of the dlls.  All the dlls 'execute_idl'
    * command call the same IDL dll which has one list of commands
    * registered by all the .ds.  The last dll is used because it was
    * the last one to push an output function into IDL's output function
    * stack. IDL will only run the last output function so we need to
    * run the dll with that output function.
    *
    * P.S. This is a hack and we need to find a better way so there is
    * only one 'execute_idl' and one output function for all dlls.
    */
   int lastLoaded = static_cast<int>(mModules.size()) - 1;
   DynamicModule* pMod = mModules[lastLoaded];
   VERIFY(pMod != NULL);

   //get the location of the function that will call the IDL
   void (*execute_idl)(const char*, const char**, const char**, Progress*) =
      reinterpret_cast<void (*)(const char*, const char**, const char**, Progress*)>(*pMod->getProcedureAddress("execute_idl"));
   VERIFY(execute_idl);

   //execute the function and capture the output
   execute_idl(command.c_str(), &pOutput, &pErrorOutput, pProgress);
   if (pOutput != NULL)
   {
      returnText += pOutput;
   }
   if (pErrorOutput != NULL)
   {
      errorText += pErrorOutput;
   }

   return true;
}

bool IdlProxy::startIdl(const char** pOutput, const char** pErrorOutput)
{
   // figure out which version of IDL to use
   std::string idlDll;

   const Filename* pDll = IdlInterpreterOptions::getSettingDLL();
   if (pDll != NULL)
   {
      idlDll = pDll->getFullPathAndName();
#if defined(WIN_API)
      for (std::string::size_type pos = idlDll.find("/"); pos != std::string::npos; pos = idlDll.find("/"))
      {
         idlDll[pos] = '\\';
      }
#endif
   }

   std::string idlVersion = IdlInterpreterOptions::getSettingVersion();

#if defined(WIN_API)
   std::string idlPostfix = (idlVersion.size() < 3) ? ".dll" : (idlVersion.substr(0, 1) + idlVersion.substr(2, 1) + ".dll");
#else
   std::string idlPostfix = (idlVersion.size() < 3) ? ".so" : (idlVersion.substr(0, 1) + idlVersion.substr(2, 1) + ".so");
#endif
   std::vector<Filename*> idlModules = IdlInterpreterOptions::getSettingModules();

   for (std::vector<Filename*>::size_type i = 0; i < idlModules.size(); ++i)
   {
      std::string startDll = idlModules[i]->getFullPathAndName();
      startDll += idlPostfix;

      DynamicModule* pModule = Service<PlugInManagerServices>()->getDynamicModule(startDll);
      if (pModule != NULL)
      {
         External* pExternal = ModuleManager::instance()->getService();
         bool(*start_idl)(const char*, External*, const char**, const char**) =
            reinterpret_cast<bool(*)(const char*, External*, const char**, const char**)>(
            pModule->getProcedureAddress("start_idl"));
         if (start_idl != NULL && start_idl(idlDll.c_str(), pExternal, pOutput, pErrorOutput) != 0)
         {
            mModules.push_back(pModule);
            mIdlRunning = true;
         }
      }
   }

   return mIdlRunning;
}

IdlInterpreter::IdlInterpreter()
{
   setName("IdlInterpreter");
   setDescription("Provides command line utilities to execute IDL commands.");
   setDescriptorId("{09BBB1FD-D12A-43B8-AB09-95AA8028BFFE}");
   setCopyright(IDL_COPYRIGHT);
   setVersion(IDL_VERSION_NUMBER);
   setProductionStatus(IDL_IS_PRODUCTION_RELEASE);
   allowMultipleInstances(false);
   setWizardSupported(false);
}

IdlInterpreter::~IdlInterpreter()
{
}

bool IdlInterpreter::getInputSpecification(PlugInArgList*& pArgList)
{
   VERIFY((pArgList = Service<PlugInManagerServices>()->getPlugInArgList()) != NULL);
   VERIFY(pArgList->addArg<Progress>(ProgressArg(), NULL));
   VERIFY(pArgList->addArg<std::string>(CommandArg(), std::string()));

   return true;
}

bool IdlInterpreter::getOutputSpecification(PlugInArgList*& pArgList)
{
   VERIFY((pArgList = Service<PlugInManagerServices>()->getPlugInArgList()) != NULL);
   VERIFY(pArgList->addArg<std::string>(OutputTextArg()));
   VERIFY(pArgList->addArg<std::string>(ReturnTypeArg()));

   return true;
}

bool IdlInterpreter::execute(PlugInArgList* pInArgList, PlugInArgList* pOutArgList)
{
   VERIFY(pInArgList != NULL && pOutArgList != NULL);
   if (!IdlInterpreterOptions::getSettingInteractiveAvailable())
   {
      std::string returnType("Error");
      std::string returnText = "Interactive interpreter has been disabled.";
      VERIFY(pOutArgList->setPlugInArgValue(ReturnTypeArg(), &returnType));
      VERIFY(pOutArgList->setPlugInArgValue(OutputTextArg(), &returnText));
      return true;
   }
   
   std::string command;
   if (!pInArgList->getPlugInArgValue(CommandArg(), command))
   {
      return false;
   }
   Progress* pProgress = pInArgList->getPlugInArgValue<Progress>(ProgressArg());
   std::vector<PlugIn*> plugins = Service<PlugInManagerServices>()->getPlugInInstances("IdlProxy");
   if (plugins.size() != 1)
   {
      return false;
   }
   IdlProxy* pProxy = dynamic_cast<IdlProxy*>(plugins.front());
   VERIFY(pProxy != NULL);

   std::string returnText;
   std::string errorText;
   if (!pProxy->processCommand(command, returnText, errorText, pProgress))
   {
      std::string returnType("Error");
      returnText += "\n" + errorText;
      VERIFY(pOutArgList->setPlugInArgValue(ReturnTypeArg(), &returnType));
      VERIFY(pOutArgList->setPlugInArgValue(OutputTextArg(), &returnText));
      return true;
   }
   // Populate the output arg list
   if (errorText.empty())
   {
      std::string returnType("Output");
      VERIFY(pOutArgList->setPlugInArgValue(ReturnTypeArg(), &returnType));
      VERIFY(pOutArgList->setPlugInArgValue(OutputTextArg(), &returnText));
   }
   else
   {
      std::string returnType("Error");
      VERIFY(pOutArgList->setPlugInArgValue(ReturnTypeArg(), &returnType));
      VERIFY(pOutArgList->setPlugInArgValue(OutputTextArg(), &errorText));
   }

   return true;
}

void IdlInterpreter::getKeywordList(std::vector<std::string>& list) const
{
   list.push_back("ARRAY_TO_IDL");
   list.push_back("REFRESH_DISPLAY");
   list.push_back("GET_CURRENT_NAME");
   list.push_back("GET_DATA_NAME");
   list.push_back("GET_LAYER_NAME");
   list.push_back("SET_LAYER_POSITION");
   list.push_back("GET_LAYER_POSITION");
   list.push_back("GET_NUM_LAYERS");
   list.push_back("SET_COLORMAP");
   list.push_back("GET_STRETCH_VALUES");
   list.push_back("SET_STRETCH_VALUES");
   list.push_back("SET_STRETCH_METHOD");
   list.push_back("GET_STRETCH_METHOD");
   list.push_back("SET_STRETCH_TYPE");
   list.push_back("GET_STRETCH_TYPE");
   list.push_back("ENABLE_FILTER");
   list.push_back("DISABLE_FILTER");
   list.push_back("SHOW_LAYER");
   list.push_back("HIDE_LAYER");
   list.push_back("DISABLE_GPU");
   list.push_back("ENABLE_GPU");
   list.push_back("EXECUTE_WIZARD");
   list.push_back("GET_METADATA");
   list.push_back("SET_METADATA");
   list.push_back("COPY_METADATA");
   list.push_back("ARRAY_TO_OPTICKS");
   list.push_back("CHANGE_DATA_TYPE");
   list.push_back("REPORT_PROGRESS");
}

bool IdlInterpreter::getKeywordDescription(const std::string& keyword, std::string& description) const
{
   if (keyword == "ARRAY_TO_IDL")
   {
      description = "Binds a dataset to an IDL variable as a 1 dimensional array";
   }
   else if (keyword == "GET_CURRENT_NAME")
   {
      description = "Method for getting the name of the currently selected element";
   }
   else if (keyword == "GET_DATA_NAME")
   {
      description = "Method for getting the name of the Spectral Element of a layer";
   }
   else if (keyword == "GET_LAYER_NAME")
   {
      description = "Method for getting the name of the layer at a position in the layer list";
   }
   else if (keyword == "SET_LAYER_POSITION")
   {
      description = "Method for setting the position in the layer list of a named layer";
   }
   else if (keyword == "GET_LAYER_POSITION")
   {
      description = "Method for getting the position in the layer list of a named layer";
   }
   else if (keyword == "GET_NUM_LAYERS")
   {
      description = "RMethod for getting the total number of layers in a window";
   }
   else if (keyword == "SET_COLORMAP")
   {
      description = "Method for setting the color map for a grayscale image";
   }
   else if (keyword == "GET_STRETCH_VALUES")
   {
      description = "Method for getting the stretch values of the named layer";
   }
   else if (keyword == "SET_STRETCH_VALUES")
   {
      description = "Method for setting the stretch values of the named layer";
   }
   else if (keyword == "SET_STRETCH_METHOD")
   {
      description = "Method for setting the stretch method of the named layer";
   }
   else if (keyword == "GET_STRETCH_METHOD")
   {
      description = "Method for getting the stretch method of the named layer";
   }
   else if (keyword == "SET_STRETCH_TYPE")
   {
      description = "Method for setting the stretch type of the named layer";
   }
   else if (keyword == "GET_STRETCH_TYPE")
   {
      description = "Method for getting the stretch type of the named layer";
   }
   else if (keyword == "ENABLE_FILTER")
   {
      description = "Method for disabling an applied filter to a GPU enabled layer";
   }
   else if (keyword == "DISABLE_FILTER")
   {
      description = "Method for disabling an applied filter to a GPU enabled layer";
   }
   else if (keyword == "SHOW_LAYER")
   {
      description = "Method for showing a hidden layer.";
   }
   else if (keyword == "HIDE_LAYER")
   {
      description = "Method for hiding a shown layer.";
   }
   else if (keyword == "DISABLE_GPU")
   {
      description = "Method for disabling the gpu of a layer.  The GPU is an advanced graphics modes which allows filters to be applied.";
   }
   else if (keyword == "ENABLE_GPU")
   {
      description = "Method for enabling the gpu of a layer.  The GPU is an advanced graphics modes which allows filters to be applied.";
   }
   else if (keyword == "EXECUTE_WIZARD")
   {
      description = "Method for executing a Wizard.  This is intended as a means to execute a plugin.";
   }
   else if (keyword == "GET_METADATA")
   {
      description = "Method for getting information is support areas of Opticks, such as the collection parameters, metadata or wizard objects";
   }
   else if (keyword == "SET_METADATA")
   {
      description = "Method for setting information is support areas of Opticks, such as the collection parameters, metadata or wizard objects";
   }
   else if (keyword == "COPY_METADATA")
   {
      description = "Method for copying metadata from one dataset to another dataset.";
   }
   else if (keyword == "ARRAY_TO_OPTICKS")
   {
      description = "Method for taking a 1 dimensional array from IDL space, and putting it in Opticks space.";
   }
   else if (keyword == "CHANGE_DATA_TYPE")
   {
      description = "Method for changing the data type of the RasterElement.  This function replaces the contents of the spectral cube, so any map_array function would have to be redone";
   }
   else if (keyword == "REPORT_PROGRESS")
   {
      description = "Reports progress, via progress oialog, if a progress object has been supplied to the plugin.";
   }
   else
   {
      return false;
   }

   return true;
}

void IdlInterpreter::getUserDefinedTypes(std::vector<std::string>&) const 
{
}

bool IdlInterpreter::getTypeDescription(const std::string&, std::string&) const 
{
   return false;
}

IdlInterpreterWizardItem::IdlInterpreterWizardItem()
{
   setName("IDL Interpreter");
   setDescription("Allow execution of IDL code from within a wizard.");
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
   VERIFY(pArgList->addArg<std::string>(IdlInterpreter::CommandArg(), std::string()));

   return true;
}

bool IdlInterpreterWizardItem::getOutputSpecification(PlugInArgList*& pArgList)
{
   VERIFY((pArgList = Service<PlugInManagerServices>()->getPlugInArgList()) != NULL);
   VERIFY(pArgList->addArg<std::string>(IdlInterpreter::OutputTextArg()));
   VERIFY(pArgList->addArg<std::string>(IdlInterpreter::ReturnTypeArg()));

   return true;
}

bool IdlInterpreterWizardItem::execute(PlugInArgList* pInArgList, PlugInArgList* pOutArgList)
{
   VERIFY(pInArgList != NULL && pOutArgList != NULL);
   
   std::string command;
   if (!pInArgList->getPlugInArgValue(IdlInterpreter::CommandArg(), command))
   {
      return false;
   }
   ProgressTracker progress(pInArgList->getPlugInArgValue<Progress>(ProgressArg()),
      "Execute IDL command.", "extras", "{592287e7-abbf-4581-9389-93b3bd5d8070}");

   std::vector<PlugIn*> plugins = Service<PlugInManagerServices>()->getPlugInInstances("IdlProxy");
   if (plugins.size() != 1)
   {
      progress.report("Unable to locate an IDL interpreter.", 0, ERRORS, true);
      return false;
   }
   IdlProxy* pProxy = dynamic_cast<IdlProxy*>(plugins.front());
   VERIFY(pProxy != NULL);

   progress.report("Executing IDL command.", 1, NORMAL);
   std::string returnText;
   std::string errorText;
   if (!pProxy->processCommand(command, returnText, errorText, progress.getCurrentProgress()))
   {
      progress.report("Error executing IDL command.", 0, ERRORS, true);
      return false;
   }
   // Populate the output arg list
   if (errorText.empty())
   {
      std::string returnType("Output");
      VERIFY(pOutArgList->setPlugInArgValue(IdlInterpreter::ReturnTypeArg(), &returnType));
      VERIFY(pOutArgList->setPlugInArgValue(IdlInterpreter::OutputTextArg(), &returnText));
   }
   else
   {
      std::string returnType("Error");
      VERIFY(pOutArgList->setPlugInArgValue(IdlInterpreter::ReturnTypeArg(), &returnType));
      VERIFY(pOutArgList->setPlugInArgValue(IdlInterpreter::OutputTextArg(), &errorText));
   }

   progress.report("Executing IDL command.", 100, NORMAL);
   progress.upALevel();
   return true;
}

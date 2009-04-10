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
#include "IDLVersion.h"
#include "IdlInterpreter.h"
#include "IdlInterpreterOptions.h"
#include "ModuleManager.h"
#include "PlugInArgList.h"
#include "PlugInFactory.h"
#include "PlugInManagerServices.h"

PLUGINFACTORY(IdlInterpreter);

IdlInterpreter::IdlInterpreter() : mIdlRunning(false)
{
   setName("IdlInterpreter");
   setDescription("Provides command line utilities to execute IDL commands.");
   setDescriptorId("{09BBB1FD-D12A-43B8-AB09-95AA8028BFFE}");
   setCopyright(IDL_COPYRIGHT);
   setVersion(IDL_VERSION_NUMBER);
   setProductionStatus(IDL_IS_PRODUCTION_RELEASE);
}

IdlInterpreter::~IdlInterpreter()
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
   
   if (!pInArgList->getPlugInArgValue(CommandArg(), mCommand))
   {
      return false;
   }
   Progress* pProgress = pInArgList->getPlugInArgValue<Progress>(ProgressArg());

   if (!mIdlRunning && !startIdl())
   {
      return false;
   }
   if (mModules.empty())
   {
      return false;
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
   const char*(*execute_idl)(const char*, Progress*) =
      reinterpret_cast<const char*(*)(const char*, Progress*)>(*pMod->getProcedureAddress("execute_idl"));
   VERIFY(execute_idl);

   //execute the function and capture the output
   std::string returnType = "Output";
   std::string returnText = execute_idl(mCommand.c_str(), pProgress);

   // Populate the output arg list
   VERIFY(pOutArgList->setPlugInArgValue(ReturnTypeArg(), &returnType));
   VERIFY(pOutArgList->setPlugInArgValue(OutputTextArg(), &returnText));

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

bool IdlInterpreter::startIdl()
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
         bool(*start_idl)(const char*, External*) =
            reinterpret_cast<bool(*)(const char*, External*)>(pModule->getProcedureAddress("start_idl"));
         if (start_idl != NULL && start_idl(idlDll.c_str(), pExternal) != 0)
         {
            mModules.push_back(pModule);
            mIdlRunning = true;
         }
      }
   }

   return mIdlRunning;
}

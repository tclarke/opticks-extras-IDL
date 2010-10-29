/*
 * The information in this file is
 * Copyright(c) 2007 Ball Aerospace & Technologies Corporation
 * and is subject to the terms and conditions of the
 * GNU Lesser General Public License Version 2.1
 * The license text is available from   
 * http://www.gnu.org/licenses/lgpl.html
 */

#include "AnimationCommands.h"
#include "AppConfig.h"
#include "AppVerify.h"
#include "ArrayCommands.h"
#include "GpuCommands.h"
#include "IdlFunctions.h"
#include "IdlStart.h"
#include "LayerCommands.h"
#include "MetadataCommands.h"
#include "MiscCommands.h"
#include "PlugInRegistration.h"
#include "VisualizationCommands.h"
#include "WindowCommands.h"

#include <string>
#include <stdio.h>
#include <idl_export.h>
#if defined(WIN_API)
#include <idl_callproxy.h>
#endif

Progress* spProgress = NULL;

namespace
{
   std::string startupOutput;
   output_callback_t spSendOutput = NULL;

   void PassthroughOutFunc(int flags, char* pBuf, int n)
   {
      if (n == 0 || spSendOutput == NULL)
      {
         return;
      }
      spSendOutput(pBuf, n, (flags & IDL_TOUT_F_STDERR), (flags & IDL_TOUT_F_NLPOST));
   }

   void StartupOutFunc(int flags, char* pBuf, int n)
   {
      // If there is a message, save it
      if (n != 0)
      {
         if (startupOutput.size() < startupOutput.max_size())
         {
            try
            {
               startupOutput += std::string(pBuf, n);
               if (flags & IDL_TOUT_F_NLPOST)
               {
                  startupOutput += "\n";
               }
            }
            catch(...)
            {
               startupOutput.clear();
               startupOutput = "could not generate output\n";
            }
         }
      }
   }
}

extern "C" LINKAGE int start_idl(const char* pLocation,
                                 External* pExternal,
                                 const char** pOutput,
                                 output_callback_t pReportOutput)
{
   VERIFYRV(pExternal != NULL, 0);
   ModuleManager::instance()->setService(pExternal);
   static int initialized = 0;
   if (initialized++ > 0)
   {
      return 1;
   }
#ifdef WIN_API
   /** Enable this to debug load problems **/
   //IDL_CallProxyDebug(IDL_CPDEBUG_ALL);

   if (IDL_CallProxyInit(const_cast<char*>(pLocation)) == 0)
   {
      return 0;
   }
#endif
   // Register our output function
   startupOutput.clear();
   IDL_ToutPush(StartupOutFunc);
   // IDL seems to push the "welcome" banner to stderr so
   // we need to do this in order to see the banner and the
   // output of the first command

#if IDL_VERSION_MAJOR == 6 && IDL_VERSION_MINOR <= 1
# if defined(WIN_API)
   if (IDL_Win32Init(IDL_INIT_BACKGROUND, NULL, NULL, NULL) == 0)
   {
      return 0;
   }
# else
   if (IDL_Init(IDL_INIT_BACKGROUND, NULL, NULL) == 0)
   {
      return 0;
   }
# endif
#else
   IDL_INIT_DATA initData;
   initData.options = IDL_INIT_BACKGROUND;
   if (IDL_Initialize(&initData) == 0)
   {
      return 0;
   }
#endif

   // Add system routines
   bool success = true;
   success &= addAnimationCommands();
   success &= addArrayCommands();
   success &= addGpuCommands();
   success &= addLayerCommands();
   success &= addMetadataCommands();
   success &= addMiscCommands();
   success &= addVisualizationCommands();
   success &= addWindowCommands();
   if (!success)
   {
      IDL_Message(IDL_M_GENERIC, IDL_MSG_RET, "Error adding system routines.");
   }
   if (pOutput != NULL)
   {
      *pOutput = startupOutput.c_str();
   }
   IDL_ToutPop();

   if (success)
   {
      spSendOutput = pReportOutput;
      IDL_ToutPush(PassthroughOutFunc);
   }

   return success ? 1 : 0;
}

extern "C" LINKAGE int execute_idl(int commandCount,
                                   char** pCommands,
                                   int scope,
                                   Progress* pProgress)
{
   spProgress = pProgress;
   if (scope != 0)
   {
      IDL_ExecuteStr("MESSAGE, /RESET");
   }
   //the idl command to execute the contents of the passed in std::string
   int retVal = IDL_Execute(commandCount, pCommands);
   if (scope != 0)
   {
      IDL_ExecuteStr("MESSAGE, /RESET");
   }
   spProgress = NULL;
   return retVal;
}

extern "C" LINKAGE int close_idl()
{
   IdlFunctions::cleanupWizardObjects();
   spSendOutput = NULL;
   IDL_ToutPop();
   IDL_Cleanup(IDL_TRUE);
   return 1;
}
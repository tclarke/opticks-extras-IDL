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
   std::string output;
   std::string errorOutput;
   bool execSuccess = true;

   void OutFunc(int flags, char* pBuf, int n)
   {
      std::string& out = (flags & IDL_TOUT_F_STDERR) ? errorOutput : output;
      // If there is a message, save it
      if (n != 0)
      {
         if (out.size() < out.max_size() && execSuccess)
         {
            try
            {
               out += std::string(pBuf, n);
               if (flags & IDL_TOUT_F_NLPOST)
               {
                  out += "\n";
               }
            }
            catch(...)
            {
               errorOutput.clear();
               errorOutput = "could not generate output\n";
               execSuccess = false;
            }
         }
      }
   }
}

extern "C" LINKAGE int start_idl(const char* pLocation, External* pExternal, const char** pOutput, const char** pErrorOutput)
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
   IDL_ToutPush(OutFunc);

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
      if (pErrorOutput != NULL)
      {
         *pErrorOutput = errorOutput.c_str();
      }
   }
   // IDL seems to push the "welcome" banner to stderr so
   // we need to do this in order to see the banner and the
   // output of the first command
   if (pOutput != NULL)
   {
      output = errorOutput + "\n" + output;
      *pOutput = output.c_str();
   }
   else if (pErrorOutput != NULL)
   {
      *pErrorOutput = errorOutput.c_str();
   }
 
   return success ? 1 : 0;
}

extern "C" LINKAGE void execute_idl(const char* pCommand, const char** pOutput, const char** pErrorOutput, Progress* pProgress)
{
   spProgress = pProgress;
   execSuccess = true;
   output.clear();
   errorOutput.clear();
   //the idl command to execute the contents of the passed in std::string
   IDL_ExecuteStr(const_cast<char*>(pCommand));
   spProgress = NULL;
   if (pOutput != NULL)
   {
      *pOutput = output.c_str();
   }
   if (pErrorOutput != NULL)
   {
      *pErrorOutput = errorOutput.c_str();
   }
}
/*
 * The information in this file is
 * subject to the terms and conditions of the
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
#include "ModuleManager.h"
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
bool success = true;

void OutFunc(int flags, char* pBuf, int n)
{
   // If there is a message, save it
   if (n != 0)
   {
      unsigned int n = output.max_size();
      if (output.size() < n && success)
      {
         try
         {
            if (output.empty())
            {
               output += pBuf;
            }
            else
            {
               output += "\n";
               output += pBuf;
            }
         }
         catch(...)
         {
            output.clear();
            output = "could not generate output\n";
            success = false;
         }
      }
   }
}
}

IDLSTART_EXPORT int start_idl(const char* pLocation, External* pExternal)
{
   VERIFYRV(pExternal != NULL, 0);
   ModuleManager::instance()->setService(pExternal);
   if (IDL_CallProxyInit(const_cast<char*>(pLocation)) == 0)
   {
      return 0;
   }
   // Register our output function
   IDL_ToutPush(OutFunc);

#ifdef WIN_API
   VERIFYRV(IDL_Win32Init(0, 0, 0, NULL), 0);
#else
   VERIFYRV(IDL_Init(0, NULL, 0), 0);
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

   return success ? 1 : 0;
}

IDLSTART_EXPORT const char* execute_idl(const char* pCommand, Progress* pProgress)
{
   spProgress = pProgress;
   success = true;
   output.clear();
   //the idl command to execute the contents of the passed in std::string
   IDL_ExecuteStr(const_cast<char*>(pCommand));
   spProgress = NULL;
   return output.c_str();
}

IDLSTART_EXPORT int close_idl()
{
   IDL_ToutPop();
   IDL_Cleanup(IDL_FALSE);
   return 1;
}

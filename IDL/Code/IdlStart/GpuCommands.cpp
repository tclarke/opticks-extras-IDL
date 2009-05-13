/*
 * The information in this file is
 * Copyright(c) 2007 Ball Aerospace & Technologies Corporation
 * and is subject to the terms and conditions of the
 * GNU Lesser General Public License Version 2.1
 * The license text is available from   
 * http://www.gnu.org/licenses/lgpl.html
 */

#include "GpuCommands.h"
#include "IdlFunctions.h"
#include "RasterLayer.h"

#include <string>
#include <vector>
#include <stdio.h>
#include <idl_export.h>

/**
 * \defgroup gpucommands GPU Commands
 */
/*@{*/

/**
 * Enable a named GPU filter.
 * Dynamic texture generation must be active for this to have any effect.
 *
 * @param[in] [1]
 *            The name of the filter to enable.
 * @param[in] LAYER @opt
 *            The name of the raster layer to update. Defaults to
 *            the layer containing the primary raster element.
 * @param[in] WINDOW @opt
 *            The name of the window to update. Defaults to the active window.
 * @rsof
 * @usage print,enable_filter("EdgeDetection")
 * @endusage
 */
IDL_VPTR enable_filter(int argc, IDL_VPTR pArgv[], char* pArgk)
{
   IDL_VPTR idlPtr;
   typedef struct
   {
      IDL_KW_RESULT_FIRST_FIELD;
      int windowExists;
      IDL_STRING windowName;
      int layerNameExists;
      IDL_STRING layerName;
   } KW_RESULT;
   //IDL_KW_FAST_SCAN is the type of scan we are using, following it is the
   //name of the keyword, followed by the type, the mask(which should be 1),
   //flags, a boolean whether the value was populated and finally the value itself

   static IDL_KW_PAR kw_pars[] = {
      IDL_KW_FAST_SCAN,
      {"LAYER", IDL_TYP_STRING, 1, 0, reinterpret_cast<int*>(IDL_KW_OFFSETOF(layerNameExists)),
         reinterpret_cast<char*>(IDL_KW_OFFSETOF(layerName))},
      {"WINDOW", IDL_TYP_STRING, 1, 0, reinterpret_cast<int*>(IDL_KW_OFFSETOF(windowExists)),
         reinterpret_cast<char*>(IDL_KW_OFFSETOF(windowName))},
      {NULL}
   };

   IdlFunctions::IdlKwResource<KW_RESULT> kw(argc, pArgv, pArgk, kw_pars, 0, 1);

   std::string windowName;
   std::string layerName;
   std::string filter;

   if (kw->windowExists)
   {
      windowName = IDL_STRING_STR(&kw->windowName);
   }
   if (kw->layerNameExists)
   {
      layerName = IDL_STRING_STR(&kw->layerName);
   }
   bool bSuccess = false;
   if (argc < 1)
   {
      IDL_Message(IDL_M_GENERIC, IDL_MSG_RET, "function takes "
         "'window' and 'layer' as optional keywords.");
      return IDL_StrToSTRING("failure");
   }
   filter = IDL_VarGetString(pArgv[0]);
   RasterLayer* pLayer = dynamic_cast<RasterLayer*>(IdlFunctions::getLayerByName(windowName, layerName));
   if (pLayer != NULL)
   {
      //determine the type, allow strings to be converted into an enum for later filters
      std::vector<std::string> supportedFilters = pLayer->getSupportedFilters();
      for (std::vector<std::string>::const_iterator filtersIt = supportedFilters.begin();
           filtersIt != supportedFilters.end(); ++filtersIt)
      {
         if (filter == *filtersIt)
         {
            pLayer->enableFilter(*filtersIt);
            bSuccess = true;
            break;
         }
      }
   }
   if (bSuccess)
   {
      idlPtr = IDL_StrToSTRING("success");
   }
   else
   {
      idlPtr = IDL_StrToSTRING("failure");
   }
   return idlPtr;
}

/**
 * Disable a named GPU filter.
 *
 * @param[in] [1]
 *            The name of the filter to disable.
 * @param[in] LAYER @opt
 *            The name of the raster layer to update. Defaults to
 *            the layer containing the primary raster element.
 * @param[in] WINDOW @opt
 *            The name of the window to update. Defaults to the active window.
 * @rsof
 * @usage print,disable_filter("EdgeDetection")
 * @endusage
 */
IDL_VPTR disable_filter(int argc, IDL_VPTR pArgv[], char* pArgk)
{
   IDL_VPTR idlPtr;
   typedef struct
   {
      IDL_KW_RESULT_FIRST_FIELD;
      int windowExists;
      IDL_STRING windowName;
      int layerNameExists;
      IDL_STRING layerName;
   } KW_RESULT;

   //IDL_KW_FAST_SCAN is the type of scan we are using, following it is the
   //name of the keyword, followed by the type, the mask(which should be 1),
   //flags, a boolean whether the value was populated and finally the value itself
   static IDL_KW_PAR kw_pars[] = {
      IDL_KW_FAST_SCAN,
      {"LAYER", IDL_TYP_STRING, 1, 0, reinterpret_cast<int*>(IDL_KW_OFFSETOF(layerNameExists)),
         reinterpret_cast<char*>(IDL_KW_OFFSETOF(layerName))},
      {"WINDOW", IDL_TYP_STRING, 1, 0, reinterpret_cast<int*>(IDL_KW_OFFSETOF(windowExists)),
         reinterpret_cast<char*>(IDL_KW_OFFSETOF(windowName))},
      {NULL}
   };

   IdlFunctions::IdlKwResource<KW_RESULT> kw(argc, pArgv, pArgk, kw_pars, 0, 1);

   std::string windowName;
   std::string layerName;
   std::string filter;
   bool bSuccess = false;

   if (kw->windowExists)
   {
      windowName = IDL_STRING_STR(&kw->windowName);
   }
   if (kw->layerNameExists)
   {
      layerName = IDL_STRING_STR(&kw->layerName);
   }

   if (argc < 1)
   {
      IDL_Message(IDL_M_GENERIC, IDL_MSG_RET, "function takes  "
         "'window' and 'layer' as optional keywords.");
      return IDL_StrToSTRING("failure");
   }
   filter = IDL_VarGetString(pArgv[0]);
   RasterLayer* pLayer = dynamic_cast<RasterLayer*>(IdlFunctions::getLayerByName(windowName, layerName));
   if (pLayer != NULL)
   {
      //determine the type, allow strings to be converted into an enum for later filters
      std::vector<std::string> supportedFilters = pLayer->getSupportedFilters();
      for (std::vector<std::string>::const_iterator filtersIt = supportedFilters.begin();
           filtersIt != supportedFilters.end(); ++filtersIt)
      {
         if (filter == *filtersIt)
         {
            pLayer->disableFilter(*filtersIt);
            bSuccess = true;
            break;
         }
      }
   }
   if (bSuccess)
   {
      idlPtr = IDL_StrToSTRING("success");
   }
   else
   {
      idlPtr = IDL_StrToSTRING("failure");
   }
   return idlPtr;
}

/**
 * Disable GPU processing for a raster layer.
 *
 * @param[in] [1]
 *            The name of the raster layer to update.
 * @param[in] WINDOW @opt
 *            The name of the window to update. Defaults to the active window.
 * @rsof
 * @usage print,disable_gpu("layer.tif")
 * @endusage
 */
IDL_VPTR disable_gpu(int argc, IDL_VPTR pArgv[], char* pArgk)
{
   IDL_VPTR idlPtr;
   typedef struct
   {
      IDL_KW_RESULT_FIRST_FIELD;
      int windowExists;
      IDL_STRING windowName;
   } KW_RESULT;

   //IDL_KW_FAST_SCAN is the type of scan we are using, following it is the
   //name of the keyword, followed by the type, the mask(which should be 1),
   //flags, a boolean whether the value was populated and finally the value itself
   static IDL_KW_PAR kw_pars[] = {
      IDL_KW_FAST_SCAN,
      {"WINDOW", IDL_TYP_STRING, 1, 0, reinterpret_cast<int*>(IDL_KW_OFFSETOF(windowExists)),
         reinterpret_cast<char*>(IDL_KW_OFFSETOF(windowName))},
      {NULL}
   };

   IdlFunctions::IdlKwResource<KW_RESULT> kw(argc, pArgv, pArgk, kw_pars, 0, 1);

   std::string windowName;
   std::string layerName;

   if (kw->windowExists)
   {
      windowName = IDL_STRING_STR(&kw->windowName);
   }
   bool bSuccess = false;
   if (argc < 1)
   {
      IDL_Message(IDL_M_GENERIC, IDL_MSG_RET, "function takes a layer name as a parameter with a "
         "'window' as an optional keyword.");
      return IDL_StrToSTRING("failure");
   }
   //the layer name as a parameter
   layerName = IDL_VarGetString(pArgv[0]);
   RasterLayer* pLayer = dynamic_cast<RasterLayer*>(IdlFunctions::getLayerByName(windowName, layerName));
   if (pLayer != NULL)
   {
      pLayer->enableGpuImage(false);
      bSuccess = true;
   }
   if (bSuccess)
   {
      idlPtr = IDL_StrToSTRING("success");
   }
   else
   {
      idlPtr = IDL_StrToSTRING("failure");
   }
   return idlPtr;
}

/**
 * Enable GPU processing for a raster layer.
 * This enables dynamic texture generation and GPU filters.
 *
 * @param[in] [1]
 *            The name of the raster layer to update.
 * @param[in] WINDOW @opt
 *            The name of the window to update. Defaults to the active window.
 * @rsof
 * @usage print,enable_gpu("layer.tif")
 * @endusage
 */
IDL_VPTR enable_gpu(int argc, IDL_VPTR pArgv[], char* pArgk)
{
   IDL_VPTR idlPtr;
   typedef struct
   {
      IDL_KW_RESULT_FIRST_FIELD;
      int windowExists;
      IDL_STRING windowName;
   } KW_RESULT;

   //IDL_KW_FAST_SCAN is the type of scan we are using, following it is the
   //name of the keyword, followed by the type, the mask(which should be 1),
   //flags, a boolean whether the value was populated and finally the value itself
   static IDL_KW_PAR kw_pars[] = {
      IDL_KW_FAST_SCAN,
      {"WINDOW", IDL_TYP_STRING, 1, 0, reinterpret_cast<int*>(IDL_KW_OFFSETOF(windowExists)),
         reinterpret_cast<char*>(IDL_KW_OFFSETOF(windowName))},
      {NULL}
   };

   IdlFunctions::IdlKwResource<KW_RESULT> kw(argc, pArgv, pArgk, kw_pars, 0, 1);

   std::string windowName;
   std::string layerName;
   bool bSuccess = false;

   if (kw->windowExists)
   {
      windowName = IDL_STRING_STR(&kw->windowName);
   }

   if (argc < 1)
   {
      IDL_Message(IDL_M_GENERIC, IDL_MSG_RET, "function takes a layer name as a parameter with a "
         "'window' as an optional keyword.");
      return IDL_StrToSTRING("failure");
   }
   //the layer name as a parameter
   layerName = IDL_VarGetString(pArgv[0]);
   RasterLayer* pLayer = dynamic_cast<RasterLayer*>(IdlFunctions::getLayerByName(windowName, layerName));
   if (pLayer != NULL)
   {
      pLayer->enableGpuImage(true);
      bSuccess = true;
   }
   if (bSuccess)
   {
      idlPtr = IDL_StrToSTRING("success");
   }
   else
   {
      idlPtr = IDL_StrToSTRING("failure");
   }
   return idlPtr;
}
/*@}*/

static IDL_SYSFUN_DEF2 func_definitions[] = {
   {reinterpret_cast<IDL_SYSRTN_GENERIC>(disable_filter), "DISABLE_FILTER",0,5,IDL_SYSFUN_DEF_F_KEYWORDS,0},
   {reinterpret_cast<IDL_SYSRTN_GENERIC>(disable_gpu), "DISABLE_GPU",0,5,IDL_SYSFUN_DEF_F_KEYWORDS,0},
   {reinterpret_cast<IDL_SYSRTN_GENERIC>(enable_filter), "ENABLE_FILTER",0,5,IDL_SYSFUN_DEF_F_KEYWORDS,0},
   {reinterpret_cast<IDL_SYSRTN_GENERIC>(enable_gpu), "ENABLE_GPU",0,5,IDL_SYSFUN_DEF_F_KEYWORDS,0},
   {NULL, NULL, 0, 0, 0, 0}
};

bool addGpuCommands()
{
   int funcCnt = -1;
   while (func_definitions[++funcCnt].name != NULL); // do nothing in the loop body
   return IDL_SysRtnAdd(func_definitions, IDL_TRUE, funcCnt) != 0;
}

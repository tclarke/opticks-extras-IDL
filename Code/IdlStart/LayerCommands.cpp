/*
 * The information in this file is
 * Copyright(c) 2007 Ball Aerospace & Technologies Corporation
 * and is subject to the terms and conditions of the
 * GNU Lesser General Public License Version 2.1
 * The license text is available from   
 * http://www.gnu.org/licenses/lgpl.html
 */

#include "DesktopServices.h"
#include "IdlFunctions.h"
#include "Layer.h"
#include "LayerList.h"
#include "LayerCommands.h"
#include "RasterElement.h"
#include "SpatialDataView.h"
#include "SpatialDataWindow.h"

#include <string>
#include <stdio.h>
#include <idl_export.h>
#include <idl_callproxy.h>

IDL_VPTR get_current_name(int argc, IDL_VPTR pArgv[], char* pArgk)
{
   typedef struct
   {
      IDL_KW_RESULT_FIRST_FIELD;
      int datasetExists;
      IDL_LONG dataset;
      int fileExists;
      IDL_LONG file;
      int windowExists;
      IDL_LONG window;
   } KW_RESULT;
   KW_RESULT kw;

   //IDL_KW_FAST_SCAN is the type of scan we are using, following it is the
   //name of the keyword, followed by the type, the mask(which should be 1),
   //flags, a boolean whether the value was populated and finally the value itself
   static IDL_KW_PAR kw_pars[] = {
      IDL_KW_FAST_SCAN,
      {"DATASET", IDL_TYP_INT, 1, 0, reinterpret_cast<int*>(IDL_KW_OFFSETOF(datasetExists)),
            reinterpret_cast<char*>(IDL_KW_OFFSETOF(dataset))},
      {"FILE", IDL_TYP_INT, 1, 0, reinterpret_cast<int*>(IDL_KW_OFFSETOF(fileExists)),
            reinterpret_cast<char*>(IDL_KW_OFFSETOF(file))},
      {"WINDOW", IDL_TYP_INT, 1, 0, reinterpret_cast<int*>(IDL_KW_OFFSETOF(windowExists)),
            reinterpret_cast<char*>(IDL_KW_OFFSETOF(window))},
      {NULL}
   };

   IDL_KWProcessByOffset(argc, pArgv, pArgk, kw_pars, 0, 1, &kw);

   std::string filename;
   std::string wizardName;
   int file = 0;
   int dataset = 0;
   int window = 0;

   if (kw.datasetExists)
   {
      if (kw.dataset != 0)
      {
         dataset = 1;
      }
   }
   else if (kw.fileExists)
   {
      if (kw.file != 0)
      {
         file = 1;
      }
   }
   else if (kw.windowExists)
   {
      if (kw.window != 0)
      {
         window = 1;
      }
   }
   else
   {
      dataset = 1;
   }

   RasterElement* pElement = IdlFunctions::getDataset("");
   if (pElement == NULL)
   {
      IDL_Message(IDL_M_GENERIC, IDL_MSG_RET, "Opticks was unable to determine the current in memory dataset.");
      return IDL_StrToSTRING("");
   }
   std::string name;
   if (dataset != 0)
   {
      // get the name of the RasterElement if dataset
      name = pElement->getName();
   }
   else if (file)
   {
      // get the filename associated with the RasterElement
      name = pElement->getFilename();
   }
   else
   {
      // get the window name
      Service<DesktopServices>()->getCurrentWorkspaceWindowName(name);
   }
   IDL_KW_FREE;
   return IDL_StrToSTRING(const_cast<char*>(name.c_str()));
}

IDL_VPTR get_data_name(int argc, IDL_VPTR pArgv[], char* pArgk)
{
   typedef struct
   {
      IDL_KW_RESULT_FIRST_FIELD;
      int windowExists;
      IDL_STRING windowName;
      int datasetExists;
      IDL_LONG dataset;
   } KW_RESULT;
   KW_RESULT kw;

   //IDL_KW_FAST_SCAN is the type of scan we are using, following it is the
   //name of the keyword, followed by the type, the mask(which should be 1),
   //flags, a boolean whether the value was populated and finally the value itself
   static IDL_KW_PAR kw_pars[] = {
      IDL_KW_FAST_SCAN,
      {"DATASET", IDL_TYP_INT, 1, 0, reinterpret_cast<int*>(IDL_KW_OFFSETOF(datasetExists)),
         reinterpret_cast<char*>(IDL_KW_OFFSETOF(dataset))},
      {"WINDOW", IDL_TYP_STRING, 1, 0, reinterpret_cast<int*>(IDL_KW_OFFSETOF(windowExists)),
         reinterpret_cast<char*>(IDL_KW_OFFSETOF(windowName))},
      {NULL}
   };

   IDL_KWProcessByOffset(argc, pArgv, pArgk, kw_pars, 0, 1, &kw);
   std::string windowName;
   std::string layerName;
   std::string name;

   if (kw.windowExists)
   {
      windowName = IDL_STRING_STR(&kw.windowName);
   }

   //retrieve the layer name passed in as a parameter
   layerName = IDL_VarGetString(pArgv[0]);
   //get the layer
   bool datasets = false;
   if (kw.datasetExists)
   {
      if (kw.dataset != 0)
      {
         datasets = true;
      }
   }
   Layer* pLayer = IdlFunctions::getLayerByName(windowName, layerName, datasets);
   if (pLayer != NULL)
   {
      //get the spectral element of the layer and return its name
      DataElement* pElement = pLayer->getDataElement();
      if (pElement != NULL)
      {
         name = pElement->getName();
      }
   }
   else
   {
      IDL_Message(IDL_M_GENERIC, IDL_MSG_RET, "the layer name passed into get_data_name "
         "was invalid.");
      IDL_KW_FREE;
      return IDL_StrToSTRING("");
   }

   IDL_KW_FREE;
   return IDL_StrToSTRING(const_cast<char*>(name.c_str()));
}

IDL_VPTR get_data_element_names(int argc, IDL_VPTR pArgv[], char* pArgk)
{
   typedef struct
   {
      IDL_KW_RESULT_FIRST_FIELD;
      int windowExists;
      IDL_STRING windowName;
   } KW_RESULT;
   KW_RESULT kw;

   //IDL_KW_FAST_SCAN is the type of scan we are using, following it is the
   //name of the keyword, followed by the type, the mask(which should be 1),
   //flags, a boolean whether the value was populated and finally the value itself
   static IDL_KW_PAR kw_pars[] = {
      IDL_KW_FAST_SCAN,
      {"WINDOW", IDL_TYP_STRING, 1, 0, reinterpret_cast<int*>(IDL_KW_OFFSETOF(windowExists)),
         reinterpret_cast<char*>(IDL_KW_OFFSETOF(windowName))},
      {NULL}
   };

   IDL_KWProcessByOffset(argc, pArgv, pArgk, kw_pars, 0, 1, &kw);
   std::string windowName;
   std::string name;
   bool bSuccess = false;
   IDL_VPTR idlPtr;
   unsigned int total = 0;
   IDL_STRING* pStrarr = NULL;

   if (kw.windowExists)
   {
      windowName = IDL_STRING_STR(&kw.windowName);
   }

   SpatialDataWindow* pWindow = NULL;
   if (windowName.empty())
   {
      pWindow = dynamic_cast<SpatialDataWindow*>(Service<DesktopServices>()->getCurrentWorkspaceWindow());
   }
   else
   {
      pWindow = dynamic_cast<SpatialDataWindow*>(Service<DesktopServices>()->getWindow(windowName, SPATIAL_DATA_WINDOW));
   }
   if (pWindow != NULL)
   {
      SpatialDataView* pView = pWindow->getSpatialDataView();
      if (pView != NULL)
      {
         LayerList* pList = pView->getLayerList();
         if (pList != NULL)
         {
            RasterElement* pElement = pList->getPrimaryRasterElement();
            if (pElement != NULL)
            {
               std::vector<std::string> names = Service<ModelServices>()->getElementNames(pElement, "");
               total = names.size();
               pStrarr = reinterpret_cast<IDL_STRING*>(IDL_MemAlloc(total * sizeof(IDL_STRING), NULL, 0));
               for (unsigned int i=0; i < total; i++)
               {
                  IDL_StrStore(&(pStrarr[i]), const_cast<char*>(names[i].c_str()));
               }
               bSuccess = true;
            }
         }
      }
      if (windowName == "all")
      {
         std::vector<std::string> names = Service<ModelServices>()->getElementNames("RasterElement");
         total = names.size();
         pStrarr = reinterpret_cast<IDL_STRING*>(IDL_MemAlloc(total * sizeof(IDL_STRING), NULL, 0));
         for (unsigned int i=0; i < total; i++)
         {
            IDL_StrStore(&(pStrarr[i]), const_cast<char*>(names[i].c_str()));
         }
         bSuccess = true;
      }
   }
   if (!bSuccess)
   {
      pStrarr = reinterpret_cast<IDL_STRING*>(IDL_MemAlloc(1 * sizeof(IDL_STRING), NULL, 0));
      char* pEmpty = "";
      IDL_StrStore(&(pStrarr[0]), pEmpty);
   }
   IDL_MEMINT dims[] = {total};
   idlPtr = IDL_ImportArray(1, dims, IDL_TYP_STRING, reinterpret_cast<UCHAR*>(pStrarr), NULL, NULL);
   IDL_KW_FREE;
   return idlPtr;
}

IDL_VPTR get_layer_name(int argc, IDL_VPTR pArgv[], char* pArgk)
{
   typedef struct
   {
      IDL_KW_RESULT_FIRST_FIELD;
      int windowExists;
      IDL_STRING windowName;
      int indexExists;
      IDL_LONG index;
   } KW_RESULT;
   KW_RESULT kw;

   //IDL_KW_FAST_SCAN is the type of scan we are using, following it is the
   //name of the keyword, followed by the type, the mask(which should be 1),
   //flags, a boolean whether the value was populated and finally the value itself
   static IDL_KW_PAR kw_pars[] = {
      IDL_KW_FAST_SCAN,
      {"INDEX", IDL_TYP_LONG, 1, 0, reinterpret_cast<int*>(IDL_KW_OFFSETOF(indexExists)),
         reinterpret_cast<char*>(IDL_KW_OFFSETOF(index))},
      {"WINDOW", IDL_TYP_STRING, 1, 0, reinterpret_cast<int*>(IDL_KW_OFFSETOF(windowExists)),
         reinterpret_cast<char*>(IDL_KW_OFFSETOF(windowName))},
      {NULL}
   };

   IDL_KWProcessByOffset(argc, pArgv, pArgk, kw_pars, 0, 1, &kw);

   std::string windowName;
   int index = -1;
   std::string name;

   if (kw.indexExists)
   {
      index = kw.index;
   }
   if (kw.windowExists)
   {
      windowName = IDL_STRING_STR(&kw.windowName);
   }

   if (argc < 1)
   {
      IDL_Message(IDL_M_GENERIC, IDL_MSG_RET, "get_layer_name takes a layer position as a keyword with a "
         "window as an optional keyword to specifiy a non current window.");
      IDL_KW_FREE;
      return IDL_StrToSTRING("");
   }
   Layer* pLayer = IdlFunctions::getLayerByIndex(windowName, index);
   if (pLayer != NULL)
   {
      name =  pLayer->getName();
   }
   IDL_KW_FREE;
   return IDL_StrToSTRING(const_cast<char*>(name.c_str()));
}

IDL_VPTR set_layer_position(int argc, IDL_VPTR pArgv[], char* pArgk)
{
   IDL_VPTR idlPtr;
   typedef struct
   {
      IDL_KW_RESULT_FIRST_FIELD;
      int windowExists;
      IDL_STRING windowName;
      int indexExists;
      IDL_LONG index;
   } KW_RESULT;
   KW_RESULT kw;

   //IDL_KW_FAST_SCAN is the type of scan we are using, following it is the
   //name of the keyword, followed by the type, the mask(which should be 1),
   //flags, a boolean whether the value was populated and finally the value itself
   static IDL_KW_PAR kw_pars[] = {
      IDL_KW_FAST_SCAN,
      {"INDEX", IDL_TYP_LONG, 1, 0, reinterpret_cast<int*>(IDL_KW_OFFSETOF(indexExists)),
         reinterpret_cast<char*>(IDL_KW_OFFSETOF(index))},
      {"WINDOW", IDL_TYP_STRING, 1, 0, reinterpret_cast<int*>(IDL_KW_OFFSETOF(windowExists)),
         reinterpret_cast<char*>(IDL_KW_OFFSETOF(windowName))},
      {NULL}
   };

   IDL_KWProcessByOffset(argc, pArgv, pArgk, kw_pars, 0, 1, &kw);

   std::string windowName;
   int index = -1;
   std::string name;

   if (kw.indexExists)
   {
      index = kw.index;
   }
   if (kw.windowExists)
   {
      windowName = IDL_STRING_STR(&kw.windowName);
   }

   if (argc < 1)
   {
      IDL_Message(IDL_M_GENERIC, IDL_MSG_RET, "set_layer_position takes a layer name as a parameter with a "
         "window as an optional keyword.  A keyword 'index' is needed to specify the position.");
      IDL_KW_FREE;
      return IDL_StrToSTRING("failure");
   }
   //the name of the layer to set the posisiton of
   name = IDL_VarGetString(pArgv[0]);
   bool bSuccess = false;
   SpatialDataView* pView = dynamic_cast<SpatialDataView*>(IdlFunctions::getViewByWindowName(windowName));
   if (pView != NULL)
   {
      Layer* pLayer = IdlFunctions::getLayerByName(windowName, name);
      if (pLayer != NULL)
      {
         pView->setLayerDisplayIndex(pLayer, index);
         bSuccess = true;
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
   IDL_KW_FREE;
   return idlPtr;
}

IDL_VPTR get_layer_position(int argc, IDL_VPTR pArgv[], char* pArgk)
{
   typedef struct
   {
      IDL_KW_RESULT_FIRST_FIELD;
      int windowExists;
      IDL_STRING windowName;
   } KW_RESULT;
   KW_RESULT kw;

   //IDL_KW_FAST_SCAN is the type of scan we are using, following it is the
   //name of the keyword, followed by the type, the mask(which should be 1),
   //flags, a boolean whether the value was populated and finally the value itself
   static IDL_KW_PAR kw_pars[] = {
      IDL_KW_FAST_SCAN,
      {"WINDOW", IDL_TYP_STRING, 1, 0, reinterpret_cast<int*>(IDL_KW_OFFSETOF(windowExists)),
         reinterpret_cast<char*>(IDL_KW_OFFSETOF(windowName))},
      {NULL}
   };

   IDL_KWProcessByOffset(argc, pArgv, pArgk, kw_pars, 0, 1, &kw);

   std::string windowName;
   std::string name;
   int index = -1;

   if (kw.windowExists)
   {
      windowName = IDL_STRING_STR(&kw.windowName);
   }

   if (argc < 1)
   {
      IDL_Message(IDL_M_GENERIC, IDL_MSG_RET, "get_layer_position takes a layer name as a parameter with a "
         "window as an optional keyword to specifiy a non current window.");
   }
   else
   {
      //layer name passed in as a parameter
      name = IDL_VarGetString(pArgv[0]);
      SpatialDataView* pView = dynamic_cast<SpatialDataView*>(IdlFunctions::getViewByWindowName(windowName));
      if (pView != NULL)
      {
         Layer* pLayer = IdlFunctions::getLayerByName(windowName, name);
         if (pLayer != NULL)
         {
            index = pView->getLayerDisplayIndex(pLayer);
         }
      }
   }
   IDL_KW_FREE;
   return IDL_GettmpInt(index);
}

IDL_VPTR get_num_layers(int argc, IDL_VPTR pArgv[], char* pArgk)
{
   typedef struct
   {
      IDL_KW_RESULT_FIRST_FIELD;
      int windowExists;
      IDL_STRING windowName;
   } KW_RESULT;
   KW_RESULT kw;

   //IDL_KW_FAST_SCAN is the type of scan we are using, following it is the
   //name of the keyword, followed by the type, the mask(which should be 1),
   //flags, a boolean whether the value was populated and finally the value itself
   static IDL_KW_PAR kw_pars[] = {
      IDL_KW_FAST_SCAN,
      {"WINDOW", IDL_TYP_STRING, 1, 0, reinterpret_cast<int*>(IDL_KW_OFFSETOF(windowExists)),
         reinterpret_cast<char*>(IDL_KW_OFFSETOF(windowName))},
      {NULL}
   };

   IDL_KWProcessByOffset(argc, pArgv, pArgk, kw_pars, 0, 1, &kw);

   std::string windowName;
   int layers = 0;

   if (kw.windowExists)
   {
      windowName = IDL_STRING_STR(&kw.windowName);
   }

   SpatialDataView* pView = dynamic_cast<SpatialDataView*>(IdlFunctions::getViewByWindowName(windowName));
   if (pView != NULL)
   {
      LayerList* pList = pView->getLayerList();
      if (pList != NULL)
      {
         layers = pList->getNumLayers();
      }
   }
   IDL_KW_FREE;
   return IDL_GettmpInt(layers);
}

IDL_VPTR show_layer(int argc, IDL_VPTR pArgv[], char* pArgk)
{
   IDL_VPTR idlPtr;
   typedef struct
   {
      IDL_KW_RESULT_FIRST_FIELD;
      int windowExists;
      IDL_STRING windowName;
   } KW_RESULT;
   KW_RESULT kw;

   //IDL_KW_FAST_SCAN is the type of scan we are using, following it is the
   //name of the keyword, followed by the type, the mask(which should be 1),
   //flags, a boolean whether the value was populated and finally the value itself
   static IDL_KW_PAR kw_pars[] = {
      IDL_KW_FAST_SCAN,
      {"WINDOW", IDL_TYP_STRING, 1, 0, reinterpret_cast<int*>(IDL_KW_OFFSETOF(windowExists)),
         reinterpret_cast<char*>(IDL_KW_OFFSETOF(windowName))},
      {NULL}
   };

   IDL_KWProcessByOffset(argc, pArgv, pArgk, kw_pars, 0, 1, &kw);

   std::string windowName;
   std::string layerName;

   if (kw.windowExists)
   {
      windowName = IDL_STRING_STR(&kw.windowName);
   }
   bool bSuccess = false;
   if (argc < 1)
   {
      IDL_Message(IDL_M_GENERIC, IDL_MSG_RET, "function takes a layer name as a parameter with "
         "'window' as an optional keyword.");
      IDL_KW_FREE;
      return IDL_StrToSTRING("failure");
   }
   //the layer name as a parameter
   layerName = IDL_VarGetString(pArgv[0]);
   SpatialDataView* pView = dynamic_cast<SpatialDataView*>(IdlFunctions::getViewByWindowName(windowName));
   if (pView != NULL)
   {
      Layer* pLayer = IdlFunctions::getLayerByName(windowName, layerName);
      if (pLayer != NULL)
      {
         pView->showLayer(pLayer);
         bSuccess = true;
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
   IDL_KW_FREE;
   return idlPtr;
}

IDL_VPTR hide_layer(int argc, IDL_VPTR pArgv[], char* pArgk)
{
   IDL_VPTR idlPtr;
   typedef struct
   {
      IDL_KW_RESULT_FIRST_FIELD;
      int windowExists;
      IDL_STRING windowName;
   } KW_RESULT;
   KW_RESULT kw;

   //IDL_KW_FAST_SCAN is the type of scan we are using, following it is the
   //name of the keyword, followed by the type, the mask(which should be 1),
   //flags, a boolean whether the value was populated and finally the value itself
   static IDL_KW_PAR kw_pars[] = {
      IDL_KW_FAST_SCAN,
      {"WINDOW", IDL_TYP_STRING, 1, 0, reinterpret_cast<int*>(IDL_KW_OFFSETOF(windowExists)),
         reinterpret_cast<char*>(IDL_KW_OFFSETOF(windowName))},
      {NULL}
   };

   IDL_KWProcessByOffset(argc, pArgv, pArgk, kw_pars, 0, 1, &kw);

   std::string windowName;
   std::string layerName;

   if (kw.windowExists)
   {
      windowName = IDL_STRING_STR(&kw.windowName);
   }
   bool bSuccess = false;
   if (argc < 1)
   {
      IDL_Message(IDL_M_GENERIC, IDL_MSG_RET, "function takes a layer name as a parameter with "
         "'window' as an optional keyword.");
      IDL_KW_FREE;
      return IDL_StrToSTRING("failure");
   }
   //the layer name as a parameter
   layerName = IDL_VarGetString(pArgv[0]);
   SpatialDataView* pView = dynamic_cast<SpatialDataView*>(IdlFunctions::getViewByWindowName(windowName));
   if (pView != NULL)
   {
      Layer* pLayer = IdlFunctions::getLayerByName(windowName, layerName);
      if (pLayer != NULL)
      {
         pView->hideLayer(pLayer);
         bSuccess = true;
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
   IDL_KW_FREE;
   return idlPtr;
}

static IDL_SYSFUN_DEF2 func_definitions[] = {
   {reinterpret_cast<IDL_SYSRTN_GENERIC>(get_current_name), "GET_CURRENT_NAME",0,5,IDL_SYSFUN_DEF_F_KEYWORDS,0},
   {reinterpret_cast<IDL_SYSRTN_GENERIC>(get_data_element_names), "GET_DATA_ELEMENT_NAMES",0,5,IDL_SYSFUN_DEF_F_KEYWORDS,0},
   {reinterpret_cast<IDL_SYSRTN_GENERIC>(get_data_name), "GET_DATA_NAME",0,5,IDL_SYSFUN_DEF_F_KEYWORDS,0},
   {reinterpret_cast<IDL_SYSRTN_GENERIC>(get_layer_name), "GET_LAYER_NAME",0,5,IDL_SYSFUN_DEF_F_KEYWORDS,0},
   {reinterpret_cast<IDL_SYSRTN_GENERIC>(get_layer_position), "GET_LAYER_POSITION",0,5,IDL_SYSFUN_DEF_F_KEYWORDS,0},
   {reinterpret_cast<IDL_SYSRTN_GENERIC>(get_num_layers), "GET_NUM_LAYERS",0,5,IDL_SYSFUN_DEF_F_KEYWORDS,0},
   {reinterpret_cast<IDL_SYSRTN_GENERIC>(hide_layer), "HIDE_LAYER",0,5,IDL_SYSFUN_DEF_F_KEYWORDS,0},
   {reinterpret_cast<IDL_SYSRTN_GENERIC>(set_layer_position), "SET_LAYER_POSITION",0,5,IDL_SYSFUN_DEF_F_KEYWORDS,0},
   {reinterpret_cast<IDL_SYSRTN_GENERIC>(show_layer), "SHOW_LAYER",0,5,IDL_SYSFUN_DEF_F_KEYWORDS,0},
   {NULL, NULL, 0, 0, 0, 0}
};

bool addLayerCommands()
{
   int funcCnt = -1;
   while (func_definitions[++funcCnt].name != NULL); // do nothing in the loop body
   return IDL_SysRtnAdd(func_definitions, IDL_TRUE, funcCnt) != 0;
}
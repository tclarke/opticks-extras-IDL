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

/**
 * \defgroup layercommands Layer Commands
 */
/*@{*/

/**
 * Get the name of the current data element or window.
 *
 * @param[in] DATASET @opt
 *            This flag gets the primary data element name of the currently active window.
 *            This is the default.
 * @param[in] FILE @opt
 *            This flag gets the currently active window's primary data element's file name.
 * @param[in] WINDOW @opt
 *            This flag gets the name of the currently active window.
 * @return The name of the element.
 * @usage print,get_current_name(/FILE)
 * @endusage
 */
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

   IdlFunctions::IdlKwResource<KW_RESULT> kw(argc, pArgv, pArgk, kw_pars, 0, 1);

   std::string filename;
   std::string wizardName;
   int file = 0;
   int dataset = 0;
   int window = 0;

   if (kw->datasetExists)
   {
      if (kw->dataset != 0)
      {
         dataset = 1;
      }
   }
   else if (kw->fileExists)
   {
      if (kw->file != 0)
      {
         file = 1;
      }
   }
   else if (kw->windowExists)
   {
      if (kw->window != 0)
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
   return IDL_StrToSTRING(const_cast<char*>(name.c_str()));
}

/**
 * Get the data element name of a specified layer.
 *
 * @param[in] [1] @opt
 *            The name of the layer. Defaults to the top most layer.
 * @param[in] WINDOW @opt
 *            The name of the window. Defaults to the active window.
 * @param[in] DATASET @opt
 *            If \p [1] is not specified and this flag it set, get the
 *            data set name of the top most raster layer.
 * @return The name of the data element.
 * @usage print,get_data_name(/DATASET)
 * @endusage
 */
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

   IdlFunctions::IdlKwResource<KW_RESULT> kw(argc, pArgv, pArgk, kw_pars, 0, 1);
   std::string windowName;
   std::string layerName;
   std::string name;

   if (kw->windowExists)
   {
      windowName = IDL_STRING_STR(&kw->windowName);
   }

   //retrieve the layer name passed in as a parameter
   layerName = IDL_VarGetString(pArgv[0]);
   //get the layer
   bool datasets = false;
   if (kw->datasetExists)
   {
      if (kw->dataset != 0)
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
      return IDL_StrToSTRING("");
   }

   return IDL_StrToSTRING(const_cast<char*>(name.c_str()));
}

/**
 * Get the names of all the data elements which are children of the primary raster element.
 *
 * @param[in] WINDOW @opt
 *            The name of the window. Defaults to the active window.
 * @return An array of data element names.
 * @usage names = get_data_element_names()
 * @endusage
 */
IDL_VPTR get_data_element_names(int argc, IDL_VPTR pArgv[], char* pArgk)
{
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
   std::string name;
   bool bSuccess = false;
   IDL_VPTR idlPtr;
   unsigned int total = 0;
   IDL_STRING* pStrarr = NULL;

   if (kw->windowExists)
   {
      windowName = IDL_STRING_STR(&kw->windowName);
   }

   SpatialDataWindow* pWindow = NULL;
   if (windowName.empty())
   {
      pWindow = dynamic_cast<SpatialDataWindow*>(Service<DesktopServices>()->getCurrentWorkspaceWindow());
   }
   else
   {
      pWindow = dynamic_cast<SpatialDataWindow*>(
         Service<DesktopServices>()->getWindow(windowName, SPATIAL_DATA_WINDOW));
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
               pStrarr = reinterpret_cast<IDL_STRING*>(malloc(total * sizeof(IDL_STRING)));
               for (unsigned int i=0; i < total; ++i)
               {
                  IDL_StrStore(&(pStrarr[i]), const_cast<char*>(names[i].c_str()));
               }
               bSuccess = true;
            }
         }
      }
   }
   else if (windowName == "all")
   {
      std::vector<std::string> names = Service<ModelServices>()->getElementNames("RasterElement");
      total = names.size();
      pStrarr = reinterpret_cast<IDL_STRING*>(malloc(total* sizeof(IDL_STRING)));
      for (unsigned int i=0; i < total; ++i)
      {
         IDL_StrStore(&(pStrarr[i]), const_cast<char*>(names[i].c_str()));
      }
      bSuccess = true;
   }
   if (total == 0)
   {
      IDL_Message(IDL_M_GENERIC, IDL_MSG_RET, "No elements matched.");
      return NULL;
   }
   if (!bSuccess)
   {
      pStrarr = reinterpret_cast<IDL_STRING*>(malloc(sizeof(IDL_STRING)));
      char* pEmpty = "";
      IDL_StrStore(&(pStrarr[0]), pEmpty);
   }
   IDL_MEMINT dims[] = {total};
   idlPtr = IDL_ImportArray(1, dims, IDL_TYP_STRING, reinterpret_cast<UCHAR*>(pStrarr),
      reinterpret_cast<IDL_ARRAY_FREE_CB>(free), NULL);
   return idlPtr;
}

/**
 * Get the name of the layer in a given position in the layer list.
 *
 * @param[in] INDEX @opt
 *            The 0 based index of the layer. Defaults to the top most layer.
 * @param[in] WINDOW @opt
 *            The name of the window. Defaults to the active window.
 * @return The name of the layer.
 * @usage print,get_layer_name(2, WINDOW="Window 1")
 * @endusage
 */
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

   IdlFunctions::IdlKwResource<KW_RESULT> kw(argc, pArgv, pArgk, kw_pars, 0, 1);

   std::string windowName;
   int index = -1;
   std::string name;

   if (kw->indexExists)
   {
      index = kw->index;
   }
   if (kw->windowExists)
   {
      windowName = IDL_STRING_STR(&kw->windowName);
   }

   if (argc < 0)
   {
      IDL_Message(IDL_M_GENERIC, IDL_MSG_RET, "Invalid arguments");
      return IDL_StrToSTRING("");
   }
   Layer* pLayer = IdlFunctions::getLayerByIndex(windowName, index);
   if (pLayer != NULL)
   {
      name =  pLayer->getName();
   }
   return IDL_StrToSTRING(const_cast<char*>(name.c_str()));
}

/**
 * Move a layer to a new position in the layer list.
 *
 * @param[in] [1]
 *            The name of the layer to move.
 * @param[in] INDEX
 *            The new 0 based index for the layer.
 * @param[in] WINDOW @opt
 *            The name of the window. Defaults to the active window.
 * @rsof
 * @usage print,set_layer_position("data.tif", INDEX=0)
 * @endusage
 */
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

   IdlFunctions::IdlKwResource<KW_RESULT> kw(argc, pArgv, pArgk, kw_pars, 0, 1);

   std::string windowName;
   int index = -1;
   std::string name;

   if (kw->indexExists)
   {
      index = kw->index;
   }
   if (kw->windowExists)
   {
      windowName = IDL_STRING_STR(&kw->windowName);
   }

   if (argc < 2)
   {
      IDL_Message(IDL_M_GENERIC, IDL_MSG_RET, "set_layer_position takes a layer name as a parameter with a "
         "window as an optional keyword.  A keyword 'index' is needed to specify the position.");
      return IDL_StrToSTRING("failure");
   }
   //the name of the layer to set the posisiton of
   name = IDL_VarGetString(pArgv[0]);
   bool bSuccess = false;
   SpatialDataView* pView = dynamic_cast<SpatialDataView*>(IdlFunctions::getViewByWindowName(windowName));
   if (pView != NULL)
   {
      Layer* pLayer = IdlFunctions::getLayerByName(windowName, name, false);
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
   return idlPtr;
}

/**
 * Get the current position of a layer in the layer list.
 *
 * @param[in] [1]
 *            The name of the layer.
 * @param[in] WINDOW @opt
 *            The name of the window. Defaults to the active window.
 * @return The 0 based position of the layer in the layer list.
 * @usage idx = get_layer_position("data.tif")
 * @endusage
 */
IDL_VPTR get_layer_position(int argc, IDL_VPTR pArgv[], char* pArgk)
{
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
   std::string name;
   int index = -1;

   if (kw->windowExists)
   {
      windowName = IDL_STRING_STR(&kw->windowName);
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
         Layer* pLayer = IdlFunctions::getLayerByName(windowName, name, false);
         if (pLayer != NULL)
         {
            index = pView->getLayerDisplayIndex(pLayer);
         }
      }
   }
   return IDL_GettmpInt(index);
}

/**
 * Get the number of layers in a window.
 *
 * @param[in] WINDOW @opt
 *            The name of the window. Defaults to the active window.
 * @return The number of layers in the window.
 * @usage num_layers = get_num_layers()
 * @endusage
 */
IDL_VPTR get_num_layers(int argc, IDL_VPTR pArgv[], char* pArgk)
{
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
   int layers = 0;

   if (kw->windowExists)
   {
      windowName = IDL_STRING_STR(&kw->windowName);
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
   return IDL_GettmpInt(layers);
}

/**
 * Show a hidden layer.
 *
 * @param[in] [1]
 *            The name of the layer to show.
 * @param[in] WINDOW @opt
 *            The name of the window. Defaults to the active window.
 * @rsof
 * @usage print,show_layer("raster.tif")
 * @endusage
 */
IDL_VPTR show_layer(int argc, IDL_VPTR pArgv[], char* pArgk)
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
      IDL_Message(IDL_M_GENERIC, IDL_MSG_RET, "function takes a layer name as a parameter with "
         "'window' as an optional keyword.");
      return IDL_StrToSTRING("failure");
   }
   //the layer name as a parameter
   layerName = IDL_VarGetString(pArgv[0]);
   SpatialDataView* pView = dynamic_cast<SpatialDataView*>(IdlFunctions::getViewByWindowName(windowName));
   if (pView != NULL)
   {
      Layer* pLayer = IdlFunctions::getLayerByName(windowName, layerName, false);
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
   return idlPtr;
}

/**
 * Hide a layer.
 *
 * @param[in] [1]
 *            The name of the layer to hide.
 * @param[in] WINDOW @opt
 *            The name of the window. Defaults to the active window.
 * @rsof
 * @usage print,hide_layer("raster.tif")
 * @endusage
 */
IDL_VPTR hide_layer(int argc, IDL_VPTR pArgv[], char* pArgk)
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
      IDL_Message(IDL_M_GENERIC, IDL_MSG_RET, "function takes a layer name as a parameter with "
         "'window' as an optional keyword.");
      return IDL_StrToSTRING("failure");
   }
   //the layer name as a parameter
   layerName = IDL_VarGetString(pArgv[0]);
   SpatialDataView* pView = dynamic_cast<SpatialDataView*>(IdlFunctions::getViewByWindowName(windowName));
   if (pView != NULL)
   {
      Layer* pLayer = IdlFunctions::getLayerByName(windowName, layerName, false);
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
   return idlPtr;
}
/*@}*/

static IDL_SYSFUN_DEF2 func_definitions[] = {
   {reinterpret_cast<IDL_SYSRTN_GENERIC>(get_current_name), "GET_CURRENT_NAME",0,5,IDL_SYSFUN_DEF_F_KEYWORDS,0},
   {reinterpret_cast<IDL_SYSRTN_GENERIC>(get_data_element_names),
   "GET_DATA_ELEMENT_NAMES",0,5,IDL_SYSFUN_DEF_F_KEYWORDS,0},
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

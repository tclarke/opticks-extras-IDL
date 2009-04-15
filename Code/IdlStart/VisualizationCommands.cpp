/*
 * The information in this file is
 * Copyright(c) 2007 Ball Aerospace & Technologies Corporation
 * and is subject to the terms and conditions of the
 * GNU Lesser General Public License Version 2.1
 * The license text is available from   
 * http://www.gnu.org/licenses/lgpl.html
 */

#include "ColorMap.h"
#include "IdlFunctions.h"
#include "RasterLayer.h"
#include "SpatialDataView.h"
#include "StringUtilities.h"
#include "VisualizationCommands.h"

#include <string>
#include <stdio.h>
#include <idl_export.h>

/**
 * \defgroup visualizationcommands Visualization Commands
 */
/*@{*/

/**
 * Set the colormap for single channel display.
 *
 * @param[in] [1]
 *            Full path name to the colormap file.
 * @param[in] LAYER @opt
 *            The name of the raster layer. Defaults to
 *            the layer containing the primary raster element.
 * @param[in] WINDOW @opt
 *            The name of the window to update. Defaults to the active window.
 * @rsof
 * @usage print,set_colormap("C:\Program Files\Opticks\SupportFiles\ColorTables\StopLight.clu")
 * @endusage
 */
IDL_VPTR set_colormap(int argc, IDL_VPTR pArgv[], char* pArgk)
{
   typedef struct
   {
      IDL_KW_RESULT_FIRST_FIELD;
      int windowExists;
      IDL_STRING windowName;
      int layerNameExists;
      IDL_STRING layerName;
      int colormapExists;
      IDL_STRING colormap;
   } KW_RESULT;
   KW_RESULT kw;

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

   IDL_KWProcessByOffset(argc, pArgv, pArgk, kw_pars, 0, 1, &kw);

   std::string windowName;
   std::string mapName;
   int index = -1;
   std::string layerName;

   if (kw.layerNameExists)
   {
      layerName = IDL_STRING_STR(&kw.layerName);
   }
   if (kw.windowExists)
   {
      windowName = IDL_STRING_STR(&kw.windowName);
   }

   if (argc < 1)
   {
      IDL_Message(IDL_M_GENERIC, IDL_MSG_RET, "function takes a colormap filename as a parameter, and a 'layer' and 'window' name as keywords.");
      IDL_KW_FREE;
      return IDL_StrToSTRING("failure");
   }
   //get the colormap filename passed as an input parameter
   mapName = IDL_VarGetString(pArgv[0]);

   //get the view and layer
   SpatialDataView* pView = dynamic_cast<SpatialDataView*>(IdlFunctions::getViewByWindowName(windowName));
   if (pView != NULL)
   {
      RasterLayer* pLayer = dynamic_cast<RasterLayer*>(IdlFunctions::getLayerByName(windowName, layerName));
      if (pLayer != NULL)
      {
         try
         {
            //the constructor of ColorMap throws an exception when the file doesn't exist
            ColorMap pMap(mapName);
            pLayer->setColorMap(mapName, pMap.getTable());
         }
         catch (...)
         {
            IDL_Message(IDL_M_GENERIC, IDL_MSG_RET, "Color map filename is invalid.");
            IDL_KW_FREE;
            return IDL_StrToSTRING("failure");
         }
      }
      else
      {
         RasterLayer* pCube = dynamic_cast<RasterLayer*>(IdlFunctions::getLayerByName(windowName, layerName));
         if (pCube != NULL)
         {
            try
            {
               //the constructor of ColorMap throws an exception when the file doesn't exist
               ColorMap pMap(mapName);
               pCube->setColorMap(mapName, pMap.getTable());
            }
            catch (...)
            {
               IDL_Message(IDL_M_GENERIC, IDL_MSG_RET, "Color map filename is invalid.");
               IDL_KW_FREE;
               return IDL_StrToSTRING("failure");
            }
         }
      }
   }
   IDL_KW_FREE;
   return IDL_StrToSTRING("success");
}

/**
 * Get the histogram stretch values for a layer.
 *
 * @param[out] MAX @opt
 *             If specified, this will contain the maximum stretch value.
 * @param[out] MIN @opt
 *             If specified, this will contain the minimum stretch value.
 * @param[in] CHANNEL @opt
 *            The color channel whose stretch value is needed. Defaults to GRAY.
 * @param[in] LAYER @opt
 *            The name of the raster layer. Defaults to
 *            the layer containing the primary raster element.
 * @param[in] WINDOW @opt
 *            The name of the window to update. Defaults to the active window.
 * @rsof
 * @usage maxval = 0
 * minval = 0
 * print,get_stretch_values(MAX=maxval, MIN=minval, CHANNEL="RED")
 * @endusage
 */
IDL_VPTR get_stretch_values(int argc, IDL_VPTR pArgv[], char* pArgk)
{
   IDL_VPTR idlPtr;
   typedef struct
   {
      IDL_KW_RESULT_FIRST_FIELD;
      int windowExists;
      IDL_STRING windowName;
      int layerNameExists;
      IDL_STRING layerName;
      int minExists;
      IDL_VPTR min;
      int maxExists;
      IDL_VPTR max;
      int channelExists;
      IDL_STRING channel;
   } KW_RESULT;
   KW_RESULT kw;

   //IDL_KW_FAST_SCAN is the type of scan we are using, following it is the
   //name of the keyword, followed by the type, the mask(which should be 1),
   //flags, a boolean whether the value was populated and finally the value itself
   static IDL_KW_PAR kw_pars[] = {
      IDL_KW_FAST_SCAN,
      {"CHANNEL", IDL_TYP_STRING, 1, 0, reinterpret_cast<int*>(IDL_KW_OFFSETOF(channelExists)),
         reinterpret_cast<char*>(IDL_KW_OFFSETOF(channel))},
      {"LAYER", IDL_TYP_STRING, 1, 0, reinterpret_cast<int*>(IDL_KW_OFFSETOF(layerNameExists)),
         reinterpret_cast<char*>(IDL_KW_OFFSETOF(layerName))},
      {"MAX", IDL_TYP_DOUBLE, 1, IDL_KW_OUT, reinterpret_cast<int*>(IDL_KW_OFFSETOF(maxExists)),
         reinterpret_cast<char*>(IDL_KW_OFFSETOF(max))},
      {"MIN", IDL_TYP_DOUBLE, 1, IDL_KW_OUT, reinterpret_cast<int*>(IDL_KW_OFFSETOF(minExists)),
         reinterpret_cast<char*>(IDL_KW_OFFSETOF(min))},
      {"WINDOW", IDL_TYP_STRING, 1, 0, reinterpret_cast<int*>(IDL_KW_OFFSETOF(windowExists)),
         reinterpret_cast<char*>(IDL_KW_OFFSETOF(windowName))},
      {NULL}
   };

   IDL_KWProcessByOffset(argc, pArgv, pArgk, kw_pars, 0, 1, &kw);
   bool bSuccess = false;

   std::string windowName;
   std::string layerName;
   std::string channel = "GRAY";

   if (kw.windowExists)
   {
      windowName = IDL_STRING_STR(&kw.windowName);
   }
   if (kw.layerNameExists)
   {
      layerName = IDL_STRING_STR(&kw.layerName);
   }
   if (kw.channelExists)
   {
      channel = IDL_STRING_STR(&kw.channel);
   }

   if (argc < 1)
   {
      IDL_Message(IDL_M_GENERIC, IDL_MSG_RET, "get_stretch_values 'layer', 'window', 'channel', 'min' and 'max' as keywords.");
   }
   else
   {
      //if no layer name is given then make sure we get the top most Raster Layer so the last parameter is set to true
      RasterLayer* pLayer = dynamic_cast<RasterLayer*>(IdlFunctions::getLayerByName(windowName, layerName, true));
      if (pLayer != NULL)
      {
         //determine the channel, default is gray
         RasterChannelType element = IdlFunctions::getRasterChannelType(channel);
         double min = 0.0;
         double max = 0.0;
         //get the stretch values from the layer
         pLayer->getStretchValues(element, min, max);
         if (kw.minExists)
         {
            //we don't know the datatype passed in, so set all of them
            kw.min->value.d = min;
            kw.min->value.f = static_cast<float>(min);
            kw.min->value.i = static_cast<short>(min);
            kw.min->value.ui = static_cast<unsigned short>(min);
            kw.min->value.ul = static_cast<unsigned int>(min);
            kw.min->value.l = static_cast<int>(min);
            kw.min->value.l64 = static_cast<unsigned long>(min);
            kw.min->value.ul64 = static_cast<unsigned long>(min);
            kw.min->value.sc = static_cast<char>(min);
            kw.min->value.c = static_cast<unsigned char>(min);
         }
         if (kw.maxExists)
         {
            //we don't know the datatype passed in to populate, so set all of them
            kw.max->value.d = max;
            kw.max->value.f = static_cast<float>(max);
            kw.max->value.i = static_cast<short>(max);
            kw.max->value.ui = static_cast<unsigned short>(max);
            kw.max->value.ul = static_cast<unsigned int>(max);
            kw.max->value.l = static_cast<int>(max);
            kw.max->value.l64 = static_cast<unsigned long>(max);
            kw.max->value.ul64 = static_cast<unsigned long>(max);
            kw.max->value.sc = static_cast<char>(max);
            kw.max->value.c = static_cast<unsigned char>(max);
         }
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

/**
 * Set the histogram stretch values for a layer.
 *
 * @param[in] MAX
 *             The new maximum stretch value.
 * @param[in] MIN
 *            The new minimum stretch value.
 * @param[in] CHANNEL @opt
 *            The color channel whose stretch value will be set. Defaults to GRAY.
 * @param[in] LAYER @opt
 *            The name of the raster layer. Defaults to
 *            the layer containing the primary raster element.
 * @param[in] WINDOW @opt
 *            The name of the window to update. Defaults to the active window.
 * @rsof
 * @usage print,set_stretch_values(MAX=100, MIN=25.6)
 * @endusage
 */
IDL_VPTR set_stretch_values(int argc, IDL_VPTR pArgv[], char* pArgk)
{
   IDL_VPTR idlPtr;
   typedef struct
   {
      IDL_KW_RESULT_FIRST_FIELD;
      int windowExists;
      IDL_STRING windowName;
      int layerNameExists;
      IDL_STRING layerName;
      int minExists;
      double min;
      int maxExists;
      double max;
      int channelExists;
      IDL_STRING channel;
   } KW_RESULT;
   KW_RESULT kw;

   //IDL_KW_FAST_SCAN is the type of scan we are using, following it is the
   //name of the keyword, followed by the type, the mask(which should be 1),
   //flags, a boolean whether the value was populated and finally the value itself
   static IDL_KW_PAR kw_pars[] = {
      IDL_KW_FAST_SCAN,
      {"CHANNEL", IDL_TYP_STRING, 1, 0, reinterpret_cast<int*>(IDL_KW_OFFSETOF(channelExists)),
         reinterpret_cast<char*>(IDL_KW_OFFSETOF(channel))},
      {"LAYER", IDL_TYP_STRING, 1, 0, reinterpret_cast<int*>(IDL_KW_OFFSETOF(layerNameExists)),
         reinterpret_cast<char*>(IDL_KW_OFFSETOF(layerName))},
      {"MAX", IDL_TYP_DOUBLE, 1, 0, reinterpret_cast<int*>(IDL_KW_OFFSETOF(maxExists)),
         reinterpret_cast<char*>(IDL_KW_OFFSETOF(max))},
      {"MIN", IDL_TYP_DOUBLE, 1, 0, reinterpret_cast<int*>(IDL_KW_OFFSETOF(minExists)),
         reinterpret_cast<char*>(IDL_KW_OFFSETOF(min))},
      {"WINDOW", IDL_TYP_STRING, 1, 0, reinterpret_cast<int*>(IDL_KW_OFFSETOF(windowExists)),
         reinterpret_cast<char*>(IDL_KW_OFFSETOF(windowName))},
      {NULL}
   };

   IDL_KWProcessByOffset(argc, pArgv, pArgk, kw_pars, 0, 1, &kw);

   std::string windowName;
   std::string layerName;
   std::string channel = "GRAY";
   double min = 0.0;
   double max = 0.0;

   if (kw.windowExists)
   {
      windowName = IDL_STRING_STR(&kw.windowName);
   }
   if (kw.layerNameExists)
   {
      layerName = IDL_STRING_STR(&kw.layerName);
   }
   if (kw.channelExists)
   {
      channel = IDL_STRING_STR(&kw.channel);
   }
   if (kw.minExists)
   {
      min = kw.min;
   }
   if (kw.maxExists)
   {
      max = kw.max;
   }

   if (argc < 1)
   {
      IDL_Message(IDL_M_GENERIC, IDL_MSG_RET, "set_stretch_values needs at least the 'min' or"
         "'max' keywords populated along with a 'layer' name.  it can have 'window' or 'channel' as optional keywords");
      IDL_KW_FREE;
      return IDL_StrToSTRING("failure");
   }
   //if no layer name is given then make sure we get the top most Raster Layer so the last parameter is set to true
   RasterLayer* pLayer = dynamic_cast<RasterLayer*>(IdlFunctions::getLayerByName(windowName, layerName, true));
   if (pLayer != NULL)
   {
      //determine the channel to set the values for
      RasterChannelType element = IdlFunctions::getRasterChannelType(channel);
      double tmpmin = 0.0;
      double tmpmax = 0.0;
      pLayer->getStretchValues(element, tmpmin, tmpmax);
      if (!kw.minExists)
      {
         min = tmpmin;
      }
      if (!kw.maxExists)
      {
         max = tmpmax;
      }
      pLayer->setStretchValues(element, min, max);
      idlPtr = IDL_StrToSTRING("success");
   }
   else
   {
      idlPtr = IDL_StrToSTRING("failure");
   }
   IDL_KW_FREE;
   return idlPtr;
}

/**
 * Set the histogram stretch method for a layer.
 *
 * @param[in] [1]
 *            The new streth method.
 * @param[in] CHANNEL @opt
 *            The color channel whose stretch method will be set. Defaults to GRAY.
 * @param[in] LAYER @opt
 *            The name of the raster layer. Defaults to
 *            the layer containing the primary raster element.
 * @param[in] WINDOW @opt
 *            The name of the window to update. Defaults to the active window.
 * @rsof
 * @usage print,set_stretch_method("PERCENTILE")
 * @endusage
 */
IDL_VPTR set_stretch_method(int argc, IDL_VPTR pArgv[], char* pArgk)
{
   IDL_VPTR idlPtr;
   typedef struct
   {
      IDL_KW_RESULT_FIRST_FIELD;
      int windowExists;
      IDL_STRING windowName;
      int layerNameExists;
      IDL_STRING layerName;
      int channelExists;
      IDL_STRING channel;
   } KW_RESULT;
   KW_RESULT kw;

   //IDL_KW_FAST_SCAN is the type of scan we are using, following it is the
   //name of the keyword, followed by the type, the mask(which should be 1),
   //flags, a boolean whether the value was populated and finally the value itself
   static IDL_KW_PAR kw_pars[] = {
      IDL_KW_FAST_SCAN,
      {"CHANNEL", IDL_TYP_STRING, 1, 0, reinterpret_cast<int*>(IDL_KW_OFFSETOF(channelExists)),
         reinterpret_cast<char*>(IDL_KW_OFFSETOF(channel))},
      {"LAYER", IDL_TYP_STRING, 1, 0, reinterpret_cast<int*>(IDL_KW_OFFSETOF(layerNameExists)),
         reinterpret_cast<char*>(IDL_KW_OFFSETOF(layerName))},
      {"WINDOW", IDL_TYP_STRING, 1, 0, reinterpret_cast<int*>(IDL_KW_OFFSETOF(windowExists)),
         reinterpret_cast<char*>(IDL_KW_OFFSETOF(windowName))},
      {NULL}
   };

   IDL_KWProcessByOffset(argc, pArgv, pArgk, kw_pars, 0, 1, &kw);

   std::string windowName;
   std::string layerName;
   std::string channel = "GRAY";
   std::string method;
   double min = 0.0;
   double max = 0.0;

   if (kw.windowExists)
   {
      windowName = IDL_STRING_STR(&kw.windowName);
   }
   if (kw.layerNameExists)
   {
      layerName = IDL_STRING_STR(&kw.layerName);
   }
   if (kw.channelExists)
   {
      channel = IDL_STRING_STR(&kw.channel);
   }
   bool bSuccess = false;
   if (argc < 1)
   {
      IDL_Message(IDL_M_GENERIC, IDL_MSG_RET, "function takes a method name as a parameter with a "
         "'window', 'layer' and 'channel' as optional keywords.");
      IDL_KW_FREE;
      return IDL_StrToSTRING("failure");
   }

   //the method name as a parameter
   method = IDL_VarGetString(pArgv[0]);
   RasterLayer* pLayer = dynamic_cast<RasterLayer*>(IdlFunctions::getLayerByName(windowName, layerName));
   if (pLayer != NULL)
   {
      //determine the channel
      RasterChannelType element = IdlFunctions::getRasterChannelType(channel);
      //determine the method
      RegionUnits eMethod = StringUtilities::fromXmlString<RegionUnits>(method);
      if (eMethod.isValid())
      {
         pLayer->setStretchUnits(element, eMethod);
         bSuccess = true;
      }
   }
   IDL_KW_FREE;
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
 * Get the histogram stretch method for a layer.
 *
 * @param[in] CHANNEL @opt
 *            The color channel whose stretch method is needed. Defaults to GRAY.
 * @param[in] LAYER @opt
 *            The name of the raster layer. Defaults to
 *            the layer containing the primary raster element.
 * @param[in] WINDOW @opt
 *            The name of the window to update. Defaults to the active window.
 * @return The current stretch method.
 * @usage method = get_stretch_method()
 * @endusage
 */
IDL_VPTR get_stretch_method(int argc, IDL_VPTR pArgv[], char* pArgk)
{
   typedef struct
   {
      IDL_KW_RESULT_FIRST_FIELD;
      int windowExists;
      IDL_STRING windowName;
      int layerNameExists;
      IDL_STRING layerName;
      int channelExists;
      IDL_STRING channel;
   } KW_RESULT;
   KW_RESULT kw;

   //IDL_KW_FAST_SCAN is the type of scan we are using, following it is the
   //name of the keyword, followed by the type, the mask(which should be 1),
   //flags, a boolean whether the value was populated and finally the value itself
   static IDL_KW_PAR kw_pars[] = {
      IDL_KW_FAST_SCAN,
      {"CHANNEL", IDL_TYP_STRING, 1, 0, reinterpret_cast<int*>(IDL_KW_OFFSETOF(channelExists)),
         reinterpret_cast<char*>(IDL_KW_OFFSETOF(channel))},
      {"LAYER", IDL_TYP_STRING, 1, 0, reinterpret_cast<int*>(IDL_KW_OFFSETOF(layerNameExists)),
         reinterpret_cast<char*>(IDL_KW_OFFSETOF(layerName))},
      {"WINDOW", IDL_TYP_STRING, 1, 0, reinterpret_cast<int*>(IDL_KW_OFFSETOF(windowExists)),
         reinterpret_cast<char*>(IDL_KW_OFFSETOF(windowName))},
      {NULL}
   };

   IDL_KWProcessByOffset(argc, pArgv, pArgk, kw_pars, 0, 1, &kw);

   std::string windowName;
   std::string layerName;
   std::string channel = "GRAY";
   std::string method;
   double min = 0.0;
   double max = 0.0;

   if (kw.windowExists)
   {
      windowName = IDL_STRING_STR(&kw.windowName);
   }
   if (kw.layerNameExists)
   {
      layerName = IDL_STRING_STR(&kw.layerName);
   }
   if (kw.channelExists)
   {
      channel = IDL_STRING_STR(&kw.channel);
   }

   if (argc < 1)
   {
      IDL_Message(IDL_M_GENERIC, IDL_MSG_RET, "function takes  "
         "'window', 'layer' and 'channel' as optional keywords.");
      IDL_KW_FREE;
      return IDL_StrToSTRING("");
   }
   RasterLayer* pLayer = dynamic_cast<RasterLayer*>(IdlFunctions::getLayerByName(windowName, layerName));
   if (pLayer != NULL)
   {
      RasterChannelType element = IdlFunctions::getRasterChannelType(channel);
      //return the stretch type as a std::string
      method = StringUtilities::toXmlString(pLayer->getStretchUnits(element));
   }
   IDL_KW_FREE;
   return IDL_StrToSTRING(const_cast<char*>(method.c_str()));
}

/**
 * Set the histogram stretch type for a layer.
 *
 * @param[in] [1]
 *            The new streth type.
 * @param[in] CHANNEL @opt
 *            The color channel whose stretch type will be set. Defaults to GRAY.
 * @param[in] LAYER @opt
 *            The name of the raster layer. Defaults to
 *            the layer containing the primary raster element.
 * @param[in] WINDOW @opt
 *            The name of the window to update. Defaults to the active window.
 * @rsof
 * @usage print,set_stretch_type("LINEAR")
 * @endusage
 */
IDL_VPTR set_stretch_type(int argc, IDL_VPTR pArgv[], char* pArgk)
{
   IDL_VPTR idlPtr = IDL_StrToSTRING("failure");
   typedef struct
   {
      IDL_KW_RESULT_FIRST_FIELD;
      int windowExists;
      IDL_STRING windowName;
      int layerNameExists;
      IDL_STRING layerName;
      int modeExists;
      IDL_STRING mode;
   } KW_RESULT;
   KW_RESULT kw;

   //IDL_KW_FAST_SCAN is the type of scan we are using, following it is the
   //name of the keyword, followed by the type, the mask(which should be 1),
   //flags, a boolean whether the value was populated and finally the value itself
   static IDL_KW_PAR kw_pars[] = {
      IDL_KW_FAST_SCAN,
      {"LAYER", IDL_TYP_STRING, 1, 0, reinterpret_cast<int*>(IDL_KW_OFFSETOF(layerNameExists)),
         reinterpret_cast<char*>(IDL_KW_OFFSETOF(layerName))},
      {"MODE", IDL_TYP_STRING, 1, 0, reinterpret_cast<int*>(IDL_KW_OFFSETOF(modeExists)),
         reinterpret_cast<char*>(IDL_KW_OFFSETOF(mode))},
      {"WINDOW", IDL_TYP_STRING, 1, 0, reinterpret_cast<int*>(IDL_KW_OFFSETOF(windowExists)),
         reinterpret_cast<char*>(IDL_KW_OFFSETOF(windowName))},
      {NULL}
   };

   IDL_KWProcessByOffset(argc, pArgv, pArgk, kw_pars, 0, 1, &kw);

   std::string windowName;
   std::string layerName;
   std::string mode;
   std::string type;

   if (kw.windowExists)
   {
      windowName = IDL_STRING_STR(&kw.windowName);
   }
   if (kw.layerNameExists)
   {
      layerName = IDL_STRING_STR(&kw.layerName);
   }
   if (kw.modeExists)
   {
      mode = IDL_STRING_STR(&kw.mode);
   }
   bool bSuccess = false;
   if (argc < 1)
   {
      IDL_Message(IDL_M_GENERIC, IDL_MSG_RET, "function takes a type name as a parameter with a "
         "'window', 'layer' and 'mode' as optional keywords.");
      IDL_KW_FREE;
      return IDL_StrToSTRING("failure");
   }
   //the stretch type
   type = IDL_VarGetString(pArgv[0]);
   RasterLayer* pLayer = dynamic_cast<RasterLayer*>(IdlFunctions::getLayerByName(windowName, layerName));
   if (pLayer != NULL)
   {
      //if a mode is set, use that, if not, get the current mode
      DisplayMode element = GRAYSCALE_MODE;
      if (mode.empty())
      {
         element = pLayer->getDisplayMode();
      }
      else
      {
         element = StringUtilities::fromXmlString<DisplayMode>(mode);
      }
      //get which stretch type was passed in
      StretchType eType = StringUtilities::fromXmlString<StretchType>(type);
      if (eType.isValid())
      {
         pLayer->setStretchType(element, eType);
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

/**
 * Get the histogram stretch type for a layer.
 *
 * @param[in] CHANNEL @opt
 *            The color channel whose stretch type is needed. Defaults to GRAY.
 * @param[in] LAYER @opt
 *            The name of the raster layer. Defaults to
 *            the layer containing the primary raster element.
 * @param[in] WINDOW @opt
 *            The name of the window to update. Defaults to the active window.
 * @return The current stretch type.
 * @usage type = get_stretch_type()
 * @endusage
 */
IDL_VPTR get_stretch_type(int argc, IDL_VPTR pArgv[], char* pArgk)
{
   typedef struct
   {
      IDL_KW_RESULT_FIRST_FIELD;
      int layerNameExists;
      IDL_STRING layerName;
      int windowExists;
      IDL_STRING windowName;
      int modeExists;
      IDL_STRING mode;
   } KW_RESULT;
   KW_RESULT kw;

   //IDL_KW_FAST_SCAN is the type of scan we are using, following it is the
   //name of the keyword, followed by the type, the mask(which should be 1),
   //flags, a boolean whether the value was populated and finally the value itself
   static IDL_KW_PAR kw_pars[] = {
      IDL_KW_FAST_SCAN,
      {"LAYER", IDL_TYP_STRING, 1, 0, reinterpret_cast<int*>(IDL_KW_OFFSETOF(layerNameExists)),
         reinterpret_cast<char*>(IDL_KW_OFFSETOF(layerName))},
      {"MODE", IDL_TYP_STRING, 1, 0, reinterpret_cast<int*>(IDL_KW_OFFSETOF(modeExists)),
         reinterpret_cast<char*>(IDL_KW_OFFSETOF(mode))},
      {"WINDOW", IDL_TYP_STRING, 1, 0, reinterpret_cast<int*>(IDL_KW_OFFSETOF(windowExists)),
         reinterpret_cast<char*>(IDL_KW_OFFSETOF(windowName))},
      {NULL}
   };

   IDL_KWProcessByOffset(argc, pArgv, pArgk, kw_pars, 0, 1, &kw);

   std::string windowName;
   std::string layerName;
   std::string mode;
   std::string type;

   if (kw.windowExists)
   {
      windowName = IDL_STRING_STR(&kw.windowName);
   }
   if (kw.modeExists)
   {
      mode = IDL_STRING_STR(&kw.mode);
   }
   if (kw.layerNameExists)
   {
      layerName = IDL_STRING_STR(&kw.layerName);
   }

   if (argc < 0)
   {
      IDL_Message(IDL_M_GENERIC, IDL_MSG_RET, "function takes  "
         "'window', 'layer' and 'mode' as optional keywords.");
      IDL_KW_FREE;
      return IDL_StrToSTRING("");
   }

   RasterLayer* pLayer = dynamic_cast<RasterLayer*>(IdlFunctions::getLayerByName(windowName, layerName));
   if (pLayer != NULL)
   {
      //if no mode is specified, use the current
      DisplayMode element = GRAYSCALE_MODE;
      if (mode.empty())
      {
         element = pLayer->getDisplayMode();
      }
      else
      {
         element = StringUtilities::fromXmlString<DisplayMode>(mode);
      }

      type = StringUtilities::toXmlString(pLayer->getStretchType(element));
   }
   IDL_KW_FREE;
   return IDL_StrToSTRING(const_cast<char*>(type.c_str()));
}
/*@}*/

static IDL_SYSFUN_DEF2 func_definitions[] = {
   {reinterpret_cast<IDL_SYSRTN_GENERIC>(get_stretch_method), "GET_STRETCH_METHOD",0,5,IDL_SYSFUN_DEF_F_KEYWORDS,0},
   {reinterpret_cast<IDL_SYSRTN_GENERIC>(get_stretch_type), "GET_STRETCH_TYPE",0,5,IDL_SYSFUN_DEF_F_KEYWORDS,0},
   {reinterpret_cast<IDL_SYSRTN_GENERIC>(get_stretch_values), "GET_STRETCH_VALUES",0,5,IDL_SYSFUN_DEF_F_KEYWORDS,0},
   {reinterpret_cast<IDL_SYSRTN_GENERIC>(set_colormap), "SET_COLORMAP",0,5,IDL_SYSFUN_DEF_F_KEYWORDS,0},
   {reinterpret_cast<IDL_SYSRTN_GENERIC>(set_stretch_method), "SET_STRETCH_METHOD",0,5,IDL_SYSFUN_DEF_F_KEYWORDS,0},
   {reinterpret_cast<IDL_SYSRTN_GENERIC>(set_stretch_type), "SET_STRETCH_TYPE",0,5,IDL_SYSFUN_DEF_F_KEYWORDS,0},
   {reinterpret_cast<IDL_SYSRTN_GENERIC>(set_stretch_values), "SET_STRETCH_VALUES",0,5,IDL_SYSFUN_DEF_F_KEYWORDS,0},
   {NULL, NULL, 0, 0, 0, 0}
};

bool addVisualizationCommands()
{
   int funcCnt = -1;
   while (func_definitions[++funcCnt].name != NULL); // do nothing in the loop body
   return IDL_SysRtnAdd(func_definitions, IDL_TRUE, funcCnt) != 0;
}

/*
 * The information in this file is
 * subject to the terms and conditions of the
 * GNU Lesser General Public License Version 2.1
 * The license text is available from
 * http://www.gnu.org/licenses/lgpl.html
 */

#include "Animation.h"
#include "AnimationController.h"
#include "AnimationFrame.h"
#include "AnimationServices.h"
#include "AnimationToolBar.h"
#include "ComplexData.h"
#include "ColorMap.h"
#include "DesktopServices.h"
#include "IdlFunctions.h"
#include "IdlStart.h"
#include "LayerList.h"
#include "ModelServices.h"
#include "ModuleManager.h"
#include "PlugInArgList.h"
#include "PlugInResource.h"
#include "Progress.h"
#include "RasterDataDescriptor.h"
#include "RasterElement.h"
#include "RasterLayer.h"
#include "SpatialDataView.h"
#include "SpatialDataWindow.h"
#include "StringUtilities.h"
#include "switchOnEncoding.h"
#include "Undo.h"
#include "WorkspaceWindow.h"

#include <stdio.h>
#include <idl_export.h>
#include <idl_callproxy.h>
#include <QtGui/QWidget>

namespace
{
std::string output;
bool success = true;
Progress* spProgress = NULL;

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

void refresh_display(int argc, IDL_VPTR pArgv[], char* pArgk)
{
   typedef struct
   {
      IDL_KW_RESULT_FIRST_FIELD;
      int datasetExists;
      IDL_STRING dataset;
   } KW_RESULT;
   KW_RESULT kw;

   //IDL_KW_FAST_SCAN is the type of scan we are using, following it is the
   //name of the keyword, followed by the type, the mask(which should be 1),
   //flags, a boolean whether the value was populated and finally the value itself
   static IDL_KW_PAR kw_pars[] = {
      IDL_KW_FAST_SCAN,
      {"DATASET", IDL_TYP_STRING, 1, 0, reinterpret_cast<int*>(IDL_KW_OFFSETOF(datasetExists)),
         reinterpret_cast<char*>(IDL_KW_OFFSETOF(dataset))},
      {NULL}
   };
   IDL_KWProcessByOffset(argc, pArgv, pArgk, kw_pars, 0, 1, &kw);

   std::string dataName;
   if (kw.datasetExists)
   {
      dataName = IDL_STRING_STR(&kw.dataset);
   }
   WorkspaceWindow* vData = Service<DesktopServices>()->getCurrentWorkspaceWindow();
   RasterElement* pElement = NULL;
   if (vData != NULL)
   {
      View* pView = vData->getView();
      if (pView !=NULL)
      {
         if (dataName.empty())
         {
            SpatialDataView* pSpatialView = dynamic_cast<SpatialDataView*>(pView);
            if (pSpatialView != NULL)
            {
               LayerList* pList = pSpatialView->getLayerList();
               if (pList != NULL)
               {
                  pElement = pList->getPrimaryRasterElement();
                  if (pElement != NULL)
                  {
                     pElement->updateData();
                  }
               }
            }
         }
         else
         {
            pElement = IdlFunctions::getDataset(dataName);
            if (pElement == NULL)
            {
               IDL_Message(IDL_M_GENERIC, IDL_MSG_RET, "Opticks was unable to determine the dataset.");
               return;
            }
            pElement->updateData();
         }
         if (pElement != NULL)
         {
            //here is a piece of code to force an update by adjusting the histogram
            //settings of the raster layer being udpated.
            RasterLayer* pLayer = dynamic_cast<RasterLayer*>(
               IdlFunctions::getLayerByRaster(pElement));
            if (pLayer != NULL)
            {
               DisplayMode mode = pLayer->getDisplayMode();
               if (mode == GRAYSCALE_MODE)
               {
                  pLayer->setDisplayMode(RGB_MODE);
               }
               else
               {
                  pLayer->setDisplayMode(GRAYSCALE_MODE);
               }
               pLayer->setDisplayMode(mode);
            }
         }
         pView->refresh();
      }
   }
}

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

IDL_VPTR close_window(int argc, IDL_VPTR pArgv[], char* pArgk)
{
   typedef struct
   {
      IDL_KW_RESULT_FIRST_FIELD;
      int typeExists;
      IDL_STRING typeName;
      int windowNameExists;
      IDL_STRING windowName;
   } KW_RESULT;
   KW_RESULT kw;

   //IDL_KW_FAST_SCAN is the type of scan we are using, following it is the
   //name of the keyword, followed by the type, the mask(which should be 1),
   //flags, a boolean whether the value was populated and finally the value itself
   static IDL_KW_PAR kw_pars[] = {
      IDL_KW_FAST_SCAN,
      {"TYPE", IDL_TYP_STRING, 1, 0, reinterpret_cast<int*>(IDL_KW_OFFSETOF(typeExists)),
         reinterpret_cast<char*>(IDL_KW_OFFSETOF(typeName))},
      {"WINDOW", IDL_TYP_STRING, 1, 0, reinterpret_cast<int*>(IDL_KW_OFFSETOF(windowNameExists)),
         reinterpret_cast<char*>(IDL_KW_OFFSETOF(windowName))},
      {NULL}
   };

   IDL_KWProcessByOffset(argc, pArgv, pArgk, kw_pars, 0, 1, &kw);

   std::string windowName;
   std::string typeName = "SPATIAL_DATA_WINDOW";
   IDL_VPTR idlPtr;
   bool bSuccess = false;
   if (kw.typeExists)
   {
      typeName = IDL_STRING_STR(&kw.typeName);
   }
   // retrieve the layer name passed in as a keyword
   if (kw.windowNameExists)
   {
      windowName = IDL_STRING_STR(&kw.windowName);
   }

   // get the layer
   Window* pWindow = NULL;
   if (!windowName.empty())
   {
      // get the window type
      WindowType windowType = StringUtilities::fromXmlString<WindowType>(typeName);
      // get the window
      pWindow = Service<DesktopServices>()->getWindow(windowName, windowType);
   }
   else
   {
      pWindow = Service<DesktopServices>()->getCurrentWorkspaceWindow();
   }
   if (pWindow != NULL)
   {
      // close the window
      bSuccess = Service<DesktopServices>()->deleteWindow(pWindow);
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

IDL_VPTR set_window_label(int argc, IDL_VPTR pArgv[], char* pArgk)
{
   typedef struct
   {
      IDL_KW_RESULT_FIRST_FIELD;
      int typeExists;
      IDL_STRING typeName;
      int windowNameExists;
      IDL_STRING windowName;
   } KW_RESULT;
   KW_RESULT kw;

   //IDL_KW_FAST_SCAN is the type of scan we are using, following it is the
   //name of the keyword, followed by the type, the mask(which should be 1),
   //flags, a boolean whether the value was populated and finally the value itself
   static IDL_KW_PAR kw_pars[] = {
      IDL_KW_FAST_SCAN,
      {"TYPE", IDL_TYP_STRING, 1, 0, reinterpret_cast<int*>(IDL_KW_OFFSETOF(typeExists)),
         reinterpret_cast<char*>(IDL_KW_OFFSETOF(typeName))},
      {"WINDOW", IDL_TYP_STRING, 1, 0, reinterpret_cast<int*>(IDL_KW_OFFSETOF(windowNameExists)),
         reinterpret_cast<char*>(IDL_KW_OFFSETOF(windowName))},
      {NULL}
   };

   IDL_KWProcessByOffset(argc, pArgv, pArgk, kw_pars, 0, 1, &kw);

   std::string windowName;
   std::string typeName = "SPATIAL_DATA_WINDOW";
   std::string label;

   IDL_VPTR idlPtr;
   bool bSuccess = false;
   //retrieve the label passed in as a parameter
   label = IDL_VarGetString(pArgv[0]);

   if (kw.typeExists)
   {
      typeName = IDL_STRING_STR(&kw.typeName);
   }
   //retrieve the layer name passed in as a keyword
   if (kw.windowNameExists)
   {
      windowName = IDL_STRING_STR(&kw.windowName);
   }

   //get the layer
   ViewWindow* pWindow = NULL;
   if (!windowName.empty())
   {
      //get the window type
      WindowType windowType = StringUtilities::fromXmlString<WindowType>(typeName);
      //get the window
      pWindow = dynamic_cast<ViewWindow*>(Service<DesktopServices>()->getWindow(windowName, windowType));
   }
   else
   {
      pWindow = Service<DesktopServices>()->getCurrentWorkspaceWindow();
   }
   if (pWindow != NULL)
   {
      //close the window
      QWidget* pTmpWidget = pWindow->getWidget();
      if (pTmpWidget != NULL)
      {
         QWidget* pWidget = dynamic_cast<QWidget*>(pTmpWidget->parent());
         if (pWidget != NULL)
         {
            pWidget->setWindowTitle(QString::fromStdString(label));
            bSuccess = true;
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
   IDL_KW_FREE;
   return idlPtr;
}


IDL_VPTR get_window_label(int argc, IDL_VPTR pArgv[], char* pArgk)
{
   typedef struct
   {
      IDL_KW_RESULT_FIRST_FIELD;
      int typeExists;
      IDL_STRING typeName;
      int windowNameExists;
      IDL_STRING windowName;
   } KW_RESULT;
   KW_RESULT kw;

   //IDL_KW_FAST_SCAN is the type of scan we are using, following it is the
   //name of the keyword, followed by the type, the mask(which should be 1),
   //flags, a boolean whether the value was populated and finally the value itself
   static IDL_KW_PAR kw_pars[] = {
      IDL_KW_FAST_SCAN,
      {"TYPE", IDL_TYP_STRING, 1, 0, reinterpret_cast<int*>(IDL_KW_OFFSETOF(typeExists)),
         reinterpret_cast<char*>(IDL_KW_OFFSETOF(typeName))},
      {"WINDOW", IDL_TYP_STRING, 1, 0, reinterpret_cast<int*>(IDL_KW_OFFSETOF(windowNameExists)),
         reinterpret_cast<char*>(IDL_KW_OFFSETOF(windowName))},
      {NULL}
   };

   IDL_KWProcessByOffset(argc, pArgv, pArgk, kw_pars, 0, 1, &kw);

   std::string windowName;
   std::string typeName = "SPATIAL_DATA_WINDOW";
   std::string label;

   IDL_VPTR idlPtr;
   bool bSuccess = false;

   if (kw.typeExists)
   {
      typeName = IDL_STRING_STR(&kw.typeName);
   }
   //retrieve the layer name passed in as a keyword
   if (kw.windowNameExists)
   {
      windowName = IDL_STRING_STR(&kw.windowName);
   }

   //get the layer
   ViewWindow* pWindow = NULL;
   if (!windowName.empty())
   {
      //get the window type
      WindowType windowType = StringUtilities::fromXmlString<WindowType>(typeName);
      //get the window
      pWindow = dynamic_cast<ViewWindow*>(Service<DesktopServices>()->getWindow(windowName, windowType));
   }
   else
   {
      pWindow = Service<DesktopServices>()->getCurrentWorkspaceWindow();
   }
   if (pWindow != NULL)
   {
      //close the window
      QWidget* pTmpWidget = pWindow->getWidget();
      if (pTmpWidget != NULL)
      {
         QWidget* pWidget = dynamic_cast<QWidget*>(pTmpWidget->parent());
         if (pWidget != NULL)
         {
            label = pWidget->windowTitle().toStdString();
         }
      }
   }

   idlPtr = IDL_StrToSTRING(const_cast<char*>(label.c_str()));
   IDL_KW_FREE;
   return idlPtr;
}

IDL_VPTR get_window_position(int argc, IDL_VPTR pArgv[], char* pArgk)
{
   IDL_VPTR idlPtr;
   typedef struct
   {
      IDL_KW_RESULT_FIRST_FIELD;
      int typeExists;
      IDL_STRING typeName;
      int windowNameExists;
      IDL_STRING windowName;
      int xPosExists;
      IDL_VPTR xPos;
      int yPosExists;
      IDL_VPTR yPos;
   } KW_RESULT;
   KW_RESULT kw;

   //IDL_KW_FAST_SCAN is the type of scan we are using, following it is the
   //name of the keyword, followed by the type, the mask(which should be 1),
   //flags, a boolean whether the value was populated and finally the value itself
   static IDL_KW_PAR kw_pars[] = {
      IDL_KW_FAST_SCAN,
      {"TYPE", IDL_TYP_STRING, 1, 0, reinterpret_cast<int*>(IDL_KW_OFFSETOF(typeExists)),
         reinterpret_cast<char*>(IDL_KW_OFFSETOF(typeName))},
      {"WINDOW", IDL_TYP_STRING, 1, 0, reinterpret_cast<int*>(IDL_KW_OFFSETOF(windowNameExists)),
         reinterpret_cast<char*>(IDL_KW_OFFSETOF(windowName))},
      {"WIN_POS_X", IDL_TYP_DOUBLE, 1, IDL_KW_OUT, reinterpret_cast<int*>(IDL_KW_OFFSETOF(xPosExists)),
         reinterpret_cast<char*>(IDL_KW_OFFSETOF(xPos))},
      {"WIN_POS_Y", IDL_TYP_DOUBLE, 1, IDL_KW_OUT, reinterpret_cast<int*>(IDL_KW_OFFSETOF(yPosExists)),
         reinterpret_cast<char*>(IDL_KW_OFFSETOF(yPos))},
      {NULL}
   };

   IDL_KWProcessByOffset(argc, pArgv, pArgk, kw_pars, 0, 1, &kw);
   std::string windowName;
   std::string typeName = "SPATIAL_DATA_WINDOW";
   std::string label;

   bool bSuccess = false;
   if (argc < 1)
   {
      IDL_Message(IDL_M_GENERIC, IDL_MSG_RET, "get_window_position 'type', 'window', 'win_x_pos', and 'win_y_pos' as keywords.");
   }
   else
   {
      if (kw.typeExists)
      {
         typeName = IDL_STRING_STR(&kw.typeName);
      }
      //retrieve the layer name passed in as a keyword
      if (kw.windowNameExists)
      {
         windowName = IDL_STRING_STR(&kw.windowName);
      }

      //get the layer
      ViewWindow* pWindow = NULL;
      if (!windowName.empty())
      {
         //get the window type
         WindowType windowType = StringUtilities::fromXmlString<WindowType>(typeName);
         //get the window
         pWindow = dynamic_cast<ViewWindow*>(Service<DesktopServices>()->getWindow(windowName, windowType));
      }
      else
      {
         pWindow = Service<DesktopServices>()->getCurrentWorkspaceWindow();
      }
      if (pWindow != NULL)
      {
         //close the window
         QWidget* pTmpWidget = pWindow->getWidget();
         if (pTmpWidget != NULL)
         {
            QWidget* pWidget = dynamic_cast<QWidget*>(pTmpWidget->parent());
            if (pWidget != NULL)
            {
               QWidget* pMain = dynamic_cast<QWidget*>(pWidget->parent());
               if (pMain != NULL)
               {
                  int xPos = pMain->x();
                  if (kw.xPosExists)
                  {
                     //we don't know the datatype passed in, so set all of them
                     kw.xPos->value.d = (double)xPos;
                     kw.xPos->value.f = (float)xPos;
                     kw.xPos->value.i = (short)xPos;
                     kw.xPos->value.ui = (unsigned short)xPos;
                     kw.xPos->value.ul = (unsigned int)xPos;
                     kw.xPos->value.l = xPos;
                     kw.xPos->value.l64 = (unsigned long)xPos;
                     kw.xPos->value.ul64 = (unsigned long)xPos;
                     kw.xPos->value.sc = (char)xPos;
                     kw.xPos->value.c = (unsigned char)xPos;
                     bSuccess = true;
                  }
                  int yPos = pMain->y();
                  if (kw.xPosExists)
                  {
                     //we don't know the datatype passed in to populate, so set all of them
                     kw.yPos->value.d = (double)yPos;
                     kw.yPos->value.f = (float)yPos;
                     kw.yPos->value.i = (short)yPos;
                     kw.yPos->value.ui = (unsigned short)yPos;
                     kw.yPos->value.ul = (unsigned int)yPos;
                     kw.yPos->value.l = yPos;
                     kw.yPos->value.l64 = (unsigned long)yPos;
                     kw.yPos->value.ul64 = (unsigned long)yPos;
                     kw.yPos->value.sc = (char)yPos;
                     kw.yPos->value.c = (unsigned char)yPos;
                     bSuccess = true;
                  }
               }
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
   }
   IDL_KW_FREE;
   return idlPtr;
}

IDL_VPTR set_window_position(int argc, IDL_VPTR pArgv[], char* pArgk)
{
   IDL_VPTR idlPtr;
   typedef struct
   {
      IDL_KW_RESULT_FIRST_FIELD;
      int typeExists;
      IDL_STRING typeName;
      int windowNameExists;
      IDL_STRING windowName;
      int xPosExists;
      IDL_LONG xPos;
      int yPosExists;
      IDL_LONG yPos;
   } KW_RESULT;
   KW_RESULT kw;

   //IDL_KW_FAST_SCAN is the type of scan we are using, following it is the
   //name of the keyword, followed by the type, the mask(which should be 1),
   //flags, a boolean whether the value was populated and finally the value itself
   static IDL_KW_PAR kw_pars[] = {
      IDL_KW_FAST_SCAN,
      {"TYPE", IDL_TYP_STRING, 1, 0, reinterpret_cast<int*>(IDL_KW_OFFSETOF(typeExists)),
         reinterpret_cast<char*>(IDL_KW_OFFSETOF(typeName))},
      {"WINDOW", IDL_TYP_STRING, 1, 0, reinterpret_cast<int*>(IDL_KW_OFFSETOF(windowNameExists)),
         reinterpret_cast<char*>(IDL_KW_OFFSETOF(windowName))},
      {"WIN_POS_X", IDL_TYP_LONG, 1, 0, reinterpret_cast<int*>(IDL_KW_OFFSETOF(xPosExists)),
         reinterpret_cast<char*>(IDL_KW_OFFSETOF(xPos))},
      {"WIN_POS_Y", IDL_TYP_LONG, 1, 0, reinterpret_cast<int*>(IDL_KW_OFFSETOF(yPosExists)),
         reinterpret_cast<char*>(IDL_KW_OFFSETOF(yPos))},
      {NULL}
   };

   IDL_KWProcessByOffset(argc, pArgv, pArgk, kw_pars, 0, 1, &kw);
   std::string windowName;
   std::string typeName = "SPATIAL_DATA_WINDOW";
   std::string label;
   int xPos = 0;
   int yPos = 0;

   bool bSuccess = false;
   if (argc < 1)
   {
      IDL_Message(IDL_M_GENERIC, IDL_MSG_RET, "get_window_position 'type', 'window', 'win_x_pos', and 'win_y_pos' as keywords.");
   }
   else
   {

      if (kw.typeExists)
      {
         typeName = IDL_STRING_STR(&kw.typeName);
      }
      //retrieve the layer name passed in as a keyword
      if (kw.windowNameExists)
      {
         windowName = IDL_STRING_STR(&kw.windowName);
      }
      if (kw.typeExists)
      {
         typeName = IDL_STRING_STR(&kw.typeName);
      }
      if (kw.xPosExists)
      {
         xPos = kw.xPos;
      }
      if (kw.yPosExists)
      {
         yPos = kw.yPos;
      }

      //get the layer
      ViewWindow* pWindow = NULL;
      if (!windowName.empty())
      {
         //get the window type
         WindowType windowType = StringUtilities::fromXmlString<WindowType>(typeName);
         //get the window
         pWindow = dynamic_cast<ViewWindow*>(Service<DesktopServices>()->getWindow(windowName, windowType));
      }
      else
      {
         pWindow = Service<DesktopServices>()->getCurrentWorkspaceWindow();
      }
      if (pWindow != NULL)
      {
         //close the window
         QWidget* pTmpWidget = pWindow->getWidget();
         if (pTmpWidget != NULL)
         {
            QWidget* pWidget = dynamic_cast<QWidget*>(pTmpWidget->parent());
            if (pWidget != NULL)
            {
               QWidget* pMain = dynamic_cast<QWidget*>(pWidget->parent());
               if (pMain != NULL)
               {
                  pMain->move(xPos,yPos);
               }
            }
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
   IDL_KW_FREE;
   return idlPtr;
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
      {"LAYER", IDL_TYP_STRING, 1, 0, reinterpret_cast<int*>(IDL_KW_OFFSETOF(windowExists)),
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
      pLayer->setStretchUnits(element, eMethod);
      bSuccess = true;
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

      pLayer->setStretchType(element, eType);
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
   IDL_KW_FREE;
   return idlPtr;
}

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
   std::string layerName;
   std::string filter;

   if (kw.windowExists)
   {
      windowName = IDL_STRING_STR(&kw.windowName);
   }
   if (kw.layerNameExists)
   {
      layerName = IDL_STRING_STR(&kw.layerName);
   }
   bool bSuccess = false;
   if (argc < 0)
   {
      IDL_Message(IDL_M_GENERIC, IDL_MSG_RET, "function takes "
         "'window' and 'layer' as optional keywords.");
      IDL_KW_FREE;
      return IDL_StrToSTRING("failure");
   }
   filter = IDL_VarGetString(pArgv[0]);
   RasterLayer* pLayer = dynamic_cast<RasterLayer*>(IdlFunctions::getLayerByName(windowName, layerName));
   if (pLayer != NULL)
   {
      //determine the type, allow strings to be converted into an enum for later filters
      std::vector<std::string> supportedFilters = pLayer->getSupportedFilters();
      for (std::vector<std::string>::const_iterator filtersIt = supportedFilters.begin(); filtersIt != supportedFilters.end(); filtersIt++)
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
   IDL_KW_FREE;
   return idlPtr;
}

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
   std::string layerName;
   std::string filter;
   bool bSuccess = false;

   if (kw.windowExists)
   {
      windowName = IDL_STRING_STR(&kw.windowName);
   }
   if (kw.layerNameExists)
   {
      layerName = IDL_STRING_STR(&kw.layerName);
   }

   if (argc < 1)
   {
      IDL_Message(IDL_M_GENERIC, IDL_MSG_RET, "function takes  "
         "'window' and 'layer' as optional keywords.");
      IDL_KW_FREE;
      return IDL_StrToSTRING("failure");
   }
   filter = IDL_VarGetString(pArgv[0]);
   RasterLayer* pLayer = dynamic_cast<RasterLayer*>(IdlFunctions::getLayerByName(windowName, layerName));
   if (pLayer != NULL)
   {
      //determine the type, allow strings to be converted into an enum for later filters
      std::vector<std::string> supportedFilters = pLayer->getSupportedFilters();
      for (std::vector<std::string>::const_iterator filtersIt = supportedFilters.begin(); filtersIt != supportedFilters.end(); filtersIt++)
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
   IDL_KW_FREE;
   return idlPtr;
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

IDL_VPTR disable_gpu(int argc, IDL_VPTR pArgv[], char* pArgk)
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
      IDL_Message(IDL_M_GENERIC, IDL_MSG_RET, "function takes a layer name as a parameter with a "
         "'window' as an optional keyword.");
      IDL_KW_FREE;
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
   IDL_KW_FREE;
   return idlPtr;
}

IDL_VPTR enable_gpu(int argc, IDL_VPTR pArgv[], char* pArgk)
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
   bool bSuccess = false;

   if (kw.windowExists)
   {
      windowName = IDL_STRING_STR(&kw.windowName);
   }

   if (argc < 1)
   {
      IDL_Message(IDL_M_GENERIC, IDL_MSG_RET, "function takes a layer name as a parameter with a "
         "'window' as an optional keyword.");
      IDL_KW_FREE;
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
   IDL_KW_FREE;
   return idlPtr;
}

IDL_VPTR execute_wizard(int argc, IDL_VPTR pArgv[], char* pArgk)
{
   IDL_VPTR idlPtr1 = IDL_StrToSTRING("success");
   IDL_VPTR idlPtr2 = IDL_StrToSTRING("failure");
   IDL_VPTR* idlPtr = &idlPtr2;
   if (argc < 1)
   {
      IDL_Message(IDL_M_GENERIC, IDL_MSG_RET, "A wizard needs to be specified.");
      return IDL_StrToSTRING("failure");
   }
   typedef struct
   {
      IDL_KW_RESULT_FIRST_FIELD;
      int batchExists;
      IDL_LONG batch;
   } KW_RESULT;
   KW_RESULT kw;

   //IDL_KW_FAST_SCAN is the type of scan we are using, following it is the
   //name of the keyword, followed by the type, the mask(which should be 1),
   //flags, a boolean whether the value was populated and finally the value itself
   static IDL_KW_PAR kw_pars[] = {
      IDL_KW_FAST_SCAN,
      {"BATCH", IDL_TYP_INT, 1, 0, reinterpret_cast<int*>(IDL_KW_OFFSETOF(batchExists)),
         reinterpret_cast<char*>(IDL_KW_OFFSETOF(batch))},
      {NULL}
   };

   IDL_KWProcessByOffset(argc, pArgv, pArgk, kw_pars, 0, 1, &kw);

   int batch = 0;

   if (kw.batchExists)
   {
      if (kw.batch != 0)
      {
         batch = 1;
      }
   }
   //the full path and name of the wizard file
   std::string wizardName = IDL_VarGetString(pArgv[0]);
   //HERE  WizardObject* pWizard = IdlFunctions::getWizardObject(wizardName);
   WizardObject* pWizard = NULL;
   bool bSuccess = false;
   if (pWizard != NULL)
   {
      bSuccess = true;

      //create the Wizard Executor plugin
      ExecutableResource pExecutor("Wizard Executor", "", NULL, batch);
      if (pExecutor.get() == NULL)
      {
         bSuccess = false;
      }
      else
      {
         PlugInArgList& pArgList = pExecutor->getInArgList();
         pArgList.setPlugInArgValue("Wizard", pWizard);
         //execute the plugin
         bSuccess &= pExecutor->execute();
      }
   }
   else
   {
      IDL_Message(IDL_M_GENERIC, IDL_MSG_RET, "unable to find wizard file.");
   }
   if (bSuccess)
   {
      idlPtr = &idlPtr1;
   }
   IDL_KW_FREE;
   return *idlPtr;
}

#if 0
IDL_VPTR get_metadata(int argc, IDL_VPTR pArgv[], char* pArgk)
{
   IDL_VPTR idlPtr;
   typedef struct
   {
      IDL_KW_RESULT_FIRST_FIELD;
      int datasetExists;
      IDL_STRING datasetName;
      int wizardExists;
      IDL_STRING wizardName;
   } KW_RESULT;
   KW_RESULT kw;

   //IDL_KW_FAST_SCAN is the type of scan we are using, following it is the
   //name of the keyword, followed by the type, the mask(which should be 1),
   //flags, a boolean whether the value was populated and finally the value itself
   static IDL_KW_PAR kw_pars[] = {
      IDL_KW_FAST_SCAN,
      {"DATASET", IDL_TYP_STRING, 1, 0, reinterpret_cast<int*>(IDL_KW_OFFSETOF(datasetExists)),
         reinterpret_cast<char*>(IDL_KW_OFFSETOF(datasetName))},
      {"WIZARD", IDL_TYP_STRING, 1, 0, reinterpret_cast<int*>(IDL_KW_OFFSETOF(wizardExists)),
         reinterpret_cast<char*>(IDL_KW_OFFSETOF(wizardName))},
      {NULL}
   };

   IDL_KWProcessByOffset(argc, pArgv, pArgk, kw_pars, 0, 1, &kw);
   if (argc < 1)
   {
      IDL_Message(IDL_M_GENERIC, IDL_MSG_RET, "GET_METADATA takes a std::string noting which "
         "piece of metadata to get, it also takes a 'wizard' name as keywords.");
      return IDL_StrToSTRING("failure");
   }

   std::string filename;
   std::string wizardName;
   int wizard = 0;
   bool bArray = false;
   //get the value to change as a parameter
   std::string element = IDL_VarGetString(pArgv[0]);
   DynamicObject* pObject = NULL;

   if (kw.datasetExists)
   {
      filename = IDL_STRING_STR(&kw.datasetName);
   }
   if (kw.wizardExists)
   {
      wizardName = IDL_STRING_STR(&kw.wizardName);
      wizard = 1;
   }

   std::string strType;
   void* pValue = NULL;
   if (!wizard)
   {
      RasterElement* pSensor = IdlFunctions::getDataset(filename);

      if (pSensor == NULL)
      {
         std::string msg = "Error could not find array.";
         IDL_Message(IDL_M_GENERIC, IDL_MSG_RET, msg);
         return IDL_StrToSTRING("");
      }
      //not a wizard, so we use the dynamic object functions to get data
      pObject = (pSensor->getMetadata();
      pValue = IdlFunctions::getDynamicObjectValue(pObject, element, strType);
   }
   else
   {
      //this is a wizard, so get the wizard object and use it to get a value
      WizardObject* pWizard = IdlFunctions::getWizardObject(wizardName);

      if (pWizard != NULL)
      {
         pValue = IdlFunctions::getWizardObjectValue(pWizard, element, strType);
         if (pValue == NULL)
         {
            IDL_Message(IDL_M_GENERIC, IDL_MSG_RET, "unable to find wizard item.");
            return IDL_StrToSTRING("");
         }
      }
      else
      {
         IDL_Message(IDL_M_GENERIC, IDL_MSG_RET, "unable to find wizard file.");
      }
   }

   //setup the vectors to pass
   int type = IDL_TYP_UNDEF;
   std::vector<unsigned char> charValue;
   std::vector<int> intValue;
   std::vector<unsigned int> uIntValue;
   std::vector<float> floatValue;
   std::vector<double> doubleValue;
   std::vector<short> shortValue;
   std::vector<unsigned short> uShortValue;
   std::vector<std::string> stringValueA;
   std::vector<Filename*> fileValue;
   void* pArrayValue = NULL;
   IDL_MEMINT total = 0;
   std::string strValue;

   int i = 0;
   //for each datatype, populate the type std::string and build a vector if necessary
   if (strType == "unsigned char" || strType == "bool" || strType == "char")
   {
      idlPtr = IDL_Gettmp();
      idlPtr->value.c = *reinterpret_cast<unsigned char*>(pValue);
      idlPtr->value.sc = *reinterpret_cast<char*>(pValue);
      idlPtr->type = IDL_TYP_BYTE;
   }
   else if (strType == "vector<unsigned char>" || strType == "vector<bool>" || strType == "vector<char>")
   {
      bArray = true;
      type = IDL_TYP_BYTE;
      charValue = *reinterpret_cast<std::vector<unsigned char>*>(pValue);
      total = static_cast<int>(charValue.size());
      char* pCharArrayValue = new char[total];
      for (i = 0; i < total; i++)
      {
         pCharArrayValue[i] = charValue[i];
      }
      pArrayValue = pCharArrayValue;
   }
   else if (strType == "short")
   {
      idlPtr = IDL_GettmpInt(*reinterpret_cast<short*>(pValue));
   }
   else if (strType == "vector<short>")
   {
      bArray = true;
      type = IDL_TYP_INT;
      shortValue = *reinterpret_cast<std::vector<short>*>(pValue);
      total = static_cast<int>(shortValue.size());
      short* pShortArrayValue = new short[total];
      for (i = 0; i < total; i++)
      {
         pShortArrayValue[i] = shortValue[i];
      }
   }
   else if (strType == "unsigned short")
   {
      idlPtr = IDL_GettmpUInt(*(unsigned short*)value);
   }
   else if (strType == "vector<unsigned short>")
   {
      bArray = true;
      type= IDL_TYP_UINT;
      uShortValue = *(std::vector<unsigned short>*)value;
      total = static_cast<int>(uShortValue.size());
      arrayValue = malloc(sizeof(unsigned short)*total);
      for (i = 0; i < total; i++)
      {
         unsigned short* ushortptr = (unsigned short*)value;
         ((unsigned short*)arrayValue)[i] = uShortValue[i];
      }
   }
   else if (strType == "int")
   {
      idlPtr = IDL_GettmpLong(*reinterpret_cast<int*>(value);
   }
   else if (strType == "vector<int>")
   {
      bArray = true;
      type = IDL_TYP_LONG;
      intValue = *(std::vector<int>*)value;
      total = static_cast<int>(intValue.size());
      arrayValue = malloc(sizeof(int)*total);
      for (i = 0; i < total; i++)
      {
         (reinterpret_cast<int*>(arrayValue)[i] = intValue[i];
      }
   }
   else if (strType == "unsigned int")
   {
      idlPtr = IDL_GettmpLong(*(unsigned int*)value);
   }
   else if (strType == "vector<unsigned int>")
   {
      bArray = true;
      type = IDL_TYP_ULONG;
      uIntValue = *(std::vector<unsigned int>*)value;
      total = static_cast<int>(uIntValue.size());
      arrayValue = malloc(sizeof(unsigned int)*total);
      for (i = 0; i < total; i++)
      {
         ((unsigned int*)arrayValue)[i] = uIntValue[i];
      }
   }
   else if (strType == "float")
   {
      idlPtr = IDL_Gettmp();
      idlPtr->value.f = *(float*)value;
      idlPtr->type = IDL_TYP_FLOAT;
   }
   else if (strType == "vector<float>")
   {
      bArray = true;
      type = IDL_TYP_FLOAT;
      floatValue = *(std::vector<float>*)value;
      total = static_cast<int>(floatValue.size());
      arrayValue = malloc(sizeof(float)*total);
      for (i = 0; i < total; i++)
      {
         ((float*)arrayValue)[i] = floatValue[i];
      }
   }
   else if (strType == "double")
   {
      idlPtr = IDL_Gettmp();
      idlPtr->value.d = *(double*)value;
      idlPtr->type = IDL_TYP_DOUBLE;
   }
   else if (strType == "vector<double>")
   {
      bArray = true;
      type = IDL_TYP_DOUBLE;
      doubleValue = *(std::vector<double>*)value;
      total = static_cast<int>(doubleValue.size());
      arrayValue = malloc(sizeof(double)*total);
      for (i = 0; i < total; i++)
      {
         ((double*)arrayValue)[i] = doubleValue[i];
      }
   }
   else if (strType == "Filename")
   {
      idlPtr = IDL_StrToSTRING(const_cast<char*>(((reinterpret_cast<Filename*>(value))->getFullPathAndName())));
   }
   else if (strType == "vector<Filename>")
   {
      bArray = true;
      type = IDL_TYP_STRING;
      fileValue = *(std::vector<Filename*>*)value;
      total = static_cast<int>(fileValue.size());
      // Make the IDL std::string array variable
      IDL_STRING* strarr = NULL;
      strarr = (IDL_STRING*)IDL_MemAlloc(total * sizeof(IDL_STRING), NULL, 0);
      for (i=0; i < total; i++)
      {
         IDL_StrStore(&(strarr[i]), reinterpret_cast<char*>(fileValue[i]->getFullPathAndName());
      }
      arrayValue = strarr;
   }
   else if (strType == "std::string")
   {
      idlPtr = IDL_StrToSTRING(reinterpret_cast<char*>((((std::string*)value)->c_str()));
   }
   else if (strType == "Filename")
   {
      idlPtr = IDL_StrToSTRING(reinterpret_cast<char*>(((((Filename*)value)->getFullPathAndName())));
   }
   else if (strType == "vector<Filename>")
   {
      bArray = true;
      type = IDL_TYP_STRING;
      fileValue = *(std::vector<Filename*>*)value;
      total = static_cast<int>(fileValue.size());
      // Make the IDL std::string array variable
      IDL_STRING* strarr = NULL;
      strarr = (IDL_STRING*)IDL_MemAlloc(total * sizeof(IDL_STRING), NULL, 0);
      for (i=0; i < total; i++)
      {
         IDL_StrStore(&(strarr[i]), reinterpret_cast<char*>(fileValue[i]->getFullPathAndName());
      }
      arrayValue = strarr;
   }
   else if (strType == "vector<std::string>")
   {
      bArray = true;
      type = IDL_TYP_STRING;
      stringValueA = *(vector<std::string>*)value;
      total = static_cast<int>(stringValueA.size());
      // Make the IDL std::string array variable
      IDL_STRING* strarr = NULL;
      strarr = (IDL_STRING*)IDL_MemAlloc(total * sizeof(IDL_STRING), NULL, 0);
      for (i=0; i < total; i++)
      {
         IDL_StrStore(&(strarr[i]), reinterpret_cast<char*>(stringValueA[i]);
      }
      arrayValue = strarr;
   }
   else
   {
      idlPtr = IDL_GettmpInt(0);
      IDL_Message(IDL_M_GENERIC, IDL_MSG_RET, "unable to determine type.");
   }
   if (bArray)
   {
      IDL_MEMINT dims[] = { total};
      idlPtr = IDL_ImportArray(1, dims, type, (unsigned char*)arrayValue, NULL, NULL);
   }
   IDL_KW_FREE;

   return idlPtr;
}

/**
*    SET_METADATA
*
*    Method for setting information is support areas of Opticks, such as the
*    metadata or wizard objects.  The first parameter,
*    element specifies what should be updated.  Its a single std::string, so layers of
*    objects are separated by '/', so the value of an element used to update a
*    Value type named 'Red' in a wizard object would be, "Value - Red/Red".
*
*    @param element
*           Looks for a meta data item with the given name, for
*           Objects within the other objects, the name of each is
*           separated by '/'.
*    @param value
*           The value to place in the metadata
*    @param BOOL - keyword
*           a keyword boolean specifying the value should be set as a boolean, values
*           for 1 are true, and 0 are false
*    @param WIZARD - keyword
*           a keyword std::string specifying if the meta data object to set values for
*           is from a wizard.  The std::string should contain the full path and name
*           of the wizard file to load and alter
*    @param DATASET - keyword
*           a keyword specifying which dataset should be altered.
*    @return
*           returns, "success" or "failure"
*
*/
IDL_VPTR set_metadata(int argc, IDL_VPTR pArgv[], char* pArgk)
{
   IDL_VPTR idlPtr;
   if (argc>5 || argc<2)
   {
      IDL_Message(IDL_M_GENERIC, IDL_MSG_RET, "SET_METADATA takes a std::string noting which "
         "piece of metadata to set, and a value to set, it also takes a 'wizard' name as keywords.");
      return IDL_StrToSTRING("failure");
   }
   typedef struct
   {
      IDL_KW_RESULT_FIRST_FIELD;
      int datasetExists;
      IDL_STRING datasetName;
      int wizardExists;
      IDL_STRING wizardName;
      int boolExists;
      IDL_LONG useBool;
      int filenameExists;
      IDL_LONG useFilename;
   } KW_RESULT;
   KW_RESULT kw;
   //IDL_KW_FAST_SCAN is the type of scan we are using, following it is the
   //name of the keyword, followed by the type, the mask(which should be 1),
   //flags, a boolean whether the value was populated and finally the value itself
   static IDL_KW_PAR kw_pars[] = {IDL_KW_FAST_SCAN,
   {"BOOL", IDL_TYP_INT, 1, 0, reinterpret_cast<int*>(IDL_KW_OFFSETOF(boolExists),
   reinterpret_cast<char*>(IDL_KW_OFFSETOF(useBool)},
   {"DATASET", IDL_TYP_STRING, 1, 0, reinterpret_cast<int*>(IDL_KW_OFFSETOF(datasetExists),
   reinterpret_cast<char*>(IDL_KW_OFFSETOF(datasetName)},
   {"FILENAME", IDL_TYP_INT, 1, 0, reinterpret_cast<int*>(IDL_KW_OFFSETOF(filenameExists),
   reinterpret_cast<char*>(IDL_KW_OFFSETOF(useFilename)},
   {"WIZARD", IDL_TYP_STRING, 1, 0, reinterpret_cast<int*>(IDL_KW_OFFSETOF(wizardExists),
   reinterpret_cast<char*>(IDL_KW_OFFSETOF(wizardName)},
   {NULL}};

   IDL_KWProcessByOffset(argc, pArgv, pArgk, kw_pars, 0, 1, &kw);

   std::string filename;
   int wizard = 0;
   IDL_MEMINT total;
   //the element
   std::string element = IDL_VarGetString(pArgv[0]);
   char* value = NULL;
   std::string strType;
   std::string wizardName;
   bool bUseFilename = false;
   bool bUseBool = false;
   DynamicObject* pObject = NULL;
   //the value
   IDL_VarGetData(pArgv[1], &total, &value, 0);

   //if there is more than one value in the variable, then make a vector
   if (total > 1)
   {
      strType = "vector<";
   }
   if (kw.datasetExists)
   {
      filename = IDL_STRING_STR(&kw.datasetName);
   }
   if (kw.wizardExists)
   {
      wizardName = IDL_STRING_STR(&kw.wizardName);
      wizard = 1;
   }
   if (kw.boolExists)
   {
      if (kw.useBool != 0)
      {
         bUseBool = true;
      }
   }
   if (kw.filenameExists)
   {
      if (kw.useFilename != 0)
      {
         bUseFilename = true;
      }
   }
   //setup the vectors to pass
   int type = pArgv[1]->type;
   std::vector<bool> boolValue;
   std::vector<unsigned char> charValue;
   std::vector<int> intValue;
   std::vector<unsigned int> uIntValue;
   std::vector<float> floatValue;
   std::vector<double> doubleValue;
   std::vector<short> shortValue;
   std::vector<unsigned short> uShortValue;
   std::vector<std::string> stringValueA;
   std::vector<Filename*> fileValue;
   void* arrayValue = NULL;
   std::string strValue;
   bool bSuccess = false;

   int i = 0;
   if (bUseBool)
   {
      strType = strType + "bool";
      for (i = 0; i < total; i++)
      {
         boolValue.push_back((bool)value[i]);
      }
      arrayValue = &boolValue;
   }
   else if (bUseFilename)
   {
      strType = strType + "Filename";
      if (total == 1)
      {
         //if there is just one, get the contents of the variable as a std::string
         strValue = IDL_VarGetString(pArgv[1]);
         FactoryResource<Filename> pFilename;
         pFilename->setFullPathAndName(strValue);
         value = reinterpret_cast<char*>(pFilename.release();
      }
      else
      {
         UCHAR* arrptr  = pArgv[1]->value.arr->data;
         for (i=0; i < total; i++)
         {
            if (((IDL_STRING*)arrptr)->s != NULL)
            {
               FactoryResource<Filename> pFilename;
               pFilename->setFullPathAndName(std::string(((IDL_STRING*)arrptr)->s));
               fileValue.push_back(pFilename.release());
            }
            arrptr += sizeof(IDL_STRING);
         }
         arrayValue = &fileValue;
      }
   }
   else
   {
      //for each datatype, populate the type std::string and build a vector if necessary
      switch (type)
      {
      case IDL_TYP_BYTE :
         strType = strType + "unsigned char";
         for (i = 0; i < total; i++)
         {
            charValue.push_back(value[i]);
         }
         arrayValue = &charValue;
         break;
      case IDL_TYP_INT :
         strType = strType + "short";
         for (i = 0; i < total; i++)
         {
            short* shortptr = (short*)value;
            shortValue.push_back(shortptr[i]);
         }
         arrayValue = &shortValue;
         break;
      case IDL_TYP_UINT :
         strType = strType + "unsigned short";
         for (i = 0; i < total; i++)
         {
            unsigned short* ushortptr = (unsigned short*)value;
            uShortValue.push_back(ushortptr[i]);
         }
         arrayValue = &uShortValue;
         break;
      case IDL_TYP_LONG :
         strType = strType + "int";
         for (i = 0; i < total; i++)
         {
            int* intptr = reinterpret_cast<int*>(value;
            intValue.push_back(intptr[i]);
         }
         arrayValue = &intValue;
         break;
      case IDL_TYP_ULONG :
         strType = strType + "unsigned int";
         for (i = 0; i < total; i++)
         {
            unsigned int* uintptr = (unsigned int*)value;
            uIntValue.push_back(uintptr[i]);
         }
         arrayValue = &uIntValue;
         break;
      case IDL_TYP_FLOAT :
         strType = strType + "float";
         for (i = 0; i < total; i++)
         {
            float* floatptr = (float*)value;
            floatValue.push_back(floatptr[i]);
         }
         arrayValue = &floatValue;
         break;
      case IDL_TYP_DOUBLE :
         strType = strType + "double";
         for (i = 0; i < total; i++)
         {
            double* doubleptr = (double*)value;
            doubleValue.push_back(doubleptr[i]);
         }
         arrayValue = &doubleValue;
         break;
      case IDL_TYP_STRING :
         strType = "std::string";
         if (total == 1)
         {
            //if there is just one, get the contents of the variable as a std::string
            strValue = IDL_VarGetString(pArgv[1]);
            value = reinterpret_cast<char*>(&strValue;
         }
         else
         {
            UCHAR* arrptr  = pArgv[1]->value.arr->data;
            for (i=0; i < total; i++)
            {
               if (((IDL_STRING*)arrptr)->s != NULL)
               {
                  stringValueA.push_back(std::string(((IDL_STRING*)arrptr)->s));
               }
               arrptr += sizeof(IDL_STRING);
            }
            arrayValue = &stringValueA;
         }
         break;
      case IDL_TYP_UNDEF :
         IDL_Message(IDL_M_GENERIC, IDL_MSG_RET, "unable to determine type.");
         break;
      }
   }
   if (total > 1)
   {
      strType = strType + ">";
      value = reinterpret_cast<char*>(arrayValue;
   }

   if (!wizard)
   {
      RasterElement* pSensor = (IdlFunctions::getDataset(filename));

      if (pSensor == NULL)
      {
         std::string msg = "Error could not find array.";
         IDL_Message(IDL_M_GENERIC, IDL_MSG_RET, msg);
         return IDL_StrToSTRING("");
      }
      //this is not a wizard, so use the DynamicObject setting code
      pObject = (DynamicObject*)(pSensor->getMetadata());
      IdlFunctions::setDynamicObjectValue(pObject, element, strType, (void*)value);
      bSuccess = true;
   }
   else
   {
      //this is a wizard object, so first get the wizard, then set a value in it
      WizardObject* pWizard = IdlFunctions::getWizardObject(wizardName);

      if (pWizard != NULL)
      {
         if (!IdlFunctions::setWizardObjectValue(pWizard, element, strType, (void*)value))
         {
            IDL_Message(IDL_M_GENERIC, IDL_MSG_RET, "unable to find item in wizard file.");
         }
         else
         {
            bSuccess = true;
         }
      }
      else
      {
         IDL_Message(IDL_M_GENERIC, IDL_MSG_RET, "unable to find wizard file.");
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
*    COPY_METADATA
*
*    Method for copying all the metadata from one data element, to another data
*    element.
*
*    @param source_element
*           The element whose metadata should be copied.
*    @param dest_element
*           The element to where the metadata will be copied.
*    @return
*           returns, "success" or "failure"
*/
IDL_VPTR copy_metadata(int argc, IDL_VPTR pArgv[], char* pArgk)
{
   if (argc<2)
   {
      IDL_Message(IDL_M_GENERIC, IDL_MSG_RET, "COPY_METADATA takes a source "
         "element, it also takes a destination element.");
      return IDL_StrToSTRING("failure");
   }

   const std::string sourceElementName = IDL_VarGetString(pArgv[0]);
   const std::string destElementName = IDL_VarGetString(pArgv[1]);

   const RasterElement* const pSourceElement =
      IdlFunctions::getDataset(sourceElementName);
   if (NULL == pSourceElement)
   {
      IDL_Message(IDL_M_GENERIC, IDL_MSG_RET, "unable to locate source element.");
      return IDL_StrToSTRING("failure");
   }
   RasterElement* const pDestElement =
      IdlFunctions::getDataset(destElementName);
   if (NULL == pDestElement)
   {
      IDL_Message(IDL_M_GENERIC, IDL_MSG_RET, "unable to locate destination element.");
      return IDL_StrToSTRING("failure");
   }

   const DynamicObject* const pSourceMetadata = pSourceElement->getMetadata();
   if (NULL == pSourceMetadata)
   {
      IDL_Message(IDL_M_GENERIC, IDL_MSG_RET, "unable to locate metadata "
         "source element.");
      return IDL_StrToSTRING("failure");
   }
   DynamicObject* const pDestMetadata = pDestElement->getMetadata();
   if (NULL == pSourceMetadata)
   {
      IDL_Message(IDL_M_GENERIC, IDL_MSG_RET, "unable to locate metadata "
         "destination element.");
      return IDL_StrToSTRING("failure");
   }

   std::vector<std::string> attributeNames;
   pSourceMetadata->getAttributeNames(attributeNames);
   for (std::vector<std::string>::const_iterator attributeNamesIter = attributeNames.begin();
      attributeNamesIter != attributeNames.end();
      ++attributeNamesIter)
   {
      const std::string attributeName = *attributeNamesIter;
      const DataVariant& attribute = pSourceMetadata->getAttribute(attributeName);
      if (!pDestMetadata->setAttribute(attributeName, attribute))
      {
         const std::string message = std::string("unable to copy attribute: ")
            + attributeName;
         IDL_Message(IDL_M_GENERIC, IDL_MSG_RET, message);
         return IDL_StrToSTRING("failure");
      }
   }

   return IDL_StrToSTRING("success");
}
#endif

IDL_VPTR array_to_idl(int argc, IDL_VPTR pArgv[], char* pArgk)
{
   if (argc<0)
   {
      IDL_Message(IDL_M_GENERIC, IDL_MSG_RET, "ARRAY_TO_IDL takes a 'dataset' name or a 'matrix' name as keywords,"
         "if used with on disk data, you can also use, 'bands_start', 'bands_end', 'height_start', 'height_end', "
         "'width_start', 'width_end' as keywords. If you don't know the dimesions of the array, the keywords 'bands', "
         "'height', and 'width' will return them so you can reform the array to the appropriate dimensions");
   }
   typedef struct
   {
      IDL_KW_RESULT_FIRST_FIELD;
      int datasetExists;
      IDL_STRING datasetName;
      int matrixExists;
      IDL_STRING matrixName;
      int startyheightExists;
      IDL_LONG startyheight;
      int endyheightExists;
      IDL_LONG endyheight;
      int endxwidthExists;
      IDL_LONG endxwidth;
      int startxwidthExists;
      IDL_LONG startxwidth;
      int bandstartExists;
      IDL_LONG bandstart;
      int bandendExists;
      IDL_LONG bandend;
      int heightExists;
      IDL_VPTR height;
      int widthExists;
      IDL_VPTR width;
      int bandsExists;
      IDL_VPTR bands;
   } KW_RESULT;
   KW_RESULT kw;

   //IDL_KW_FAST_SCAN is the type of scan we are using, following it is the
   //name of the keyword, followed by the type, the mask(which should be 1),
   //flags, a boolean whether the value was populated and finally the value itself
   static IDL_KW_PAR kw_pars[] = {
      IDL_KW_FAST_SCAN,
      {"BANDS_END", IDL_TYP_LONG, 1, 0, reinterpret_cast<int*>(IDL_KW_OFFSETOF(bandendExists)),
         reinterpret_cast<char*>(IDL_KW_OFFSETOF(bandend))},
      {"BANDS_OUT", IDL_TYP_LONG, 1, IDL_KW_OUT, reinterpret_cast<int*>(IDL_KW_OFFSETOF(bandsExists)),
         reinterpret_cast<char*>(IDL_KW_OFFSETOF(bands))},
      {"BANDS_START", IDL_TYP_LONG, 1, 0, reinterpret_cast<int*>(IDL_KW_OFFSETOF(bandstartExists)),
         reinterpret_cast<char*>(IDL_KW_OFFSETOF(bandstart))},
      {"DATASET", IDL_TYP_STRING, 1, 0, reinterpret_cast<int*>(IDL_KW_OFFSETOF(datasetExists)),
         reinterpret_cast<char*>(IDL_KW_OFFSETOF(datasetName))},
      {"HEIGHT_END", IDL_TYP_LONG, 1, 0, reinterpret_cast<int*>(IDL_KW_OFFSETOF(endyheightExists)),
         reinterpret_cast<char*>(IDL_KW_OFFSETOF(endyheight))},
      {"HEIGHT_OUT", IDL_TYP_LONG, 1, IDL_KW_OUT, reinterpret_cast<int*>(IDL_KW_OFFSETOF(heightExists)),
         reinterpret_cast<char*>(IDL_KW_OFFSETOF(height))},
      {"HEIGHT_START", IDL_TYP_LONG, 1, 0, reinterpret_cast<int*>(IDL_KW_OFFSETOF(startyheightExists)),
         reinterpret_cast<char*>(IDL_KW_OFFSETOF(startyheight))},
      {"MATRIX", IDL_TYP_STRING, 1, 0, reinterpret_cast<int*>(IDL_KW_OFFSETOF(matrixExists)),
         reinterpret_cast<char*>(IDL_KW_OFFSETOF(matrixName))},
      {"WIDTH_END", IDL_TYP_LONG, 1, 0, reinterpret_cast<int*>(IDL_KW_OFFSETOF(endxwidthExists)),
         reinterpret_cast<char*>(IDL_KW_OFFSETOF(endxwidth))},
      {"WIDTH_OUT", IDL_TYP_LONG, 1, IDL_KW_OUT, reinterpret_cast<int*>(IDL_KW_OFFSETOF(widthExists)),
         reinterpret_cast<char*>(IDL_KW_OFFSETOF(width))},
      {"WIDTH_START", IDL_TYP_LONG, 1, 0, reinterpret_cast<int*>(IDL_KW_OFFSETOF(startxwidthExists))
         , reinterpret_cast<char*>(IDL_KW_OFFSETOF(startxwidth))},
      {NULL}
   };

   IDL_KWProcessByOffset(argc, pArgv, pArgk, kw_pars, 0, 1, &kw);

   std::string filename;
   std::string matrixName;
   if (kw.datasetExists)
   {
      filename = IDL_STRING_STR(&kw.datasetName);
   }
   if (kw.matrixExists)
   {
      matrixName = IDL_STRING_STR(&kw.matrixName);
   }
   RasterElement* pData = dynamic_cast<RasterElement*>(IdlFunctions::getDataset(filename));

   if (pData == NULL)
   {
      std::string msg = "Error could not find array.";
      IDL_Message(IDL_M_GENERIC, IDL_MSG_RET, msg);
      return IDL_StrToSTRING("");
   }
   unsigned char* pRawData = NULL;

   int row = 0;
   int column = 0;
   int band = 0;
   EncodingType encoding = INT1SBYTE;
   InterleaveFormatType iType = BSQ;

   int type = IDL_TYP_UNDEF;
   int dimensions = 3;
   RasterElement* pMatrix = NULL;

   if (!kw.matrixExists)
   {
      pRawData = reinterpret_cast<unsigned char*>(pData->getRawData());
      if (pRawData == NULL || kw.startyheightExists || kw.endyheightExists || kw.startxwidthExists ||
          kw.endxwidthExists || kw.bandstartExists || kw.bandendExists)
      {
         pRawData = NULL;
         // can't get rawdata pointer or subcube selected, have to copy
         if (pData != NULL)
         {
            const RasterDataDescriptor* pDesc = dynamic_cast<const RasterDataDescriptor*>(pData->getDataDescriptor());
            if (pDesc != NULL)
            {
               iType = pDesc->getInterleaveFormat();
               unsigned int heightStart = 0;
               unsigned int heightEnd = pDesc->getRowCount()-1;
               unsigned int widthStart= 0;
               unsigned int widthEnd = pDesc->getColumnCount()-1;
               unsigned int bandStart= 0;
               unsigned int bandEnd = pDesc->getBandCount()-1;
               encoding = pDesc->getDataType();
               if (kw.startyheightExists)
               {
                  heightStart = kw.startyheight;
               }
               if (kw.endyheightExists)
               {
                  heightEnd = kw.endyheight;
               }
               if (kw.startxwidthExists)
               {
                  widthStart = kw.startxwidth;
               }
               if (kw.endxwidthExists)
               {
                  widthEnd = kw.endxwidth;
               }
               if (kw.bandstartExists)
               {
                  bandStart = kw.bandstart;
               }
               if (kw.bandendExists)
               {
                  bandEnd = kw.bandend;
               }
               column = heightEnd - heightStart+1;
               row  = widthEnd - widthStart+1;
               band = bandEnd - bandStart+1;
               //copy the subcube, determine the type
               switchOnComplexEncoding(encoding, IdlFunctions::copySubcube, pRawData, pData,
                     heightStart, heightEnd, widthStart, widthEnd, bandStart, bandEnd);
            }
         }
      }
      else
      {
         RasterDataDescriptor* pDesc = dynamic_cast<RasterDataDescriptor*>(pData->getDataDescriptor());
         if (pDesc == NULL)
         {
            return IDL_StrToSTRING("failure");
         }

         iType = pDesc->getInterleaveFormat();

         row = pDesc->getRowCount();
         column = pDesc->getColumnCount();
         band = pDesc->getBandCount();
         encoding = pDesc->getDataType();
      }
   }
   else
   {
      //the user specified a ResultsMatrix to get data from
      pMatrix = dynamic_cast<RasterElement*>(Service<ModelServices>()->getElement(
         matrixName, TypeConverter::toString<RasterElement>(), pData));
      if (pMatrix == NULL)
      {
         std::string msg = "Error could not find array.";
         IDL_Message(IDL_M_GENERIC, IDL_MSG_RET, msg);
         return IDL_StrToSTRING("");
      }
      const RasterDataDescriptor* pDesc = dynamic_cast<RasterDataDescriptor*>(pMatrix->getDataDescriptor());
      if (pDesc != NULL)
      {
         iType = pDesc->getInterleaveFormat();
         row = pDesc->getRowCount();
         column = pDesc->getRowCount();
         band = pDesc->getBandCount();
         encoding = pDesc->getDataType();
         pRawData = reinterpret_cast<unsigned char*>(pMatrix->getRawData());
      }
   }

   //set the datatype based on the encoding
   switch (encoding)
   {
      case INT1SBYTE:
         type = IDL_TYP_BYTE;
         break;
      case INT1UBYTE:
         type = IDL_TYP_BYTE;
         break;
      case INT2SBYTES:
         type = IDL_TYP_INT;
         break;
      case INT2UBYTES:
         type = IDL_TYP_UINT;
         break;
      case INT4SCOMPLEX:
         type = IDL_TYP_UNDEF;
         break;
      case INT4SBYTES:
         type = IDL_TYP_LONG;
         break;
      case INT4UBYTES:
         type = IDL_TYP_ULONG;
         break;
      case FLT4BYTES:
         type = IDL_TYP_FLOAT;
         break;
      case FLT8COMPLEX:
         type = IDL_TYP_DCOMPLEX;
         break;
      case FLT8BYTES:
         type = IDL_TYP_DOUBLE;
         break;
      default:
         type = IDL_TYP_UNDEF;
         break;
   }

   if (kw.widthExists)
   {
      //we don't know the datatype passed in, so set all of them
      kw.width->value.d = 0.0;
   }
   if (kw.heightExists)
   {
      //we don't know the datatype passed in to populate, so set all of them
      kw.width->value.d = 0.0;
   }
   if (kw.bandsExists)
   {
      //we don't know the datatype passed in to populate, so set all of them
      kw.width->value.d = 0.0;
   }
   IDL_KW_FREE;
   static IDL_MEMINT dims[] = {row, column, band};
   if (band == 1)
   {
      dimensions = 2;
   }
   if (iType == BSQ)
   {
      dims[0] = row;
      dims[1] = column;
   }
   else
   {
      dims[0] = column;
      dims[1] = row;
   }
   dims[2] = band;
   return IDL_ImportArray(dimensions, dims, type, pRawData, NULL, NULL);
}

IDL_VPTR array_to_opticks(int argc, IDL_VPTR pArgv[], char* pArgk)
{
   IDL_VPTR idlPtr;
   typedef struct
   {
      IDL_KW_RESULT_FIRST_FIELD;
      int newWindowExists;
      IDL_LONG newWindow;
      int interleaveExists;
      IDL_STRING idlInterleave;
      int unitsExists;
      IDL_STRING idlUnits;
      int datasetExists;
      IDL_STRING idlDataset;
      int heightExists;
      IDL_LONG height;
      int widthExists;
      IDL_LONG width;
      int bandsExists;
      IDL_LONG bands;
      int overwriteExists;
      IDL_LONG overwrite;
      int startxwidthExists;
      IDL_LONG startxwidth;
      int bandstartExists;
      IDL_LONG bandstart;
      int startyheightExists;
      IDL_LONG startyheight;
   } KW_RESULT;
   KW_RESULT kw;

   //IDL_KW_FAST_SCAN is the type of scan we are using, following it is the
   //name of the keyword, followed by the type, the mask(which should be 1),
   //flags, a boolean whether the value was populated and finally the value itself
   static IDL_KW_PAR kw_pars[] = {
      IDL_KW_FAST_SCAN,
      {"BANDS_END", IDL_TYP_LONG, 1, 0, reinterpret_cast<int*>(IDL_KW_OFFSETOF(bandsExists)),
         reinterpret_cast<char*>(IDL_KW_OFFSETOF(bands))},
      {"BANDS_START", IDL_TYP_LONG, 1, 0, reinterpret_cast<int*>(IDL_KW_OFFSETOF(bandstartExists)),
         reinterpret_cast<char*>(IDL_KW_OFFSETOF(bandstart))},
      {"DATASET", IDL_TYP_STRING, 1, 0, reinterpret_cast<int*>(IDL_KW_OFFSETOF(datasetExists)),
         reinterpret_cast<char*>(IDL_KW_OFFSETOF(idlDataset))},
      {"HEIGHT_END", IDL_TYP_LONG, 1, 0, reinterpret_cast<int*>(IDL_KW_OFFSETOF(heightExists)),
         reinterpret_cast<char*>(IDL_KW_OFFSETOF(height))},
      {"HEIGHT_START", IDL_TYP_LONG, 1, 0, reinterpret_cast<int*>(IDL_KW_OFFSETOF(startyheightExists)),
         reinterpret_cast<char*>(IDL_KW_OFFSETOF(startyheight))},
      {"INTERLEAVE", IDL_TYP_STRING, 1, 0, reinterpret_cast<int*>(IDL_KW_OFFSETOF(interleaveExists)),
         reinterpret_cast<char*>(IDL_KW_OFFSETOF(idlInterleave))},
      {"NEW_WINDOW", IDL_TYP_LONG, 1, IDL_KW_ZERO, reinterpret_cast<int*>(IDL_KW_OFFSETOF(newWindowExists)),
         reinterpret_cast<char*>(IDL_KW_OFFSETOF(newWindow))},
      {"OVERWRITE", IDL_TYP_LONG, 1, IDL_KW_ZERO, reinterpret_cast<int*>(IDL_KW_OFFSETOF(overwriteExists)),
         reinterpret_cast<char*>(IDL_KW_OFFSETOF(overwrite))},
      {"UNITS", IDL_TYP_STRING, 1, 0, reinterpret_cast<int*>(IDL_KW_OFFSETOF(unitsExists)),
         reinterpret_cast<char*>(IDL_KW_OFFSETOF(idlUnits))},
      {"WIDTH_END", IDL_TYP_LONG, 1, 0, reinterpret_cast<int*>(IDL_KW_OFFSETOF(widthExists)),
         reinterpret_cast<char*>(IDL_KW_OFFSETOF(width))},
      {"WIDTH_START", IDL_TYP_LONG, 1, 0, reinterpret_cast<int*>(IDL_KW_OFFSETOF(startxwidthExists)),
         reinterpret_cast<char*>(IDL_KW_OFFSETOF(startxwidth))},
      {NULL}
   };

   IDL_KWProcessByOffset(argc, pArgv, pArgk, kw_pars, 0, 1, &kw);

   IDL_MEMINT total;
   std::string unitName;
   unsigned int height = 0;
   unsigned int width = 0;
   unsigned int bands = 1;
   char* pRawData = NULL;
   std::string newDataName;
   std::string datasetName;
   int newWindow = 0;
   int overwrite = 0;
   if (argc < 2)
   {
      IDL_Message(IDL_M_GENERIC, IDL_MSG_RET, "ARRAY_TO_OPTICKS was not passed needed parameters.  It takes a 1 "
         "dimenstional array of data along with a name to represent the data, it has HEIGHT_START, HEIGHT_END, "
         "WIDTH_START, WIDTH_END, and BANDS_START, BANDS_END keywords "
         "to describe the dimensions.  A DATASET keyword to specify a dataset to associate the new data with "
         "an existing window, needed if not using the NEW_WINDOW keyword.  There also exists an INTERLEAVE keyword "
         "which defaults to BSQ, but allows BIP and BIL.  The last keyword is UNITS, which should hold "
         "a std::string you wish to label the data values with.");
      IDL_KW_FREE;
      return IDL_StrToSTRING("failure");
   }
   else
   {
      IDL_VarGetData(pArgv[0], &total, &pRawData, 0);
      newDataName = IDL_VarGetString(pArgv[1]);
   }

   bool bSuccess = false;
   EncodingType encoding;
   int type = pArgv[0]->type;
   if (kw.unitsExists)
   {
      unitName = IDL_STRING_STR(&kw.idlUnits);
   }
   if (kw.datasetExists)
   {
      datasetName = IDL_STRING_STR(&kw.idlDataset);
   }
   if (kw.heightExists)
   {
      height = kw.height;
   }
   if (kw.widthExists)
   {
      width = kw.width;
   }
   if (kw.bandsExists)
   {
      bands = kw.bands;
   }
   if (kw.newWindowExists)
   {
      if (kw.newWindow != 0)
      {
         newWindow = 1;
      }
   }
   //handle the ability to override the default interleave type
   InterleaveFormatType iType = BSQ;
   if (kw.interleaveExists)
   {
      iType = StringUtilities::fromXmlString<InterleaveFormatType>(IDL_STRING_STR(&kw.idlInterleave));
   }
   if (total != height*width*bands)
   {
      IDL_KW_FREE;
      IDL_Message(IDL_M_GENERIC, IDL_MSG_RET, "ARRAY_TO_OPTICKS error.  Passed in array size does not match size keywords.");
      return IDL_StrToSTRING("failure");
   }
   if (kw.overwriteExists)
   {
      if (kw.overwrite != 0)
      {
         overwrite = 1;
      }
   }

   //add the data as a new results matrix and view to the current dataset and window
   switch (type)
   {
      case IDL_TYP_BYTE :
         encoding = INT1SBYTE;
         if (!newWindow && !overwrite)
         {
            IdlFunctions::addMatrixToCurrentView(reinterpret_cast<char*>(pRawData), newDataName, width,
               height, bands, unitName, encoding, iType, datasetName);
            bSuccess = true;
         }
         break;
      case IDL_TYP_INT :
         encoding = INT2SBYTES;
         if (!newWindow && !overwrite)
         {
            IdlFunctions::addMatrixToCurrentView(reinterpret_cast<short*>(pRawData), newDataName, width,
               height, bands, unitName, encoding, iType, datasetName);
            bSuccess = true;
         }
         break;
      case IDL_TYP_UINT :
         encoding = INT2UBYTES;
         if (!newWindow && !overwrite)
         {
            IdlFunctions::addMatrixToCurrentView(reinterpret_cast<unsigned short*>(pRawData), newDataName,
               width, height, bands, unitName, encoding, iType, datasetName);
            bSuccess = true;
         }
         break;
      case IDL_TYP_LONG :
         encoding = INT4SBYTES;
         if (!newWindow && !overwrite)
         {
            IdlFunctions::addMatrixToCurrentView(reinterpret_cast<int*>(pRawData), newDataName, width,
               height, bands, unitName, encoding, iType, datasetName);
            bSuccess = true;
         }
         break;
      case IDL_TYP_ULONG :
         encoding = INT4UBYTES;
         if (!newWindow && !overwrite)
         {
            IdlFunctions::addMatrixToCurrentView(reinterpret_cast<unsigned int*>(pRawData), newDataName,
               width, height, bands, unitName, encoding, iType, datasetName);
            bSuccess = true;
         }
         break;
      case IDL_TYP_FLOAT :
         encoding = FLT4BYTES;
         if (!newWindow && !overwrite)
         {
            IdlFunctions::addMatrixToCurrentView(reinterpret_cast<float*>(pRawData), newDataName,
               width, height, bands, unitName, encoding, iType, datasetName);
            bSuccess = true;
         }
         break;
      case IDL_TYP_DOUBLE :
         encoding = FLT8BYTES;
         if (!newWindow && !overwrite)
         {
            IdlFunctions::addMatrixToCurrentView(reinterpret_cast<double*>(pRawData), newDataName,
               width, height, bands, unitName, encoding, iType, datasetName);
            bSuccess = true;
         }
         break;
      case IDL_TYP_DCOMPLEX:
         encoding = FLT8COMPLEX;
         if (!newWindow && !overwrite)
         {
            IdlFunctions::addMatrixToCurrentView(reinterpret_cast<FloatComplex*>(pRawData), newDataName,
               width, height, bands, unitName, encoding, iType, datasetName);
            bSuccess = true;
         }
         break;
      case IDL_TYP_UNDEF :
         encoding = UNKNOWN;
         IDL_Message(IDL_M_GENERIC, IDL_MSG_RET, "unable to determine type.");
         break;
   }
   if ((kw.newWindowExists && kw.newWindow != 0) ||
       (kw.overwriteExists && kw.overwrite != 0))
   {
      if (kw.newWindowExists && kw.newWindow != 0)
      {
         //user wants to create a new RasterElement and window
         RasterElement* pRaster = IdlFunctions::createRasterElement(pRawData, datasetName,
            newDataName, encoding, iType, height, width, bands);
         if (pRaster != NULL)
         {
            //----- Now create the spatial data window to display the data
            SpatialDataWindow* pWindow = static_cast<SpatialDataWindow*>(
               Service<DesktopServices>()->createWindow(newDataName, SPATIAL_DATA_WINDOW));

            if (pWindow != NULL)
            {
               SpatialDataView* pView = pWindow->getSpatialDataView();

               if (pView != NULL)
               {
                  UndoLock undo(pView);
                  //----- Set the spatial data in the view
                  pView->setPrimaryRasterElement(pRaster);
                  //----- Create the cube layer
                  RasterLayer* pLayer = static_cast<RasterLayer*>(pView->createLayer(RASTER, pRaster));
                  bSuccess = true;
               }
            }
         }
      }
      else
      {
         //the user wants to replace the spectral cube of the RasterElement with all new data
         RasterElement* pParent = dynamic_cast<RasterElement*>(IdlFunctions::getDataset(datasetName));
         RasterElement* pRaster = static_cast<RasterElement*>(Service<ModelServices>()->getElement(newDataName,
            TypeConverter::toString<RasterElement>(), pParent));
         if (pRaster == NULL)
         {
            pRaster = pParent;
         }
         if (pRaster != NULL)
         {
            RasterDataDescriptor* pDesc = dynamic_cast<RasterDataDescriptor*>(pRaster->getDataDescriptor());
            if (pDesc != NULL)
            {
               unsigned int heightStart = 0;
               unsigned int widthStart= 0;
               unsigned int bandStart= 0;
               if (kw.startyheightExists)
               {
                  heightStart = kw.startyheight;
               }
               if (kw.startxwidthExists)
               {
                  widthStart = kw.startxwidth;
               }
               if (kw.bandstartExists)
               {
                  bandStart = kw.bandstart;
               }
               EncodingType oldType = pDesc->getDataType();
               if (oldType != encoding)
               {
                  IDL_KW_FREE;
                  IDL_Message(IDL_M_GENERIC, IDL_MSG_RET, "ARRAY_TO_OPTICKS error.  data type of new array is not the same as the old.");
                  return IDL_StrToSTRING("failure");
               }
               IdlFunctions::changeRasterElement(pRaster, pRawData, encoding, iType, heightStart, height, widthStart,
                  width, bandStart, bands, oldType);
               bSuccess = true;
            }
         }
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

IDL_VPTR change_data_type(int argc, IDL_VPTR pArgv[], char* pArgk)
{
   IDL_VPTR idlPtr;
   //this section of code is for handling of keywords passed from IDL
   //KW_RESULTS is what gets populated by the processing function
   //IDL_KW_PAR is what tells the processing function how to populate
   //KW_RESULTS
   typedef struct
   {
      IDL_KW_RESULT_FIRST_FIELD;
      int copyExists;
      IDL_STRING copyElements[64];
      int datasetExists;
      IDL_STRING idlDataset;
      int convertExists;
      IDL_STRING convertElements[64];
      IDL_MEMINT nCopyElements;
      IDL_MEMINT nConvertElements;
      int windowLabelExists;
      IDL_STRING windowLabel;
      int windowPosXExists;
      IDL_LONG windowPosX;
      int windowPosYExists;
      IDL_LONG windowPosY;
      int newNameExists;
      IDL_STRING newName;
   } KW_RESULT;
   KW_RESULT kw;

   //IDL_KW_FAST_SCAN is the type of scan we are using, following it is the
   //name of the keyword, followed by the type, the mask(which should be 1),
   //flags, a boolean whether the value was populated and finally the value itself
#ifdef _WIN64
   static IDL_KW_ARR_DESC_R copyDesc = {
      reinterpret_cast<char*>(IDL_KW_OFFSETOF(copyElements)), 0, 64,
         reinterpret_cast<IDL_LONG64*>(IDL_KW_OFFSETOF(nCopyElements))
   };
   static IDL_KW_ARR_DESC_R convertDesc = {
      reinterpret_cast<char*>(IDL_KW_OFFSETOF(convertElements)), 0, 64,
         reinterpret_cast<IDL_LONG64*>(IDL_KW_OFFSETOF(nConvertElements))
   };
#else
   static IDL_KW_ARR_DESC_R copyDesc = {
      reinterpret_cast<char*>(IDL_KW_OFFSETOF(copyElements)), 0, 64,
         reinterpret_cast<IDL_LONG*>(IDL_KW_OFFSETOF(nCopyElements))
   };
   static IDL_KW_ARR_DESC_R convertDesc = {
      reinterpret_cast<char*>(IDL_KW_OFFSETOF(convertElements)), 0, 64,
         reinterpret_cast<IDL_LONG*>(IDL_KW_OFFSETOF(nConvertElements))
   };
#endif
   static IDL_KW_PAR kw_pars[] = {
      IDL_KW_FAST_SCAN,
      {"CONVERT_ELEMENTS", IDL_TYP_STRING, 1, 0, reinterpret_cast<int*>(IDL_KW_OFFSETOF(convertExists)),
         reinterpret_cast<char*>(&convertDesc)},
      {"COPY_ELEMENTS", IDL_TYP_STRING, 1, IDL_KW_ARRAY, reinterpret_cast<int*>(IDL_KW_OFFSETOF(copyExists)),
         reinterpret_cast<char*>(&copyDesc)},
      {"DATASET", IDL_TYP_STRING, 1, 0, reinterpret_cast<int*>(IDL_KW_OFFSETOF(datasetExists)),
         reinterpret_cast<char*>(IDL_KW_OFFSETOF(idlDataset))},
      {"NEW_NAME", IDL_TYP_STRING, 1, 0, reinterpret_cast<int*>(IDL_KW_OFFSETOF(newNameExists)),
         reinterpret_cast<char*>(IDL_KW_OFFSETOF(newName))},
      {"WINDOW_LABEL", IDL_TYP_STRING, 1, 0, reinterpret_cast<int*>(IDL_KW_OFFSETOF(windowLabelExists)),
         reinterpret_cast<char*>(IDL_KW_OFFSETOF(windowLabel))},
      {"WIN_POS_X", IDL_TYP_LONG, 1, 0/**IDL_KW_VIN*/, reinterpret_cast<int*>(IDL_KW_OFFSETOF(windowPosXExists)),
         reinterpret_cast<char*>(IDL_KW_OFFSETOF(windowPosX))},
      {"WIN_POS_Y", IDL_TYP_LONG, 1, 0/**IDL_KW_VIN*/, reinterpret_cast<int*>(IDL_KW_OFFSETOF(windowPosYExists)),
         reinterpret_cast<char*>(IDL_KW_OFFSETOF(windowPosY))},
      {NULL}
   };

   IDL_KWProcessByOffset(argc, pArgv, pArgk, kw_pars, 0, 1, &kw);

   std::string newDataTypeName;
   std::string datasetName;
   std::string newElementName;
   bool bSuccess = false;
   bool bCopyAll = false;
   bool bConvertAll = false;
   bool bCopyDataset = false;
   bool bConvertDataset = true;
   if (argc < 1)
   {
      IDL_Message(IDL_M_GENERIC, IDL_MSG_RET, "change_data_type was not passed needed parameters.  It takes a "
         "a std::string specifying the datatype you wish to change the dataset to, such as 'double' or "
         "'unsigned short'.  It also can take a dataset keyword.");
      IDL_KW_FREE;
      return IDL_StrToSTRING("failure");
   }
   else
   {
      //datatype std::string
      newDataTypeName = IDL_VarGetString(pArgv[0]);
   }

   EncodingType encoding;
   std::string rasterElement;
   std::vector<DataElement*> elementsToCopy;
   std::vector<DataElement*> elementsToConvert;

   if (kw.datasetExists)
   {
      datasetName = IDL_STRING_STR(&kw.idlDataset);
   }
   //get the primary dataset
   RasterElement* pSensor = dynamic_cast<RasterElement*>(IdlFunctions::getDataset(datasetName));
   RasterElement* pElement = NULL;
   if (pSensor == NULL)
   {
      //the dataset doesn't exist, this function won't work
      IDL_KW_FREE;
      if (IdlFunctions::getDataset(datasetName) == NULL)
      {
         IDL_Message(IDL_M_GENERIC, IDL_MSG_RET, "change_data_type critical error.  could not find dataset.");
      }
      else
      {
         IDL_Message(IDL_M_GENERIC, IDL_MSG_RET, "change_data_type critical error.  Function does not work with an on disk sensor data.");
      }
      return IDL_StrToSTRING("failure");
   }

   if (kw.convertExists)
   {
      bConvertDataset = false;
      for (int i = 0; i < kw.nConvertElements; ++i)
      {
         std::string convertName = IDL_STRING_STR(&kw.convertElements[i]);
         if (convertName == "all")
         {
            //if we are to convert everything, then make the list equal to
            //all of the raster elements
            bConvertAll = true;
            elementsToConvert = Service<ModelServices>()->getElements(pSensor, TypeConverter::toString<RasterElement>());
            break;
         }
         else if (convertName == "dataset")
         {
            bConvertDataset = true;
         }
         else
         {
            DataElement* pCurrentElement = Service<ModelServices>()->getElement(convertName, "", pSensor);
            elementsToConvert.push_back(pCurrentElement);
         }
      }
   }
   if (kw.copyExists)
   {
      bCopyDataset = false;
      for (int i = 0; i < kw.nCopyElements; ++i)
      {
         std::string copyName = IDL_STRING_STR(&kw.copyElements[i]);
         if (copyName == "all")
         {
            bCopyAll = true;
            elementsToCopy = Service<ModelServices>()->getElements(pSensor, "");
         }
         else if (copyName == "dataset")
         {
            if (!bConvertDataset)
            {
               bCopyDataset = true;
            }
         }
         else
         {
            DataElement* pCurrentElement = Service<ModelServices>()->getElement(copyName, "", pSensor);
            elementsToCopy.push_back(pCurrentElement);
         }
      }
   }
   //get the new name, if not specified, then its 'oldName_datatype'
   if (kw.newNameExists)
   {
      newElementName = IDL_STRING_STR(&kw.newName);
   }
   else
   {
      newElementName = pSensor->getName() + "_" + newDataTypeName;
   }


   //get the encoding from the data type std::string passed in
   encoding = StringUtilities::fromXmlString<EncodingType>(newDataTypeName);
   if (!encoding.isValid())
   {
      IDL_Message(IDL_M_GENERIC, IDL_MSG_RET, "unable to determine type.");
   }
   //handle the windows
   SpatialDataView* pView = NULL;
   SpatialDataView* pOriginalView = NULL;
   RasterElement* pNewSensor = NULL;
   RasterElement* pNewElement = NULL;

   std::vector<Window*> windows;
   Service<DesktopServices>()->getWindows(SPATIAL_DATA_WINDOW, windows);

   if (!windows.empty())
   {
      //get the window containing the dataset we are copying from
      for (std::vector<Window*>::iterator myIt = windows.begin(); myIt != windows.end(); ++myIt)
      {
         SpatialDataWindow* pSpatial = static_cast<SpatialDataWindow*>(*myIt);
         SpatialDataView* pTmpView = pSpatial->getSpatialDataView();
         LayerList* pList = pTmpView->getLayerList();
         if (pList->getLayer(RASTER, pSensor) != NULL)
         {
            pOriginalView = pTmpView;
            break;
         }
      }
   }
   //create the new window
   SpatialDataWindow* pWindow = static_cast<SpatialDataWindow*>(
      Service<DesktopServices>()->createWindow(newElementName, SPATIAL_DATA_WINDOW));
   if (pWindow != NULL)
   {
      QWidget* pTmpWidget = pWindow->getWidget();
      if (pTmpWidget != NULL)
      {
         QWidget* pWidget = dynamic_cast<QWidget*>(pTmpWidget->parent());
         if (pWidget != NULL)
         {
            //set the window's position and label if necessary
            if (kw.windowLabelExists)
            {
               pWidget->setWindowTitle(QString(IDL_STRING_STR(&kw.windowLabel)));
            }
            if (kw.windowPosXExists && kw.windowPosYExists)
            {
               QWidget* pMain = dynamic_cast<QWidget*>(pWidget->parent());
               if (pMain != NULL)
               {
                  pMain->move(kw.windowPosX, kw.windowPosY);
               }
            }
         }
      }
      pView = pWindow->getSpatialDataView();
   }

   if (pView != NULL)
   {
      //first handle if the primary raster element is to be copied or converted
      if (bCopyDataset || bConvertDataset)
      {
         if (bCopyDataset)
         {
            pNewSensor = static_cast<RasterElement*>(pSensor->copy(newElementName, NULL));
         }
         else
         {
            RasterDataDescriptor* pDesc = dynamic_cast<RasterDataDescriptor*>(pSensor->getDataDescriptor());
            if (pDesc != NULL)
            {
               //get the spectral cube information so we can pass it to changeRasterElement, with only the type changing
               EncodingType oldType = pDesc->getDataType();

               unsigned int ulNumColumns = pDesc->getColumnCount();
               unsigned int ulNumRows = pDesc->getRowCount();
               unsigned int ulNumBands = pDesc->getBandCount();;
               void* pData = NULL;
               //create the raster element with empty data
               InterleaveFormatType iType = pDesc->getInterleaveFormat();
               pNewSensor = IdlFunctions::createRasterElement(pData, pSensor->getName(), newElementName,
                  encoding, iType, ulNumRows, ulNumColumns, ulNumBands);
               //copy the data from the original into the new
               IdlFunctions::copyRasterElement(pSensor, pNewSensor, encoding, iType, 0, ulNumRows,
                  0, ulNumColumns, 0, ulNumBands, oldType);
               DataDescriptor* pNewDesc = pSensor->getDataDescriptor();
               if (pNewDesc != NULL)
               {
                  pNewDesc->setClassification(pDesc->getClassification());
               }
            }
         }
         if (pNewSensor != NULL)
         {
            UndoLock undo(pView);
            bSuccess = true;
            RasterLayer* pLayer = static_cast<RasterLayer*>(pView->createLayer(RASTER, pNewSensor));
            if (pLayer != NULL)
            {
               if (pOriginalView != NULL)
               {
                  //copy any existing movie
                  std::string classText;
                  // Do we really need this? It should be set from the RasterElement's classification - Trevor
                  /*pOriginalView->getClassificationText(classText);
                  pView->setClassificationText(classText);*/
                  pView->setAnimationController(pOriginalView->getAnimationController());
                  LayerList* pList = pOriginalView->getLayerList();
                  if (pList != NULL)
                  {
                     RasterLayer* pOrigLayer = dynamic_cast<RasterLayer*>(pList->getLayer(RASTER, pSensor));
                     if (pOrigLayer != NULL)
                     {
                        IdlFunctions::copyLayer(pLayer, pOrigLayer);
                     }
                  }
               }
            }
            pView->setPrimaryRasterElement(pNewSensor);
         }
      }
      //convert any RasterElements that should be converted
      const RasterElement* pTerrainElement = pSensor->getTerrain();
      for (std::vector<DataElement*>::iterator convertIt = elementsToConvert.begin();
         convertIt != elementsToConvert.end(); ++convertIt)
      {
         RasterElement* pCurrentElement = dynamic_cast<RasterElement*>(*convertIt);
         //get the data element based on the name and parent
         if (pCurrentElement != NULL)
         {
            RasterDataDescriptor* pDesc = static_cast<RasterDataDescriptor*>(
               pCurrentElement->getDataDescriptor());
            if (pDesc != NULL)
            {
               //get the spectral cube information so we can pass it to changeRasterElement, with only the type changing
               EncodingType oldType = pDesc->getDataType();

               unsigned int ulNumColumns = pDesc->getColumnCount();
               unsigned int ulNumRows = pDesc->getRowCount();
               unsigned int ulNumBands = pDesc->getBandCount();;
               InterleaveFormatType iType = pDesc->getInterleaveFormat();

               //create the element and populate its data
               pNewElement = RasterUtilities::createRasterElement(newElementName, ulNumRows, ulNumColumns,
                  ulNumBands, encoding, iType, true, pNewSensor);
               IdlFunctions::copyRasterElement(pCurrentElement, pNewElement, encoding, iType, 0, ulNumRows,
                  0, ulNumColumns, 0, ulNumBands, oldType);

               if (pNewElement != NULL)
               {
                  DataDescriptor* pNewDesc = pNewElement->getDataDescriptor();
                  if (pNewDesc != NULL)
                  {
                     pNewDesc->setClassification(pDesc->getClassification());
                  }
               }
               bSuccess = true;
               if (pOriginalView != NULL)
               {
                  LayerList* pList = pOriginalView->getLayerList();
                  if (pList != NULL)
                  {
                     RasterLayer* pOrigLayer = dynamic_cast<RasterLayer*>(pList->getLayer(RASTER, pCurrentElement));
                     if (pOrigLayer != NULL)
                     {
                        UndoLock undo(pView);
                        RasterLayer* pLayer = dynamic_cast<RasterLayer*>(pView->createLayer(RASTER, pNewElement));
                        if (pLayer != NULL)
                        {
                           IdlFunctions::copyLayer(pLayer, pOrigLayer);
                        }
                     }
                  }
               }
               if (pCurrentElement == pTerrainElement && pNewSensor != NULL)
               {
                  //if they copied the terrain's element, set the terrain for the new rasterelement
                  pNewSensor->setTerrain(dynamic_cast<RasterElement*>(pNewElement));
               }
            }
         }
      }
      //we created a new window, so now we need to populate it with anything in the copyList
      //get the terrain information, so we can set it if it should be copied
      //traverse the list of elements to copy, copying each one and creating a layer
      for (std::vector<DataElement*>::iterator myIt = elementsToCopy.begin(); myIt != elementsToCopy.end(); ++myIt)
      {
         DataElement* pCurrentElement = *myIt;
         DataElement* pCopy = NULL;
         //get the dataelement based on the name and parent
         if (pCurrentElement != NULL)
         {
            bool bCopied = false;
            if (pOriginalView != NULL)
            {
               LayerList* pList = pOriginalView->getLayerList();
               if (pList != NULL)
               {
                  //see if the data element has a layer associated with it
                  //if it does, copy the layer, and add it to the new view
                  std::vector<Layer*> layers;
                  pList->getLayers(layers);
                  for (std::vector<Layer*>::iterator myItL = layers.begin(); myItL != layers.end(); ++myItL)
                  {
                     Layer* pCurrentLayer = *myItL;
                     if (pCurrentLayer != NULL)
                     {
                        if (pCurrentElement == pCurrentLayer->getDataElement())
                        {
                           std::string newElementName = pCurrentElement->getName();
                           pCopy = pCurrentElement->copy(newElementName, pNewSensor);
                           if (pCopy != NULL)
                           {
                              UndoLock undo(pView);
                              Layer* pNewLayer = pView->createLayer(pCurrentLayer->getLayerType(), pCopy);
                              if (pNewLayer != NULL)
                              {
                                 pView->addLayer(pNewLayer);
                                 pCopy = dynamic_cast<DataElement*>(pNewLayer->getDataElement());
                                 bCopied = true;
                              }
                           }
                        }
                     }
                  }
               }
            }
            if (!bCopied)
            {
               //the layer did not exist, just copy the element
               pCopy = pCurrentElement->copy(pCurrentElement->getName(), pNewSensor);
            }
            if (pCurrentElement == pTerrainElement && pNewSensor != NULL)
            {
               //if they copied the terrain's element, set the terrain for the new rasterelement
               if (pNewSensor->getTerrain() == NULL)
               {
                  pNewSensor->setTerrain(static_cast<RasterElement*>(pCopy));
               }
            }
         }
      }
#pragma message(__FILE__ "(" STRING(__LINE__) ") : warning : there's no way to copy the georeferencing plug-in, add it when available (tclarke)")
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

void report_progress(int argc, IDL_VPTR pArgv[], char* pArgk)
{
   if (spProgress == NULL)
   {
      return;
   }

   typedef struct
   {
      IDL_KW_RESULT_FIRST_FIELD;
      int messageExists;
      IDL_STRING message;
      int percentExists;
      IDL_LONG percent;
      int reportingLevelExists;
      IDL_STRING reportingLevel;
   } KW_RESULT;
   KW_RESULT kw;

   //IDL_KW_FAST_SCAN is the type of scan we are using, following it is the
   //name of the keyword, followed by the type, the mask(which should be 1),
   //flags, a boolean whether the value was populated and finally the value itself
   static IDL_KW_PAR kw_pars[] = {
      IDL_KW_FAST_SCAN,
      {"MESSAGE", IDL_TYP_STRING, 1, 0, reinterpret_cast<int*>(IDL_KW_OFFSETOF(messageExists)),
         reinterpret_cast<char*>(IDL_KW_OFFSETOF(message))},
      {"PERCENT", IDL_TYP_LONG, 1, 0, reinterpret_cast<int*>(IDL_KW_OFFSETOF(percentExists)),
         reinterpret_cast<char*>(IDL_KW_OFFSETOF(percent))},
      {"REPORTING_LEVEL", IDL_TYP_STRING, 1, 0, reinterpret_cast<int*>(IDL_KW_OFFSETOF(reportingLevelExists)),
         reinterpret_cast<char*>(IDL_KW_OFFSETOF(reportingLevel))},
      {NULL}
   };

   IDL_KWProcessByOffset(argc, pArgv, pArgk, kw_pars, 0, 1, &kw);

   //get the message
   std::string message = "Error.";
   if (kw.messageExists)
   {
      message = IDL_STRING_STR(&kw.message);
   }

   //get the percentage complete
   int percent = 0;
   if (kw.percentExists)
   {
      percent = kw.percent;
   }

   //get the level of reporting granularity
   std::string reportingLevelText = "WARNING";
   if (kw.reportingLevelExists)
   {
      reportingLevelText = IDL_STRING_STR(&kw.reportingLevel);
   }
   ReportingLevel reportingLevel;
   if (reportingLevelText == "NORMAL")
   {
      reportingLevel = NORMAL;
   }
   else if (reportingLevelText == "WARNING")
   {
      reportingLevel = WARNING;
   }
   else if (reportingLevelText == "ABORT")
   {
      reportingLevel = ABORT;
   }
   else if (reportingLevelText == "ERRORS")
   {
      reportingLevel = ERRORS;
   }

   //display progress
   spProgress->updateProgress(message, percent, reportingLevel);
}

IDL_VPTR create_animation(int argc, IDL_VPTR pArgv[], char* pArgk)
{
   typedef struct
   {
      IDL_KW_RESULT_FIRST_FIELD;
      int animationControllerNameExists;
      IDL_STRING animationControllerName;
      int datasetExists;
      IDL_STRING idlDataset;
      int copydatasetExists;
      IDL_STRING copyDataset;
      int framesExists;
      IDL_VPTR frames;
      int timesExists;
      IDL_VPTR times;
   } KW_RESULT;
   KW_RESULT kw;

   //IDL_KW_FAST_SCAN is the type of scan we are using, following it is the
   //name of the keyword, followed by the type, the mask(which should be 1),
   //flags, a boolean whether the value was populated and finally the value itself
   static IDL_KW_PAR kw_pars[] = {
      IDL_KW_FAST_SCAN,
      {"CONTROLLER_NAME", IDL_TYP_STRING, 1, 0, reinterpret_cast<int*>(IDL_KW_OFFSETOF(animationControllerNameExists)),
         reinterpret_cast<char*>(IDL_KW_OFFSETOF(animationControllerName))},
      {"COPY_DATASET", IDL_TYP_STRING, 1, 0, reinterpret_cast<int*>(IDL_KW_OFFSETOF(copydatasetExists)),
         reinterpret_cast<char*>(IDL_KW_OFFSETOF(copyDataset))},
      {"DATASET", IDL_TYP_STRING, 1, 0, reinterpret_cast<int*>(IDL_KW_OFFSETOF(datasetExists)),
         reinterpret_cast<char*>(IDL_KW_OFFSETOF(idlDataset))},
      {"FRAMES", IDL_TYP_UNDEF, 1, IDL_KW_VIN, reinterpret_cast<int*>(IDL_KW_OFFSETOF(framesExists)),
         reinterpret_cast<char*>(IDL_KW_OFFSETOF(frames))},
      {"TIMES", IDL_TYP_UNDEF, 1, IDL_KW_VIN, reinterpret_cast<int*>(IDL_KW_OFFSETOF(timesExists)),
         reinterpret_cast<char*>(IDL_KW_OFFSETOF(times))},
      {NULL}
   };

   IDL_KWProcessByOffset(argc, pArgv, pArgk, kw_pars, 0, 1, &kw);

   std::string copyDataName;
   std::string datasetName;
   static int animationIndex = 1;

   double* pTimes = NULL;
   int nTimes = 0;
   IDL_VPTR tmpTimes;
   IDL_VPTR tmpFrames;
   bool bTimesConverted = false;
   bool bFramesConverted = false;
   if (kw.timesExists)
   {
      if ((kw.times->flags & ~IDL_V_ARR))
      {
         IDL_KW_FREE;
         IDL_Message(IDL_M_GENERIC, IDL_MSG_RET, "CREATE_ANIMATION critical error.  Times keyword must be an array.");
         return IDL_StrToSTRING("failure");
      }
      else
      {
         if (kw.times->type != IDL_TYP_DOUBLE)
         {
            IDL_VPTR tmp1 = kw.times;
            tmpTimes = IDL_CvtDbl(1, &tmp1);
            kw.times = tmpTimes;
            bTimesConverted = true;
         }
         UCHAR* pTmpValues = kw.times->value.arr->data;
         pTimes = reinterpret_cast<double*>(pTmpValues);
         IDL_MEMINT tmpSize = kw.times->value.arr->n_elts;
         nTimes = static_cast<int>(tmpSize);
      }
   }
   short* pFrames = NULL;
   int nFrames = 0;
   if (kw.framesExists)
   {
      if ((kw.frames->flags & ~IDL_V_ARR))
      {
         IDL_KW_FREE;
         IDL_Message(IDL_M_GENERIC, IDL_MSG_RET, "CREATE_ANIMATION critical error.  Times keyword must be an array.");
         return IDL_StrToSTRING("failure");
      }
      else
      {
         if (kw.frames->type != IDL_TYP_INT)
         {
            IDL_VPTR tmp1 = kw.frames;
            tmpFrames = IDL_CvtFix(1, &tmp1);
            kw.frames = tmpFrames;
            bFramesConverted = true;
         }
         UCHAR* pTmpValues = kw.frames->value.arr->data;
         pFrames = reinterpret_cast<short*>(pTmpValues);
         IDL_MEMINT tmpSize = kw.frames->value.arr->n_elts;
         nFrames = static_cast<int>(tmpSize);
      }
   }

   std::string filename;
   if (kw.datasetExists)
   {
      filename = IDL_STRING_STR(&kw.idlDataset);
   }
   std::string copyfilename;
   if (kw.copydatasetExists)
   {
      copyfilename = IDL_STRING_STR(&kw.copyDataset);
   }
   std::string controllerName;
   if (kw.animationControllerNameExists)
   {
      controllerName = IDL_STRING_STR(&kw.animationControllerName);
   }
   else
   {
      QString cName = QString("Animation Controller %1").arg(animationIndex);
      controllerName = cName.toStdString();
   }

   RasterElement* pData = dynamic_cast<RasterElement*>(IdlFunctions::getDataset(filename));

   if (pData == NULL)
   {
      IDL_KW_FREE;
      IDL_Message(IDL_M_GENERIC, IDL_MSG_RET, "CREATE_ANIMATION critical error.  No dataset.");
      return IDL_StrToSTRING("failure");
   }

   RasterElement* pCopyData = NULL;
   RasterLayer* pCopyLayer = NULL;
   Animation* pCopyAnimation = NULL;
   AnimationController* pCopyController = NULL;
   RasterDataDescriptor* pCopyDescriptor = NULL;

   if (!copyfilename.empty())
   {
      //a copy_dataset keyword was used, pull out the controller and the animation from that dataset
      pCopyData = dynamic_cast<RasterElement*>(IdlFunctions::getDataset(copyfilename));
      pCopyLayer = dynamic_cast<RasterLayer*>(IdlFunctions::getLayerByRaster(pCopyData));
      if (pCopyLayer != NULL)
      {
         pCopyAnimation = pCopyLayer->getAnimation();
         SpatialDataView* pCopyView = dynamic_cast<SpatialDataView*>(pCopyLayer->getView());
         if (pCopyView != NULL)
         {
            pCopyController = pCopyView->getAnimationController();
         }
      }
      if (pCopyData != NULL)
      {
         pCopyDescriptor = dynamic_cast<RasterDataDescriptor*>
            (pCopyData->getDataDescriptor());
      }
   }

   //get the descriptor so we know how many bands are in the dataset
   RasterDataDescriptor* pDescriptor = dynamic_cast<RasterDataDescriptor*>(pData->getDataDescriptor());
   if (pDescriptor == NULL)
   {
      IDL_KW_FREE;
      IDL_Message(IDL_M_GENERIC, IDL_MSG_RET, "CREATE_ANIMATION critical error.  No Descriptor.");
      return IDL_StrToSTRING("failure");
   }

   // build animation frame list
   std::vector<AnimationFrame> animationFrames;

   unsigned int animationFrame = 0;
   //copy the band info from the input dataset into the dataset whose animation we are creating
   std::vector<DimensionDescriptor> bands = pDescriptor->getBands();
   std::vector<DimensionDescriptor> newbands;

   bool bTimeSet = false;
   //copy the times into the frames
   for (std::vector<DimensionDescriptor>::iterator iter = bands.begin(); iter != bands.end(); ++iter)
   {
      double time = 0.0;
      int frame = 0;
      //if it is set to copy another element, then get its information and put it in the animation
      if (pCopyAnimation != NULL)
      {
         const std::vector<AnimationFrame>& frames = pCopyAnimation->getFrames();
         if (frames.size() > animationFrame)
         {
            time = frames[animationFrame].mTime;
            bTimeSet = true;
         }
         if (pCopyDescriptor != NULL)
         {
            std::vector<DimensionDescriptor> copyBands = pCopyDescriptor->getBands();

            if (copyBands.size() > animationFrame)
            {
               DimensionDescriptor current = *iter;
               current.setActiveNumber(copyBands[animationFrame].getActiveNumber());
               current.setOnDiskNumber(copyBands[animationFrame].getOnDiskNumber());
               current.setOriginalNumber(copyBands[animationFrame].getOriginalNumber());
               newbands.push_back(current);
            }
         }
      }
      //if the user entered a specific set of times, then add that to the animation
      if (animationFrame < static_cast<unsigned int>(nTimes))
      {
         time = pTimes[animationFrame];
         bTimeSet = true;
      }
      //if the user added frame numbers for each animation, then set them in the descriptor
      if (animationFrame < static_cast<unsigned int>(nFrames))
      {
         frame = pFrames[animationFrame];
         DimensionDescriptor current = *iter;
         current.setActiveNumber(animationFrame);
         current.setOnDiskNumber(frame);
         current.setOriginalNumber(frame);
         newbands.push_back(current);
      }
      AnimationFrame aFrame(datasetName, animationFrame, time);
      animationFrames.push_back(aFrame);
      ++animationFrame;
   }

   if (newbands.size() > 0)
   {
      pDescriptor->setBands(newbands);
   }
   AnimationController* pController = Service<AnimationServices>()->getAnimationController(controllerName);
   FrameType frameType = bTimeSet ? FRAME_TIME : FRAME_ID;
   if (pController == NULL)
   {
      pController = Service<AnimationServices>()->createAnimationController(controllerName, frameType);
   }
   if (bTimesConverted)
   {
      IDL_Deltmp(tmpTimes);
   }
   if (bFramesConverted)
   {
      IDL_Deltmp(tmpFrames);
   }

   VERIFYRV(pController != NULL, IDL_StrToSTRING("failure"));

   VERIFYRV(animationFrames.empty(), IDL_StrToSTRING("failure"));

   Animation* pAnimation = pController->getAnimation(pData->getName());
   if (pAnimation != NULL)
   {
      pController->destroyAnimation(pAnimation);
   }
   pAnimation = pController->createAnimation(pData->getName());

   VERIFYRV(pAnimation != NULL, IDL_StrToSTRING("failure"));
   pAnimation->setFrames(animationFrames);

   RasterLayer* pAnimLayer = dynamic_cast<RasterLayer*>(IdlFunctions::getLayerByRaster(pData));
   if (pAnimLayer != NULL)
   {
      pAnimLayer->setAnimation(pAnimation);
   }
   AnimationToolBar* pAnimationToolbar =
      dynamic_cast<AnimationToolBar*>(Service<DesktopServices>()->getWindow("Animation", TOOLBAR));
   if (pAnimationToolbar != NULL)
   {
      //set the controller to be active and its properties to the copy controller's
      AnimationController* pCurrentController = pAnimationToolbar->getAnimationController();
      {
         pAnimationToolbar->setAnimationController(pController);
         bool bCanDrop = false;
         if (pCopyController != NULL)
         {
            pController->setMinimumFrameRate(pCopyController->getMinimumFrameRate());
            pController->setAnimationCycle(pCopyController->getAnimationCycle());
            pController->setIntervalMultiplier(pCopyController->getIntervalMultiplier());
            bCanDrop = pCopyController->getCanDropFrames();
         }
         pController->setCanDropFrames(bCanDrop);
      }
   }
   IDL_KW_FREE;
   return IDL_StrToSTRING("success");
}

IDL_VPTR get_interval_multiplier(int argc, IDL_VPTR pArgv[], char* pArgk)
{
   IDL_VPTR idlPtr;
   typedef struct
   {
      IDL_KW_RESULT_FIRST_FIELD;
      int animationControllerNameExists;
      IDL_STRING animationControllerName;
   } KW_RESULT;
   KW_RESULT kw;

   //IDL_KW_FAST_SCAN is the type of scan we are using, following it is the
   //name of the keyword, followed by the type, the mask(which should be 1),
   //flags, a boolean whether the value was populated and finally the value itself
   static IDL_KW_PAR kw_pars[] = {
      IDL_KW_FAST_SCAN,
      {"CONTROLLER_NAME", IDL_TYP_STRING, 1, 0, reinterpret_cast<int*>(IDL_KW_OFFSETOF(animationControllerNameExists)),
         reinterpret_cast<char*>(IDL_KW_OFFSETOF(animationControllerName))},
      {NULL}
   };

   IDL_KWProcessByOffset(argc, pArgv, pArgk, kw_pars, 0, 1, &kw);

   std::string controllerName;
   if (kw.animationControllerNameExists)
   {
      controllerName = IDL_STRING_STR(&kw.animationControllerName);
   }

   AnimationController* pController = NULL;
   if (!controllerName.empty())
   {
      pController = Service<AnimationServices>()->getAnimationController(controllerName);
   }
   AnimationToolBar* pToolbar =
      dynamic_cast<AnimationToolBar*>(Service<DesktopServices>()->getWindow("Animation", TOOLBAR));
   if (pToolbar != NULL)
   {
      pController = pToolbar->getAnimationController();
   }

   if (pController != NULL)
   {
      double value = pController->getIntervalMultiplier();
      idlPtr = IDL_Gettmp();
      idlPtr->value.d = value;
      idlPtr->type = IDL_TYP_DOUBLE;
   }
   IDL_KW_FREE;

   return idlPtr;
}

IDL_VPTR set_interval_multiplier(int argc, IDL_VPTR pArgv[], char* pArgk)
{
   IDL_VPTR idlPtr;
   typedef struct
   {
      IDL_KW_RESULT_FIRST_FIELD;
      int animationControllerNameExists;
      IDL_STRING animationControllerName;
   } KW_RESULT;
   KW_RESULT kw;

   //IDL_KW_FAST_SCAN is the type of scan we are using, following it is the
   //name of the keyword, followed by the type, the mask(which should be 1),
   //flags, a boolean whether the value was populated and finally the value itself
   static IDL_KW_PAR kw_pars[] = {
      IDL_KW_FAST_SCAN,
      {"CONTROLLER_NAME", IDL_TYP_STRING, 1, 0, reinterpret_cast<int*>(IDL_KW_OFFSETOF(animationControllerNameExists)),
         reinterpret_cast<char*>(IDL_KW_OFFSETOF(animationControllerName))},
      {NULL}
   };

   IDL_KWProcessByOffset(argc, pArgv, pArgk, kw_pars, 0, 1, &kw);

   bool bSuccess = false;
   IDL_MEMINT total;
   //the element
   char* pValue = NULL;
   //the value
   IDL_VPTR v = IDL_CvtDbl(1, pArgv);
   IDL_VarGetData(v, &total, &pValue, 0);

   double interval = 1;
   if (total > 0)
   {
      interval = *reinterpret_cast<double*>(pValue);
   }

   IDL_Deltmp(v);
   std::string controllerName;
   if (kw.animationControllerNameExists)
   {
      controllerName = IDL_STRING_STR(&kw.animationControllerName);
   }

   AnimationController* pController = NULL;
   if (!controllerName.empty())
   {
      pController = Service<AnimationServices>()->getAnimationController(controllerName);
   }
   AnimationToolBar* pToolbar =
      dynamic_cast<AnimationToolBar*>(Service<DesktopServices>()->getWindow("Animation", TOOLBAR));
   if (pToolbar != NULL)
   {
      pController = pToolbar->getAnimationController();
   }
   if (pController != NULL)
   {
      pController->setIntervalMultiplier(interval);
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
   IDL_KW_FREE;

   return idlPtr;
}

IDL_VPTR get_animation_state(int argc, IDL_VPTR pArgv[], char* pArgk)
{
   IDL_VPTR idlPtr;
   typedef struct
   {
      IDL_KW_RESULT_FIRST_FIELD;
      int animationControllerNameExists;
      IDL_STRING animationControllerName;
   } KW_RESULT;
   KW_RESULT kw;

   //IDL_KW_FAST_SCAN is the type of scan we are using, following it is the
   //name of the keyword, followed by the type, the mask(which should be 1),
   //flags, a boolean whether the value was populated and finally the value itself
   static IDL_KW_PAR kw_pars[] = {
      IDL_KW_FAST_SCAN,
      {"CONTROLLER_NAME", IDL_TYP_STRING, 1, 0, reinterpret_cast<int*>(IDL_KW_OFFSETOF(animationControllerNameExists)),
         reinterpret_cast<char*>(IDL_KW_OFFSETOF(animationControllerName))},
      {NULL}
   };

   IDL_KWProcessByOffset(argc, pArgv, pArgk, kw_pars, 0, 1, &kw);

   std::string controllerName;
   if (kw.animationControllerNameExists)
   {
      controllerName = IDL_STRING_STR(&kw.animationControllerName);
   }

   AnimationController* pController = NULL;
   if (!controllerName.empty())
   {
      pController = Service<AnimationServices>()->getAnimationController(controllerName);
   }
   AnimationToolBar* pToolbar =
      dynamic_cast<AnimationToolBar*>(Service<DesktopServices>()->getWindow("Animation", TOOLBAR));
   if (pToolbar != NULL)
   {
      pController = pToolbar->getAnimationController();
   }
   if (pController != NULL)
   {
      std::string strValue = StringUtilities::toXmlString(pController->getAnimationState());
      idlPtr = IDL_StrToSTRING(const_cast<char*>(strValue.c_str()));
   }
   return idlPtr;
}

IDL_VPTR set_animation_state(int argc, IDL_VPTR pArgv[], char* pArgk)
{
   IDL_VPTR idlPtr;
   typedef struct
   {
      IDL_KW_RESULT_FIRST_FIELD;
      int animationControllerNameExists;
      IDL_STRING animationControllerName;
   } KW_RESULT;
   KW_RESULT kw;

   //IDL_KW_FAST_SCAN is the type of scan we are using, following it is the
   //name of the keyword, followed by the type, the mask(which should be 1),
   //flags, a boolean whether the value was populated and finally the value itself
   static IDL_KW_PAR kw_pars[] = {
      IDL_KW_FAST_SCAN,
      {"CONTROLLER_NAME", IDL_TYP_STRING, 1, 0, reinterpret_cast<int*>(IDL_KW_OFFSETOF(animationControllerNameExists)),
         reinterpret_cast<char*>(IDL_KW_OFFSETOF(animationControllerName))},
      {NULL}
   };

   IDL_KWProcessByOffset(argc, pArgv, pArgk, kw_pars, 0, 1, &kw);

   bool bSuccess = false;
   //the element
   char* pValue = IDL_VarGetString(pArgv[0]);

   std::string controllerName;
   if (kw.animationControllerNameExists)
   {
      controllerName = IDL_STRING_STR(&kw.animationControllerName);
   }

   AnimationController* pController = NULL;
   if (!controllerName.empty())
   {
      pController = Service<AnimationServices>()->getAnimationController(controllerName);
   }
   AnimationToolBar* pToolbar =
      dynamic_cast<AnimationToolBar*>(Service<DesktopServices>()->getWindow("Animation", TOOLBAR));
   if (pToolbar != NULL)
   {
      pController = pToolbar->getAnimationController();
   }
   if (pController != NULL)
   {
      AnimationState eValue = StringUtilities::fromXmlString<AnimationState>(pValue);
      pController->setAnimationState(eValue);
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

IDL_VPTR get_animation_cycle(int argc, IDL_VPTR pArgv[], char* pArgk)
{
   IDL_VPTR idlPtr;
   typedef struct
   {
      IDL_KW_RESULT_FIRST_FIELD;
      int animationControllerNameExists;
      IDL_STRING animationControllerName;
   } KW_RESULT;
   KW_RESULT kw;

   //IDL_KW_FAST_SCAN is the type of scan we are using, following it is the
   //name of the keyword, followed by the type, the mask(which should be 1),
   //flags, a boolean whether the value was populated and finally the value itself
   static IDL_KW_PAR kw_pars[] = {
      IDL_KW_FAST_SCAN,
      {"CONTROLLER_NAME", IDL_TYP_STRING, 1, 0, reinterpret_cast<int*>(IDL_KW_OFFSETOF(animationControllerNameExists)),
         reinterpret_cast<char*>(IDL_KW_OFFSETOF(animationControllerName))},
      {NULL}
   };

   IDL_KWProcessByOffset(argc, pArgv, pArgk, kw_pars, 0, 1, &kw);

   std::string controllerName;
   if (kw.animationControllerNameExists)
   {
      controllerName = IDL_STRING_STR(&kw.animationControllerName);
   }

   AnimationController* pController = NULL;
   if (!controllerName.empty())
   {
      pController = Service<AnimationServices>()->getAnimationController(controllerName);
   }
   AnimationToolBar* pToolbar =
      dynamic_cast<AnimationToolBar*>(Service<DesktopServices>()->getWindow("Animation", TOOLBAR));
   if (pToolbar != NULL)
   {
      pController = pToolbar->getAnimationController();
   }
   if (pController != NULL)
   {
      std::string strValue = StringUtilities::toXmlString(pController->getAnimationCycle());
      idlPtr = IDL_StrToSTRING(const_cast<char*>(strValue.c_str()));
   }
   return idlPtr;
}

IDL_VPTR set_animation_cycle(int argc, IDL_VPTR pArgv[], char* pArgk)
{
   IDL_VPTR idlPtr;
   typedef struct
   {
      IDL_KW_RESULT_FIRST_FIELD;
      int animationControllerNameExists;
      IDL_STRING animationControllerName;
   } KW_RESULT;
   KW_RESULT kw;

   //IDL_KW_FAST_SCAN is the type of scan we are using, following it is the
   //name of the keyword, followed by the type, the mask(which should be 1),
   //flags, a boolean whether the value was populated and finally the value itself
   static IDL_KW_PAR kw_pars[] = {
      IDL_KW_FAST_SCAN,
      {"CONTROLLER_NAME", IDL_TYP_STRING, 1, 0, reinterpret_cast<int*>(IDL_KW_OFFSETOF(animationControllerNameExists)),
         reinterpret_cast<char*>(IDL_KW_OFFSETOF(animationControllerName))},
      {NULL}
   };

   IDL_KWProcessByOffset(argc, pArgv, pArgk, kw_pars, 0, 1, &kw);

   bool bSuccess = false;
   //the element
   char* pValue = IDL_VarGetString(pArgv[0]);

   std::string controllerName;
   if (kw.animationControllerNameExists)
   {
      controllerName = IDL_STRING_STR(&kw.animationControllerName);
   }

   AnimationController* pController = NULL;
   if (!controllerName.empty())
   {
      pController = Service<AnimationServices>()->getAnimationController(controllerName);
   }
   AnimationToolBar* pToolbar =
      dynamic_cast<AnimationToolBar*>(Service<DesktopServices>()->getWindow("Animation", TOOLBAR));
   if (pToolbar != NULL)
   {
      pController = pToolbar->getAnimationController();
   }
   if (pController != NULL)
   {
      AnimationCycle eValue = StringUtilities::fromXmlString<AnimationCycle>(pValue);
      pController->setAnimationCycle(eValue);
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

IDL_VPTR enable_can_drop_frames(int argc, IDL_VPTR pArgv[], char* pArgk)
{
   IDL_VPTR idlPtr;
   typedef struct
   {
      IDL_KW_RESULT_FIRST_FIELD;
      int animationControllerNameExists;
      IDL_STRING animationControllerName;
   } KW_RESULT;
   KW_RESULT kw;

   //IDL_KW_FAST_SCAN is the type of scan we are using, following it is the
   //name of the keyword, followed by the type, the mask(which should be 1),
   //flags, a boolean whether the value was populated and finally the value itself
   static IDL_KW_PAR kw_pars[] = {
      IDL_KW_FAST_SCAN,
      {"CONTROLLER_NAME", IDL_TYP_STRING, 1, 0, reinterpret_cast<int*>(IDL_KW_OFFSETOF(animationControllerNameExists)),
         reinterpret_cast<char*>(IDL_KW_OFFSETOF(animationControllerName))},
      {NULL}
   };

   IDL_KWProcessByOffset(argc, pArgv, pArgk, kw_pars, 0, 1, &kw);

   std::string controllerName;
   if (kw.animationControllerNameExists)
   {
      controllerName = IDL_STRING_STR(&kw.animationControllerName);
   }

   AnimationController* pController = NULL;
   if (!controllerName.empty())
   {
      pController = Service<AnimationServices>()->getAnimationController(controllerName);
   }
   AnimationToolBar* pToolbar =
      dynamic_cast<AnimationToolBar*>(Service<DesktopServices>()->getWindow("Animation", TOOLBAR));
   if (pToolbar != NULL)
   {
      pController = pToolbar->getAnimationController();
   }
   bool bSuccess = false;
   if (pController != NULL)
   {
      pController->setCanDropFrames(true);
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

IDL_VPTR disable_can_drop_frames(int argc, IDL_VPTR pArgv[], char* pArgk)
{
   IDL_VPTR idlPtr;
   typedef struct
   {
      IDL_KW_RESULT_FIRST_FIELD;
      int animationControllerNameExists;
      IDL_STRING animationControllerName;
   } KW_RESULT;
   KW_RESULT kw;

   //IDL_KW_FAST_SCAN is the type of scan we are using, following it is the
   //name of the keyword, followed by the type, the mask(which should be 1),
   //flags, a boolean whether the value was populated and finally the value itself
   static IDL_KW_PAR kw_pars[] = {
      IDL_KW_FAST_SCAN,
      {"CONTROLLER_NAME", IDL_TYP_STRING, 1, 0, reinterpret_cast<int*>(IDL_KW_OFFSETOF(animationControllerNameExists)),
         reinterpret_cast<char*>(IDL_KW_OFFSETOF(animationControllerName))},
      {NULL}
   };

   IDL_KWProcessByOffset(argc, pArgv, pArgk, kw_pars, 0, 1, &kw);

   bool bSuccess = false;
   std::string controllerName;
   if (kw.animationControllerNameExists)
   {
      controllerName = IDL_STRING_STR(&kw.animationControllerName);
   }

   AnimationController* pController = NULL;
   if (!controllerName.empty())
   {
      pController = Service<AnimationServices>()->getAnimationController(controllerName);
   }
   AnimationToolBar* pToolbar =
      dynamic_cast<AnimationToolBar*>(Service<DesktopServices>()->getWindow("Animation", TOOLBAR));
   if (pToolbar != NULL)
   {
      pController = pToolbar->getAnimationController();
   }
   if (pController != NULL)
   {
      pController->setCanDropFrames(false);
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

IDL_VPTR get_configuration_setting(int argc, IDL_VPTR pArgv[])
{
   IDL_VPTR idlPtr;

   //the element
   std::string value;
   //the value
   std::string path(IDL_VarGetString(pArgv[0]));

   if (!path.empty())
   {
      const DataVariant& setting = Service<ConfigurationSettings>()->getSetting(path);
      if (setting.isValid())
      {
         value = setting.toDisplayString();
      }
   }
   idlPtr = IDL_StrToSTRING(const_cast<char*>(value.c_str()));

   return idlPtr;
}

static IDL_SYSFUN_DEF2 func_definitions[] = {
   {reinterpret_cast<IDL_SYSRTN_GENERIC>(array_to_idl), "ARRAY_TO_IDL",0,12,IDL_SYSFUN_DEF_F_KEYWORDS,0},
   {reinterpret_cast<IDL_SYSRTN_GENERIC>(array_to_opticks), "ARRAY_TO_OPTICKS",0,12,IDL_SYSFUN_DEF_F_KEYWORDS,0},
   {reinterpret_cast<IDL_SYSRTN_GENERIC>(change_data_type), "CHANGE_DATA_TYPE",0,5,IDL_SYSFUN_DEF_F_KEYWORDS,0},
   {reinterpret_cast<IDL_SYSRTN_GENERIC>(close_window), "CLOSE_WINDOW",0,5,IDL_SYSFUN_DEF_F_KEYWORDS,0},
//   {reinterpret_cast<IDL_SYSRTN_GENERIC>(copy_metadata), "COPY_METADATA",0,2,IDL_SYSFUN_DEF_F_KEYWORDS,0},
   {reinterpret_cast<IDL_SYSRTN_GENERIC>(create_animation), "CREATE_ANIMATION",0,5,IDL_SYSFUN_DEF_F_KEYWORDS,0},
   {reinterpret_cast<IDL_SYSRTN_GENERIC>(disable_can_drop_frames), "DISABLE_CAN_DROP_FRAMES",0,5,IDL_SYSFUN_DEF_F_KEYWORDS,0},
   {reinterpret_cast<IDL_SYSRTN_GENERIC>(disable_filter), "DISABLE_FILTER",0,5,IDL_SYSFUN_DEF_F_KEYWORDS,0},
   {reinterpret_cast<IDL_SYSRTN_GENERIC>(disable_gpu), "DISABLE_GPU",0,5,IDL_SYSFUN_DEF_F_KEYWORDS,0},
   {reinterpret_cast<IDL_SYSRTN_GENERIC>(enable_can_drop_frames), "ENABLE_CAN_DROP_FRAMES",0,5,IDL_SYSFUN_DEF_F_KEYWORDS,0},
   {reinterpret_cast<IDL_SYSRTN_GENERIC>(enable_filter), "ENABLE_FILTER",0,5,IDL_SYSFUN_DEF_F_KEYWORDS,0},
   {reinterpret_cast<IDL_SYSRTN_GENERIC>(enable_gpu), "ENABLE_GPU",0,5,IDL_SYSFUN_DEF_F_KEYWORDS,0},
   {reinterpret_cast<IDL_SYSRTN_GENERIC>(execute_wizard), "EXECUTE_WIZARD",0,5,IDL_SYSFUN_DEF_F_KEYWORDS,0},
   {reinterpret_cast<IDL_SYSRTN_GENERIC>(get_animation_cycle), "GET_ANIMATION_CYCLE",0,5,IDL_SYSFUN_DEF_F_KEYWORDS,0},
   {reinterpret_cast<IDL_SYSRTN_GENERIC>(get_animation_state), "GET_ANIMATION_STATE",0,5,IDL_SYSFUN_DEF_F_KEYWORDS,0},
   {reinterpret_cast<IDL_SYSRTN_GENERIC>(get_configuration_setting), "GET_CONFIGURATION_SETTING",0,1,0,0},
   {reinterpret_cast<IDL_SYSRTN_GENERIC>(get_current_name), "GET_CURRENT_NAME",0,5,IDL_SYSFUN_DEF_F_KEYWORDS,0},
   {reinterpret_cast<IDL_SYSRTN_GENERIC>(get_data_element_names), "GET_DATA_ELEMENT_NAMES",0,5,IDL_SYSFUN_DEF_F_KEYWORDS,0},
   {reinterpret_cast<IDL_SYSRTN_GENERIC>(get_data_name), "GET_DATA_NAME",0,5,IDL_SYSFUN_DEF_F_KEYWORDS,0},
   {reinterpret_cast<IDL_SYSRTN_GENERIC>(get_interval_multiplier), "GET_INTERVAL_MULTIPLIER",0,5,IDL_SYSFUN_DEF_F_KEYWORDS,0},
   {reinterpret_cast<IDL_SYSRTN_GENERIC>(get_layer_name), "GET_LAYER_NAME",0,5,IDL_SYSFUN_DEF_F_KEYWORDS,0},
   {reinterpret_cast<IDL_SYSRTN_GENERIC>(get_layer_position), "GET_LAYER_POSITION",0,5,IDL_SYSFUN_DEF_F_KEYWORDS,0},
//   {reinterpret_cast<IDL_SYSRTN_GENERIC>(get_metadata), "GET_METADATA",0,5,IDL_SYSFUN_DEF_F_KEYWORDS,0},
   {reinterpret_cast<IDL_SYSRTN_GENERIC>(get_num_layers), "GET_NUM_LAYERS",0,5,IDL_SYSFUN_DEF_F_KEYWORDS,0},
   {reinterpret_cast<IDL_SYSRTN_GENERIC>(get_stretch_method), "GET_STRETCH_METHOD",0,5,IDL_SYSFUN_DEF_F_KEYWORDS,0},
   {reinterpret_cast<IDL_SYSRTN_GENERIC>(get_stretch_type), "GET_STRETCH_TYPE",0,5,IDL_SYSFUN_DEF_F_KEYWORDS,0},
   {reinterpret_cast<IDL_SYSRTN_GENERIC>(get_stretch_values), "GET_STRETCH_VALUES",0,5,IDL_SYSFUN_DEF_F_KEYWORDS,0},
   {reinterpret_cast<IDL_SYSRTN_GENERIC>(get_window_label), "GET_WINDOW_LABEL",0,5,IDL_SYSFUN_DEF_F_KEYWORDS,0},
   {reinterpret_cast<IDL_SYSRTN_GENERIC>(get_window_position), "GET_WINDOW_POSITION",0,5,IDL_SYSFUN_DEF_F_KEYWORDS,0},
   {reinterpret_cast<IDL_SYSRTN_GENERIC>(hide_layer), "HIDE_LAYER",0,5,IDL_SYSFUN_DEF_F_KEYWORDS,0},
   {reinterpret_cast<IDL_SYSRTN_GENERIC>(set_animation_cycle), "SET_ANIMATION_CYCLE",0,5,IDL_SYSFUN_DEF_F_KEYWORDS,0},
   {reinterpret_cast<IDL_SYSRTN_GENERIC>(set_animation_state), "SET_ANIMATION_STATE",0,5,IDL_SYSFUN_DEF_F_KEYWORDS,0},
   {reinterpret_cast<IDL_SYSRTN_GENERIC>(set_colormap), "SET_COLORMAP",0,5,IDL_SYSFUN_DEF_F_KEYWORDS,0},
   {reinterpret_cast<IDL_SYSRTN_GENERIC>(set_interval_multiplier), "SET_INTERVAL_MULTIPLIER",0,5,IDL_SYSFUN_DEF_F_KEYWORDS,0},
   {reinterpret_cast<IDL_SYSRTN_GENERIC>(set_layer_position), "SET_LAYER_POSITION",0,5,IDL_SYSFUN_DEF_F_KEYWORDS,0},
//   {reinterpret_cast<IDL_SYSRTN_GENERIC>(set_metadata), "SET_METADATA",0,5,IDL_SYSFUN_DEF_F_KEYWORDS,0},
   {reinterpret_cast<IDL_SYSRTN_GENERIC>(set_stretch_method), "SET_STRETCH_METHOD",0,5,IDL_SYSFUN_DEF_F_KEYWORDS,0},
   {reinterpret_cast<IDL_SYSRTN_GENERIC>(set_stretch_type), "SET_STRETCH_TYPE",0,5,IDL_SYSFUN_DEF_F_KEYWORDS,0},
   {reinterpret_cast<IDL_SYSRTN_GENERIC>(set_stretch_values), "SET_STRETCH_VALUES",0,5,IDL_SYSFUN_DEF_F_KEYWORDS,0},
   {reinterpret_cast<IDL_SYSRTN_GENERIC>(set_window_label), "SET_WINDOW_LABEL",0,5,IDL_SYSFUN_DEF_F_KEYWORDS,0},
   {reinterpret_cast<IDL_SYSRTN_GENERIC>(set_window_position), "SET_WINDOW_POSITION",0,5,IDL_SYSFUN_DEF_F_KEYWORDS,0},
   {reinterpret_cast<IDL_SYSRTN_GENERIC>(show_layer), "SHOW_LAYER",0,5,IDL_SYSFUN_DEF_F_KEYWORDS,0},
   {NULL, NULL, 0, 0, 0, 0}
};

static IDL_SYSFUN_DEF2 proc_definitions[] = {
   {reinterpret_cast<IDL_SYSRTN_GENERIC>(refresh_display), "REFRESH_DISPLAY",0,5,IDL_SYSFUN_DEF_F_KEYWORDS,0},
   {reinterpret_cast<IDL_SYSRTN_GENERIC>(report_progress), "REPORT_PROGRESS",0,5,IDL_SYSFUN_DEF_F_KEYWORDS,0},
   {NULL, NULL, 0, 0, 0, 0}
};
}

IDLSTART_EXPORT int start_idl(const char* pLocation, External* pExternal)
{
   VERIFYRV(pExternal != NULL, 0);
   ModuleManager::instance()->setService(pExternal);
   VERIFYRV(IDL_CallProxyInit(const_cast<char*>(pLocation)), 0);
   // Register our output function
   IDL_ToutPush(OutFunc);

#ifdef WIN_API
   VERIFYRV(IDL_Win32Init(0, 0, 0, NULL), 0);
#else
   VERIFYRV(IDL_Init(0, NULL, 0), 0);
#endif

   int funcCnt = -1;
   while (func_definitions[++funcCnt].name != NULL); // do nothing in the loop body
   int procCnt = -1;
   while (proc_definitions[++procCnt].name != NULL); // do nothing in the loop body

   // Add system routines
   if (!IDL_SysRtnAdd(func_definitions, IDL_TRUE, funcCnt) || !IDL_SysRtnAdd(proc_definitions, IDL_FALSE, procCnt))
   {
      IDL_Message(IDL_M_GENERIC, IDL_MSG_RET, "Error adding system routines.");
   }

   return 1;
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

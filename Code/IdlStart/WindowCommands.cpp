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
#include "LayerList.h"
#include "RasterElement.h"
#include "RasterLayer.h"
#include "SpatialDataView.h"
#include "StringUtilities.h"
#include "View.h"
#include "WindowCommands.h"
#include "WorkspaceWindow.h"

#include <QtGui/QWidget>
#include <string>
#include <stdio.h>
#include <idl_export.h>
#include <idl_callproxy.h>

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
   std::string typeName = "SpatialDataWindow";
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
   std::string typeName = "SpatialDataWindow";
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
   std::string typeName = "SpatialDataWindow";
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
   std::string typeName = "SpatialDataWindow";
   std::string label;

   bool bSuccess = false;
   if (argc < 1)
   {
      IDL_Message(IDL_M_GENERIC, IDL_MSG_RET, "get_window_position 'type', 'window', 'win_pos_x', and 'win_pos_y' as keywords.");
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
   }
   idlPtr = IDL_StrToSTRING("failure");
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
   std::string typeName = "SpatialDataWindow";
   std::string label;
   int xPos = 0;
   int yPos = 0;

   bool bSuccess = false;
   if (argc < 1)
   {
      IDL_Message(IDL_M_GENERIC, IDL_MSG_RET, "set_window_position 'type', 'window', 'win_pos_x', and 'win_pos_x' as keywords.");
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
   IDL_KW_FREE;
   return idlPtr;
}

static IDL_SYSFUN_DEF2 func_definitions[] = {
   {reinterpret_cast<IDL_SYSRTN_GENERIC>(close_window), "CLOSE_WINDOW",0,5,IDL_SYSFUN_DEF_F_KEYWORDS,0},
   {reinterpret_cast<IDL_SYSRTN_GENERIC>(get_window_label), "GET_WINDOW_LABEL",0,5,IDL_SYSFUN_DEF_F_KEYWORDS,0},
   {reinterpret_cast<IDL_SYSRTN_GENERIC>(get_window_position), "GET_WINDOW_POSITION",0,5,IDL_SYSFUN_DEF_F_KEYWORDS,0},
   {reinterpret_cast<IDL_SYSRTN_GENERIC>(set_window_label), "SET_WINDOW_LABEL",0,5,IDL_SYSFUN_DEF_F_KEYWORDS,0},
   {reinterpret_cast<IDL_SYSRTN_GENERIC>(set_window_position), "SET_WINDOW_POSITION",0,5,IDL_SYSFUN_DEF_F_KEYWORDS,0},
   {NULL, NULL, 0, 0, 0, 0}
};

static IDL_SYSFUN_DEF2 proc_definitions[] = {
   {reinterpret_cast<IDL_SYSRTN_GENERIC>(refresh_display), "REFRESH_DISPLAY",0,5,IDL_SYSFUN_DEF_F_KEYWORDS,0},
   {NULL, NULL, 0, 0, 0, 0}
};

bool addWindowCommands()
{
   int funcCnt = -1;
   while (func_definitions[++funcCnt].name != NULL); // do nothing in the loop body
   int procCnt = -1;
   while (proc_definitions[++procCnt].name != NULL); // do nothing in the loop body
   return IDL_SysRtnAdd(func_definitions, IDL_TRUE, funcCnt) != 0 &&
          IDL_SysRtnAdd(proc_definitions, IDL_FALSE, procCnt) != 0;
}

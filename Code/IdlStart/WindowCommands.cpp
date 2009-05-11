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

/**
 * \defgroup windowcommands Window Commands
 */
/*@{*/

/**
 * This procedure marks a RasterElement's data as having changed.
 *
 * @param[in] DATASET @opt
 *            The name of the RasterElement to refresh. Defaults to the primary dataset for the active view.
 * @usage refresh_display,DATASET="Dataset1.tif"
 * @endusage
 */
void refresh_display(int argc, IDL_VPTR pArgv[], char* pArgk)
{
   typedef struct
   {
      IDL_KW_RESULT_FIRST_FIELD;
      int datasetExists;
      IDL_STRING dataset;
   } KW_RESULT;

   //IDL_KW_FAST_SCAN is the type of scan we are using, following it is the
   //name of the keyword, followed by the type, the mask(which should be 1),
   //flags, a boolean whether the value was populated and finally the value itself
   static IDL_KW_PAR kw_pars[] = {
      IDL_KW_FAST_SCAN,
      {"DATASET", IDL_TYP_STRING, 1, 0, reinterpret_cast<int*>(IDL_KW_OFFSETOF(datasetExists)),
         reinterpret_cast<char*>(IDL_KW_OFFSETOF(dataset))},
      {NULL}
   };
   IdlFunctions::IdlKwResource<KW_RESULT> kw(argc, pArgv, pArgk, kw_pars, 0, 1);

   std::string dataName;
   if (kw->datasetExists)
   {
      dataName = IDL_STRING_STR(&kw->dataset);
   }
   WorkspaceWindow* pData = Service<DesktopServices>()->getCurrentWorkspaceWindow();
   RasterElement* pElement = NULL;
   if (pData != NULL)
   {
      View* pView = pData->getView();
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
         pView->refresh();
      }
   }
}

/**
 * Close the specified window.
 *
 * @param[in] TYPE @opt
 *            The type of window specified by WINDOW. Ignored if WINDOW is not specified.
 *            Defaults to SpatialDataWindow.
 * @param[in] WINDOW @opt
 *            The name of the window to close. Defaults to the active window.
 * @rsof
 * @usage print,close_window(WINDOW="Window Name")
 * @endusage
 */
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

   IdlFunctions::IdlKwResource<KW_RESULT> kw(argc, pArgv, pArgk, kw_pars, 0, 1);

   std::string windowName;
   std::string typeName = "SpatialDataWindow";
   IDL_VPTR idlPtr;
   bool bSuccess = false;
   if (kw->typeExists)
   {
      typeName = IDL_STRING_STR(&kw->typeName);
   }
   // retrieve the layer name passed in as a keyword
   if (kw->windowNameExists)
   {
      windowName = IDL_STRING_STR(&kw->windowName);
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
   return idlPtr;
}

/**
 * Change the name of a window.
 *
 * @note This changes the name of the window only and not the view nor any layers.
 *
 * @param[in] [1]
 *            The new name for the window.
 * @param[in] TYPE @opt
 *            The type of window specified by WINDOW. Ignored if WINDOW is not specified.
 *            The default is \p SpatialDataWindow.
 * @param[in] WINDOW @opt
 *            The name of the window to manipulate.
 *            The default is the active window.
 * @rsof
 * @usage print,set_window_label("New Window Name", WINDOW="Old Window Name")
 * @endusage
 */
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

   IdlFunctions::IdlKwResource<KW_RESULT> kw(argc, pArgv, pArgk, kw_pars, 0, 1);

   std::string windowName;
   std::string typeName = "SpatialDataWindow";
   std::string label;

   IDL_VPTR idlPtr;
   bool bSuccess = false;
   //retrieve the label passed in as a parameter
   label = IDL_VarGetString(pArgv[0]);

   if (kw->typeExists)
   {
      typeName = IDL_STRING_STR(&kw->typeName);
   }
   //retrieve the layer name passed in as a keyword
   if (kw->windowNameExists)
   {
      windowName = IDL_STRING_STR(&kw->windowName);
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
   return idlPtr;
}

/**
 * Get the name of a window.
 *
 * @note This obtains the window's title bar label and not the name of
 *       the Opticks Window, View, nor any Layers.
 *
 * @param[in] TYPE @opt
 *            The type of window specified by WINDOW. Ignored if WINDOW is not specified.
 *            The default is \p SpatialDataWindow.
 * @param[in] WINDOW @opt
 *            The name of the window to access.
 *            The default is the active window.
 * @rsof
 * @usage window_name = get_window_label(WINDOW="Window Name")
 * @endusage
 */
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

   IdlFunctions::IdlKwResource<KW_RESULT> kw(argc, pArgv, pArgk, kw_pars, 0, 1);

   std::string windowName;
   std::string typeName = "SpatialDataWindow";
   std::string label;

   IDL_VPTR idlPtr;
   bool bSuccess = false;

   if (kw->typeExists)
   {
      typeName = IDL_STRING_STR(&kw->typeName);
   }
   //retrieve the layer name passed in as a keyword
   if (kw->windowNameExists)
   {
      windowName = IDL_STRING_STR(&kw->windowName);
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
   return idlPtr;
}

/**
 * Get the position of a window.
 * This returns the position of the window in coordinates relative to the top left
 * of the workspace.
 *
 * @param[out] WIN_POS_X
 *             This will contain the \c x position of the window.
 * @param[out] WIN_POS_Y
 *             This will contain the \c y position of the window.
 * @param[in] TYPE @opt
 *            The type of window specified by WINDOW. Ignored if WINDOW is not specified.
 *            The default is \p SpatialDataWindow.
 * @param[in] WINDOW @opt
 *            The name of the window to access.
 *            The default is the active window.
 * @rsof
 * @usage xpos = 0
 * ypos = 0
 * print,get_window_position(WIN_POS_X=xpos, WIN_POS_Y=ypos, WINDOW="Window Name")
 * @endusage
 */
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

   IdlFunctions::IdlKwResource<KW_RESULT> kw(argc, pArgv, pArgk, kw_pars, 0, 1);
   std::string windowName;
   std::string typeName = "SpatialDataWindow";
   std::string label;

   bool bSuccess = false;
   if (argc < 2)
   {
      IDL_Message(IDL_M_GENERIC, IDL_MSG_RET,
         "get_window_position 'type', 'window', 'win_pos_x', and 'win_pos_y' as keywords.");
   }
   else
   {
      if (kw->typeExists)
      {
         typeName = IDL_STRING_STR(&kw->typeName);
      }
      //retrieve the layer name passed in as a keyword
      if (kw->windowNameExists)
      {
         windowName = IDL_STRING_STR(&kw->windowName);
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
                  if (kw->xPosExists)
                  {
                     //we don't know the datatype passed in, so set all of them
                     kw->xPos->value.d = static_cast<double>(xPos);
                     kw->xPos->value.f = static_cast<float>(xPos);
                     kw->xPos->value.i = static_cast<short>(xPos);
                     kw->xPos->value.ui = static_cast<unsigned short>(xPos);
                     kw->xPos->value.ul = static_cast<unsigned int>(xPos);
                     kw->xPos->value.l = xPos;
                     kw->xPos->value.l64 = static_cast<unsigned long>(xPos);
                     kw->xPos->value.ul64 = static_cast<unsigned long>(xPos);
                     kw->xPos->value.sc = static_cast<char>(xPos);
                     kw->xPos->value.c = static_cast<unsigned char>(xPos);
                     bSuccess = true;
                  }
                  int yPos = pMain->y();
                  if (kw->yPosExists)
                  {
                     //we don't know the datatype passed in to populate, so set all of them
                     kw->yPos->value.d = static_cast<double>(yPos);
                     kw->yPos->value.f = static_cast<float>(yPos);
                     kw->yPos->value.i = static_cast<short>(yPos);
                     kw->yPos->value.ui = static_cast<unsigned short>(yPos);
                     kw->yPos->value.ul = static_cast<unsigned int>(yPos);
                     kw->yPos->value.l = yPos;
                     kw->yPos->value.l64 = static_cast<unsigned long>(yPos);
                     kw->yPos->value.ul64 = static_cast<unsigned long>(yPos);
                     kw->yPos->value.sc = static_cast<char>(yPos);
                     kw->yPos->value.c = static_cast<unsigned char>(yPos);
                     bSuccess = true;
                  }
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
   return idlPtr;
}

/**
 * Set the position of a window.
 * This moves a window to a new position in coordinates relative to the top left
 * of the workspace.
 *
 * @param[out] WIN_POS_X
 *             This is the \c x position of the window.
 * @param[out] WIN_POS_Y
 *             This is the \c y position of the window.
 * @param[in] TYPE @opt
 *            The type of window specified by WINDOW. Ignored if WINDOW is not specified.
 *            The default is \p SpatialDataWindow.
 * @param[in] WINDOW @opt
 *            The name of the window to manipulate.
 *            The default is the active window.
 * @rsof
 * @usage xpos = 100
 * ypos = 55
 * print,set_window_position(WIN_POS_X=xpos, WIN_POS_Y=ypos, WINDOW="Window Name")
 * @endusage
 */
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

   IdlFunctions::IdlKwResource<KW_RESULT> kw(argc, pArgv, pArgk, kw_pars, 0, 1);
   std::string windowName;
   std::string typeName = "SpatialDataWindow";
   std::string label;
   int xPos = 0;
   int yPos = 0;

   bool bSuccess = false;
   if (argc < 2)
   {
      IDL_Message(IDL_M_GENERIC, IDL_MSG_RET,
         "set_window_position 'type', 'window', 'win_pos_x', and 'win_pos_x' as keywords.");
   }
   else
   {

      if (kw->typeExists)
      {
         typeName = IDL_STRING_STR(&kw->typeName);
      }
      //retrieve the layer name passed in as a keyword
      if (kw->windowNameExists)
      {
         windowName = IDL_STRING_STR(&kw->windowName);
      }
      if (kw->typeExists)
      {
         typeName = IDL_STRING_STR(&kw->typeName);
      }
      if (kw->xPosExists)
      {
         xPos = kw->xPos;
      }
      if (kw->yPosExists)
      {
         yPos = kw->yPos;
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
   return idlPtr;
}
/*@}*/

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

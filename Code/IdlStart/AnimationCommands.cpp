/*
 * The information in this file is
 * Copyright(c) 2007 Ball Aerospace & Technologies Corporation
 * and is subject to the terms and conditions of the
 * GNU Lesser General Public License Version 2.1
 * The license text is available from   
 * http://www.gnu.org/licenses/lgpl.html
 */

#include "Animation.h"
#include "AnimationCommands.h"
#include "AnimationController.h"
#include "AnimationServices.h"
#include "AnimationToolBar.h"
#include "DesktopServices.h"
#include "IdlFunctions.h"
#include "RasterDataDescriptor.h"
#include "RasterElement.h"
#include "RasterLayer.h"
#include "SpatialDataView.h"
#include "StringUtilities.h"

#include <QtCore/QString>
#include <string>
#include <vector>
#include <stdio.h>
#include <idl_export.h>
#include <idl_callproxy.h>

/**
 * \defgroup animationcommands Animation Commands
 */
/*@{*/

/**
 * Create a new animation.
 * Create a time or frame based animation and attach it to a raster layer.
 *
 * @param[in] DATASET
 *        The name of the RasterElement which will be animated.
 * @param[in] CONTROLLER_NAME @opt
 *        The name of the animation controller. A new one will
 *        be created if necessary. The default creates a name based on an
 *        increasing integer.
 * @param[in] COPY_DATASET @opt
 *        If specified, the named dataset is retrieved. The default animation
 *        controller associated with the first RasterLayer containing this dataset
 *        is cloned and used as the basis for the new animation. The default is
 *        not to clone a controller.
 * @param[in] FRAMES @opt
 *        If specified, this is an int array of frame numbers for the new animation. The default is to use
 *        the active frame numbers in DATASET.
 * @param[in] TIMES @opt
 *        If specified, this is a double array of frame times for the new animation. The default is to
 *        create a frame based animation.
 * @rsof
 * @usage print,create_animation(DATASET="MyAnimationDataset")
 * @endusage
 */
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
         IDL_Message(IDL_M_GENERIC, IDL_MSG_RET, "CREATE_ANIMATION critical error.  Frames keyword must be an array.");
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

   VERIFYRV(!animationFrames.empty(), IDL_StrToSTRING("failure"));

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

/**
 * Get the animation interval multiplier for an animation controller.
 *
 * @param[in] CONTROLLER_NAME @opt
 *            The name of the animation controller. Defaults to
 *            the active animation controller.
 * @return The current interval multiplier for the specified controller.
 * @usage interval = get_interval_multiplier(CONTROLLER_NAME="Controller 1")
 * @endusage
 */
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

/**
 * Change the animation interval multiplier for an animation controller.
 *
 * This multiplier changes the playback rate. A multiplier of 2.0 will double
 * the playback rate and a multiplier of 0.5 will half the playback rate.
 *
 * @param[in] [1]
 *            The new interval multiplier.
 * @param[in] CONTROLLER_NAME @opt
 *            The name of the animation controller. Defaults to
 *            the active animation controller.
 * @rsof
 * @usage print,set_interval_multiplier(2.0, CONTROLLER_NAME="Controller 1")
 * @endusage
 */
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

/**
 * Get the animation play state for an animation controller.
 *
 * @param[in] CONTROLLER_NAME @opt
 *            The name of the animation controller. Defaults to
 *            the active animation controller.
 * @return The current animation play state for the specified controller.
 * @usage state = get_animation_state(CONTROLLER_NAME="Controller 1")
 * @endusage
 */
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

/**
 * Change the animation play state for an animation controller.
 *
 * @param[in] [1]
 *            The new animation state.
 * @param[in] CONTROLLER_NAME @opt
 *            The name of the animation controller. Defaults to
 *            the active animation controller.
 * @rsof
 * @usage print,set_animation_state("play_forward", CONTROLLER_NAME="Controller 1")
 * @endusage
 */
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
      if (eValue.isValid())
      {
         pController->setAnimationState(eValue);
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
 * Get the animation cycle mode on an animation controller.
 *
 * @param[in] CONTROLLER_NAME @opt
 *            The name of the animation controller. Defaults to
 *            the active animation controller.
 * @return The current animation cycle mode for the specified controller.
 * @usage cycle_mode = get_animation_cycle(CONTROLLER_NAME="Controller 1")
 * @endusage
 */
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

/**
 * Change the animation cycle mode on an animation controller.
 *
 * @param[in] [1]
 *            The new animation cycle mode.
 * @param[in] CONTROLLER_NAME @opt
 *            The name of the animation controller. Defaults to
 *            the active animation controller.
 * @rsof
 * @usage print,set_animation_cycle("repeat", CONTROLLER_NAME="Controller 1")
 * @endusage
 */
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
      if (eValue.isValid())
      {
         pController->setAnimationCycle(eValue);
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
 * Enable frame dropping on an animation controller.
 *
 * Allows the application to skip frames if necessary to maintain
 * the playback rate.
 *
 * @param[in] CONTROLLER_NAME @opt
 *            The name of the animation controller where drop frames will be
 *            enabled. Defaults to the active animation controller.
 * @rsof
 * @usage print,enable_can_drop_frames(CONTROLLER_NAME="Controller 1")
 * @endusage
 */
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

/**
 * Disable frame dropping on an animation controller.
 *
 * Cause the application to play all frames even if playback needs to slow down.skip frames if necessary to maintain
 *
 * @param[in] CONTROLLER_NAME @opt
 *            The name of the animation controller where drop frames will be
 *            disabled. Defaults to the active animation controller.
 * @rsof
 * @usage print,disable_can_drop_frames(CONTROLLER_NAME="Controller 1")
 * @endusage
 */
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
/*@}*/

static IDL_SYSFUN_DEF2 func_definitions[] = {
   {reinterpret_cast<IDL_SYSRTN_GENERIC>(create_animation), "CREATE_ANIMATION",0,5,IDL_SYSFUN_DEF_F_KEYWORDS,0},
   {reinterpret_cast<IDL_SYSRTN_GENERIC>(disable_can_drop_frames), "DISABLE_CAN_DROP_FRAMES",0,5,IDL_SYSFUN_DEF_F_KEYWORDS,0},
   {reinterpret_cast<IDL_SYSRTN_GENERIC>(enable_can_drop_frames), "ENABLE_CAN_DROP_FRAMES",0,5,IDL_SYSFUN_DEF_F_KEYWORDS,0},
   {reinterpret_cast<IDL_SYSRTN_GENERIC>(get_animation_cycle), "GET_ANIMATION_CYCLE",0,5,IDL_SYSFUN_DEF_F_KEYWORDS,0},
   {reinterpret_cast<IDL_SYSRTN_GENERIC>(get_animation_state), "GET_ANIMATION_STATE",0,5,IDL_SYSFUN_DEF_F_KEYWORDS,0},
   {reinterpret_cast<IDL_SYSRTN_GENERIC>(get_interval_multiplier), "GET_INTERVAL_MULTIPLIER",0,5,IDL_SYSFUN_DEF_F_KEYWORDS,0},
   {reinterpret_cast<IDL_SYSRTN_GENERIC>(set_animation_cycle), "SET_ANIMATION_CYCLE",0,5,IDL_SYSFUN_DEF_F_KEYWORDS,0},
   {reinterpret_cast<IDL_SYSRTN_GENERIC>(set_animation_state), "SET_ANIMATION_STATE",0,5,IDL_SYSFUN_DEF_F_KEYWORDS,0},
   {reinterpret_cast<IDL_SYSRTN_GENERIC>(set_interval_multiplier), "SET_INTERVAL_MULTIPLIER",0,5,IDL_SYSFUN_DEF_F_KEYWORDS,0},
   {NULL, NULL, 0, 0, 0, 0}
};

bool addAnimationCommands()
{
   int funcCnt = -1;
   while (func_definitions[++funcCnt].name != NULL); // do nothing in the loop body
   return IDL_SysRtnAdd(func_definitions, IDL_TRUE, funcCnt) != 0;
}

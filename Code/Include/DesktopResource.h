/*
 * The information in this file is
 * Copyright(c) 2007 Ball Aerospace & Technologies Corporation
 * and is subject to the terms and conditions of the
 * GNU Lesser General Public License Version 2.1
 * The license text is available from   
 * http://www.gnu.org/licenses/lgpl.html
 */

#ifndef DESKTOPRESOURCE_H
#define DESKTOPRESOURCE_H

#include "AoiLayer.h"
#include "DesktopServices.h"
#include "GcpLayer.h"
#include "LatLonLayer.h"
#include "LayerList.h"
#include "PlotView.h"
#include "ProductView.h"
#include "PseudocolorLayer.h"
#include "RasterLayer.h"
#include "Resource.h"
#include "SpatialDataView.h"
#include "ThresholdLayer.h"
#include "TiePointLayer.h"
#include "TypeConverter.h"
#include "ViewWindow.h"
#include <QtGui/QWidget>

/**
 * The %ViewObject is a trait object for use with the %Resource template. 
 *
 * It provides capability for getting and returning views.
 * 
 * @see ViewResource
 */
class ViewObject
{
public:
   class Args
   {
   public:
      Args() :
         mGetExisting(true),
         mpParentWindow(NULL)
      {
      }

      Args(bool getExisting, const std::string& name, const std::string& classType) :
         mName(name),
         mGetExisting(getExisting),
         mpParentWindow(NULL)
      {
         if (classType == TypeConverter::toString<SpatialDataView>())
         {
            mType = SPATIAL_DATA_VIEW;
         }
         else if (classType == TypeConverter::toString<ProductView>())
         {
            mType = PRODUCT_VIEW;
         }
         //else if (classType == TypeConverter::toString<PlotView>())
         else if (classType == "PlotView")
         {
            mType = PLOT_VIEW;
         }
      }

      std::string mName;
      ViewType mType;
      bool mGetExisting;
      mutable ViewWindow* mpParentWindow;
   };

   void* obtainResource(const Args& args) const
   {
      if (!args.mType.isValid())
      {
         return NULL;
      }
      View* pView = NULL;
      Service<DesktopServices> pDesktop;
      if (args.mGetExisting)
      {
         if (args.mName.empty())
         {
            pView = pDesktop->getCurrentWorkspaceWindowView();
            if (pView != NULL && pView->getViewType() != args.mType)
            {
               pView = NULL;
            }
         }
         else
         {
            ViewWindow* pWindow = NULL;
            switch(args.mType)
            {
            case SPATIAL_DATA_VIEW:
               pWindow = static_cast<ViewWindow*>(pDesktop->getWindow(args.mName, SPATIAL_DATA_WINDOW));
               break;
            case PRODUCT_VIEW:
               pWindow = static_cast<ViewWindow*>(pDesktop->getWindow(args.mName, PRODUCT_WINDOW));
               break;
            case PLOT_VIEW:
               pWindow = static_cast<ViewWindow*>(pDesktop->getWindow(args.mName, PLOT_WINDOW));
               break;
            default:
               return NULL;
            }
            pView = (pWindow == NULL) ? NULL : pWindow->getView();
         }
      }
      if (pView == NULL)
      {
         ViewWindow* pWindow = NULL;
         switch(args.mType)
         {
         case SPATIAL_DATA_VIEW:
            pWindow = static_cast<ViewWindow*>(pDesktop->createWindow(args.mName, SPATIAL_DATA_WINDOW));
            break;
         case PRODUCT_VIEW:
            pWindow = static_cast<ViewWindow*>(pDesktop->createWindow(args.mName, PRODUCT_WINDOW));
            break;
         case PLOT_VIEW:
            pWindow = static_cast<ViewWindow*>(pDesktop->createWindow(args.mName, PLOT_WINDOW));
            break;
         default:
            return NULL;
         }
         pView = (pWindow == NULL) ? NULL : pWindow->getView();
         args.mpParentWindow = pWindow;
      }
      if (args.mpParentWindow == NULL && dynamic_cast<QWidget*>(pView) != NULL)
      {
         args.mpParentWindow = dynamic_cast<ViewWindow*>(dynamic_cast<QWidget*>(pView)->parent());
      }
      return pView;
   }
   void releaseResource(const Args& args, View* pObject) const
   {
      VERIFYNRV(args.mpParentWindow);
      Service<DesktopServices>()->deleteWindow(args.mpParentWindow);
   }
};

/**
 *  This is a %Resource class that wraps a View. 
 *
 *  When the %ViewResource object goes out of scope, the view will be deleted.
 */
template<class T>
class ViewResource : public Resource<T, ViewObject>
{
public:
   explicit ViewResource(const std::string& name, bool getExisting = true) :
      Resource<T, ViewObject>(Args(getExisting, name, TypeConverter::toString<T>()))
   {
      if (getArgs().mGetExisting)
      {
         release();
      }
   }

   /**
    * Create a ViewResource which owns the provided view and will
    * destroy it when the resource is destroyed.
    *
    * @param pView the view that will be owned by this resource.
    */
   explicit ViewResource(T* pView = NULL) :
      Resource<T, ViewObject>(pView, Args())
   {
   }
};

#endif

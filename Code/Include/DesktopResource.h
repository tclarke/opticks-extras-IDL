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
         mShouldRelease(false)
      {
      }

      Args(bool getExisting, const std::string& name, const std::string& classType) :
         mName(name),
         mGetExisting(getExisting),
         mShouldRelease(false)
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
      mutable bool mShouldRelease;
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
         if (pView != NULL)
         {
            // by default, we want a weak reference to existing objects
            args.mShouldRelease = true;
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
      }
      return pView;
   }
   void releaseResource(const Args& args, View* pObject) const
   {
      Service<DesktopServices>()->deleteWindow(
           dynamic_cast<ViewWindow*>(dynamic_cast<QWidget*>(pObject)->parent()));
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
      if (getArgs().mShouldRelease)
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

/**
 * The %LayerObject is a trait object for use with the %Resource template. 
 *
 * It provides capability for getting and returning layers.
 * 
 * @see LayerResource
 */
class LayerObject
{
public:
   class Args
   {
   public:
      Args() :
         mpElement(NULL),
         mpView(NULL),
         mGetExisting(true),
         mShouldRelease(false)
      {
      }

      Args(bool getExisting, const std::string& name, DataElement* pElement, const std::string& classType, SpatialDataView* pView) :
         mName(name),
         mpElement(pElement),
         mpView(pView),
         mGetExisting(getExisting),
         mShouldRelease(false)
      {
         if (classType == TypeConverter::toString<AnnotationLayer>())
         {
            mType = ANNOTATION;
         }
         else if (classType == TypeConverter::toString<AoiLayer>())
         {
            mType = AOI_LAYER;
         }
         else if (classType == TypeConverter::toString<GcpLayer>())
         {
            mType = GCP_LAYER;
         }
         else if (classType == TypeConverter::toString<LatLonLayer>())
         {
            mType = LAT_LONG;
         }
         else if (classType == TypeConverter::toString<PseudocolorLayer>())
         {
            mType = PSEUDOCOLOR;
         }
         else if (classType == TypeConverter::toString<RasterLayer>())
         {
            mType = RASTER;
         }
         else if (classType == TypeConverter::toString<ThresholdLayer>())
         {
            mType = THRESHOLD;
         }
         else if (classType == TypeConverter::toString<TiePointLayer>())
         {
            mType = TIEPOINT_LAYER;
         }
      }

      std::string mName;
      DataElement* mpElement;
      LayerType mType;
      SpatialDataView* mpView;
      bool mGetExisting;
      mutable bool mShouldRelease;
   };

   void* obtainResource(const Args& args) const
   {
      if (!args.mType.isValid() || args.mpView == NULL)
      {
         return NULL;
      }
      Layer* pLayer = NULL;
      LayerList* pList = args.mpView->getLayerList();
      if (args.mGetExisting)
      {
         if (args.mName.empty() && args.mpElement == NULL)
         {
            pLayer = args.mpView->getTopMostLayer(args.mType);
         }
         else
         {
            pLayer = pList->getLayer(args.mType, args.mpElement, args.mName);
         }
         if (pLayer != NULL)
         {
            // by default, we want a weak reference to existing objects
            args.mShouldRelease = true;
         }
      }
      if (pLayer == NULL && args.mpElement != NULL)
      {
         pLayer = args.mpView->createLayer(args.mType, args.mpElement, args.mName);
      }
      return pLayer;
   }
   void releaseResource(const Args& args, Layer* pObject) const
   {
      if (args.mpView != NULL)
      {
         args.mpView->deleteLayer(pObject);
      }
   }
};

/**
 *  This is a %Resource class that wraps a Layer. 
 *
 *  When the %LayerResource object goes out of scope, the Layer will be deleted.
 */
template<class T>
class LayerResource : public Resource<T, LayerObject>
{
public:
   explicit LayerResource(SpatialDataView* pView, const std::string& name, bool getExisting = true) :
      Resource<T, LayerObject>(Args(getExisting, name, NULL, TypeConverter::toString<T>(), pView))
   {
      if (getArgs().mShouldRelease)
      {
         release();
      }
   }

   explicit LayerResource(SpatialDataView* pView, DataElement* pElement, bool getExisting = true) :
      Resource<T, LayerObject>(Args(getExisting, std::string(), pElement, TypeConverter::toString<T>(), pView))
   {
      if (getArgs().mShouldRelease)
      {
         release();
      }
   }

   explicit LayerResource(SpatialDataView* pView, const std::string& name, DataElement* pElement, bool getExisting = true) :
      Resource<T, LayerObject>(Args(getExisting, name, pElement, TypeConverter::toString<T>(), pView))
   {
      if (getArgs().mShouldRelease)
      {
         release();
      }
   }

   /**
    * Create a LayerResource which owns the provided layer and will
    * destroy it when the resource is destroyed.
    *
    * @param pLayer the layer that will be owned by this resource.
    */
   explicit LayerResource(T* pLayer = NULL) :
      Resource<T, LayerObject>(pLayer, Args())
   {
   }
};

#endif

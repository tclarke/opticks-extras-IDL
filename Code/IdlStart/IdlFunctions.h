/*
 * The information in this file is
 * subject to the terms and conditions of the
 * GNU Lesser General Public License Version 2.1
 * The license text is available from   
 * http://www.gnu.org/licenses/lgpl.html
 */

#ifndef IDLFUNCTIONS_H
#define IDLFUNCTIONS_H

#include "DataAccessor.h"
#include "DataAccessorImpl.h"
#include "DataRequest.h"
#include "DataVariant.h"
#include "ObjectResource.h"
#include "RasterUtilities.h"
#include "TypesFile.h"
#include "Units.h"
#include <stdio.h>
#include <idl_export.h>
#include <vector>

class DataElement;
class DataRequest;
class DynamicObject;
class Layer;
class RasterDataDescriptor;
class RasterElement;
class RasterLayer;
class View;
class WizardObject;

/**
 * These are internal support methods not used in IDL.
 * \cond INTERNAL
 */
namespace IdlFunctions
{
   RasterElement* getDataset(const std::string& name = "");
   WizardObject* getWizardObject(const std::string& wizardName);
   bool setWizardObjectValue(WizardObject* pObject, const std::string& name, const DataVariant& value);
   DataVariant getWizardObjectValue(const WizardObject* pObject, const std::string& name);
   Layer* getLayerByName(const std::string& windowName, const std::string& layerName, bool onlyTopmostRaster = false);
   Layer* getLayerByRaster(RasterElement* pElement);
   Layer* getLayerByIndex(const std::string& windowName, int index);
   View* getViewByWindowName(const std::string& windowName);

   RasterElement* createRasterElement(void* pData, const std::string& datasetName,
      const std::string& newName, EncodingType datatype, InterleaveFormatType iType,
      unsigned int rows, unsigned int cols, unsigned int bands);
   bool changeRasterElement(RasterElement* pRasterElement, void* pData,
      EncodingType datatype, InterleaveFormatType iType, unsigned int startRow,
      unsigned int rows, unsigned int startCol, unsigned int cols,
      unsigned int startBand, unsigned int bands, EncodingType oldType);
   bool copyRasterElement(RasterElement* pRasterElement, RasterElement* pNewElement,
      EncodingType datatype, InterleaveFormatType iType, unsigned int startRow,
      unsigned int rows, unsigned int startCol, unsigned int cols,
      unsigned int startBand, unsigned int bands, EncodingType oldType);

   template<typename T>
   bool addMatrixToCurrentView(T* pMatrix, const std::string& name,
      unsigned int width, unsigned int height,
      unsigned int bands, const std::string& unit, EncodingType type,
      InterleaveFormatType ftype = BSQ,
      const std::string& filename = "")
   {
      bool bReturn  = false;
      SpatialDataView* pView = NULL;

      //get the current view and sensor data
      RasterElement* pElement = dynamic_cast<RasterElement*>(getDataset(filename));

      WorkspaceWindow* pData = Service<DesktopServices>()->getCurrentWorkspaceWindow();
      if (pData != NULL)
      {
         pView = dynamic_cast<SpatialDataView*>(pData->getView());
      }
      RasterElement* pRaster = NULL;

      //see if the results matrix already exists
      pRaster = static_cast<RasterElement*>(Service<ModelServices>()->getElement(name,
         TypeConverter::toString<RasterElement>(), pElement));
      if (pRaster == NULL)
      {
         //it doesn't exist, so we can make a new one
         pRaster = RasterUtilities::createRasterElement(name, height, width, bands, type, ftype, true, pElement);
         if (pRaster != NULL)
         {
            RasterDataDescriptor* pParam = static_cast<RasterDataDescriptor*>(pRaster->getDataDescriptor());
            if (pParam != NULL)
            {
               for (unsigned int b = 0; b < bands; ++b)
               {
                  try
                  {
                     ftype = pParam->getInterleaveFormat();
                     DataAccessor daImage = pRaster->getDataAccessor(
                           getNewDataRequest(pParam, 0, 0, height-1, width-1, b));

                     //populate the resultsmatrix with the data
                     for (unsigned int r = 0; r < height; ++r)
                     {
                        if (!daImage.isValid())
                        {
                           throw std::exception();
                        }
                        T* pPixel = reinterpret_cast<T*>(daImage->getRow());
                        for (unsigned int c = 0; c < width; ++c)
                        {
                           unsigned int dataOffset = width * height * b + width * r + c;
                           pPixel[c] = pMatrix[dataOffset];
                        }
                        daImage->nextRow();
                     }
                  }
                  catch(...)
                  {
                     IDL_Message(IDL_M_GENERIC, IDL_MSG_RET, "error in copying array values to Opticks.");
                     return false;
                  }
                  Units* pScale = pParam->getUnits();
                  pScale->setUnitType(CUSTOM_UNIT);
                  pScale->setUnitName(unit);
                  pParam->setUnits(pScale);
               }
            }
            //the matrix is now created, so we should add it to the current view
            if (pView != NULL)
            {
               UndoLock undo(pView);
               Layer* pLayer = pView->createLayer(RASTER, pRaster, name);
               if (pLayer != NULL)
               {
                  pView->addLayer(pLayer);
                  bReturn = true;
               }
            }
         }
         else
         {
            IDL_Message(IDL_M_GENERIC, IDL_MSG_RET, "matrix initialization failed.");
            return false;
         }
      }
      else
      {
         IDL_Message(IDL_M_GENERIC, IDL_MSG_RET, "matrix already exists.");
         return false;
      }
      return bReturn;
   }

   template<typename T>
   static void copySubcube(T* pData, RasterElement* pElement, unsigned int heightStart, unsigned int heightEnd,
      unsigned int widthStart, unsigned int widthEnd, unsigned int bandStart, unsigned int bandEnd)
   {
      unsigned int row = heightEnd - heightStart+1;
      unsigned int column = widthEnd - widthStart+1;
      unsigned int band = bandEnd - bandStart+1;
      pData = NULL;

      if (pElement != NULL)
      {
         unsigned int r = 0;
         unsigned int c = 0;
         unsigned int b = 0;
         try
         {
            pData = new T[row*column*band];
         }
         catch (...)
         {
            std::string msg = "Not enough memory to allocate array";
            IDL_Message(IDL_M_GENERIC, IDL_MSG_RET, msg.c_str());
            return;
         }
         try
         {
            InterleaveFormatType type = BSQ;
            RasterDataDescriptor* pParam = static_cast<RasterDataDescriptor*>(pElement->getDataDescriptor());
            //get the data from the disk, and store it into a new in memory block
            for (b = bandStart; b <= bandEnd; b++)
            {
               DataAccessor daImage = pElement->getDataAccessor(
                  getNewDataRequest(pParam, heightStart, widthStart, heightEnd, widthEnd, b));

               for (r = 0; r < row; ++r)
               {
                  if (!daImage.isValid())
                  {
                     throw std::exception();
                  }
                  T* pPixel = reinterpret_cast<T*>(daImage->getRow());
                  for (c = 0; c < column; ++c)
                  {
                     //unsigned int dataOffset = r * column + c;
                     unsigned int dataOffset = column * row * (b-bandStart) + column * r + c;
                     pData[dataOffset] = pPixel[c];
                  }
                  daImage->nextRow();
               }
            }
         }
         catch (...)
         {
            delete pData;
            pData = NULL;
            std::string msg = "error in copying array values to Opticks.";
            IDL_Message(IDL_M_GENERIC, IDL_MSG_RET, msg.c_str());
            return;
         }
      }
   }

   DataRequest* getNewDataRequest(RasterDataDescriptor* pParam,
      unsigned int rowStart, unsigned int colStart,
      unsigned int rowEnd, unsigned int colEnd,
      int band = -1);
   void copyLayer(RasterLayer* pLayer, const RasterLayer* pOrigLayer);
   RasterChannelType getRasterChannelType(const std::string& color);

   // Trevor, this container is accessed only in getWizardObject
   static std::vector<WizardObject*> mpWizards;
}
///\endcond INTERNAL

#endif
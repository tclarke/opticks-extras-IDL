/*
 * The information in this file is
 * Copyright(c) 2007 Ball Aerospace & Technologies Corporation
 * and is subject to the terms and conditions of the
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
   template<typename T>
   class IdlKwResource
   {
   public:
      IdlKwResource(int argc, IDL_VPTR* pArgv, char* pArgk,
         IDL_KW_PAR* pKw_list, IDL_VPTR* pPlainArgs, int mask)
      {
         IDL_KWProcessByOffset(argc, pArgv, pArgk, pKw_list, pPlainArgs, mask, &kw);
      }
      ~IdlKwResource()
      {
         IDL_KW_FREE; // This expects a var in the current scope called "kw"
      }

      operator T&() { return kw; }
      T* operator->() { return &kw; }

   private:
      T kw; // does not start with 'm' so IDL_KW_FREE will work
   };

   RasterElement* getDataset(const std::string& name = "");
   bool clearWizardObject(const std::string& wizardName);
   WizardObject* getWizardObject(const std::string& wizardName);
   void cleanupWizardObjects();
   bool setWizardObjectValue(WizardObject* pObject, const std::string& name, const DataVariant& value);
   DataVariant getWizardObjectValue(const WizardObject* pObject, const std::string& name);
   Layer* getLayerByName(const std::string& windowName, const std::string& layerName, bool onlyRasterElements = true);
   Layer* getLayerByRaster(RasterElement* pElement);
   Layer* getLayerByIndex(const std::string& windowName, int index);
   View* getViewByWindowName(const std::string& windowName);

   RasterElement* createRasterElement(char* pData, const std::string& datasetName,
      const std::string& newName, EncodingType datatype, bool inMemory, InterleaveFormatType iType,
      unsigned int rows, unsigned int cols, unsigned int bands);
   bool changeRasterElement(RasterElement* pRasterElement, char* pData,
      EncodingType datatype, InterleaveFormatType iType, unsigned int startRow,
      unsigned int rows, unsigned int startCol, unsigned int cols,
      unsigned int startBand, unsigned int bands, EncodingType oldType);

   template<typename T>
   bool addMatrixToCurrentView(T* pMatrix, const std::string& name,
      unsigned int width, unsigned int height,
      unsigned int bands, const std::string& unit, EncodingType type,
      bool inMemory, 
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
      if (pRaster != NULL)
      {
         IDL_Message(IDL_M_GENERIC, IDL_MSG_RET, "matrix already exists.");
         return false;
      }

      //it doesn't exist, so we can make a new one
      ModelResource<RasterElement> pRasterRes(RasterUtilities::createRasterElement(name, height, width, bands, type, ftype, inMemory, pElement));
      if (pRasterRes.get() == NULL)
      {
         IDL_Message(IDL_M_GENERIC, IDL_MSG_RET, "matrix initialization failed.");
         return false;
      }
      pRaster = pRasterRes.get();
      RasterDataDescriptor* pParam = static_cast<RasterDataDescriptor*>(pRaster->getDataDescriptor());
      if (pParam == NULL)
      {
         IDL_Message(IDL_M_GENERIC, IDL_MSG_RET, "error in copying array values to Opticks.");
         return false;
      }
      unsigned int bytesPerElement = pParam->getBytesPerElement();
      if (ftype == BSQ)
      {
         for (unsigned int band = 0; band < bands; ++band)
         {
            try
            {
               FactoryResource<DataRequest> pRequest;
               pRequest->setInterleaveFormat(BSQ);
               pRequest->setRows(pParam->getActiveRow(0), pParam->getActiveRow(height-1), 1);
               pRequest->setColumns(pParam->getActiveColumn(0), pParam->getActiveColumn(width-1), width);
               pRequest->setBands(pParam->getActiveBand(band), pParam->getActiveBand(band), 1);
               pRequest->setWritable(true);
               DataAccessor daImage = pRaster->getDataAccessor(pRequest.release());

               //populate the resultsmatrix with the data
               for (unsigned int row = 0; row < height; ++row)
               {
                  if (!daImage.isValid())
                  {
                     throw std::exception();
                  }
                  memcpy(daImage->getRow(), pMatrix + (width * height * band) + (width * row), width * bytesPerElement);
                  daImage->nextRow();
               }
            }
            catch(...)
            {
               IDL_Message(IDL_M_GENERIC, IDL_MSG_RET, "error in copying array values to Opticks.");
               return false;
            }
         }
      }
      else if (ftype == BIP)
      {
         try
         {
            FactoryResource<DataRequest> pRequest;
            pRequest->setInterleaveFormat(BIP);
            pRequest->setRows(pParam->getActiveRow(0), pParam->getActiveRow(height-1), 1);
            pRequest->setColumns(pParam->getActiveColumn(0), pParam->getActiveColumn(width-1), width);
            pRequest->setWritable(true);
            DataAccessor daImage = pRaster->getDataAccessor(pRequest.release());
            for (unsigned int row = 0; row < height; ++row)
            {
               if (!daImage.isValid())
               {
                  throw std::exception();
               }
               memcpy(daImage->getRow(), pMatrix + (row * width * bands), width * bands * bytesPerElement);
               daImage->nextRow();
            }
         }
         catch (...)
         {
            IDL_Message(IDL_M_GENERIC, IDL_MSG_RET, "error in copying array values to Opticks.");
            return false;
         }
      }
      else if (ftype == BIL)
      {
         try
         {
            FactoryResource<DataRequest> pRequest;
            pRequest->setInterleaveFormat(BIL);
            pRequest->setRows(pParam->getActiveRow(0), pParam->getActiveRow(height-1), 1);
            pRequest->setColumns(pParam->getActiveColumn(0), pParam->getActiveColumn(width-1), width);
            pRequest->setBands(pParam->getActiveBand(0), pParam->getActiveBand(bands-1), bands);
            pRequest->setWritable(true);
            DataAccessor daImage = pRaster->getDataAccessor(pRequest.release());
            for (unsigned int row = 0; row < height; ++row)
            {
               if (!daImage.isValid())
               {
                  throw std::exception();
               }
               memcpy(daImage->getRow(), pMatrix + (row * width * bands), width * bands * bytesPerElement);
               daImage->nextRow();
            }
         }
         catch (...)
         {
            IDL_Message(IDL_M_GENERIC, IDL_MSG_RET, "error in copying array values to Opticks.");
            return false;
         }
      }
      Units* pScale = pParam->getUnits();
      pScale->setUnitType(CUSTOM_UNIT);
      pScale->setUnitName(unit);
      pParam->setUnits(pScale);
      pRasterRes.release();
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
      return bReturn;
   }

   template<typename T>
   static void copySubcube(T* pData, RasterElement* pElement, unsigned int heightStart, unsigned int heightEnd,
      unsigned int widthStart, unsigned int widthEnd, unsigned int bandStart, unsigned int bandEnd)
   {
      unsigned int height = heightEnd - heightStart+1;
      unsigned int width = widthEnd - widthStart+1;
      unsigned int bands = bandEnd - bandStart+1;
      if (pElement != NULL)
      {
         try
         {
            RasterDataDescriptor* pParam = static_cast<RasterDataDescriptor*>(pElement->getDataDescriptor());
            unsigned int bytesPerElement = pParam->getBytesPerElement();
            InterleaveFormatType interleave = pParam->getInterleaveFormat();
            if (interleave == BSQ)
            {
               //get the data from the disk, and store it into a new in memory block
               for (unsigned int band = 0; band < bands; ++band)
               {
                  FactoryResource<DataRequest> pRequest;
                  pRequest->setInterleaveFormat(BSQ);
                  pRequest->setRows(pParam->getActiveRow(heightStart), pParam->getActiveRow(heightEnd), 1);
                  pRequest->setColumns(pParam->getActiveColumn(widthStart), pParam->getActiveColumn(widthEnd), width);
                  pRequest->setBands(pParam->getActiveBand(bandStart + band), pParam->getActiveBand(bandStart + band), 1);
                  DataAccessor daImage = pElement->getDataAccessor(pRequest.release());

                  for (unsigned int row = 0; row < height; ++row)
                  {
                     for (unsigned int col = 0; col < width; ++col)
                     {
                        daImage->toPixel(heightStart + row, widthStart + col);
                        if (!daImage.isValid())
                        {
                           throw std::exception();
                        }
                        memcpy(pData + (width * height * band) + (width * row) + col, daImage->getColumn(), bytesPerElement);
                     }
                  }
               }
            }
            else if (interleave == BIP)
            {
               FactoryResource<DataRequest> pRequest;
               pRequest->setInterleaveFormat(BIP);
               pRequest->setRows(pParam->getActiveRow(heightStart), pParam->getActiveRow(heightEnd), 1);
               pRequest->setColumns(pParam->getActiveColumn(widthStart), pParam->getActiveColumn(widthEnd), width);
               DataAccessor daImage = pElement->getDataAccessor(pRequest.release());
               for (unsigned int row = 0; row < height; ++row)
               {
                  for (unsigned int col = 0; col < width; ++col)
                  {
                     daImage->toPixel(heightStart + row, widthStart + col);
                     if (!daImage.isValid())
                     {
                        throw std::exception();
                     }
                     for (unsigned int band = 0; band < bands; ++band)
                     {
                        memcpy(pData + (row * width * bands) + (col * bands) + band, static_cast<T*>(daImage->getColumn()) + (band + bandStart), bytesPerElement);
                     }
                  }
               }
            }
            else if (interleave == BIL)
            {
               for (unsigned int row = 0; row < height; ++row)
               {
                  for (unsigned int band = 0; band < bands; ++band)
                  {
                     FactoryResource<DataRequest> pRequest;
                     pRequest->setInterleaveFormat(BIL);
                     pRequest->setRows(pParam->getActiveRow(heightStart + row), pParam->getActiveRow(heightStart + row), 1);
                     pRequest->setColumns(pParam->getActiveColumn(widthStart), pParam->getActiveColumn(widthEnd), width);
                     pRequest->setBands(pParam->getActiveBand(bandStart + band), pParam->getActiveBand(bandStart + band), 1);
                     DataAccessor daImage = pElement->getDataAccessor(pRequest.release());
                     for (unsigned int col = 0; col < width; ++col)
                     {
                        daImage->toPixel(heightStart + row, widthStart + col);
                        if (!daImage.isValid())
                        {
                           throw std::exception();
                        }
                        memcpy(pData + (row * width * bands) + (band * width) + col, daImage->getColumn(), bytesPerElement);
                     }
                  }
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

   RasterChannelType getRasterChannelType(const std::string& color);

   static std::vector<WizardObject*> spWizards;
}
///\endcond INTERNAL

#endif
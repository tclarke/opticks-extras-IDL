/*
 * The information in this file is
 * Copyright(c) 2007 Ball Aerospace & Technologies Corporation
 * and is subject to the terms and conditions of the
 * GNU Lesser General Public License Version 2.1
 * The license text is available from   
 * http://www.gnu.org/licenses/lgpl.html
 */

#include "DataAccessor.h"
#include "DataAccessorImpl.h"
#include "DataRequest.h"
#include "DesktopResource.h"
#include "DesktopServices.h"
#include "DynamicObject.h"
#include "IdlFunctions.h"
#include "LayerList.h"
#include "ModelServices.h"
#include "ObjectResource.h"
#include "RasterDataDescriptor.h"
#include "RasterElement.h"
#include "RasterFileDescriptor.h"
#include "RasterLayer.h"
#include "RasterUtilities.h"
#include "SpatialDataView.h"
#include "SpatialDataWindow.h"
#include "StringUtilities.h"
#include "switchOnEncoding.h"
#include "TypeConverter.h"
#include "WizardItem.h"
#include "WizardObject.h"
#include "xmlreader.h"
#include <stdio.h>
#include <idl_export.h>
#include <QtCore/QString>
#include <QtCore/QStringList>
#include <string>
#include <vector>

RasterElement* IdlFunctions::getDataset(const std::string& name)
{
   DataElement* pElement = NULL;
   if (name.empty())
   {
      ViewResource<SpatialDataView> pView(name, true);
      if (pView.get() == NULL)
      {
         IDL_Message(IDL_M_GENERIC, IDL_MSG_RET, "No spatial data window selected.");
         return NULL;
      }
      LayerList* pList = pView->getLayerList();
      pElement = (pList == NULL) ? NULL : pList->getPrimaryRasterElement();
      pView.release();
   }
   else
   {
      QString str = QString::fromStdString(name);
      QStringList list = str.split(QString("=>"));

      bool first = true;
      Service<ModelServices> pModel;
      while (!list.isEmpty())
      {
         QString partName = list.front();
         list.pop_front();

         pElement = static_cast<RasterElement*>(pModel->getElement(partName.toStdString(), "", pElement));
         if (pElement == NULL && first)
         {
            //the element was not found to be top level
            //check for an element with the => in the name
            std::vector<DataElement*> elements = pModel->getElements("");
            for (std::vector<DataElement*>::iterator element = elements.begin();
               element != elements.end(); ++element)
            {
               if ((*element)->getName() == name)
               {
                  pElement = *element;
                  break;
               }
            }
         }
         first = false;
      }
   }
   return dynamic_cast<RasterElement*>(pElement);
}

bool IdlFunctions::setWizardObjectValue(WizardObject* pObject, const std::string& name, const DataVariant& value)
{
   if (pObject == NULL)
   {
      return false;
   }
   std::vector<std::string> parts = StringUtilities::split(name, '/');
   if (parts.size() != 2)
   {
      return false;
   }
   std::string itemName = parts[0];
   std::string nodeName = parts[1];

   const std::vector<WizardItem*>& items = pObject->getItems();
   for (std::vector<WizardItem*>::const_iterator item = items.begin(); item != items.end(); ++item)
   {
      if ((*item)->getName() == itemName)
      {
         WizardNode* pNode = (*item)->getOutputNode(nodeName, value.getTypeName());
         if (pNode != NULL)
         {
            pNode->setValue(value.getPointerToValueAsVoid());
            return true;
         }
         else if (value.getTypeName() == TypeConverter::toString<std::string>())
         {
            pNode = (*item)->getOutputNode(nodeName, TypeConverter::toString<Filename>());
            if (pNode != NULL)
            {
               FactoryResource<Filename> pFilename;
               pFilename->setFullPathAndName(dv_cast<std::string>(value));
               pNode->setValue(pFilename.release());
               return true;
            }
         }
         break;
      }
   }
   return false;
}

DataVariant IdlFunctions::getWizardObjectValue(const WizardObject* pObject, const std::string& name)
{
   if (pObject == NULL)
   {
      return DataVariant();
   }
   std::vector<std::string> parts = StringUtilities::split(name, '/');
   if (parts.size() != 2)
   {
      return DataVariant();
   }
   std::string itemName = parts[0];
   std::string nodeName = parts[1];

   const std::vector<WizardItem*>& items = pObject->getItems();
   for (std::vector<WizardItem*>::const_iterator item = items.begin(); item != items.end(); ++item)
   {
      if ((*item)->getName() == itemName)
      {
         std::vector<WizardNode*> nodes = (*item)->getOutputNodes();
         for (std::vector<WizardNode*>::const_iterator node = nodes.begin(); node != nodes.end(); ++node)
         {
            if ((*node)->getName() == nodeName)
            {
               return DataVariant((*node)->getType(), (*node)->getValue(), false);
            }
         }
      }
   }

   return DataVariant();
}

RasterElement* IdlFunctions::createRasterElement(void* pData,
                                                 const std::string& datasetName,
                                                 const std::string& newName,
                                                 EncodingType datatype, 
                                                 InterleaveFormatType iType,
                                                 unsigned int rows,
                                                 unsigned int cols,
                                                 unsigned int bands)
{
   RasterElement* pInputRaster = getDataset(datasetName);
   DataElement* pParent = NULL;
   if (pInputRaster != NULL)
   {
      pParent = pInputRaster->getParent();
   }
   ModelResource<RasterElement> pRaster(
      RasterUtilities::createRasterElement(newName, rows, cols, bands, datatype, iType, true, pParent));
   if (pRaster.get() == NULL)
   {
      IDL_Message(IDL_M_GENERIC, IDL_MSG_RET, "Could not create new RasterElement, may already exist.");
      return NULL;
   }
   RasterDataDescriptor* pDesc = static_cast<RasterDataDescriptor*>(pRaster->getDataDescriptor());
   if (pDesc != NULL)
   {
      if (pData != NULL)
      {
         try
         {
            size_t rowSize = pDesc->getBytesPerElement() * cols;
            //populate the resultsmatrix with the data
            for (unsigned int band = 0; band < bands; ++band)
            {
               DataAccessor daImage = pRaster->getDataAccessor(getNewDataRequest(pDesc, 0, 0, rows-1, cols-1, band));
               for (unsigned int row = 0; row < rows; ++row)
               {
                  if (!daImage.isValid())
                  {
                     throw std::exception();
                  }
                  memcpy(daImage->getRow(), pData, rowSize);
                  daImage->nextRow();
               }
            }
         }
         catch (...)
         {
            std::string msg = "error in copying array values to Opticks.";
            IDL_Message(IDL_M_GENERIC, IDL_MSG_RET, msg.c_str());
            return false;
         }
      }
      if (pInputRaster != NULL)
      {
         RasterDataDescriptor* pDesc = static_cast<RasterDataDescriptor*>(pRaster->getDataDescriptor());
         RasterDataDescriptor* pInputDesc = static_cast<RasterDataDescriptor*>(pInputRaster->getDataDescriptor());
         VERIFY(pDesc != NULL && pInputDesc != NULL);
         //----- Set the Bands Info
         std::vector<DimensionDescriptor> bandsVec = RasterUtilities::generateDimensionVector(bands, true, true);
         pDesc->setBands(bandsVec);
      }
   }
   return pRaster.release();
}

bool IdlFunctions::changeRasterElement(RasterElement* pRasterElement, void* pData,
                                       EncodingType datatype, InterleaveFormatType iType, unsigned int startRow, 
                                       unsigned int rows, unsigned int startCol, unsigned int cols, 
                                       unsigned int startBand, unsigned int bands, EncodingType oldType)
{
   int index = 0;
   unsigned int i = 0;
   bool bSuccess = false;

   if (pRasterElement != NULL && pData != NULL)
   {
      RasterDataDescriptor* pDesc = static_cast<RasterDataDescriptor*>(pRasterElement->getDataDescriptor());
      if (pDesc == NULL)
      {
         return false;
      }

      try
      {
         std::size_t rowSize = pDesc->getBytesPerElement() * cols;
         for (unsigned int band = startBand; band < startBand+bands; ++band)
         {
            DataRequest* pRequest = getNewDataRequest(pDesc, startRow, startCol,
               rows + startRow - 1, cols + startCol - 1, band);

            DataAccessor daImage = pRasterElement->getDataAccessor(pRequest);

            for (unsigned int row = startRow; row < startRow+rows; ++row)
            {
               if (!daImage.isValid())
               {
                  throw std::exception();
               }
               memcpy(daImage->getRow(), pData, rowSize);
               daImage->nextRow();
            }
         }
      }
      catch (...)
      {
         std::string msg = "error in copying array values to Opticks.";
         IDL_Message(IDL_M_GENERIC, IDL_MSG_RET, msg.c_str());
         return false;
      }
      pRasterElement->updateData();
   }

   return bSuccess;
}

Layer* IdlFunctions::getLayerByRaster(RasterElement* pElement)
{
   VERIFYRV(pElement != NULL, NULL);
   Layer* pDatasetLayer = NULL;
   std::vector<Window*> windows;
   Service<DesktopServices>()->getWindows(SPATIAL_DATA_WINDOW, windows);

   for (std::vector<Window*>::const_iterator windowIter = windows.begin();
      windowIter != windows.end(); ++windowIter)
   {
      SpatialDataWindow* pSpatialDataWindow = dynamic_cast<SpatialDataWindow*>(*windowIter);
      if (pSpatialDataWindow != NULL)
      {
         SpatialDataView* pSpatialDataView = dynamic_cast<SpatialDataView*>(pSpatialDataWindow->getView());
         VERIFYRV(pSpatialDataView != NULL, NULL);

         LayerList* pLayerList = pSpatialDataView->getLayerList();
         VERIFYRV(pLayerList != NULL, NULL);

         // Retrieve all the raster layers in the current window
         std::vector<Layer*> layers;
         pLayerList->getLayers(RASTER, layers);
         for (std::vector<Layer*>::const_iterator layerIter = layers.begin();
            layerIter != layers.end(); ++layerIter)
         {
            RasterLayer* pRasterLayer = dynamic_cast<RasterLayer*>(*layerIter);
            VERIFYRV(pRasterLayer != NULL, NULL);

            DataElement* pDataElement = pRasterLayer->getDataElement();
            if (pDataElement == pElement)
            {
               pDatasetLayer = pRasterLayer;
               break;
            }
         }
      }
   }
   return pDatasetLayer;
}

Layer* IdlFunctions::getLayerByName(const std::string& windowName,
                                    const std::string& layerName,
                                    bool onlyRasterElements)
{
   Layer* pReturn = NULL;

   SpatialDataView* pView = dynamic_cast<SpatialDataView*>(getViewByWindowName(windowName));
   if (pView != NULL)
   {
      LayerList* pList = pView->getLayerList();
      if (pList != NULL)
      {
         if (onlyRasterElements)
         {
            pReturn = pView->getTopMostLayer(RASTER);
         }
         else
         {
            pReturn = pView->getTopMostLayer();
         }
         std::vector<Layer*> layers;
         pList->getLayers(layers);
         Layer* pLayer = NULL;

         if (layerName.empty())
         {
            std::string tmpName;
            bool bFound = false;
            //traverse the list of layers looking for a layer that matches the passed in name
            for (std::vector<Layer*>::const_iterator iter = layers.begin(); iter != layers.end(); ++iter)
            {
               pLayer = *iter;
               if (pLayer != NULL)
               {
                  tmpName = pLayer->getName();
                  if (tmpName == layerName)
                  {
                     pReturn = pLayer;
                     bFound = true;
                     break;
                  }
               }
            }
            if (!bFound)
            {
               pReturn = NULL;
            }
         }
      }
   }
   return pReturn;
}

Layer* IdlFunctions::getLayerByIndex(const std::string& windowName, int index)
{
   Layer* pReturn = NULL;

   SpatialDataView* pView = dynamic_cast<SpatialDataView*>(getViewByWindowName(windowName));
   if (pView != NULL)
   {
      //get the current layer if the index is invalid
      if (index < 0)
      {
         pReturn = pView->getTopMostLayer();
      }
      else
      {
         LayerList* pList = pView->getLayerList();
         if (pList != NULL)
         {
            std::vector<Layer*> layers;
            pList->getLayers(layers);
            if (index < static_cast<int>(layers.size()))
            {
               pReturn = layers.at(index);
            }
         }
      }
   }

   return pReturn;
}

View* IdlFunctions::getViewByWindowName(const std::string& windowName)
{
   Service<DesktopServices> pDesktop;
   SpatialDataWindow* pWindow = NULL;
   if (windowName.empty())
   {
      pWindow = dynamic_cast<SpatialDataWindow*>(pDesktop->getWindow(windowName.c_str(), SPATIAL_DATA_WINDOW));
   }
   else 
   {
      pWindow = dynamic_cast<SpatialDataWindow*>(pDesktop->getCurrentWorkspaceWindow());
   }
   if (pWindow == NULL)
   {
      IDL_Message(IDL_M_GENERIC, IDL_MSG_RET, "Opticks was unable to determine the current window.");
      return (NULL);
   }

   return (pWindow->getSpatialDataView());
}

WizardObject* IdlFunctions::getWizardObject(const std::string& wizardName)
{
   WizardObject* pWizard = NULL;

   //traverse the list of items, looking for the one that matches the item name
   for (std::vector<WizardObject*>::const_iterator iter = spWizards.begin(); iter != spWizards.end(); ++iter)
   {
      WizardObject* pItem = *iter;
      if (pItem != NULL)
      {
         if (pItem->getName() == wizardName)
         {
            pWizard = pItem;
            break;
         }
      }
   }
   if (pWizard == NULL)
   {
      //the wizard object doesn't exist, read it from the file
      FactoryResource<WizardObject> pNewWizard;
      VERIFYRV(pNewWizard.get() != NULL, NULL);

      FactoryResource<Filename> pWizardFilename;
      pWizardFilename->setFullPathAndName(wizardName);

      bool bSuccess = false;
      XmlReader xml;
      XERCES_CPP_NAMESPACE_QUALIFIER DOMDocument* pDocument = xml.parse(pWizardFilename.get());
      if (pDocument != NULL)
      {
         XERCES_CPP_NAMESPACE_QUALIFIER DOMElement* pRoot = pDocument->getDocumentElement();
         if (pRoot != NULL)
         {
            unsigned int version = atoi(A(pRoot->getAttribute(X("version"))));
            bSuccess = pNewWizard->fromXml(pRoot, version);
         }
      }

      if (bSuccess == false)
      {
         return NULL;
      }
      pWizard = pNewWizard.release();
      pWizard->setName(wizardName);
      spWizards.push_back(pWizard);
   }
   return pWizard;
}

void IdlFunctions::cleanupWizardObjects()
{
   for (std::vector<WizardObject*>::const_iterator iter = spWizards.begin(); iter != spWizards.end(); ++iter)
   {
      FactoryResource<WizardObject> pWizard(*iter);
   }
   spWizards.clear();
}

DataRequest* IdlFunctions::getNewDataRequest(RasterDataDescriptor* pParam,
                                             unsigned int rowStart, unsigned int colStart,
                                             unsigned int rowEnd, unsigned int colEnd,
                                             int band)
{
   FactoryResource<DataRequest> pRequest;
   if (pParam != NULL)
   {
      InterleaveFormatType type = pParam->getInterleaveFormat();
      if (band < 0 && type != BIP)
      {
         type = BIP;
      }
      else if (band >= 0 && type == BIP)
      {
         type = BSQ;
      }
      pRequest->setInterleaveFormat(type);
      pRequest->setRows(pParam->getActiveRow(rowStart), pParam->getActiveRow(rowEnd));
      pRequest->setColumns(pParam->getActiveColumn(colStart), pParam->getActiveColumn(colEnd));
      if (type != BIP)
      {
         pRequest->setBands(pParam->getActiveBand(band), pParam->getActiveBand(band));
      }
      pRequest->setWritable(true);
   }

   return pRequest.release();
}

void IdlFunctions::copyLayer(RasterLayer* pLayer, const RasterLayer* pOrigLayer)
{
   if (pLayer != NULL && pOrigLayer != NULL)
   {
      pLayer->setAnimation(pOrigLayer->getAnimation());
      pLayer->setXOffset(pOrigLayer->getXOffset());
      pLayer->setYOffset(pOrigLayer->getYOffset());
      pLayer->setXScaleFactor(pOrigLayer->getXScaleFactor());
      pLayer->setYScaleFactor(pOrigLayer->getYScaleFactor());
      pLayer->enableFilters(pOrigLayer->getEnabledFilterNames());
      pLayer->setAlpha(pOrigLayer->getAlpha());
      pLayer->setColorMap(pOrigLayer->getColorMapName(), pOrigLayer->getColorMap());
      pLayer->setComplexComponent(pOrigLayer->getComplexComponent());
      pLayer->enableGpuImage(pOrigLayer->isGpuImageEnabled());
   }
}

RasterChannelType IdlFunctions::getRasterChannelType(const std::string& color)
{
   RasterChannelType element = GRAY;
   if (color == "RED")
   {
      element = RED;
   }
   else if (color == "GREEN")
   {
      element = GREEN;
   }
   else if (color == "BLUE")
   {
      element = BLUE;
   }
   return element;
}

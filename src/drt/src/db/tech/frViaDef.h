/* Authors: Lutong Wang and Bangqi Xu */
/*
 * Copyright (c) 2019, The Regents of the University of California
 * All rights reserved.
 *
 * Redistribution and use in source and binary forms, with or without
 * modification, are permitted provided that the following conditions are met:
 *     * Redistributions of source code must retain the above copyright
 *       notice, this list of conditions and the following disclaimer.
 *     * Redistributions in binary form must reproduce the above copyright
 *       notice, this list of conditions and the following disclaimer in the
 *       documentation and/or other materials provided with the distribution.
 *     * Neither the name of the University nor the
 *       names of its contributors may be used to endorse or promote products
 *       derived from this software without specific prior written permission.
 *
 * THIS SOFTWARE IS PROVIDED BY THE COPYRIGHT HOLDERS AND CONTRIBUTORS "AS IS"
 * AND ANY EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT NOT LIMITED TO, THE
 * IMPLIED WARRANTIES OF MERCHANTABILITY AND FITNESS FOR A PARTICULAR PURPOSE
 * ARE DISCLAIMED. IN NO EVENT SHALL THE REGENTS BE LIABLE FOR ANY DIRECT,
 * INDIRECT, INCIDENTAL, SPECIAL, EXEMPLARY, OR CONSEQUENTIAL DAMAGES
 * (INCLUDING, BUT NOT LIMITED TO, PROCUREMENT OF SUBSTITUTE GOODS OR SERVICES;
 * LOSS OF USE, DATA, OR PROFITS; OR BUSINESS INTERRUPTION) HOWEVER CAUSED AND
 * ON ANY THEORY OF LIABILITY, WHETHER IN CONTRACT, STRICT LIABILITY, OR TORT
 * (INCLUDING NEGLIGENCE OR OTHERWISE) ARISING IN ANY WAY OUT OF THE USE OF THIS
 * SOFTWARE, EVEN IF ADVISED OF THE POSSIBILITY OF SUCH DAMAGE.
 */

#ifndef _FR_VIADEF_H_
#define _FR_VIADEF_H_

#include <iostream>
#include <memory>
#include <vector>

#include "db/obj/frShape.h"
#include "frBaseTypes.h"
#include "odb/db.h"
#include "utl/Logger.h"

namespace fr {
class frLef58CutClass
{
 public:
  // constructors
  frLef58CutClass() : rule_(nullptr) {}
  // getters
  odb::dbTechLayerCutClassRule* getDbTechLayerCutClassRule() const
  {
    return rule_;
  }
  void getName(std::string& in) const { in = rule_->getName(); }
  std::string getName() const { return rule_->getName(); }
  frCoord getViaWidth() const { return rule_->getWidth(); }
  bool hasViaLength() const { return rule_->isLengthValid(); }
  frCoord getViaLength() const
  {
    return (rule_->isLengthValid()) ? rule_->getLength() : rule_->getWidth();
  }
  frUInt4 getNumCut() const
  {
    return (rule_->isCutsValid()) ? rule_->getNumCuts() : 1;
  }
  // setters
  void setDbTechLayerCutClassRule(odb::dbTechLayerCutClassRule* ruleIn)
  {
    rule_ = ruleIn;
  }
  void report(utl::Logger* logger)
  {
    logger->report("CUTCLASS name {} viaWidth {} viaLength {} numCut {}",
                   getName(),
                   getViaWidth(),
                   getViaLength(),
                   getNumCut());
  }

 private:
  odb::dbTechLayerCutClassRule* rule_;
};

class frViaDef
{
 public:
  // constructors
  frViaDef()
      : id_(0),
        name(),
        isDefault(false),
        layer1Figs(),
        layer2Figs(),
        cutFigs(),
        cutClass(nullptr),
        cutClassIdx(-1),
        addedByRouter(false),
        layer1ShapeBox(),
        layer2ShapeBox(),
        cutShapeBox()
  {
  }
  frViaDef(const std::string& nameIn)
      : id_(0),
        name(nameIn),
        isDefault(false),
        layer1Figs(),
        layer2Figs(),
        cutFigs(),
        cutClass(nullptr),
        cutClassIdx(-1),
        addedByRouter(false),
        layer1ShapeBox(),
        layer2ShapeBox(),
        cutShapeBox()
  {
  }
  // getters
  void getName(std::string& nameIn) const { nameIn = name; }
  std::string getName() const { return name; }
  frLayerNum getLayer1Num() const
  {
    if (layer1Figs.size()) {
      return (layer1Figs.at(0))->getLayerNum();
    } else {
      std::cout << "Error: via does not have shape on layer 1" << std::endl;
      exit(1);
    }
  }
  frLayerNum getLayer2Num() const
  {
    if (layer2Figs.size()) {
      return (layer2Figs.at(0))->getLayerNum();
    } else {
      std::cout << "Error: via does not have shape on layer 2" << std::endl;
      exit(1);
    }
  }
  frLayerNum getCutLayerNum() const
  {
    if (cutFigs.size()) {
      return (cutFigs.at(0))->getLayerNum();
    } else {
      std::cout << "Error: via does not have shape on layer cut" << std::endl;
      exit(1);
    }
  }
  const std::vector<std::unique_ptr<frShape>>& getLayer1Figs() const
  {
    return layer1Figs;
  }
  const std::vector<std::unique_ptr<frShape>>& getLayer2Figs() const
  {
    return layer2Figs;
  }
  const std::vector<std::unique_ptr<frShape>>& getCutFigs() const
  {
    return cutFigs;
  }
  bool getDefault() const { return isDefault; }
  int getNumCut() const { return cutFigs.size(); }
  bool hasCutClass() const { return (cutClass != nullptr); }
  frLef58CutClass* getCutClass() const { return cutClass; }
  int getCutClassIdx() const { return cutClassIdx; }
  bool isAddedByRouter() const { return addedByRouter; }
  bool isMultiCut() const { return (cutFigs.size() > 1) ? true : false; }
  // setters
  void addLayer1Fig(std::unique_ptr<frShape> figIn)
  {
    Rect box = figIn->getBBox();
    layer1ShapeBox.merge(box);
    layer1Figs.push_back(std::move(figIn));
  }
  void addLayer2Fig(std::unique_ptr<frShape> figIn)
  {
    Rect box = figIn->getBBox();
    layer2ShapeBox.merge(box);
    layer2Figs.push_back(std::move(figIn));
  }
  void addCutFig(std::unique_ptr<frShape> figIn)
  {
    Rect box = figIn->getBBox();
    cutShapeBox.merge(box);
    cutFigs.push_back(std::move(figIn));
  }
  void setDefault(bool isDefaultIn) { isDefault = isDefaultIn; }
  void setCutClass(frLef58CutClass* in) { cutClass = in; }
  void setCutClassIdx(int in) { cutClassIdx = in; }
  void setAddedByRouter(bool in) { addedByRouter = in; }
  const Rect& getLayer1ShapeBox() { return layer1ShapeBox; }
  const Rect& getLayer2ShapeBox() { return layer2ShapeBox; }
  const Rect& getCutShapeBox() { return cutShapeBox; }
  const Rect& getShapeBox(frLayerNum lNum)
  {
    if (lNum == getLayer1Num())
      return layer1ShapeBox;
    if (lNum == getLayer2Num())
      return layer2ShapeBox;
    throw std::invalid_argument("Error: via does not have shape on layer "
                                + std::to_string(lNum));
  }
  void setId(int in) { id_ = in; }
  int getId() const { return id_; }

 private:
  int id_;
  std::string name;
  bool isDefault;
  std::vector<std::unique_ptr<frShape>> layer1Figs;
  std::vector<std::unique_ptr<frShape>> layer2Figs;
  std::vector<std::unique_ptr<frShape>> cutFigs;
  frLef58CutClass* cutClass;
  int cutClassIdx;
  bool addedByRouter;

  Rect layer1ShapeBox;
  Rect layer2ShapeBox;
  Rect cutShapeBox;
};
}  // namespace fr
#endif
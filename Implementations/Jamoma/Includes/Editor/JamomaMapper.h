/*!
 * \file JamomaMapper.h
 *
 * \brief
 *
 * \details
 *
 * \author Théo de la Hogue
 *
 * \copyright This code is licensed under the terms of the "CeCILL-C"
 * http://www.cecill.info
 */

#pragma once

#include "Editor/Mapper.h"
#include "Editor/Curve.h"
#include "Editor/Message.h"
#include "Editor/TimeConstraint.h"
#include "Editor/TimeNode.h"
#include "Editor/TimeValue.h"
#include "Editor/Value.h"
#include "Network/Address.h"

#include "JamomaTimeProcess.h"

using namespace OSSIA;
using namespace std;

class JamomaMapper : public Mapper, public JamomaTimeProcess
{
  
private:
  
# pragma mark -
# pragma mark Implementation specific
  
  shared_ptr<Address>   mDriverAddress;
  shared_ptr<Address>   mDrivenAddress;
  Value *               mDrive = nullptr;
  
  shared_ptr<Message>   mMessageToSend;
  Value*                mValueToSend = nullptr;
  
public:
  
# pragma mark -
# pragma mark Life cycle
  
  JamomaMapper(shared_ptr<Address>,
               shared_ptr<Address>,
               const Value*);
  
  JamomaMapper(const JamomaMapper *);
  
  shared_ptr<Mapper> clone() const override;
  
  ~JamomaMapper();

# pragma mark -
# pragma mark Execution

  shared_ptr<StateElement> state(const TimeValue&, const TimeValue&) override;
  
# pragma mark -
# pragma mark Accessors
  
  const shared_ptr<Address> getDriverAddress() const override;
  
  const shared_ptr<Address> getDrivenAddress() const override;
  
  const Value * getDriving() const override;
  
private:
  
# pragma mark -
# pragma mark Implementation specific
  
  vector<const TimeValue> computePositions(const Value*);
  
  Value* computeValueAtPositions(const Value* drive, vector<const TimeValue&> position);
};

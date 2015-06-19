#include "Editor/JamomaTimeEvent.h"
#include "Editor/State.h"

# pragma mark -
# pragma mark Life cycle

JamomaTimeEvent::JamomaTimeEvent(shared_ptr<Expression> anExpression) :
mExpression(anExpression)
{
  mState = State::create();
}

JamomaTimeEvent::~JamomaTimeEvent()
{}

# pragma mark -
# pragma mark Execution

void JamomaTimeEvent::play(bool log, string name) const
{
  mState->launch();
  // note : what else ?
}

# pragma mark -
# pragma mark Edition

void JamomaTimeEvent::addState(const std::shared_ptr<State> state)
{
  // store a state if it is not already stored
  if (find(mState->stateElements().begin(),
           mState->stateElements().end(),
           state) == mState->stateElements().end())
  {
    mState->stateElements().push_back(state);
  }
}

void JamomaTimeEvent::removeState(const std::shared_ptr<State> state)
{
  mState->stateElements().erase(find(mState->stateElements().begin(), mState->stateElements().end(), state));
}

# pragma mark -
# pragma mark Accessors

const shared_ptr<State> & JamomaTimeEvent::getState() const
{
  return mState;
}

const shared_ptr<Expression> & JamomaTimeEvent::getExpression() const
{
  return mExpression;
}

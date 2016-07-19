#pragma once
#include <Editor/Value/ValueBase.h>
#include <string>

namespace OSSIA
{

/*! \details String value */
struct OSSIA_EXPORT String final : public Value
{
  std::string value;

  /*! constructor
   \param std::string value */
  String(std::string v = "") :
      Value{Type::STRING},
      value(std::move(v))
  {
  }
  virtual ~String();
  String(const String&) = delete;

  /*! clone */
  Value * clone() const override;

  /*! equal operator */
  bool operator== (const Value&) const override;

  /*! different operator */
  bool operator!= (const Value&) const override;

  /*! greater than operator */
  bool operator> (const Value&) const override;

  /*! greater than and equal operator */
  bool operator>= (const Value&) const override;

  /*! less than operator */
  bool operator< (const Value&) const override;

  /*! less than and equal operator */
  bool operator<= (const Value&) const override;
};

}

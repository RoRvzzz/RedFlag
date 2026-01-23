#include "../../../../main.h"
#include "../../../../../misc/Umodule/Umodule.hpp"
#include "../../../Classes/offsets/offsets.hpp"
void RBX::Instance::SetFloatValue(float value) const
{
    Umodule::write<float>(this->address + Offsets::Value, value);
}
float RBX::Instance::GetFloatFromValue() const
{
  return  Umodule::read<float>(this->address +  Offsets::Value);
}
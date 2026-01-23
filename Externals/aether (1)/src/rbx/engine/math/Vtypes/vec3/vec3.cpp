#include "../../../../main.h"
#include "../../../../../misc/Umodule/Umodule.hpp"
#include "../../../Classes/offsets/offsets.hpp"
RBX::Vector3 RBX::Instance::getVec3FromValue() const
{
    Umodule::read<RBX::Vector3>(this->address +  Offsets::Value);
}
void RBX::Instance::SetVec3FromValue(RBX::Vector3 Value)
{
    Umodule::write<RBX::Vector3>(this->address +  Offsets::Value, Value);
}
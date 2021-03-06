#include "LocationChange.h"
#include "GuiMap.h"

LocationChange::LocationChange(u16 locationIndex, u8 field, u32 data)
	: locationIndex(locationIndex), field(field), data(data)
{

}

void LocationChange::Reverse(void *guiMap)
{
	u32 replacedData = ((GuiMap*)guiMap)->GetLocationFieldData(locationIndex, field);
	((GuiMap*)guiMap)->SetLocationFieldData(locationIndex, field, data);
	data = replacedData;
}

int32_t LocationChange::GetType()
{
	return (int32_t)UndoTypes::LocationChange;
}

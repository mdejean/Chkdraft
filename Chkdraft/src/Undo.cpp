#include "Undo.h"
#include "GuiMap.h"

UNDOS::UNDOS(IObserveUndos &observer) : observer(observer)
{
	
}

UNDOS::~UNDOS()
{
	undos.clear();
	redos.clear();
}

void UNDOS::AddUndo(ReversiblePtr action)
{
	if ( action->Count() > 0 )
	{
		undos.push_front(action);
		AdjustChangeCount(action->GetType(), 1);
	}
}

void UNDOS::doUndo(int32_t type, void *obj)
{
	ReversiblePtr reversible = popUndo(type);

	if ( reversible != nullptr )
	{
		reversible->Reverse(obj);
		redos.push_front(reversible);
		AdjustChangeCount(reversible->GetType(), -1);
	}
}

void UNDOS::doRedo(int32_t type, void *obj)
{
	ReversiblePtr reversible = popRedo(type);

	if ( reversible != nullptr )
	{
		reversible->Reverse(obj);
		undos.push_front(reversible);
		AdjustChangeCount(reversible->GetType(), 1);
	}
}

void UNDOS::ResetChangeCount()
{
	changeCounters.clear();
}

ReversiblePtr UNDOS::popUndo(int32_t type)
{
	auto it = undos.begin();
	while ( it != undos.end() )
	{
		if ( type == (*it)->GetType() )
		{
			ReversiblePtr toReturn = (*it);
			undos.erase(it);
			return toReturn;
		}
	}
	return nullptr;
}

ReversiblePtr UNDOS::popRedo(int32_t type)
{
	auto it = redos.begin();
	while ( it != redos.end() )
	{
		if ( type == (*it)->GetType() )
		{
			ReversiblePtr toReturn = (*it);
			redos.erase(it);
			return toReturn;
		}
		++it;
	}
	return nullptr;
}

void UNDOS::AdjustChangeCount(int32_t type, int32_t adjustBy)
{
	auto typeCounter = changeCounters.find(type);
	if ( typeCounter != changeCounters.end() )
	{
		auto newCount = typeCounter->second + adjustBy;
		if ( newCount == 0 )
			changeCounters.erase(type);
		else
			typeCounter->second = newCount;
	}
	else
		changeCounters.insert(std::pair<int32_t, int32_t>(type, adjustBy));

	if ( changeCounters.size() == 0 )
		observer.ChangesReversed();
	else
		observer.ChangesMade();
}
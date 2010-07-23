/****************************************************************************
Copyright (c) 2010 cocos2d-x.org

http://www.cocos2d-x.org

Permission is hereby granted, free of charge, to any person obtaining a copy
of this software and associated documentation files (the "Software"), to deal
in the Software without restriction, including without limitation the rights
to use, copy, modify, merge, publish, distribute, sublicense, and/or sell
copies of the Software, and to permit persons to whom the Software is
furnished to do so, subject to the following conditions:

The above copyright notice and this permission notice shall be included in
all copies or substantial portions of the Software.

THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND, EXPRESS OR
IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF MERCHANTABILITY,
FITNESS FOR A PARTICULAR PURPOSE AND NONINFRINGEMENT. IN NO EVENT SHALL THE
AUTHORS OR COPYRIGHT HOLDERS BE LIABLE FOR ANY CLAIM, DAMAGES OR OTHER
LIABILITY, WHETHER IN AN ACTION OF CONTRACT, TORT OR OTHERWISE, ARISING FROM,
OUT OF OR IN CONNECTION WITH THE SOFTWARE OR THE USE OR OTHER DEALINGS IN
THE SOFTWARE.
****************************************************************************/

#include "CCScheduler.h"
#include "ccMacros.h"
#include "support/data_support/utlist.h"
#include "cocoa/NSMutableArray.h"

#include <assert.h>

// data structures

// A list double-linked list used for "updates with priority"
typedef struct _listEntry
{
	struct	_listEntry	*prev, *next;
	SelectorProtocol	*target;		// not retained (retained by hashUpdateEntry)
	INT32				priority;
	bool				paused;
	
} tListEntry;

typedef struct _hashUpdateEntry
{
	tListEntry			**list;		// Which list does it belong to ?
	tListEntry			*entry;		// entry in the list
	SelectorProtocol	*target;		// hash key (retained)
	UT_hash_handle		hh;
} tHashUpdateEntry;

// Hash Element used for "selectors with interval"
typedef struct _hashSelectorEntry
{
	NSMutableArray<CCTimer*>	*timers;
	SelectorProtocol			*target;	// hash key (retained)
	UINT32						timerIndex;
	CCTimer						*currentTimer;
	bool						currentTimerSalvaged;
	bool						paused;
	UT_hash_handle				hh;
} tHashSelectorEntry;

// implementation CCTimer

CCTimer* CCTimer::timerWithTarget(SelectorProtocol *pTarget, SEL_SCHEDULE pfnSelector)
{
	CCTimer *pTimer = new CCTimer();

	pTimer->initWithTarget(pTarget, pfnSelector);
	pTimer->autorelease();

	return pTimer;
}

CCTimer* CCTimer::timerWithTarget(SelectorProtocol *pTarget, SEL_SCHEDULE pfnSelector, ccTime fSeconds)
{
	CCTimer *pTimer = new CCTimer();

	pTimer->initWithTarget(pTarget, pfnSelector, fSeconds);
	pTimer->autorelease();

	return pTimer;
}

CCTimer* CCTimer::initWithTarget(SelectorProtocol *pTarget, SEL_SCHEDULE pfnSelector)
{
    return initWithTarget(pTarget, pfnSelector, 0);
}

CCTimer* CCTimer::initWithTarget(SelectorProtocol *pTarget, SEL_SCHEDULE pfnSelector, ccTime fSeconds)
{
	m_pTarget = pTarget;
	m_pfnSelector = pfnSelector;
	m_fElapsed = -1;
	m_fInterval = fSeconds;

	return this;
}

void CCTimer::update(ccTime dt)
{
	if (m_fElapsed == -1)
	{
		m_fElapsed = 0;
	}
	else
	{
		m_fElapsed += dt;
	}

	if (m_fElapsed >= m_fInterval)
	{
		if (m_pfnSelector)
		{
			(m_pTarget->*m_pfnSelector)(m_fElapsed);
			m_fElapsed = 0;
		}
	}
}


// implementation of CCScheduler

static CCScheduler *pSharedScheduler;

CCScheduler::CCScheduler(void)
{
	assert(pSharedScheduler == NULL);
}

CCScheduler::~CCScheduler(void)
{
	unscheduleAllSelectors();

	delete pSharedScheduler;
	pSharedScheduler = NULL;
}

CCScheduler* CCScheduler::getSharedScheduler(void)
{
	if (! pSharedScheduler)
	{
		pSharedScheduler = new CCScheduler();
		pSharedScheduler->init();
	}

	return pSharedScheduler;
}

CCScheduler* CCScheduler::init(void)
{
	m_fTimeScale = 1.0f;

	// used to trigger CCTimer#update
	// m_pfnUpdateSelector = &CCScheduler::update;
	// impMethod = (TICK_IMP) [CCTimer instanceMethodForSelector:updateSelector];

	// updates with priority
	m_pUpdates0List = NULL;
	m_pUpdatesNegList = NULL;
	m_pUpdatesPosList = NULL;
	m_pHashForUpdates = NULL;

	// selectors with interval
	m_pCurrentTarget = NULL;
    m_bCurrentTargetSalvaged = false;
	m_pHashForSelectors = NULL;

	return this;
}

void CCScheduler::removeHashElement(_hashSelectorEntry *pElement)
{
	delete pElement->timers;
	HASH_DEL(m_pHashForSelectors, pElement);
	free(pElement);
}

void CCScheduler::scheduleTimer(CCTimer *pTimer)
{
	assert(false);
}

void CCScheduler::unscheduleTimer(CCTimer *pTimer)
{
	assert(false);
}

void CCScheduler::unscheduleAllTimers()
{
	assert(false);
}

void CCScheduler::scheduleSelector(SEL_SCHEDULE pfnSelector, SelectorProtocol *pTarget, float fInterval, bool bPaused)
{
	assert(pfnSelector != NULL);
	assert(pTarget != NULL);

	tHashSelectorEntry *pElement = NULL;
	HASH_FIND_INT(m_pHashForSelectors, &pTarget, pElement);

	if (! pElement)
	{
		pElement = (tHashSelectorEntry *)calloc(sizeof(*pElement), 1);
		pElement->target = pTarget;
		HASH_ADD_INT(m_pHashForSelectors, target, pElement);

		// Is this the 1st element ? Then set the pause level to all the selectors of this target
		pElement->paused = bPaused;
	}
	else
	{
		assert(pElement->paused == bPaused);
	}

	if (pElement->timers == NULL)
	{
		pElement->timers = new NSMutableArray<CCTimer*>(10);
	}
    
	// NSMutableArray will increase it's capacity automatically
	//else if( element->timers->num == element->timers->max )
		//ccArrayDoubleCapacity(element->timers);

	CCTimer *pTimer = new CCTimer();
	pTimer->initWithTarget(pTarget, pfnSelector, fInterval);
	pElement->timers->addObject(pTimer);
	pTimer->release();
}

void CCScheduler::unscheduleSelector(SEL_SCHEDULE pfnSelector, SelectorProtocol *pTarget)
{
	// explicity handle nil arguments when removing an object
	if (pTarget == NULL || pfnSelector == NULL)
	{
		return;
	}

	assert(pTarget != NULL);
	assert(pfnSelector != NULL);

	tHashSelectorEntry *pElement = NULL;
	HASH_FIND_INT(m_pHashForSelectors, &pTarget, pElement);

	if (pElement)
	{
		NSMutableArray<CCTimer*>::NSMutableArrayIterator iter;
		UINT32 i;
		for (iter = pElement->timers->begin(), i = 0; iter != pElement->timers->end(); ++iter, ++i)
		{
			CCTimer *pTimer = *iter;

			if (pfnSelector == pTimer->m_pfnSelector)
			{
				if (pTimer == pElement->currentTimer && (! pElement->currentTimerSalvaged))
				{
					pElement->currentTimer->retain();
					pElement->currentTimerSalvaged = true;
				}

				pElement->timers->removeObjectAtIndex(i);

				// update timerIndex in case we are in tick:, looping over the actions
				if (pElement->timerIndex >= i)
				{
					pElement->timerIndex--;
				}

				if (pElement->timers->count() == 0)
				{
					if (m_pCurrentTarget == pElement)
					{
						m_bCurrentTargetSalvaged = true;
					}
					else
					{
						removeHashElement(pElement);
					}
				}

				return;
			}
		}
	}
}

void CCScheduler::priorityIn(tListEntry **ppList, SelectorProtocol *pTarget, Int32 nPriority, bool bPaused)
{
	tListEntry *pListElement = (tListEntry *)malloc(sizeof(*pListElement));

	pListElement->target = pTarget;
	pListElement->priority = nPriority;
	pListElement->paused = bPaused;
	pListElement->next = pListElement->prev = NULL;
	// listElement->impMethod = (TICK_IMP) [target methodForSelector:updateSelector];

	// empey list ?
	if (! *ppList)
	{
		DL_APPEND(*ppList, pListElement);
	}
	else
	{
		bool bAdded = false;

		for (tListEntry *pElement = *ppList; pElement; pElement = pElement->next)
		{
			if (nPriority < pElement->priority)
			{
				if (pElement == *ppList)
				{
					DL_PREPEND(*ppList, pListElement);
				}
				else
				{
					pListElement->next = pElement;
					pListElement->prev = pElement->prev;

					pElement->prev->next = pListElement;
					pElement->prev = pListElement;
				}

				bAdded = true;
				break;
			}
		}

		// Not added? priority has the higher value. Append it.
		if (! bAdded)
		{
			DL_APPEND(*ppList, pListElement);
		}
	}
}

void CCScheduler::appendIn(_listEntry **ppList, SelectorProtocol *pTarget, bool bPaused)
{
	tListEntry *pListElement = (tListEntry *)malloc(sizeof(*pListElement));

	pListElement->target = pTarget;
	pListElement->paused = bPaused;
	// listElement->impMethod = (TICK_IMP) [target methodForSelector:updateSelector];

	DL_APPEND(*ppList, pListElement);

	// update hash entry for quicker access
	tHashUpdateEntry *pHashElement = (tHashUpdateEntry *)calloc(sizeof(*pHashElement), 1);
	// hashElement->target = [target retain];
	pHashElement->list = ppList;
	pHashElement->entry = pListElement;
	HASH_ADD_INT(m_pHashForUpdates, target, pHashElement);
}

void CCScheduler::scheduleUpdateForTarget(SelectorProtocol *pTarget, INT32 nPriority, bool bPaused)
{
#if COCOS2D_DEBUG >= 1
	tHashUpdateEntry *pHashElement = NULL;
	HASH_FIND_INT(m_pHashForUpdates, &target, pHashElement);
	assert(pHashElement == NULL);
#endif

	// most of the updates are going to be 0, that's way there
	// is an special list for updates with priority 0
	if (nPriority == 0)
	{
		appendIn(&m_pUpdates0List, pTarget, bPaused);
	} else
	if (nPriority < 0)
	{
		priorityIn(&m_pUpdatesNegList, pTarget, nPriority, bPaused);
	}
	else
	{
		// priority > 0
		priorityIn(&m_pUpdatesPosList, pTarget, nPriority, bPaused);
	}
}

void CCScheduler::unscheduleUpdateForTarget(SelectorProtocol *pTarget)
{
	if (pTarget == NULL)
	{
		return;
	}

	tHashUpdateEntry *pElement = NULL;
	HASH_FIND_INT(m_pHashForUpdates, &pTarget, pElement);
	if (pElement)
	{
		// list entry
		DL_DELETE(*pElement->list, pElement->entry);
		free(pElement->entry);

		// hash entry
		HASH_DEL(m_pHashForUpdates, pElement);
		free(pElement);
	}
}

void CCScheduler::unscheduleAllSelectors(void)
{
	// Custom Selectors
    tHashSelectorEntry *pElement;
	for (pElement = m_pHashForSelectors; pElement != NULL;)
	{
		pElement = (tHashSelectorEntry *)pElement->hh.next;
		unscheduleAllSelectorsForTarget(pElement->target);
	}

	// Updates selectors
	tListEntry *pEntry, *pTmp;
	DL_FOREACH_SAFE(m_pUpdates0List, pEntry, pTmp)
	{
		unscheduleUpdateForTarget(pEntry->target);
	}
	DL_FOREACH_SAFE(m_pUpdatesNegList, pEntry, pTmp)
	{
        unscheduleUpdateForTarget(pEntry->target);
	}
	DL_FOREACH_SAFE(m_pUpdatesPosList, pEntry, pTmp)
	{
        unscheduleUpdateForTarget(pEntry->target);
	}
}

void CCScheduler::unscheduleAllSelectorsForTarget(SelectorProtocol *pTarget)
{
	// explicit NULL handling
	if (pTarget == NULL)
	{
		return;
	}

	// Custom Selectors
	tHashSelectorEntry *pElement = NULL;
	HASH_FIND_INT(m_pHashForSelectors, &pTarget, pElement);

	if (pElement)
	{
		if (pElement->timers->containsObject(pElement->currentTimer)
			&& (! pElement->currentTimerSalvaged))
		{
			pElement->currentTimer->retain();
			pElement->currentTimerSalvaged = true;
		}
		pElement->timers->removeAllObjects();

		if (m_pCurrentTarget == pElement)
		{
			m_bCurrentTargetSalvaged = true;
		}
		else
		{
			removeHashElement(pElement);
		}
	}

	// update selector
	unscheduleUpdateForTarget(pTarget);
}

void CCScheduler::resumeTarget(SelectorProtocol *pTarget)
{
	assert(pTarget != NULL);

	// custom selectors
	tHashSelectorEntry *pElement = NULL;
	HASH_FIND_INT(m_pHashForSelectors, &pTarget, pElement);
	if (pElement)
	{
		pElement->paused = false;
	}

	// update selector
	tHashUpdateEntry *pElementUpdate = NULL;
	HASH_FIND_INT(m_pHashForUpdates, &pTarget, pElementUpdate);
	if (pElementUpdate)
	{
		assert(pElementUpdate->entry != NULL);
		pElementUpdate->entry->paused = false;
	}
}

void CCScheduler::pauseTarget(SelectorProtocol *pTarget)
{
	assert(pTarget != NULL);

	// custom selectors
	tHashSelectorEntry *pElement = NULL;
	HASH_FIND_INT(m_pHashForSelectors, &pTarget, pElement);
	if (pElement)
	{
		pElement->paused = true;
	}

	// update selector
	tHashUpdateEntry *pElementUpdate = NULL;
	HASH_FIND_INT(m_pHashForUpdates, &pTarget, pElementUpdate);
	if (pElementUpdate)
	{
		assert(pElementUpdate->entry != NULL);
		pElementUpdate->entry->paused = true;
	}
}

// main loop
void CCScheduler::tick(ccTime dt)
{
	if (m_fTimeScale != 1.0f)
	{
		dt *= m_fTimeScale;
	}

	// Iterate all over the Updates selectors
	tListEntry *pEntry, *pTmp;

	// updates with priority < 0
	DL_FOREACH_SAFE(m_pUpdatesNegList, pEntry, pTmp)
	{
		if (! pEntry->paused)
		{
			pEntry->target->update(dt);
		}
	}

	// updates with priority == 0
	DL_FOREACH_SAFE(m_pUpdates0List, pEntry, pTmp)
	{
		if (! pEntry->paused)
		{
			pEntry->target->update(dt);
		}
	}

	// updates with priority > 0
	DL_FOREACH_SAFE(m_pUpdatesPosList, pEntry, pTmp)
	{
		if (! pEntry->paused)
		{
			pEntry->target->update(dt);
		}
	}

	// Interate all over the custom selectors
	for (tHashSelectorEntry *elt = m_pHashForSelectors; elt != NULL; )
	{
		m_pCurrentTarget = elt;
		m_bCurrentTargetSalvaged = false;

		if (! m_pCurrentTarget->paused)
		{
			// The 'timers' array may change while inside this loop
			for (elt->timerIndex = 0; elt->timerIndex < elt->timers->count(); ++(elt->timerIndex))
			{
				elt->currentTimer = elt->timers->getObjectAtIndex(elt->timerIndex);
				elt->currentTimerSalvaged = false;

				elt->currentTimer->update(dt);

				if (elt->currentTimerSalvaged)
				{
					// The currentTimer told the remove itself. To prevent the timer from
					// accidentally deallocating itself before finishing its step, we retained
					// it. Now that step is done, it's safe to release it.
					elt->currentTimer->release();
				}

				elt->currentTimer = NULL;
			}
		}

		// elt, at this moment, is still valid
		// so it is safe to ask this here (issue #490)
		elt = (tHashSelectorEntry *)elt->hh.next;

		// only delete currentTarget if no actions were scheduled during the cycle (issue #481)
		if (m_bCurrentTargetSalvaged && m_pCurrentTarget->timers->count() == 0)
		{
			removeHashElement(m_pCurrentTarget);
		}
	}

	m_pCurrentTarget = NULL;
}
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

#ifndef __TOUCH_DISPATCHER_CCTOUCH_HANDLER_H__
#define __TOUCH_DISPATCHER_CCTOUCH_HANDLER_H__

#include "CCTouchDelegateProtocol.h"
#include "CCTouchDispatcher.h"
#include "cocoa/NSObject.h"
#include "cocoa/NSSet.h"

/**
 CCTouchHandler
 Object than contains the delegate and priority of the event handler.
*/
class CCTouchHandler : public NSObject
{
public:
	virtual ~CCTouchHandler(void);

	// delegate
	CCTouchDelegate* getDelegate();
	void setDelegate(CCTouchDelegate *pDelegate);

	// priority
	INT32 getPriority(void);
	void setPriority(INT32 nPriority);

	// enabled selectors
	INT32 getEnabledSelectors(void);
	void setEnalbedSelectors(INT32 nValue);

	// initializes a TouchHandler with a delegate and a priority
	virtual CCTouchHandler* initWithDelegate(CCTouchDelegate *pDelegate, INT32 nPriority);

public:
	// allocates a TouchHandler with a delegate and a priority 
	static CCTouchHandler* handlerWithDelegate(CCTouchDelegate *pDelegate, INT32 nPriority);

protected:
	CCTouchDelegate *m_pDelegate;
	INT32 m_nPriority;
	INT32 m_nEnabledSelectors;
};

/** CCStandardTouchHandler
 It forwardes each event to the delegate.
 */
class CCStandardTouchHandler : public CCTouchHandler
{
public:
	virtual CCTouchHandler* initWithDelegate(CCTouchDelegate *pDelegate, INT32 nPriority);

public:
	static CCStandardTouchHandler* handlerWithDelegate(CCStandardTouchDelegate *pDelegate, INT32 nPriority);
};

/**
 CCTargetedTouchHandler
 Object than contains the claimed touches and if it swallos touches.
 Used internally by TouchDispatcher
 */
class CCTargetedTouchHandler : public CCTouchHandler
{
public:
    ~CCTargetedTouchHandler(void);

    // whether or not the touches are swallowed
	bool isSwallowsTouches(void);
	void setSwallowsTouches(bool bSwallowsTouches);

	// MutableSet that contains the claimed touches
	NSMutableSet* getClaimedTouches(void);

	CCTouchHandler* initWithDelegate(CCTouchDelegate *pDelegate, INT32 nPriority, bool bSwallow);

public:
	static CCTargetedTouchHandler* handlerWithDelegate(CCTouchDelegate *pDelegate, INT32 nPriority, bool bSwallow);

protected:
	bool m_bSwallowsTouches;
	NSMutableSet *m_pClaimedTouches;
};

#endif // __TOUCH_DISPATCHER_CCTOUCH_HANDLER_H__
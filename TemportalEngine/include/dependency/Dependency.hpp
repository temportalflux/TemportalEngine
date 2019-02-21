#ifndef DEPENDENCY_H
#define DEPENDENCY_H

#include "Api.h"

class TEMPORTALENGINE_API Dependency
{
private:
	bool mHasBeenInitialized;

protected:
	void setInitialized(bool initialized) { mHasBeenInitialized = initialized; }

public:
	virtual ~Dependency() = default;
	virtual bool initialize() { return false; };
	bool hasBeenInitialized() { return mHasBeenInitialized; }
	virtual void terminate() {};
};

class DependencyNull : public Dependency
{
public:
	bool initialize() override { /* STUB: Null Service */ return false; }
	void terminate() override { /* STUB: Null Service */ }
};

#endif //DEPENDENCY_H
